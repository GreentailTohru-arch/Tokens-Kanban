"""Loopback management shares one physical connection with the statistics sender."""
import hmac,json,secrets,threading,time,urllib.parse,urllib.request,webbrowser,os,re
from pathlib import Path
from http.server import ThreadingHTTPServer
from postcard_server import Handler
RUNTIME=Path(os.environ.get('LOCALAPPDATA',str(Path.home()/'.local/share')))/'TokensKanban'/'manager.json'
def existing_manager(open_browser=True,path=RUNTIME):
    try:
        value=json.loads(path.read_text(encoding='utf-8'));port=value['port'];token=value['token']
        if type(port) is not int or not 1024<=port<=65535 or not re.fullmatch('[0-9a-f]{32}',token):return False
        url=f'http://127.0.0.1:{port}/';request=urllib.request.Request(url+'api/info',headers={'Authorization':'Bearer '+token})
        with urllib.request.build_opener(urllib.request.ProxyHandler({})).open(request,timeout=.5) as response:info=json.load(response)
        if info.get('app')!='TokensKanban' or info.get('protocol')!=1:return False
        if open_browser:webbrowser.open(url+'#'+token)
        return True
    except (OSError,ValueError,KeyError,TypeError):return False
class LinkManager:
    def __init__(self):self.lock=threading.RLock();self.connection=None;self.kind='';self.revision=0
    def attach(self,connection):self.connection=connection;self.kind='USB' if str(connection.port).startswith('COM') else 'Wi-Fi' if str(connection.port).startswith('Wi-Fi') else '蓝牙';self.revision+=1
    def detach(self):self.connection=None;self.kind='';self.revision+=1
    def info(self):return {'app':'TokensKanban','protocol':1,'kind':'电脑同步 · '+self.kind,'connected':self.connection is not None,'revision':self.revision}
class ManagementServer:
    def __init__(self,manager,runtime=RUNTIME):
        self.manager=manager;self.token=secrets.token_hex(16);self.last_open=-1e9
        owner=self
        class ManagedHandler(Handler):
            def valid_host(self):return self.headers.get('Host') in ('127.0.0.1:'+str(owner.server.server_port),'localhost:'+str(owner.server.server_port))
            def auth(self):return hmac.compare_digest(self.headers.get('Authorization',''),'Bearer '+owner.token)
            def answer(self,status,text,kind='text/plain; charset=utf-8'):
                data=text.encode();self.send_response(status);self.send_header('Content-Type',kind);self.send_header('Content-Length',str(len(data)));self.send_header('Cache-Control','no-store');self.send_header('X-Content-Type-Options','nosniff');self.end_headers();self.wfile.write(data)
            def do_GET(self):
                if not self.valid_host():return self.answer(403,'Host not allowed')
                route=urllib.parse.urlsplit(self.path).path
                if route=='/api/info':
                    if not self.auth():return self.answer(401,'Access token required')
                    info=manager.info()
                    return self.answer(200,json.dumps(info,ensure_ascii=False),'application/json')
                if route=='/':
                    from pathlib import Path
                    raw=(Path(__file__).parent/'postcard/index.html').read_text(encoding='utf-8');raw=raw.replace('</head>','<meta name="passport-device" content="1"><meta name="passport-token" content="'+owner.token+'"></head>');return self.answer(200,raw,'text/html; charset=utf-8')
                super().do_GET()
            def do_POST(self):
                if not self.valid_host():return self.answer(403,'Host not allowed')
                if self.path!='/api/rpc':return self.answer(404,'Not found')
                if not self.auth():return self.answer(401,'Access token required')
                if self.headers.get('Origin') and self.headers['Origin'] not in ('http://127.0.0.1:'+str(owner.server.server_port),'http://localhost:'+str(owner.server.server_port)):return self.answer(403,'Origin not allowed')
                try:
                    self.connection.settimeout(5)
                    size=int(self.headers.get('Content-Length','0'));assert 0<size<=1024
                    data=self.rfile.read(size).decode('ascii').strip();assert data.startswith('PC_') and '\n' not in data and '\r' not in data and '\0' not in data
                except (ValueError,UnicodeError,AssertionError):return self.answer(400,'One bounded management command required')
                try:
                    from token_events import rpc
                    with manager.lock:
                        if manager.connection is None:return self.answer(503,'Device not connected')
                        reply=rpc(manager.connection,data)
                    self.answer(200,reply+'\n')
                except OSError:self.answer(503,'Device unavailable; reconnecting')
        self.server=ThreadingHTTPServer(('127.0.0.1',0),ManagedHandler);self.thread=threading.Thread(target=self.server.serve_forever,daemon=True);self.thread.start()
        self.url=f'http://127.0.0.1:{self.server.server_port}/#{self.token}'
        self.runtime=runtime
        try:
            runtime.parent.mkdir(parents=True,exist_ok=True);temp=runtime.with_suffix('.tmp');temp.write_text(json.dumps({'pid':os.getpid(),'port':self.server.server_port,'token':self.token}),encoding='utf-8');os.chmod(temp,0o600);os.replace(temp,runtime)
        except OSError:pass
    def open(self,now=None):
        now=time.monotonic() if now is None else now
        if now-self.last_open<60:return False
        self.last_open=now;webbrowser.open(self.url);return True
    def close(self):
        self.server.shutdown();self.server.server_close();self.thread.join(timeout=2)
        try:
            if json.loads(self.runtime.read_text(encoding='utf-8')).get('token')==self.token:self.runtime.unlink()
        except (OSError,ValueError):pass
