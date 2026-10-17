"""LAN discovery and shared management transport, with local fake device endpoints."""
import collections,hashlib,hmac,json,socket,sys,tempfile,threading,time,unittest,urllib.request,urllib.error
from pathlib import Path
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
import token_wifi
from management_server import LinkManager,ManagementServer,existing_manager
TOKEN='01'*16;IDENTITY='123456789abc'
class Fake:
    port='COM-MOCK'
    def __init__(self):self.lines=collections.deque();self.commands=[]
    def write(self,line):self.commands.append(line);self.lines.append({'PC_WIFI_ACCESS':f'PC_WIFI_ACCESS {IDENTITY} {TOKEN}','PC_WIFI_GET':'PC_WIFI 1 3 1 1 0 127.0.0.1 0'}.get(line.decode().strip(),'PC_READY 1').encode()+b'\n')
    def readline(self,*args):return self.lines.popleft() if self.lines else b''
class Tests(unittest.TestCase):
    def test_pair_cache_and_legacy(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'private'/'network.json';self.assertTrue(token_wifi.remember(Fake(),path));self.assertEqual(len(token_wifi.load_devices(path)),1)
            with patch('token_events.rpc',return_value='PC_INVALID'):self.assertFalse(token_wifi.remember(Fake(),path))
            self.assertEqual(len(token_wifi.load_devices(path)),1);path.write_text('[]');self.assertEqual(token_wifi.load_devices(path),[])
    def test_discovery_authentication(self):
        sock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);sock.bind(('127.0.0.1',0));port=sock.getsockname()[1]
        def respond():
            data,peer=sock.recvfrom(128);nonce=data.decode().split()[-1];base=f'TK_DEVICE 1 {IDENTITY} v19 127.0.0.1 '
            sock.sendto((base+'0'*64).encode(),peer)
            signature=hmac.new(bytes.fromhex(TOKEN),f'{IDENTITY} 127.0.0.1 {nonce}'.encode(),hashlib.sha256).hexdigest();sock.sendto((base+signature).encode(),peer);sock.close()
        thread=threading.Thread(target=respond);thread.start();found=token_wifi.discover([{'id':IDENTITY,'token':TOKEN,'ip':''}],timeout=.3,targets=('127.0.0.1',),port=port);thread.join();self.assertEqual(found[0]['ip'],'127.0.0.1')
        self.assertEqual(token_wifi.discover([],timeout=.01),[])
    def test_spoofed_discovery_rejected(self):
        sock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);sock.bind(('127.0.0.1',0));port=sock.getsockname()[1]
        def respond():
            _,peer=sock.recvfrom(128);sock.sendto((f'TK_DEVICE 1 {IDENTITY} v19 127.0.0.1 '+'0'*64).encode(),peer);sock.close()
        thread=threading.Thread(target=respond);thread.start();found=token_wifi.discover([{'id':IDENTITY,'token':TOKEN,'ip':''}],timeout=.2,targets=('127.0.0.1',),port=port);thread.join();self.assertEqual(found,[])
    def test_http_transport_and_bounds(self):
        class Handler(BaseHTTPRequestHandler):
            def log_message(self,*args):pass
            def do_POST(self):
                data=self.rfile.read(int(self.headers['Content-Length']));self.send_response(200 if self.headers.get('Authorization')=='Bearer '+TOKEN else 401);self.end_headers();self.wfile.write(b'CT_READY 4\n' if data==b'CT_HELLO 4\n' else b'PC_READY 1\n')
        server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
        try:
            connection=token_wifi.WiFiConnection('127.0.0.1',TOKEN,server.server_port);connection.write(b'CT_HELLO 4\n');self.assertEqual(connection.readline(),b'CT_READY 4\n')
            for line in [b'',b'a'*1025,b'PC_HELLO\nPC_VERSION']:
                with self.assertRaises(OSError):connection.write(line)
            connection.close()
            with self.assertRaises(OSError):connection.write(b'PC_HELLO')
            wrong=token_wifi.WiFiConnection('127.0.0.1','02'*16,server.server_port)
            with self.assertRaises(OSError):wrong.write(b'PC_HELLO')
        finally:server.shutdown();server.server_close()
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
