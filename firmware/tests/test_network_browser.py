"""Browser and local HTTP transport integration; all device/network data are fixtures."""
import base64,io,json,runpy,sys,threading
from pathlib import Path
from http.server import ThreadingHTTPServer
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from postcard_server import Handler
from playwright.sync_api import sync_playwright,expect
from PIL import Image
import zxingcpp
TOKEN='01'*16
class Local(Handler):
 def do_GET(self):
  if self.path=='/':
   text=(Path(__file__).resolve().parents[1]/'companion/postcard/index.html').read_text(encoding='utf-8').replace('</head>',f'<meta name="passport-device" content="1"><meta name="passport-token" content="{TOKEN}"></head>');data=text.encode();self.send_response(200);self.send_header('Content-Type','text/html');self.end_headers();self.wfile.write(data)
  else:super().do_GET()
def main():
 out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);server=ThreadingHTTPServer(('127.0.0.1',0),Local);threading.Thread(target=server.serve_forever,daemon=True).start()
 with sync_playwright() as p:
  browser=p.chromium.launch(executable_path=sys.argv[2],headless=True);page=browser.new_page(viewport={'width':1280,'height':940});errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  state={'connected':True,'revision':1,'enabled':1,'configured':1,'ssid':'Test WiFi','ap':0,'commands':[],'request':0,'net':3}
  def info(route):route.fulfill(json={'connected':state['connected'],'revision':state['revision'],'kind':'电脑同步 · USB'})
  def rpc(route):
   assert route.request.headers['authorization']=='Bearer '+TOKEN
   line=route.request.post_data;state['commands'].append(line);v=line.split();reply={'PC_HELLO':'PC_READY 1','PC_CLOCK':'PC_CLOCK_OK','PC_CFG_GET2':'PC_CFG 2 0 1 3 35 1','PC_DISPLAY_GET':'PC_DISPLAY 1 75','PC_SOUND_BANKS':'PC_SOUND_BANKS 3','PC_PAGES_GET':'PC_PAGES 1 15 0 1 2 3','PC_VERSION':'PC_VERSION v19','PC_WIFI_GET':f'PC_WIFI 1 {state["net"]} {state["enabled"]} {state["configured"]} {state["ap"]} 192.168.1.50 0','PC_WIFI_SSID':'PC_WIFI_SSID '+state['ssid'].encode().hex(),'PC_WIFI_ACCESS':f'PC_WIFI_ACCESS 123456789abc {TOKEN}','PC_WIFI_AP':'PC_WIFI_AP '+b'Tokens-Kanban-ABCD'.hex()+' 0123456789ab','PC_WIFI_STATUS':f'PC_WIFI_STATE {state["request"]} 2'}.get(v[0],'PC_INVALID')
   if v[0] in ('PC_WIFI_SET','PC_WIFI_ENABLE','PC_WIFI_FORGET','PC_WIFI_SETUP'):
    state['request']=int(v[1]);reply=f'PC_WIFI_STATE {v[1]} 2'
    if v[0]=='PC_WIFI_SET':state.update(ssid=bytes.fromhex(v[2]).decode(),configured=1,enabled=1)
    if v[0]=='PC_WIFI_ENABLE':state['enabled']=int(v[2])
    if v[0]=='PC_WIFI_SETUP':state['ap']=1
    if v[0]=='PC_WIFI_FORGET':state.update(ssid='',configured=0,enabled=0)
   route.fulfill(body=reply+'\n',content_type='text/plain')
  page.route('**/api/info',info);page.route('**/api/rpc',rpc);page.goto(f'http://127.0.0.1:{server.server_port}/')
  expect(page.locator('#connection')).to_have_text('电脑同步 · USB已连接');page.locator('#wifi').click();expect(page.locator('#wifiState')).to_contain_text('已连接');expect(page.locator('#wifiSavedSSID')).to_contain_text('Test WiFi')
  qr=Image.open(io.BytesIO(base64.b64decode(page.locator('#wifiQR').evaluate('c=>c.toDataURL()').split(',')[1])));decoded=zxingcpp.read_barcode(qr);assert decoded and decoded.text=='http://192.168.1.50/#'+TOKEN
  page.locator('#wifiSSID').fill('中文无线');page.locator('#wifiPassword').fill('12345678');page.locator('#wifiSave').click();expect(page.locator('#wifiSavedSSID')).to_contain_text('中文无线');expect(page.locator('#wifiPassword')).to_have_value('');assert any(c.startswith('PC_WIFI_SET ') and 'e4b8ad' in c for c in state['commands'])
  page.locator('#wifiSSID').fill('中文'*12);page.locator('#wifiSave').click();expect(page.locator('#status')).to_have_attribute('data-kind','error');assert '最多32字节' in page.locator('#status').inner_text();page.locator('#wifiSSID').fill('')
  page.locator('#wifiSetup').click();expect(page.locator('#wifiAPInfo')).to_be_visible();qr=Image.open(io.BytesIO(base64.b64decode(page.locator('#wifiJoinQR').evaluate('c=>c.toDataURL()').split(',')[1])));assert zxingcpp.read_barcode(qr).text=='WIFI:T:WPA;S:Tokens-Kanban-ABCD;P:0123456789ab;;'
  page.screenshot(path=str(out/'wifi-desktop.png'),full_page=True)
  for width in (390,320):
   page.set_viewport_size({'width':width,'height':844});page.evaluate('scrollTo(0,0)');assert page.evaluate('document.documentElement.scrollWidth<=innerWidth');ys=page.locator('.topbar>button,.toolbar button').evaluate_all('nodes=>nodes.map(n=>n.getBoundingClientRect().y)');assert max(ys)-min(ys)<2;page.screenshot(path=str(out/f'wifi-mobile-{width}.png'),full_page=True)
  state['connected']=False;expect(page.locator('#connection')).to_have_text('未连接',timeout=10000);state.update(connected=True,revision=2);expect(page.locator('#connection')).to_have_text('电脑同步 · USB已连接',timeout=15000)
  page.locator('#disconnect').click();expect(page.locator('#connection')).to_have_text('未连接');page.wait_for_timeout(5500);expect(page.locator('#connection')).to_have_text('未连接');page.locator('#wifi').click();expect(page.locator('#connection')).to_have_text('电脑同步 · USB已连接')
  # Short power operation does not turn off the virtual system or alter its page/volume.
  result=page.evaluate('''async()=>{const {ExperienceModel:M}=await import('./simulator.js');const m=new M({mask:15,order:[0,1,2,3],volume:35,idle:0,startup:false});m.screen(1);if(m.awake||!m.on)return false;m.press('down',2);m.release('down',3);return m.awake&&m.volume===35&&m.period===0;}''');assert result
  assert not errors,errors;result={'autoSharedHTTPConnection':'PASS','networkSaveUTF8AndLimits':'PASS','LANAndWiFiJoinQRDecode':'PASS','disconnectAndAutoReconnect':'PASS','singleRow320390':'PASS','shortPowerSleepWake':'PASS','realWiFiHardware':'NOT RUN'};(out/'network-browser.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));browser.close()
 server.shutdown()
if __name__=='__main__':main()
