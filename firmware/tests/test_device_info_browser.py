"""Read/write/test workflow against a simulated diagnostic-capable peripheral."""
from browser_navigation import navigate
from pathlib import Path
import sys,threading,runpy,json
from http.server import ThreadingHTTPServer
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from postcard_server import Handler
from playwright.sync_api import sync_playwright,expect

def main():
    repo=Path(__file__).resolve().parents[1];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
    mock=runpy.run_path(str(repo/'tests/test_postcard_browser.py'),run_name='tests')['MOCK']
    injected=r'''
   if(v[0]==='PC_INFO')reply=window.noDiagnostics?'PC_INVALID':'PC_INFO 1';
   if(v[0]==='PC_INFO_SYSTEM')reply='PC_SYSTEM '+(window.freezeDevice?10000:10000+Math.floor(performance.now()))+' 245 160 200000 80000 60000 40000';
   if(v[0]==='PC_INFO_STORAGE')reply='PC_STORAGE 8388608 7798784 524288 12 500';
   if(v[0]==='PC_INFO_BATTERY')reply=window.noBattery?'PC_BATTERY -1 -1 500':'PC_BATTERY 86 4050 500';
   if(v[0]==='PC_INFO_CHIP')reply=`PC_CHIP ESP32-C3 1 101 98c377f4deb0 ${window.testELFPrefix||'e0e2b3b00'} v17`;
   if(v[0]==='PC_INFO_KEYS')reply='PC_KEYS 3300 '+(window.keyCounts||[0,0,0]).join(' ')+' 0 0';
   if(v[0]==='PC_INFO_NAME')reply='PC_NAME '+Array.from(new TextEncoder().encode(window.deviceName||"You's AI Passport"),b=>b.toString(16).padStart(2,'0')).join('');
   if(v[0]==='PC_NAME_BLE')reply=window.legacyName?'PC_INFO_INVALID':'PC_NAME_BLE 1 29';
   if(v[0]==='PC_NAME_SET'){window.nameId=+v[1];window.nameState=window.failName?3:2;if(!window.failName)window.deviceName=new TextDecoder().decode(Uint8Array.from(v[2].match(/../g),s=>parseInt(s,16)));reply=`PC_NAME_STATE ${window.nameId} 1`;}
   if(v[0]==='PC_NAME_STATE')reply=`PC_NAME_STATE ${window.nameId} ${window.nameState}`;
   if(v[0]==='PC_TEST'){window.testId=+v[1];window.testKind=+v[2];window.testCalls=(window.testCalls||[]).concat(+v[2]);reply=`PC_TEST_STATE ${v[1]} 1 ${v[2]} -1 -1 0`;}
   if(v[0]==='PC_TEST_STATE')reply=`PC_TEST_STATE ${window.testId} ${window.failTest?3:2} ${window.testKind} 512 128 6400`;
   if(v[0]==='PC_SCREEN'){window.screenCalls=(window.screenCalls||[]).concat(+v[1]);reply='PC_SCREEN_OK';}
'''
    mock=mock.replace("   if(v[0]==='PC_HELLO')",injected+"   if(v[0]==='PC_HELLO')")
    server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
    with sync_playwright() as p:
        browser=p.chromium.launch(executable_path=sys.argv[2],headless=True);page=browser.new_page(viewport={'width':1280,'height':980});page.route('https://api.github.com/**',lambda r:r.fulfill(status=200,json={'tag_name':'v22','name':'Tokens Kanban v22','assets':[],'html_url':'https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/tag/v22'}));errors=[];page.on('pageerror',lambda e:errors.append(str(e)));page.add_init_script(mock);page.clock.install();page.goto(f'http://127.0.0.1:{server.server_port}/')
        expect(page.locator('#fields')).to_be_enabled();expect(page.locator('#deviceName')).to_have_value("You's AI Passport");expect(page.locator('#deviceCpu')).to_have_text('--');expect(page.locator('#diagFields')).to_have_attribute('disabled','')
        page.clock.run_for(1000)
        if page.locator('#status').is_visible():
            page.locator('#status .notice-close').click();page.clock.run_for(500)
        navigate(page,'deviceNav')
        expect(page.locator('[data-screen-color="6"]')).to_be_disabled();assert page.locator('#screenPatternPreview').count()==0;assert page.evaluate('window.screenCalls||[]')==[]
        assert page.evaluate('window.testCalls||[]')==[]
        page.locator('#usb').click();expect(page.locator('#deviceConnectionState')).to_contain_text('设备实时')
        expect(page.locator('#deviceCpu')).to_have_text('24.5%');expect(page.locator('#deviceFrequency')).to_have_text('160 MHz');expect(page.locator('#deviceFlash')).to_have_text('8 MiB');expect(page.locator('#deviceHeap')).to_have_text('60.0%');expect(page.locator('#deviceVoltage')).to_have_text('4.050 V');expect(page.locator('#deviceCapacity')).to_contain_text('额定值')
        page.locator('#deviceName').fill('我的 AI Passport');page.locator('#deviceNameSave').click();expect(page.locator('#deviceNameHint')).to_contain_text('已读取');assert page.evaluate('window.deviceName')=='我的 AI Passport'
        expect(page.locator('#deviceNameHint')).to_contain_text('同步蓝牙名称')
        page.locator('#deviceName').fill('一二三四五六七八九十');page.locator('#deviceNameSave').click();expect(page.locator('#deviceNameHint')).to_contain_text('缩短显示')
        page.evaluate('window.legacyName=true');page.locator('#deviceName').fill('我的 AI Passport');page.locator('#deviceNameSave').click();expect(page.locator('#deviceNameHint')).to_contain_text('当前固件仅修改设备名称');page.evaluate('window.legacyName=false')
        page.evaluate('window.failName=true');page.locator('#deviceName').fill('失败的名称');page.locator('#deviceNameSave').click();expect(page.locator('#deviceNameHint')).to_contain_text('保存未确认');assert page.evaluate('window.deviceName')=='我的 AI Passport';page.evaluate('window.failName=false')
        page.locator('#deviceName').fill('一'*11);expect(page.locator('#deviceNameError')).to_contain_text('32');assert not page.locator('#deviceName').evaluate('(e)=>e.checkValidity()')
        page.locator('#deviceName').fill('我的 AI Passport')
        # Periodic polls and a user command run concurrently through the actual RPC queue.
        page.clock.run_for(2200);page.locator('#diagBattery').click();page.clock.run_for(200);expect(page.locator('#diagBatteryState')).to_have_text('电量计读取成功')
        page.locator('#diagMic').click();page.clock.run_for(200);expect(page.locator('#diagMicState')).to_contain_text('6400 采样')
        page.locator('#diagSpeaker').click();page.clock.run_for(200);expect(page.locator('#diagSpeakerState')).to_contain_text('以实机听感确认')
        assert page.evaluate('window.testCalls')==[1,2,3]
        page.evaluate('window.failTest=true');page.locator('#diagMic').click();page.clock.run_for(200);expect(page.locator('#diagMicState')).to_contain_text('测试失败');page.evaluate('window.failTest=false')
        page.get_by_role('button',name='实机红色测试',exact=True).click();expect(page.locator('#diagScreenState')).to_contain_text('15秒');page.get_by_role('button',name='结束测试',exact=True).click();expect(page.locator('#diagScreenState')).to_have_text('已结束屏幕测试');assert page.evaluate('window.screenCalls')==[1,-1]
        for pattern in [5,6]:
            page.locator(f'[data-screen-color="{pattern}"]').click();expect(page.locator('#diagScreenState')).to_contain_text('实机显示测试色')
            assert page.evaluate('window.screenCalls')[-1]==pattern
        page.locator('[data-screen-color="-1"]').click()
        page.evaluate('window.keyCounts=[1,0,2]');page.clock.run_for(2200);expect(page.locator('#diagKey0')).to_contain_text('1');expect(page.locator('#diagKey2')).to_contain_text('2')
        page.evaluate('window.noBattery=true');page.clock.run_for(2200);expect(page.locator('#deviceBattery')).to_have_text('--');expect(page.locator('#deviceVoltage')).to_have_text('--');page.evaluate('window.noBattery=false')
        page.evaluate('window.freezeDevice=true');page.clock.run_for(11000);expect(page.locator('#deviceConnectionState')).to_have_text('数据已过期');expect(page.locator('#deviceCpu')).to_have_text('--')
        page.evaluate('window.freezeDevice=false');page.clock.run_for(2200);expect(page.locator('#deviceCpu')).to_have_text('24.5%')
        page.locator('#deviceConnectionState').scroll_into_view_if_needed();page.screenshot(path=str(out/'device-info-desktop.png'),full_page=True)
        page.locator('#disconnect').click();expect(page.locator('#deviceCpu')).to_have_text('--');expect(page.locator('#diagFields')).to_have_attribute('disabled','');page.locator('#ble').click();expect(page.locator('#deviceName')).to_have_value('我的 AI Passport');expect(page.locator('#deviceConnectionState')).to_contain_text('蓝牙')
        page.set_viewport_size({'width':390,'height':844});page.clock.run_for(200);assert page.evaluate('document.documentElement.scrollWidth<=innerWidth');page.locator('.device-overview').screenshot(path=str(out/'device-info-mobile.png'));page.locator('.device-tests').screenshot(path=str(out/'device-tests-mobile.png'))
        page.mouse.move(0,0);page.clock.run_for(5000)
        page.locator('#disconnect').click();expect(page.locator('#status')).to_contain_text('连接已断开');page.locator('.notice-close').click();page.clock.run_for(500);expect(page.locator('#status')).to_be_hidden();page.evaluate('window.noDiagnostics=true');page.locator('#usb').click();expect(page.locator('#deviceConnectionState')).to_contain_text('暂不支持');expect(page.locator('#deviceCpu')).to_have_text('--');expect(page.locator('#deviceNameSave')).to_be_disabled()
        assert not errors,errors;result={'actualFirmware':'NOT RUN - mock peripheral','USBBLEProtocolAndReconnect':'PASS','defaultAndSavedName':'PASS','nameByteLimitAndSaveFailure':'PASS','runtimeFields':'PASS','missingSensorAndStale':'PASS','explicitOnlySensorTests':'PASS','screenTestCommands':'PASS','oldFirmwareGating':'PASS','mobile':'PASS'};(out/'device-info-verification.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result));browser.close()
    server.shutdown()
if __name__=='__main__':main()
