"""Virtual device controls; local mock peripheral only, never opens a real device."""
from pathlib import Path
import sys,runpy,threading,json,hashlib
from http.server import ThreadingHTTPServer
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from postcard_server import Handler
from playwright.sync_api import sync_playwright,expect

def main():
    out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
    repo=Path(__file__).resolve().parents[1]
    mock=runpy.run_path(str(repo/'tests/test_postcard_browser.py'),run_name='tests')['MOCK']
    server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
    with sync_playwright() as p:
        browser=p.chromium.launch(executable_path=sys.argv[2],headless=True)
        page=browser.new_page(viewport={'width':1280,'height':980});errors=[];page.on('pageerror',lambda e:errors.append(str(e)));page.add_init_script(mock)
        page.clock.install();page.goto(f'http://127.0.0.1:{server.server_port}/');expect(page.locator('#fields')).to_be_enabled()
        expect(page.locator('#experiencePage')).to_be_visible();expect(page.locator('#upload')).to_be_disabled()
        assert page.locator('.sidebar nav button').all_text_contents()==['体验模拟','页面管理','系统检测','明信片','设置','关于']
        # Exercise the same event model at deterministic boundary timestamps.
        assert page.evaluate('''async()=>{
          const {ExperienceModel:M}=await import('./simulator.js');const c={mask:15,order:[0,1,2,3],volume:35,idle:1,startup:false};let m=new M(c);
          m.press('ok',0);m.release('ok',10);m.press('ok',100);m.release('ok',110);if(m.page!==1||m.details)return false;
          m.reset(c,0);m.press('ok',0);m.tick(501);m.release('ok',502);m.tick(900);if(m.details)return false;
          m.reset(c,0);m.press('up',0);m.tick(499);if(m.overlay||m.volume!==35)return false;m.tick(500);m.tick(680);m.release('up',700);if(m.volume!==45||m.period!==0||!m.overlay)return false;
          m.press('down',800);m.release('down',801);if(m.volume!==40||m.period!==0)return false;
          m.press('ok',900);m.release('ok',901);m.press('ok',1000);m.release('ok',1001);m.tick(1450);if(m.page||m.details||m.overlay)return false;
          m.reset({...c,idle:0},0);m.press('down',0);m.tick(500);m.release('down',501);m.tick(3500);if(!m.overlay)return false;m.tick(3501);if(m.overlay)return false;
          m.reset(c,0);m.tick(1000);if(m.awake)return false;m.press('down',1100);m.tick(2000);m.release('down',2001);if(!m.awake||m.volume!==35||m.period!==0||m.overlay)return false;
          m.reset({...c,volume:100},0);m.press('up',0);m.tick(500);m.tick(1000);if(m.volume!==100)return false;m.reset({...c,volume:0},0);m.press('down',0);m.tick(500);if(m.volume!==0)return false;
          m.sync({...c,mask:8,order:[3,2,1,0]});if(m.page!==3)return false;m.press('ok',1100);m.release('ok',1101);m.press('ok',1200);m.release('ok',1201);if(m.page!==3)return false;
          return true;
        }''')
        # Real pointer single/double/hold, with the UI's actual 50ms timer.
        screen=page.locator('#simScreen')
        page.locator('#simOk').click();page.clock.run_for(400);expect(screen).to_have_attribute('data-details','true')
        page.locator('#simOk').dblclick(delay=80);expect(screen).to_have_attribute('data-page','1');expect(screen).to_have_attribute('data-details','true')
        page.locator('#simDown').click();expect(screen).to_have_attribute('data-period','1')
        page.locator('#simReset').click();box=page.locator('#simUp').bounding_box();page.mouse.move(box['x']+box['width']/2,box['y']+box['height']/2);page.mouse.down();page.clock.run_for(1100);page.mouse.up()
        expect(screen).to_have_attribute('data-overlay','true');assert int(screen.get_attribute('data-volume'))>35;expect(screen).to_have_attribute('data-period','0')
        page.screenshot(path=str(out/'experience-volume.png'),full_page=True)
        page.locator('#simOk').dblclick(delay=70);page.clock.run_for(400);expect(screen).to_have_attribute('data-page','0');expect(screen).to_have_attribute('data-details','false');expect(screen).to_have_attribute('data-overlay','false')
        # Power and first wake have no accidental navigation/volume change.
        b=page.locator('#simPower').bounding_box();page.mouse.move(b['x']+b['width']/2,b['y']+b['height']/2);page.mouse.down();page.clock.run_for(2100);page.mouse.up();expect(screen).to_have_attribute('data-on','false');page.locator('#simDown').click();expect(screen).to_have_attribute('data-period','0')
        page.mouse.move(b['x']+b['width']/2,b['y']+b['height']/2);page.mouse.down();page.clock.run_for(600);page.mouse.up();page.clock.run_for(3100);expect(screen).to_have_attribute('data-on','true')
        # Mock real settings read changes simulator draft values, while simulated changes never write.
        page.locator('#usb').click();page.locator('#settingsNav').click();expect(page.locator('#settingsFields')).to_be_enabled()
        page.locator('#idleTime').select_option('30');page.locator('#soundVolume').fill('70');page.locator('#doneTone').select_option('4');page.locator('#quotaTone').select_option('7');page.locator('#startupSound').uncheck()
        page.locator('#experienceNav').click();expect(screen).to_have_attribute('data-volume','70')
        page.clock.run_for(30100);expect(screen).to_have_attribute('data-awake','false')
        page.locator('#simDown').click();expect(screen).to_have_attribute('data-awake','true');expect(screen).to_have_attribute('data-volume','70');expect(screen).to_have_attribute('data-period','0')
        page.clock.run_for(500);page.locator('#simDevice').focus();page.keyboard.press('ArrowDown');expect(screen).to_have_attribute('data-period','1')
        assert page.evaluate('window.testConfig.values')==[0,1,3,35] and page.evaluate('window.testConfig.sounds')==[]
        # Saved page order and unsaved module/postcard drafts are shared with the simulator.
        page.evaluate('window.testPages=[12,3,2,1,0]');page.locator('#disconnect').click();page.locator('#usb').click()
        page.locator('#postcardNav').click();page.locator('#title').fill('春夏秋冬');page.locator('#subtitle').fill('把日常带在身边')
        page.locator('#experienceNav').click();page.locator('#simReset').click();expect(screen).to_have_attribute('data-page','3')
        page.locator('#simOk').click();page.clock.run_for(400);expect(screen).to_have_attribute('data-contact','true')
        page.screenshot(path=str(out/'experience-postcard-contact.png'),full_page=True)
        page.locator('#simOk').dblclick(delay=80);expect(screen).to_have_attribute('data-page','2')
        before=screen.evaluate('(c)=>c.toDataURL()');page.locator('#monitorNav').click();page.locator('#monReset').click()
        for i in range(1,7):page.get_by_role('switch',name=f'第{i}张卡片开启',exact=True).uncheck()
        page.locator('#experienceNav').click();after=screen.evaluate('(c)=>c.toDataURL()');assert after!=before
        page.locator('#simOk').dblclick(delay=80);expect(screen).to_have_attribute('data-page','3')
        assert page.evaluate('window.testCard.saved') is False and page.evaluate('window.testConfig.sounds')==[]
        # Supplied recordings really decode in the browser. No generated oscillator/tone.
        durations=page.evaluate('''async()=>{const ctx=new AudioContext(),out={};for(const file of ['codex-notification.wav','codex-error.wav','opencode-complete.wav','opencode-error.wav','antigravity-complete.wav','antigravity-error.wav','startup.wav']){const r=await fetch('./sim-audio/'+file);out[file]=(await ctx.decodeAudioData(await r.arrayBuffer())).duration;}await ctx.close();return out;}''')
        assert all(d>0 for d in durations.values())
        page.locator('#simReset').click();page.screenshot(path=str(out/'experience-desktop.png'),full_page=True)
        # A cancelled pointer/blur must stop continuous volume changes.
        page.locator('#simDevice').focus();page.keyboard.down('ArrowUp');page.clock.run_for(600);page.evaluate("window.dispatchEvent(new Event('blur'))");frozen=screen.get_attribute('data-volume');page.clock.run_for(600);assert screen.get_attribute('data-volume')==frozen;page.keyboard.up('ArrowUp')
        page.locator('#simReset').click()
        for width in [390,320]:
            page.set_viewport_size({'width':width,'height':844});page.evaluate('scrollTo(0,0)');page.clock.run_for(100);assert page.evaluate('document.documentElement.scrollWidth<=innerWidth'),width
            for key in ['simPower','simUp','simDown','simOk']:
                b=page.locator('#'+key).bounding_box();assert b['x']>=0 and b['x']+b['width']<=width,(key,b,width)
            page.screenshot(path=str(out/f'experience-mobile-{width}.png'),full_page=True)
        # Touch gestures operate the same buttons without synthetic mouse clicks doubling actions.
        touch=browser.new_context(viewport={'width':390,'height':844},has_touch=True,is_mobile=True);tp=touch.new_page();tp.clock.install();tp.goto(f'http://127.0.0.1:{server.server_port}/');expect(tp.locator('#fields')).to_be_enabled();tp.locator('#simDown').tap();expect(tp.locator('#simScreen')).to_have_attribute('data-period','1');touch.close()
        assert not errors,errors
        result={'modelBoundaries':'PASS','pointerClickDoubleHold':'PASS','wakeConsumption':'PASS','power':'PASS','keyboardAndTouch':'PASS','draftPageCardPostcardSettings':'PASS','noDeviceWritesOrSounds':'PASS','audioDecodedSeconds':durations,'320And390Layout':'PASS','realHardware':'NOT APPLICABLE - web only'}
        (out/'verification.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result));browser.close()
    server.shutdown()
if __name__=='__main__':main()
