"""Exercise BLE transport boundaries without claiming hardware radio acceptance."""
import asyncio
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from token_ble import BLEConnection,SERVICE,TX,discover
import token_ble

class Client:
    def __init__(self,device,timeout=10,winrt=None):
        assert winrt=={'use_cached_services':False}
        self.is_connected=False;self.writes=[];self.callback=None;self.partial=bytearray()
        self.services=SimpleNamespace(get_service=lambda uuid:uuid==SERVICE)
    async def connect(self):self.is_connected=True
    async def disconnect(self):
        assert self.callback is None
        self.is_connected=False
    async def start_notify(self,uuid,callback):
        assert uuid==TX;self.callback=callback
    async def stop_notify(self,uuid):
        assert uuid==TX;self.callback=None
    async def write_gatt_char(self,uuid,data,response):
        assert len(data)<=20 and response
        self.writes.append(bytes(data));self.partial.extend(data)
        if self.partial==b'CT_HELLO 4\n':
            self.callback(None,b'CT_READY ');self.callback(None,b'4\n');self.partial.clear()

async def scan(**kwargs):
    return {'one':(SimpleNamespace(address='AA',name='Codex Passport'),SimpleNamespace(service_uuids=[SERVICE]))}

class BLETests(unittest.TestCase):
    def fake(self):return patch.dict(sys.modules,bleak=SimpleNamespace(BleakClient=Client,BleakScanner=SimpleNamespace(discover=scan)))
    def test_handshake_fragmentation_and_cleanup(self):
        with self.fake():
            c=BLEConnection.open('AA')
            try:
                c.write(b'x'*71+b'\n')
                self.assertEqual([len(x) for x in c.client.writes[1:]],[20,20,20,12])
                c._notify(None,b'CT_ACK ');c._notify(None,b'42\n')
                self.assertEqual(c.readline(),b'CT_ACK 42\n')
                c._notify(None,b'x'*600+b'\nCT_ACK 43\n')
                self.assertEqual(c.readline(),b'CT_ACK 43\n')
                c.client.is_connected=False
                with self.assertRaises(OSError):c.write(b'fail')
                c.client.callback=None  # Lost links have no active notification subscription.
            finally:c.close()
            self.assertFalse(c.thread.is_alive())
    def test_repeated_connect_closes_notification_before_dispose(self):
        with self.fake():
            for _ in range(3):
                c=BLEConnection.open('AA');client=c.client
                c.close()
                self.assertIsNone(client.callback)
                self.assertFalse(client.is_connected)
                self.assertFalse(c.thread.is_alive())
    def test_unknown_address_never_connects(self):
        with self.fake():
            with self.assertRaisesRegex(OSError,'No matching'):BLEConnection.open('BB')
    def test_windows_refresh_timeout_releases_then_retries_cached(self):
        attempts=[]
        class Stalled(Client):
            def __init__(self,device,timeout=10,winrt=None):
                self.cached=winrt['use_cached_services']
                super().__init__(device,timeout,{'use_cached_services':False})
                attempts.append(self)
            async def connect(self):
                if not self.cached:raise asyncio.TimeoutError()
                await super().connect()
        with self.fake(),patch('bleak.BleakClient',Stalled),patch.object(token_ble.sys,'platform','win32'):
            c=BLEConnection.open('AA')
            try:
                self.assertEqual([x.cached for x in attempts],[False,True])
                self.assertFalse(attempts[0].is_connected)
                self.assertTrue(c.client.is_connected)
            finally:c.close()
    def test_multiple_devices_are_not_guessed(self):
        async def multi(**kwargs):
            found=await scan(**kwargs);found['two']=(SimpleNamespace(address='BB'),SimpleNamespace(service_uuids=[SERVICE]));return found
        with self.fake(),patch('bleak.BleakScanner.discover',multi):
            with self.assertRaisesRegex(OSError,'Multiple'):BLEConnection.open()
    def test_service_filter_rejects_unrelated_name(self):
        async def unrelated(**kwargs):return {'x':(SimpleNamespace(name='Codex Passport'),SimpleNamespace(service_uuids=[]))}
        with self.fake(),patch('bleak.BleakScanner.discover',unrelated):self.assertEqual(asyncio.run(discover()),[])

if __name__=='__main__':unittest.main()
