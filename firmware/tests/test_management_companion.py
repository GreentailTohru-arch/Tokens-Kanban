"""Authenticated loopback management shares one USB/BLE connection."""
import collections,hashlib,hmac,json,socket,sys,tempfile,threading,time,unittest,urllib.request,urllib.error
from pathlib import Path
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from management_server import LinkManager,ManagementServer,existing_manager
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
            with self.assertRaises(urllib.error.HTTPError):urllib.request.urlopen(url+'api/info')
            with self.assertRaises(urllib.error.HTTPError):request('api/info',extra={'Host':'attacker.example'})
            with request('api/info') as response:self.assertFalse(json.load(response)['connected'])
            with self.assertRaises(urllib.error.HTTPError):request('api/rpc',b'PC_HELLO')
            fake=Fake()
            with manager.lock:manager.attach(fake)
            with request('api/rpc',b'PC_HELLO') as response:self.assertEqual(response.read(),b'PC_READY 1\n')
            for invalid in [b'CT_HELLO 4',b'PC_HELLO\nPC_VERSION',b'PC_'+b'x'*1024]:
                with self.assertRaises(urllib.error.HTTPError):request('api/rpc',invalid)
            with self.assertRaises(urllib.error.HTTPError):request('api/rpc',b'PC_HELLO',{'Origin':'https://attacker.example'})
            with patch('management_server.webbrowser.open') as opened:
                self.assertTrue(server.open(now=0));self.assertFalse(server.open(now=59));self.assertTrue(server.open(now=60));self.assertEqual(opened.call_count,2)
                self.assertTrue(existing_manager(path=runtime));self.assertEqual(opened.call_count,3)
            with manager.lock:manager.detach();manager.attach(Fake())
            with request('api/info') as response:self.assertTrue(json.load(response)['connected'])
        finally:server.close();self.assertFalse(runtime.exists());temporary.cleanup()
if __name__=='__main__':unittest.main()
