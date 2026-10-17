"""BLE transport with bounded framing, write responses and snapshot ACKs."""
import asyncio
import concurrent.futures
import queue
import threading
import sys

SERVICE='9e7a0001-4b51-4a4c-9b50-2f53f077c301'
RX='9e7a0002-4b51-4a4c-9b50-2f53f077c301'
TX='9e7a0003-4b51-4a4c-9b50-2f53f077c301'

async def discover():
    from bleak import BleakScanner
    found=await BleakScanner.discover(timeout=6,return_adv=True,service_uuids=[SERVICE])
    return [device for device,adv in found.values() if SERVICE in [u.lower() for u in adv.service_uuids]]

class BLEConnection:
    def __init__(self):
        self.loop=asyncio.new_event_loop()
        self.thread=threading.Thread(target=self.loop.run_forever,daemon=True)
        self.lines=queue.Queue(maxsize=16); self.buffer=bytearray();self.dropping=False
        self.client=None;self.port='BLE';self.closed=False;self.subscribed=False
        self.thread.start()

    def _run(self,coroutine,timeout=20):
        future=asyncio.run_coroutine_threadsafe(coroutine,self.loop)
        try: return future.result(timeout=timeout)
        except Exception as error:
            future.cancel()
            raise OSError('Bluetooth: '+str(error)) from error

    @classmethod
    def open(cls,address=None):
        connection=cls()
        try:
            connection._run(connection._connect(address),40)
            connection.write(b'CT_HELLO 4\n')
            import time
            deadline=time.monotonic()+3
            while time.monotonic()<deadline:
                if connection.readline().strip()==b'CT_READY 4': return connection
            raise OSError('Bluetooth firmware handshake failed; install v4 firmware')
        except BaseException:
            connection.close();raise

    async def _connect(self,address):
        from bleak import BleakClient
        devices=await discover()
        if address: devices=[d for d in devices if d.address.casefold()==address.casefold()]
        if len(devices)!=1:
            raise OSError('No matching Passport found' if not devices else 'Multiple Passports; use --ble-address')
        self.client=BleakClient(devices[0],timeout=10,winrt={'use_cached_services':False})
        try:
            await asyncio.wait_for(self.client.connect(),12)
        except (TimeoutError,asyncio.TimeoutError):
            # Windows can stall refreshing unchanged GATT services after a flash.
            # Release this session before one cached retry; the service and
            # CT_READY handshake below must still validate the actual device.
            if sys.platform!='win32':raise
            try:await asyncio.wait_for(self.client.disconnect(),3)
            except Exception:pass
            self.client=BleakClient(devices[0],timeout=10,winrt={'use_cached_services':True})
            await asyncio.wait_for(self.client.connect(),12)
        if not self.client.services.get_service(SERVICE): raise OSError('Missing Passport service')
        await self.client.start_notify(TX,self._notify)
        self.subscribed=True
        self.port='BLE '+devices[0].address

    def _notify(self,characteristic,data):
        del characteristic
        for byte in data:
            if byte==10:
                if not self.dropping:
                    if self.lines.full(): self.lines.get_nowait()
                    self.lines.put_nowait(bytes(self.buffer)+b'\n')
                self.buffer.clear();self.dropping=False
            elif byte!=13:
                if len(self.buffer)>=511: self.dropping=True
                if not self.dropping:self.buffer.append(byte)

    async def _write(self,data):
        if not self.client or not self.client.is_connected: raise OSError('Disconnected')
        # Works at the minimum ATT MTU (23); no MTU negotiation assumption.
        for pos in range(0,len(data),20):
            await self.client.write_gatt_char(RX,data[pos:pos+20],response=True)

    def write(self,data):
        self._run(self._write(data),10)
        return len(data)

    def readline(self,size=256):
        try: return self.lines.get(timeout=.25)[:size]
        except queue.Empty: return b''

    async def _disconnect(self):
        if not self.client:return
        try:
            # Explicitly release the CCCD before disposing WinRT handles; simply
            # closing the connection can cancel the next subscription in-process.
            if self.subscribed and self.client.is_connected:
                try:await asyncio.wait_for(self.client.stop_notify(TX),2)
                except (Exception,asyncio.TimeoutError):pass
        finally:
            self.subscribed=False
            await self.client.disconnect()
            # Let Windows deliver final handle/session events before loop shutdown.
            await asyncio.sleep(.2)

    def close(self):
        if self.closed:return
        self.closed=True
        try: self._run(self._disconnect(),5)
        except OSError:pass
        self.loop.call_soon_threadsafe(self.loop.stop);self.thread.join(timeout=3)
        if not self.thread.is_alive():self.loop.close()
