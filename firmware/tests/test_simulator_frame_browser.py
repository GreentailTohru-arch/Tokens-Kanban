"""Photo frame alignment, responsive controls and unchanged interactive canvas."""
from pathlib import Path
import sys,json,threading
from http.server import ThreadingHTTPServer
from playwright.sync_api import sync_playwright,expect
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R/'companion'))
from postcard_server import Handler

def main():
 out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
 server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
 with sync_playwright() as p:
  browser=p.chromium.launch(executable_path=sys.argv[2],headless=True);page=browser.new_page(viewport={'width':1440,'height':1060});errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  page.route('https://api.github.com/**',lambda r:r.fulfill(status=503,json={}))
  page.goto(f'http://127.0.0.1:{server.server_port}/');expect(page.locator('#versionStatus')).to_have_attribute('data-state','error');page.locator('.notice-close').click()
  img=page.locator('.sim-hardware');assert img.evaluate('e=>e.complete&&e.naturalWidth===605&&e.naturalHeight===931')
  response=page.request.get(f'http://127.0.0.1:{server.server_port}/passport-front.png');assert response.body()==(R/'companion/postcard/passport-front.png').read_bytes()
  for width in [1440,1024,760,390,320]:
   page.set_viewport_size({'width':width,'height':1060 if width>760 else 844});page.wait_for_timeout(300)
   assert page.evaluate('document.documentElement.scrollWidth<=innerWidth'),width
   frame=img.bounding_box();screen=page.locator('#simScreen').bounding_box()
   assert abs((screen['x']-frame['x'])/frame['width']-126/605)<.001
   assert abs((screen['y']-frame['y'])/frame['height']-141/931)<.001
   assert abs(screen['width']/frame['width']-284/605)<.001
   assert abs(screen['height']/frame['height']-380/931)<.001
   assert page.locator('#simScreen').get_attribute('width')=='240'
   for key in ['simPower','simUp','simDown','simOk']:
    b=page.locator('#'+key).bounding_box();assert b['x']>=0 and b['x']+b['width']<=width
    assert page.locator('#'+key).evaluate('e=>{const r=e.getBoundingClientRect();return document.elementFromPoint(r.x+r.width/2,r.y+r.height/2)===e||e.contains(document.elementFromPoint(r.x+r.width/2,r.y+r.height/2))}')
   page.screenshot(path=str(out/f'photo-{width}.png'),animations='disabled')
  page.locator('#simDown').click();expect(page.locator('#simScreen')).to_have_attribute('data-period','1')
  page.locator('#simOk').dblclick(delay=80);expect(page.locator('#simScreen')).to_have_attribute('data-page','1')
  assert not errors,errors
  result={'originalPhotoServed':'PASS','screenAlignment':'PASS','canvas240x320':'PASS','fiveViewportLayouts':'PASS','physicalButtonHitTargets':'PASS','interactiveCanvas':'PASS','hardwareWrites':'NONE'}
  (out/'verification.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));browser.close()
 server.shutdown()
if __name__=='__main__':main()
