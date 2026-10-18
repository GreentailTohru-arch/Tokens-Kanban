"""Authenticated loopback management shares one USB/BLE connection."""
import collections,hashlib,hmac,json,socket,sys,tempfile,threading,time,unittest,urllib.request,urllib.error
from pathlib import Path
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from management_server import LinkManager,ManagementServer,existing_manager,request_stop
TOKEN='01'*16;IDENTITY='123456789abc'
class Fake:
    port='COM-MOCK'
    def __init__(self):self.lines=collections.deque();self.commands=[]
    def write(self,line):self.commands.append(line);self.lines.append(b'PC_READY 1\n')
    def readline(self,*args):return self.lines.popleft() if self.lines else b''
class Tests(unittest.TestCase):
    def test_proxy_auth_single_owner_reconnect_open(self):
        temporary=tempfile.TemporaryDirectory();runtime=Path(temporary.name)/'manager.json'
        manager=LinkManager();server=ManagementServer(manager,runtime=runtime);url=server.url.split('#')[0];headers={'Authorization':'Bearer '+server.token}
        def request(path,data=None,extra=None):return urllib.request.urlopen(urllib.request.Request(url+path,data=data,headers={**headers,**(extra or {})}),timeout=3)
        try:
            for asset in ['shortcut-picker.js','shortcut-picker.css','card-storage.js']:
                with request(asset+'?v=30') as response:
                    self.assertEqual(response.read(),(Path(__file__).resolve().parents[1]/'companion/postcard'/asset).read_bytes())
            with patch('pet_remote.custom_shortcut_path',return_value=Path(temporary.name)/'controls.json'):
                bindings=['Alt+T','','','','Ctrl+Shift+S','','','']
                with request('api/pet-shortcut',json.dumps({'shortcuts':bindings}).encode()) as response:self.assertEqual(response.status,200)
                with request('api/pet-shortcut') as response:self.assertEqual(json.load(response)['shortcuts'],bindings)
                with self.assertRaises(urllib.error.HTTPError):request('api/pet-shortcut',json.dumps({'shortcuts':['bad']*8}).encode())
                with request('api/pet-shortcut') as response:self.assertEqual(json.load(response)['shortcuts'],bindings)
            with self.assertRaises(urllib.error.HTTPError):urllib.request.urlopen(url+'api/info')
            with self.assertRaises(urllib.error.HTTPError):request('api/info',extra={'Host':'attacker.example'})
            with request('api/info') as response:self.assertFalse(json.load(response)['connected'])
            with self.assertRaises(urllib.error.HTTPError):request('api/rpc',b'PC_HELLO')
            fake=Fake()
            with manager.lock:manager.attach(fake)
            with request('api/rpc',b'PC_HELLO') as response:self.assertEqual(response.read(),b'PC_READY 1\n')
            recent = manager.last_rpc
            self.assertGreater(recent, 0)
            with request('api/rpc',b'PC_INFO_SYSTEM') as response:response.read()
            self.assertEqual(manager.last_rpc, recent)
            with request('api/rpc',b'PC_LOGO_STATUS 1') as response:response.read()
            self.assertGreaterEqual(manager.last_rpc, recent)
            for invalid in [b'CT_HELLO 4',b'PC_HELLO\nPC_VERSION',b'PC_'+b'x'*1024]:
                with self.assertRaises(urllib.error.HTTPError):request('api/rpc',invalid)
            with self.assertRaises(urllib.error.HTTPError):request('api/rpc',b'PC_HELLO',{'Origin':'https://attacker.example'})
            with patch('management_server.webbrowser.open') as opened:
                self.assertTrue(server.open(now=0));self.assertFalse(server.open(now=59));self.assertTrue(server.open(now=60));self.assertEqual(opened.call_count,2)
                self.assertTrue(existing_manager(path=runtime));self.assertEqual(opened.call_count,3)
            with manager.lock:manager.detach();manager.attach(Fake())
            with request('api/info') as response:self.assertTrue(json.load(response)['connected'])
            self.assertFalse(manager.stop_event.is_set())
            with self.assertRaises(urllib.error.HTTPError):
                urllib.request.urlopen(urllib.request.Request(url+'api/stop',data=b''))
            with self.assertRaises(urllib.error.HTTPError):
                request('api/stop',b'',{'Origin':'https://attacker.example'})
            self.assertFalse(manager.stop_event.is_set())
            self.assertIn('已请求停止',request_stop(path=runtime))
            # The handler flushes its HTTP response before signaling the sender.
            self.assertTrue(manager.stop_event.wait(1))
        finally:server.close();self.assertFalse(runtime.exists());temporary.cleanup()
if __name__=='__main__':unittest.main()
