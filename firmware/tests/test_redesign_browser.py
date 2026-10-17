from pathlib import Path
import sys,runpy,threading,json
from http.server import ThreadingHTTPServer
from playwright.sync_api import sync_playwright,expect
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R/'companion'));from postcard_server import Handler
from browser_navigation import navigate

def main():
 out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
 mock=runpy.run_path(str(R/'tests/test_postcard_browser.py'),run_name='tests')['MOCK']
 server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
 with sync_playwright() as p:
  browser=p.chromium.launch(executable_path=sys.argv[2],headless=True)
  page=browser.new_page(viewport={'width':1440,'height':1000});page.add_init_script(mock);errors=[];page.on('pageerror',lambda e:errors.append(str(e)));page.goto(f'http://127.0.0.1:{server.server_port}/');expect(page.locator('#fields')).to_be_enabled();page.wait_for_timeout(350)
  expected=['体验模拟','页面管理','系统检测','明信片','设置','主题','关于'];assert page.locator('.sidebar nav button').all_text_contents()==expected
  for name in ['experience','pages','monitor','postcard','settings','theme','about']:
   navigate(page,name+'Nav');page.wait_for_timeout(320);assert page.evaluate('document.documentElement.scrollWidth<=innerWidth'),name;expect(page.locator('#'+name+'Page')).to_be_visible();page.screenshot(animations='disabled',path=str(out/(name+'-desktop.png')),full_page=True)
  # Real notice manager with a deterministic browser clock; no device mutation.
  page.clock.install();page.evaluate("async()=>{const {NoticeCenter}=await import('./notices.js?v=atelier-20');window.testNotices=new NoticeCenter(document.querySelector('#status'));}")
  def show(kind='success',text='设置已保存到设备'):
   page.mouse.move(0,900);page.locator('#menuToggle').focus()
   page.evaluate('([text,kind])=>window.testNotices.show(text,kind)',[text,kind]);page.clock.run_for(250)
  show();expect(page.locator('#status')).to_be_visible();page.clock.run_for(1850);expect(page.locator('#status')).to_be_visible();page.clock.run_for(300);expect(page.locator('#status')).to_be_hidden()
  show('error','连接失败，请重试');page.clock.run_for(3300);expect(page.locator('#status')).to_be_visible();page.clock.run_for(900);expect(page.locator('#status')).to_be_hidden()
  show();page.locator('#status .notice-close').click();page.clock.run_for(200);expect(page.locator('#status')).to_be_hidden()
  show();page.locator('#status .notice-close').focus();page.clock.run_for(6000);expect(page.locator('#status')).to_be_visible();page.locator('#status .notice-close').press('Escape');page.clock.run_for(200);expect(page.locator('#status')).to_be_hidden()
  # Swipe away, tiny drag snapback, cancellation, and replacing a dismissed message.
  show();box=page.locator('#status').bounding_box();x=box['x']+25;y=box['y']+box['height']/2;page.mouse.move(x,y);page.mouse.down();page.mouse.move(x+85,y,steps=6);page.mouse.up();page.clock.run_for(200);expect(page.locator('#status')).to_be_hidden()
  show();box=page.locator('#status').bounding_box();x=box['x']+25;y=box['y']+box['height']/2;page.mouse.move(x,y);page.mouse.down();page.mouse.move(x+12,y,steps=2);page.mouse.up();page.clock.run_for(200);expect(page.locator('#status')).to_be_visible();page.mouse.move(0,900);page.clock.run_for(2400);expect(page.locator('#status')).to_be_hidden()
  show();page.evaluate("window.testNotices.dismiss();window.testNotices.show('新的消息')");page.clock.run_for(200);expect(page.locator('#status')).to_contain_text('新的消息')
  for width in [1280,1024,800,761,760,390,320]:
   page.set_viewport_size({'width':width,'height':900});page.clock.run_for(350)
   for name in ['experience','pages','monitor','postcard','settings','theme','about']:
    navigate(page,name+'Nav');page.clock.run_for(350);assert page.evaluate('document.documentElement.scrollWidth<=innerWidth'),(width,name)
    if width in [390,320]:page.screenshot(animations='disabled',path=str(out/(name+'-mobile-'+str(width)+'.png')),full_page=True)
   show('error','连接失败，请稍后再试。');ys=page.locator('.toolbar button,#menuToggle').evaluate_all('es=>es.map(e=>e.getBoundingClientRect().y)');assert max(ys)-min(ys)<2
   b=page.locator('#status').bounding_box();assert b['x']>=0 and b['x']+b['width']<=width
   if width<=760:assert b['y']<=12
   page.screenshot(animations='disabled',path=str(out/('notice-'+str(width)+'.png')))
   page.evaluate('window.testNotices.dismiss()');page.clock.run_for(200)
  navigate(page,'experienceNav');page.clock.run_for(350);page.evaluate('scrollTo(0,1000)');navigate(page,'settingsNav');page.clock.run_for(350);assert page.evaluate('scrollY')==0
  page.emulate_media(reduced_motion='reduce');show();assert page.locator('#status').evaluate("e=>getComputedStyle(e).animationName")=='none'
  touch=browser.new_context(viewport={'width':390,'height':844},is_mobile=True,has_touch=True);tp=touch.new_page();tp.goto(f'http://127.0.0.1:{server.server_port}/');expect(tp.locator('#fields')).to_be_enabled();tp.evaluate("async()=>{const {NoticeCenter}=await import('./notices.js');window.swipeNotice=new NoticeCenter(document.querySelector('#status'));swipeNotice.show('向上滑动关闭');}");tp.wait_for_timeout(250)
  cdp=touch.new_cdp_session(tp);b=tp.locator('#status').bounding_box();x=b['x']+b['width']/2;y=b['y']+b['height']-8
  cdp.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':x,'y':y}]});cdp.send('Input.dispatchTouchEvent',{'type':'touchMove','touchPoints':[{'x':x,'y':max(0,y-45)}]});cdp.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]});expect(tp.locator('#status')).to_be_hidden();touch.close()
  assert not errors,errors
  result={'sevenPagesAndContent':'PASS','allPages320to1440':'PASS','singleRowToolbar':'PASS','success2200msError4200ms':'PASS','closeEscapeSwipeSnapback':'PASS','focusPausesDismissal':'PASS','replacementRace':'PASS','reducedMotion':'PASS','touchSwipeUp':'PASS','pageSwitchScrollReset':'PASS','hardwareWrites':'NONE'};(out/'verification.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));browser.close()
 server.shutdown()
if __name__=='__main__':main()
