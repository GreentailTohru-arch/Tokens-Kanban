"""Browser and local HTTP transport integration; all device/network data are fixtures."""
import json,sys,threading
from pathlib import Path
from http.server import ThreadingHTTPServer
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from postcard_server import Handler
from playwright.sync_api import sync_playwright,expect
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
  state={'connected':True,'revision':1,'commands':[]}
  def info(route):route.fulfill(json={'connected':state['connected'],'revision':state['revision'],'kind':'电脑同步 · USB'})
  def rpc(route):
   assert route.request.headers['authorization']=='Bearer '+TOKEN
   line=route.request.post_data;state['commands'].append(line);v=line.split();reply={'PC_HELLO':'PC_READY 1','PC_CLOCK':'PC_CLOCK_OK','PC_CFG_GET2':'PC_CFG 2 0 1 3 35 1','PC_DISPLAY_GET':'PC_DISPLAY 1 75','PC_SOUND_BANKS':'PC_SOUND_BANKS 3','PC_PAGES_GET':'PC_PAGES 1 15 0 1 2 3','PC_VERSION':'PC_VERSION v20'}.get(v[0],'PC_INVALID');route.fulfill(body=reply+'\n',content_type='text/plain')
  page.route('**/api/info',info);page.route('**/api/rpc',rpc);page.goto(f'http://127.0.0.1:{server.server_port}/')
  expect(page.locator('#connection')).to_have_text('电脑同步 · USB已连接');assert page.locator('#wifi,#wifiForm,.network-panel').count()==0;assert not any(x.startswith('PC_WIFI') for x in state['commands'])
  page.locator('#settingsNav').click();expect(page.locator('#brightness')).to_have_value('75');page.screenshot(path=str(out/'local-desktop.png'),full_page=True)
  for width in (390,320):
   page.set_viewport_size({'width':width,'height':844});page.evaluate('scrollTo(0,0)');assert page.evaluate('document.documentElement.scrollWidth<=innerWidth');ys=page.locator('.topbar>button,.toolbar button').evaluate_all('nodes=>nodes.map(n=>n.getBoundingClientRect().y)');assert max(ys)-min(ys)<2;page.screenshot(path=str(out/f'local-mobile-{width}.png'),full_page=True)
  page.emulate_media(reduced_motion='reduce')
  assert page.evaluate("[...document.querySelectorAll('*')].every(n=>{const s=getComputedStyle(n);return s.animationName==='none'&&s.transitionDuration.split(',').every(v=>parseFloat(v)===0);})"),'Reduced-motion preference not honored'
  page.emulate_media(reduced_motion='no-preference')
  state['connected']=False;expect(page.locator('#connection')).to_have_text('未连接',timeout=10000);state.update(connected=True,revision=2);expect(page.locator('#connection')).to_have_text('电脑同步 · USB已连接',timeout=15000)
  page.locator('#disconnect').click();expect(page.locator('#connection')).to_have_text('未连接');page.wait_for_timeout(5500);expect(page.locator('#connection')).to_have_text('未连接');page.locator('#usb').click();expect(page.locator('#connection')).to_have_text('电脑同步 · USB已连接')
  assert not errors,errors;result={'autoSharedUSBProxy':'PASS','noWiFiControlsOrCommands':'PASS','manualDisconnectAndReconnect':'PASS','singleRow320390':'PASS','realDevice':'NOT RUN'};(out/'local-browser.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));browser.close()
 server.shutdown()
if __name__=='__main__':main()
