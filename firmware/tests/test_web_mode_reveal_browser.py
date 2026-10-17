"""Verify theme-reveal origin, navigation safety and fallback without a device."""
from pathlib import Path
import sys,threading,json
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from postcard_server import Handler
from http.server import ThreadingHTTPServer
from playwright.sync_api import sync_playwright,expect
out=Path(sys.argv[1]);out.mkdir(exist_ok=True)
s=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=s.serve_forever,daemon=True).start()
with sync_playwright() as p:
 b=p.chromium.launch(executable_path=sys.argv[2],headless=True);ctx=b.new_context(viewport={'width':1280,'height':900},color_scheme='light');ctx.route('https://api.github.com/**',lambda r:r.fulfill(status=503,json={}));page=ctx.new_page();errors=[];page.on('pageerror',lambda e:errors.append(str(e)));url=f'http://127.0.0.1:{s.server_port}/';page.goto(url);page.wait_for_timeout(1200)
 if page.locator('#status').is_visible():page.locator('#status .notice-close').click();page.wait_for_timeout(250)
 assert page.locator('.web-mode-copy').inner_text()=='外观模式'
 palette=page.evaluate('DeviceTheme.values()')
 page.evaluate('''() => {const original=Element.prototype.animate;Element.prototype.animate=function(keys,options){const a=original.call(this,keys,options);if(options?.pseudoElement==='::view-transition-new(root)'){window.reveal={keys,options};window.revealAnimation=a;if(window.pauseReveal){a.pause();a.currentTime=180;}}return a;};window.pauseReveal=true;}''')
 for width in [1280,390]:
  page.set_viewport_size({'width':width,'height':900});page.wait_for_timeout(300)
  if width==390:page.locator('#menuToggle').click();page.wait_for_timeout(300)
  for mode in ['dark','light']:
   page.evaluate('window.reveal=null');rect=page.locator('#webModeToggle').bounding_box();page.locator('#webModeToggle').click();page.wait_for_function('()=>window.reveal!==null');expect(page.locator('html')).to_have_attribute('data-web-mode',mode)
   info=page.evaluate('reveal');point=f"at {rect['x']+rect['width']/2:g}px {rect['y']+rect['height']/2:g}px";assert point in info['keys']['clipPath'][0],(point,info)
   page.screenshot(path=str(out/f'{width}-{mode}-mid.png'));page.evaluate('revealAnimation.finish()');page.wait_for_function('()=>!document.documentElement.classList.contains("web-mode-reveal")')
   assert page.evaluate('DeviceTheme.values()')==palette;expect(page.locator('#draftNotice')).to_be_hidden()
 page.evaluate('window.pauseReveal=false;document.querySelector("#webModeToggle").click();document.querySelector("#webModeToggle").click();document.querySelector("#webModeToggle").click()');expect(page.locator('html')).to_have_attribute('data-web-mode','dark');page.wait_for_function('()=>!document.documentElement.classList.contains("web-mode-reveal")');page.reload();expect(page.locator('html')).to_have_attribute('data-web-mode','dark')
 page.set_viewport_size({'width':1280,'height':900});page.wait_for_timeout(300);page.emulate_media(reduced_motion='reduce');page.evaluate('window.reveal=null');page.locator('#webModeToggle').focus();page.keyboard.press('Enter');expect(page.locator('html')).to_have_attribute('data-web-mode','light');assert page.evaluate('window.reveal===null')
 page.emulate_media(reduced_motion='no-preference');page.evaluate('document.startViewTransition=undefined');page.locator('#webModeToggle').click();expect(page.locator('html')).to_have_attribute('data-web-mode','dark');assert not errors,errors
 print('Origin, both directions, mobile, rapid clicks, persistence, reduced motion, unsupported browser fallback: PASS');b.close()
s.shutdown()

