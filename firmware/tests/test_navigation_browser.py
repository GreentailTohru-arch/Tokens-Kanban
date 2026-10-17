"""Draft tracking and close protection with simulated peripherals; no real device access."""
from browser_navigation import navigate
from pathlib import Path
import sys,runpy,threading,json
from http.server import ThreadingHTTPServer
from playwright.sync_api import sync_playwright,expect,Error
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from postcard_server import Handler

def main():
 repo=Path(__file__).resolve().parents[1];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
 mock=runpy.run_path(str(repo/'tests/test_postcard_browser.py'),run_name='tests')['MOCK']
 for filename in ['test_management_browser.py','test_device_info_browser.py']:
  injected=(repo/'tests'/filename).read_text(encoding='utf-8').split("injected=r'''",1)[1].split("'''",1)[0].replace('decode(v[3])','unpack(v[3])')
  mock=mock.replace("   if(v[0]==='PC_HELLO')",injected+"   if(v[0]==='PC_HELLO')")
 mock="window.unpack=hex=>{const b=hex.match(/../g).map(s=>parseInt(s,16)),o=[];for(let i=0;i<b.length;){const t=b[i++],n=(t&127)+1;if(t&128){const a=b[i++],c=b[i++];for(let j=0;j<n;j++)o.push(a,c);}else{o.push(...b.slice(i,i+n*2));i+=n*2;}}return o;};\n"+mock
 server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
 with sync_playwright() as p:
  browser=p.chromium.launch(executable_path=sys.argv[2],headless=True);page=browser.new_page(viewport={'width':1280,'height':960});errors=[];page.on('pageerror',lambda e:errors.append(str(e)));page.add_init_script(mock);url=f'http://127.0.0.1:{server.server_port}/';page.route('https://api.github.com/**',lambda r:r.fulfill(status=503,json={}));page.goto(url);expect(page.locator('#versionStatus')).to_have_attribute('data-state','error');page.locator('#status .notice-close').click();page.locator('#status').wait_for(state='hidden')
  guarded=lambda:page.evaluate("(()=>{const e=new Event('beforeunload',{cancelable:true});dispatchEvent(e);return e.defaultPrevented;})()")
  expect(page.locator('#fields')).to_be_enabled()
  def guarded():return page.evaluate("(()=>{const e=new Event('beforeunload',{cancelable:true});dispatchEvent(e);return e.defaultPrevented;})()")
  expect(page.locator('#menuToggle')).to_have_attribute('aria-expanded','true')
  page.evaluate('() => Promise.all(document.querySelector(".app-shell").getAnimations().map(a=>a.finished))')
  before=page.locator('#pageContent').bounding_box();page.locator('#menuToggle').click();expect(page.locator('#pageSidebar')).not_to_be_visible();page.wait_for_timeout(250);after=page.locator('#pageContent').bounding_box();assert after['width']>before['width']+150
  page.locator('#menuToggle').click();expect(page.locator('#pageSidebar')).to_be_visible();assert not guarded()
  for width in [760,480,390,320]:
   page.set_viewport_size({'width':width,'height':844});page.wait_for_timeout(250)
   if page.locator('#menuToggle').get_attribute('aria-expanded')=='true':page.locator('#menuToggle').click()
   expect(page.locator('#pageSidebar')).not_to_be_visible()
   boxes=[page.locator(q).bounding_box() for q in ['#menuToggle','#ble','#usb','#disconnect','#upload','#connection']]
   centers=[b['y']+b['height']/2 for b in boxes];assert max(centers)-min(centers)<2,boxes
   assert boxes[-1]['x']+boxes[-1]['width']<=width
   assert page.locator('#ble .button-label').evaluate('(e)=>getComputedStyle(e).display')=='none'
   assert page.locator('#ble').get_attribute('aria-label')=='搜索蓝牙设备'
   page.locator('#menuToggle').click();expect(page.locator('#sidebarBackdrop')).to_be_visible();assert page.locator('#pageContent').evaluate('(e)=>e.inert')
   assert page.locator('.sidebar nav').evaluate('(e)=>getComputedStyle(e).flexDirection')=='column'
   assert page.locator('.sidebar nav button').count()==8
   expect(page.locator('#aboutNav')).to_be_visible();page.locator('[data-web-mode-option=dark]').focus();page.keyboard.press('Tab');assert page.locator('#menuToggle').evaluate('(e)=>e===document.activeElement'),page.evaluate('document.activeElement.outerHTML');page.keyboard.press('Shift+Tab');assert page.locator('[data-web-mode-option=dark]').evaluate('(e)=>e===document.activeElement'),page.evaluate('document.activeElement.outerHTML')
   page.evaluate('() => Promise.all(document.querySelector(".sidebar").getAnimations().map(a=>a.finished))');page.screenshot(path=str(out/f'side-panel-{width}.png'))
   page.keyboard.press('Escape');expect(page.locator('#menuToggle')).to_have_attribute('aria-expanded','false');assert not page.locator('#pageContent').evaluate('(e)=>e.inert')
   page.locator('#menuToggle').click();page.locator('#sidebarBackdrop').click(position={'x':width-4,'y':200});expect(page.locator('#menuToggle')).to_have_attribute('aria-expanded','false')
   for target in ['experience','pages','monitor','postcard','settings','theme','device','about']:
    navigate(page,target+'Nav');expect(page.locator('#'+target+'Page')).to_be_visible();expect(page.locator('#menuToggle')).to_have_attribute('aria-expanded','false');assert page.evaluate('document.documentElement.scrollWidth<=innerWidth')
   page.evaluate('() => Promise.all(document.querySelector(".sidebar").getAnimations().map(a=>a.finished))');page.evaluate('scrollTo(0,0)');page.screenshot(path=str(out/f'single-toolbar-{width}.png'));assert not guarded()
  page.locator('#usb').click();navigate(page,'settingsNav');page.locator('#soundVolume').fill('70');expect(page.locator('#draftNotice')).to_be_visible()
  boxes=[page.locator(q).bounding_box() for q in ['#menuToggle','#ble','#usb','#disconnect','#upload','#saveAll','#connection']];assert max(b['y']+b['height']/2 for b in boxes)-min(b['y']+b['height']/2 for b in boxes)<2;assert page.evaluate('document.documentElement.scrollWidth<=innerWidth');assert guarded()
  page.locator('#saveAll').click();expect(page.locator('#draftNotice')).to_be_hidden();assert not guarded();page.wait_for_timeout(300);page.screenshot(path=str(out/'saved-toolbar-mobile.png'))
  assert not errors,errors
  result={'alwaysSingleRow320to1280':'PASS','narrowIconsWithAccessibleNames':'PASS','desktopCollapse':'PASS','eightGroupedPagesAndAutoClose':'PASS','escapeBackdropFocusTrap':'PASS','dirtySaveStillWorks':'PASS','hardware':'mock only'};(out/'navigation-verification.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result));browser.close()
 server.shutdown()
if __name__=='__main__':main()
