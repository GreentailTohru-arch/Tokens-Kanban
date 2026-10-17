"""Automatic release discovery and device comparison; all peripheral/GitHub data mocked."""
from pathlib import Path
import sys, threading, json, runpy, hashlib
from http.server import ThreadingHTTPServer
from playwright.sync_api import sync_playwright, expect
R=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(R/'companion'))
from postcard_server import Handler

def main():
 out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
 table=bytes([255])*3072;sha=hashlib.sha256(table).hexdigest()
 base='https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/download/v18/'
 manifest=dict(version='v18',chip='ESP32-C3',appOffset=65536,appMax=0x730000,appBytes=65536,appSHA256='0'*64,appURL=base+'Tokens-Kanban-v18-app.bin',fullURL=base+'Tokens-Kanban-v18-full.bin',partitionSHA256=sha,partitionHex=table.hex(),allowedPartitions=[sha])
 mock=runpy.run_path(str(R/'tests/test_postcard_browser.py'),run_name='tests')['MOCK']
 mock=mock.replace("   if(v[0]==='PC_HELLO')","   if(v[0]==='PC_VERSION')reply=window.unknownVersion?'PC_INVALID':'PC_VERSION '+(window.deviceVersion||'v17');\n   if(v[0]==='PC_HELLO')")
 server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
 with sync_playwright() as p:
  browser=p.chromium.launch(executable_path=sys.argv[2],headless=True);page=browser.new_page(viewport={'width':1440,'height':1000});page.add_init_script(mock)
  calls=[];errors=[];state={'fail':False,'manifest':True};page.on('pageerror',lambda e:errors.append(str(e)))
  def release(route):
   calls.append(route.request.url)
   route.fulfill(status=503 if state['fail'] else 200,json={'tag_name':'v18','draft':False,'prerelease':False,'assets':[]})
  page.route('https://api.github.com/**',release)
  page.route('https://raw.githubusercontent.com/**',lambda r:r.fulfill(status=200 if state['manifest'] else 404,json=manifest))
  page.goto(f'http://127.0.0.1:{server.server_port}/');expect(page.locator('#versionLabel')).to_have_text('最新 v18');expect(page.locator('#checkUpdate')).to_be_enabled();assert len(calls)==1
  page.locator('#usb').click();expect(page.locator('#versionLabel')).to_have_text('更新至 v18');expect(page.locator('#versionStatus')).to_have_attribute('data-state','update');expect(page.locator('#status')).to_contain_text('发现新版本');assert len(calls)==1
  page.locator('#versionStatus').click();expect(page.locator('#aboutPage')).to_be_visible();expect(page.locator('#installUpdate')).to_be_enabled();expect(page.locator('#updateDialog')).not_to_be_visible()
  for version,text in [('v18','已是最新'),('v20','比公开版本')]:
   page.locator('#disconnect').click();page.evaluate('(v)=>window.deviceVersion=v',version);page.locator('#usb').click();expect(page.locator('#updateState')).to_contain_text(text);expect(page.locator('#versionLabel')).to_have_text(version)
   if version=='v20':expect(page.locator('#installUpdate')).to_be_disabled()
  assert len(calls)==1
  page.locator('#disconnect').click();expect(page.locator('#updateState')).to_contain_text('连接设备后比对');page.evaluate('window.unknownVersion=true');page.locator('#usb').click();expect(page.locator('#aboutDeviceVersion')).to_contain_text('无版本接口');expect(page.locator('#versionLabel')).to_have_text('最新 v18')
  state['fail']=True;page.locator('#checkUpdate').click();expect(page.locator('#versionStatus')).to_have_attribute('data-state','error');expect(page.locator('#updateState')).to_contain_text('无法检查');expect(page.locator('#installUpdate')).to_be_disabled()
  state['fail']=False;state['manifest']=False;page.locator('#checkUpdate').click();expect(page.locator('#updateState')).to_contain_text('尚未提供');expect(page.locator('#versionLabel')).to_have_text('最新 v18');expect(page.locator('#installUpdate')).to_be_disabled()
  state['manifest']=True;page.locator('#checkUpdate').click();expect(page.locator('#installUpdate')).to_be_enabled()
  # Toolbar remains one row even with drafts and a persistent version indicator.
  page.locator('#title').evaluate("e=>{e.value='测试草稿';e.dispatchEvent(new Event('input',{bubbles:true}));}")
  for width in [320,360,390,760,761,1024,1280,1440]:
   page.set_viewport_size({'width':width,'height':900});page.wait_for_timeout(300)
   assert page.evaluate('document.documentElement.scrollWidth<=innerWidth'),width
   page.locator('#title').evaluate("e=>{e.value='测试草稿';e.dispatchEvent(new Event('input',{bubbles:true}));}")
   bounds=[page.locator('#'+i).bounding_box() for i in ['menuToggle','ble','usb','disconnect','upload','versionStatus','connection','saveAll']]
   assert all(bounds),(width,bounds)
   centers=[b['y']+b['height']/2 for b in bounds];assert max(centers)-min(centers)<5,(width,bounds)
  page.locator('#draftNotice').evaluate('(e)=>e.hidden=true');page.screenshot(path=str(out/'version-desktop.png'),animations='disabled',full_page=True)
  page.set_viewport_size({'width':390,'height':844});page.screenshot(path=str(out/'version-mobile.png'),animations='disabled',full_page=True)
  assert not errors,errors
  result={'startupDiscovery':'PASS','connectedOlderEqualAheadUnknown':'PASS','reconnectReusesResult':'PASS','offlineRetry':'PASS','missingManifestKeepsVersion':'PASS','noAutomaticInstall':'PASS','toolbar320to1440WithDraft':'PASS','hardware':'MOCK ONLY'}
  (out/'verification.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result));browser.close()
 server.shutdown()
if __name__=='__main__':main()
