"""Layout regression: preserve every control, responsive frames and section navigation.
No device connection or firmware writing is performed.
"""
from pathlib import Path
import sys, threading, json, re
from http.server import ThreadingHTTPServer
from playwright.sync_api import sync_playwright, expect
from browser_navigation import navigate,set_web_mode
R=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(R/'companion'))
from postcard_server import Handler

def main():
 out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
 server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
 with sync_playwright() as p:
  browser=p.chromium.launch(executable_path=sys.argv[2],headless=True)
  page=browser.new_page(viewport={'width':1440,'height':1000});errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  page.route('https://api.github.com/**',lambda r:r.fulfill(status=503,json={}))
  page.goto(f'http://127.0.0.1:{server.server_port}/');assert page.locator('#pageContent').evaluate('(e)=>getComputedStyle(e).containerType')=='inline-size';expect(page.locator('#fields')).to_be_enabled()
  expect(page.locator('#versionStatus')).to_have_attribute('data-state','error')
  navigate(page,'postcardNav');page.locator('#title').fill('保留的明信片草稿')
  navigate(page,'settingsNav');navigate(page,'deviceNav');expect(page.locator('#upload')).to_be_disabled()
  expect(page.locator('#deviceCpu')).to_have_text('--');expect(page.locator('#diagFields')).to_have_attribute('disabled','')
  page.locator('[data-screen-color="5"]').click();expect(page.locator('#screenPatternPreview')).to_have_attribute('data-pattern','5')
  navigate(page,'postcardNav');expect(page.locator('#title')).to_have_value('保留的明信片草稿')
  pages=['experience','pages','monitor','postcard','theme','settings','device','about']
  for width in [1440,1024,768,390,320]:
   page.set_viewport_size({'width':width,'height':1000 if width>760 else 844});page.wait_for_timeout(300)
   for mode in ['light','dark']:
    if page.locator('#menuToggle').get_attribute('aria-expanded')!='true':page.locator('#menuToggle').click()
    set_web_mode(page,mode)
    for name in pages:
     navigate(page,name+'Nav');page.evaluate('() => Promise.all(document.querySelector(".app-shell").getAnimations({subtree:true}).filter(a=>a.effect?.getTiming().iterations!==Infinity).map(a=>a.finished))');expect(page.locator('#'+name+'Page')).to_be_visible()
     assert page.locator('#pageContent>:not([hidden]):is(section,main)').count()==1
     assert page.locator('#'+name+'Nav').get_attribute('aria-current')=='page'
     assert page.evaluate('document.documentElement.scrollWidth<=innerWidth'),(width,mode,name)
     ids=page.locator('[id]').evaluate_all('(es)=>es.map(e=>e.id)');assert len(ids)==len(set(ids))
     if width in [1440,390]:
      page.screenshot(path=str(out/f'{name}-{mode}-{width}.png'),full_page=True,animations='disabled')
   # Section links must land below the persistent toolbar, without changing pages.
   navigate(page,'settingsNav');page.locator('a[href="#soundSettings"]').click()
   y=page.locator('#soundSettings').bounding_box()['y'];bottom=page.locator('.topbar').bounding_box()['height']
   assert y>=bottom-1,(width,y,bottom)
  navigate(page,'experienceNav');page.locator('#simOk').dblclick(delay=70)
  expect(page.locator('#simState')).not_to_contain_text('用量首页')
  navigate(page,'themeNav');expect(page.locator('#themeMachineHost #simDevice')).to_be_visible()
  navigate(page,'experienceNav');expect(page.locator('#experienceStage #simDevice')).to_be_visible()
  assert page.locator('#experiencePage #deviceName').count()==0
  assert page.locator('#devicePage #deviceName').count()==1
  assert not errors,errors
  result={'eightPagesBothModes320to1440':'PASS','draftRetainedAcrossPages':'PASS','deviceActionsSeparatedFromSimulation':'PASS','stickyToolbarSectionLinks':'PASS','interactiveMachineRetained':'PASS','duplicateIds':False,'pageErrors':errors,'hardware':'NOT RUN'}
  (out/'layout-verification.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result));browser.close()
 server.shutdown()
if __name__=='__main__':main()


