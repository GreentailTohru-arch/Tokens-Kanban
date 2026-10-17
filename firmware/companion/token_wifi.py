"""Paired LAN transport: authenticated discovery, bounded RPC, no cloud relay."""
from pathlib import Path
import collections,hashlib,hmac,ipaddress,json,os,re,secrets,socket,time,urllib.request
CACHE=Path(os.environ.get('LOCALAPPDATA',str(Path.home()/'.local/share')))/'TokensKanban'/'network-devices.json'
def load_devices(path=CACHE):
    try:
        rows=json.loads(path.read_text(encoding='utf-8'))
        return [r for r in rows if re.fullmatch('[0-9a-f]{12}',r.get('id','')) and re.fullmatch('[0-9a-f]{32}',r.get('token','')) and len(r.get('ip',''))<=15][:32]
    except (OSError,ValueError,TypeError,AttributeError):return []
def remember(connection,path=CACHE):
    from token_events import rpc
    access=rpc(connection,'PC_WIFI_ACCESS').split()
    if len(access)!=3 or access[0]!='PC_WIFI_ACCESS' or not re.fullmatch('[0-9a-f]{12}',access[1]) or not re.fullmatch('[0-9a-f]{32}',access[2]):return False
    state=rpc(connection,'PC_WIFI_GET').split()
    ip=state[6] if len(state)==8 and state[0]=='PC_WIFI' else ''
    if ip=='-':ip=''
    if ip:
        try:ipaddress.IPv4Address(ip)
        except ipaddress.AddressValueError:return False
    rows=load_devices(path);rows=[r for r in rows if r['id']!=access[1]];rows.append({'id':access[1],'token':access[2],'ip':ip})
    path.parent.mkdir(parents=True,exist_ok=True);temp=path.with_suffix('.tmp');temp.write_text(json.dumps(rows),encoding='utf-8');os.chmod(temp,0o600);os.replace(temp,path);return True

def discover(rows,timeout=1.5,targets=('255.255.255.255',),port=43759):
    if not rows:return []
    known={r['id']:r for r in rows};nonces={i:secrets.token_hex(8) for i in known};found={}
    with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as s:
        s.setsockopt(socket.SOL_SOCKET,socket.SO_BROADCAST,1);s.settimeout(.15)
        for host in targets:
            for identity,nonce in nonces.items():
                try:s.sendto(f'TK_DISCOVER 1 {identity} {nonce}'.encode(),(host,port))
                except OSError:pass
        end=time.monotonic()+timeout
        while time.monotonic()<end:
            try:data,peer=s.recvfrom(256)
            except socket.timeout:continue
            try:
                p=data.decode('ascii').split();assert len(p)==6 and p[:2]==['TK_DEVICE','1'];identity,version,ip,mac=p[2:];row=known[identity]
                assert re.fullmatch('v[1-9][0-9]*',version) and str(ipaddress.IPv4Address(ip))==peer[0]
                expected=hmac.new(bytes.fromhex(row['token']),f'{identity} {ip} {nonces[identity]}'.encode(),hashlib.sha256).hexdigest()
                if hmac.compare_digest(mac,expected):found[identity]={**row,'ip':ip}
            except (UnicodeError,ValueError,AssertionError,KeyError):continue
    return list(found.values())
class WiFiConnection:
    def __init__(self,host,token,port=80):
        ipaddress.IPv4Address(host)
        if not re.fullmatch('[0-9a-f]{32}',token):raise OSError('Invalid paired device token')
        self.host=host;self.endpoint=host+(':'+str(port) if port!=80 else '');self.token=token;self.port='Wi-Fi '+host;self.closed=False;self.lines=collections.deque()
        self.opener=urllib.request.build_opener(urllib.request.ProxyHandler({}))
    @classmethod
    def open(cls,host=None,path=CACHE):
        rows=load_devices(path)
        if host:rows=[r for r in rows if r['ip']==host]
        live=discover(rows)
        if not live:live=[r for r in rows if r['ip']]
        if not live:raise OSError('No paired Wi-Fi Passport found; configure once via USB/Bluetooth')
        if len(live)>1:raise OSError('Multiple paired Wi-Fi devices; select --wifi-host IPv4')
        for row in live:
            c=cls(row['ip'],row['token'])
            try:
                c.write(b'CT_HELLO 4\n')
                if c.readline().strip()!=b'CT_READY 4':raise OSError('Wi-Fi handshake failed')
                return c
            except OSError:c.close()
        raise OSError('Paired Wi-Fi Passport unavailable')
    def write(self,data):
        if self.closed:raise OSError('Wi-Fi link closed')
        if not data or len(data)>1024 or b'\n' in data.rstrip(b'\r\n') or b'\r' in data.rstrip(b'\r\n'):raise OSError('One bounded command required')
        request=urllib.request.Request('http://'+self.endpoint+'/api/rpc',data=data,headers={'Authorization':'Bearer '+self.token,'Content-Type':'text/plain'},method='POST')
        try:
            with self.opener.open(request,timeout=3) as response:reply=response.read(257)
            if not reply or len(reply)>256:raise OSError('Invalid Wi-Fi reply')
            self.lines.append(reply)
        except (OSError,ValueError) as error:raise OSError('Wi-Fi device did not respond') from error
        return len(data)
    def readline(self,size=256):return self.lines.popleft()[:size] if self.lines else b''
    def reset_input_buffer(self):self.lines.clear()
    def close(self):self.closed=True;self.lines.clear()
