"""Draft tracking and close protection with simulated peripherals; no real device access."""
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
  browser=p.chromium.launch(executable_path=sys.argv[2],headless=True);page=browser.new_page(viewport={'width':1280,'height':960});errors=[];page.on('pageerror',lambda e:errors.append(str(e)));page.add_init_script(mock);url=f'http://127.0.0.1:{server.server_port}/';page.goto(url)
  guarded=lambda:page.evaluate("(()=>{const e=new Event('beforeunload',{cancelable:true});dispatchEvent(e);return e.defaultPrevented;})()")
  expect(page.locator('#fields')).to_be_enabled();assert not guarded();page.locator('#ble').click();expect(page.locator('#connection')).to_have_text('蓝牙已连接');assert not guarded()
  expect(page.locator('#status')).to_have_attribute('data-kind','success');assert page.locator('#status').evaluate('(e)=>e.closest("header")!==null');green=page.locator('#status').evaluate('(e)=>getComputedStyle(e).backgroundColor')
  page.locator('#settingsNav').click();assert page.locator('#idleTime option').last.inner_text()=='保持常亮（不建议）';assert page.locator('#idleTime option').last.get_attribute('value')=='0'
  original=page.locator('#soundVolume').input_value();page.locator('#soundVolume').fill('70');assert guarded();expect(page.locator('#draftNotice')).to_be_visible();page.locator('#soundVolume').fill(original);assert not guarded();expect(page.locator('#draftNotice')).to_be_hidden()
  page.locator('#soundVolume').fill('70');page.evaluate('window.failConfig=true');page.locator('#upload').click();expect(page.locator('#status')).to_have_attribute('data-kind','error');red=page.locator('#status').evaluate('(e)=>getComputedStyle(e).backgroundColor');assert red!=green and guarded();page.screenshot(path=str(out/'error-desktop.png'))
  page.evaluate('window.failConfig=false');page.locator('#upload').click();expect(page.locator('#settingsState')).to_have_text('设置已保存到设备');assert not guarded();expect(page.locator('#status')).to_have_attribute('data-kind','success')
  page.locator('#pagesNav').click();page.get_by_role('switch',name='统计分析',exact=True).uncheck();assert guarded();page.locator('#monitorNav').click();page.locator('#monUpLabel').fill('U');page.locator('#monUpLabel').dispatch_event('input');assert guarded()
  page.locator('#experienceNav').click();page.locator('#deviceName').fill('我的设备');page.locator('#postcardNav').click();original_title=page.locator('#title').input_value();page.locator('#title').fill('新的标题');assert guarded()
  # Canceling a real browser close/reload prompt must retain the Bluetooth connection.
  dialogs=[]
  def cancel(d):dialogs.append(d.type);d.dismiss()
  page.once('dialog',cancel)
  try:page.reload(timeout=5000)
  except Error:pass
  assert dialogs==['beforeunload'];expect(page.locator('#connection')).to_have_text('蓝牙已连接');expect(page.locator('#title')).to_have_value('新的标题')
  page.locator('#saveAll').click();expect(page.locator('#draftNotice')).to_be_hidden(timeout=30000);assert not guarded();expect(page.locator('#status')).to_contain_text('全部修改已保存')
  assert page.evaluate('window.deviceName')=='我的设备';assert page.evaluate('window.testCard.saved') is True
  page.locator('#settingsNav').click();page.locator('#logoFile').set_input_files(repo/'assets/images/startup-preview.png');expect(page.locator('#logoDialog')).to_be_visible();assert guarded();page.locator('#logoCancel').click();assert not guarded()
  page.locator('#logoFile').set_input_files(repo/'assets/images/startup-preview.png');page.locator('#logoApply').click();expect(page.locator('#logoDialog')).to_be_hidden();assert guarded();page.locator('#saveAll').click();expect(page.locator('#draftNotice')).to_be_hidden();assert not guarded()
  page.locator('#logoDefault').click();assert guarded();page.locator('#logoSave').click();expect(page.locator('#logoState')).to_contain_text('已恢复原默认');assert not guarded();page.locator('#logoDefault').click();assert not guarded()
  # Simulation controls and previews never count as configuration edits.
  page.locator('#experienceNav').click();page.locator('#simOk').click();page.wait_for_timeout(350);assert not guarded()
  for width in [390,320]:
   page.set_viewport_size({'width':width,'height':844});page.locator('#postcardNav').click();page.locator('#title').fill('手机未保存');page.evaluate('scrollTo(0,900)');page.wait_for_timeout(250);assert page.evaluate('document.documentElement.scrollWidth<=innerWidth');box=page.locator('#draftNotice').bounding_box();assert box['y']>=0 and box['y']+box['height']<844;page.screenshot(path=str(out/f'unsaved-mobile-{width}.png'));page.locator('#title').fill('新的标题');assert not guarded()
  # A completely saved page reloads without any close prompt.
  page.reload();expect(page.locator('#fields')).to_be_enabled();assert not guarded();assert not errors,errors
  result={'topGreenAndRedNotices':'PASS','savedAndUnchangedNoPrompt':'PASS','failedSaveKeepsGuard':'PASS','allSixDraftSections':'PASS','realBeforeUnloadCancelKeepsBLE':'PASS','saveAllAndConfirmedReadback':'PASS','logoCancelDefaultAndSave':'PASS','simulatorIgnored':'PASS','alwaysOnLast':'PASS','mobile320390':'PASS','deviceAccess':'mock only'};(out/'notice-verification.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result));browser.close()
 server.shutdown()
if __name__=='__main__':main()
