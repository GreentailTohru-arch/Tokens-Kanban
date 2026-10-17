"""Supplied brand bytes, day/night favicon, and complete-frame page switching."""
from pathlib import Path
import sys,json,threading,base64,hashlib,re
from http.server import ThreadingHTTPServer
from playwright.sync_api import sync_playwright,expect
from browser_navigation import navigate
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R/'companion'))
from postcard_server import Handler

def main():
 out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
 server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start();url=f'http://127.0.0.1:{server.server_port}/'
 with sync_playwright() as p:
  browser=p.chromium.launch(executable_path=sys.argv[2],headless=True);page=browser.new_page(viewport={'width':1440,'height':960});errors=[];page.on('pageerror',lambda e:errors.append(str(e)));page.route('https://api.github.com/**',lambda r:r.fulfill(status=503,json={}))
  page.add_init_script("""window.partialPaints=[];window.frameCopies=0;for(const name of ['fillRect','fillText','clearRect','stroke','fill']){const orig=CanvasRenderingContext2D.prototype[name];CanvasRenderingContext2D.prototype[name]=function(...a){if(this.canvas.id==='simScreen')window.partialPaints.push(name);return orig.apply(this,a)}}const draw=CanvasRenderingContext2D.prototype.drawImage;CanvasRenderingContext2D.prototype.drawImage=function(...a){if(this.canvas.id==='simScreen'){if(a[0].width!==240||a[0].height!==320)throw Error('Partial frame');window.frameCopies++}return draw.apply(this,a)};""")
  page.goto(url);expect(page.locator('#fields')).to_be_enabled()
  for mode in ['light','dark']:
   page.locator('[data-web-mode-option='+mode+']').click();navigate(page,'aboutNav');expect(page.locator('#brandFavicon')).to_have_attribute('href','brand-'+mode+'.svg');expect(page.locator('.about-mark .brand-'+mode)).to_be_visible();assert page.locator('.about-mark img:visible').count()==1;assert page.locator('.brand-symbol img:visible').count()==1
   for image in page.locator('.brand-image:visible').all():assert image.evaluate('e=>e.complete&&e.naturalWidth>0')
   response=page.request.get(url+'brand-'+mode+'.svg');assert response.ok and response.headers['content-type'].startswith('image/svg+xml');raw=base64.b64decode(re.search('base64,([^" ]+)',response.text())[1]);assert raw.startswith(b'\x89PNG\r\n\x1a\n')
   page.screenshot(path=str(out/(mode+'-desktop.png')),full_page=True)
  navigate(page,'experienceNav');screen=page.locator('#simScreen')
  for target in [1,2,3,0]:
   page.locator('#simOk').dblclick(delay=80);expect(screen).to_have_attribute('data-page',str(target));assert screen.evaluate('e=>e.getAnimations().length')==0;assert page.locator('#experiencePage').evaluate('e=>getComputedStyle(e).animationName')=='none';page.wait_for_timeout(350)
  assert page.evaluate('partialPaints')==[];assert page.evaluate('frameCopies')>4
  page.locator('#simDown').hover();page.mouse.down();page.wait_for_timeout(700);page.mouse.up();expect(screen).to_have_attribute('data-overlay','true');assert page.locator('#simVolumeBar').evaluate('e=>getComputedStyle(e).animationName')=='sim-volume-in';page.locator('#simOk').click()
  page.wait_for_timeout(550);navigate(page,'themeNav');page.locator('#simOk').dblclick(delay=80);expect(screen).to_have_attribute('data-page','1');assert page.evaluate('partialPaints')==[]
  for width in [390,320]:
   page.set_viewport_size({'width':width,'height':844});navigate(page,'aboutNav');assert page.evaluate('document.documentElement.scrollWidth<=innerWidth');page.screenshot(path=str(out/f'about-{width}.png'),full_page=True)
  assert not errors,errors;browser.close()
 result={'lightDarkBrandAndFavicon':'PASS','desktopAndMobile':'PASS','noWebPageReveal':'PASS','fourPagesAtomicFrame':'PASS','themePreviewAtomicFrame':'PASS','volumeSlideRetained':'PASS','physicalRefresh':'NOT RUN'};(out/'verification.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));server.shutdown()
if __name__=='__main__':main()
