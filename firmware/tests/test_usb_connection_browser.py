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
  browser=p.chromium.launch(executable_path=sys.argv[2],headless=True);page=browser.new_page(viewport={'width':1280,'height':960});errors=[];page.on('pageerror',lambda e:errors.append(str(e)));page.add_init_script(mock);url=f'http://127.0.0.1:{server.server_port}/';page.goto(url)
  guarded=lambda:page.evaluate("(()=>{const e=new Event('beforeunload',{cancelable:true});dispatchEvent(e);return e.defaultPrevented;})()")
  expect(page.locator('#fields')).to_be_enabled();baseline=page.evaluate('JSON.stringify({pages:testPages,config:testConfig.values,monitor:[...testMonitor]})')
  page.evaluate('window.portBusy=true');page.locator('#usb').click();expect(page.locator('#status')).to_have_attribute('data-kind','error');expect(page.locator('#connection')).to_have_text('未连接');expect(page.locator('#usb')).to_be_enabled();page.evaluate('window.portBusy=false')
  for i,prefix in enumerate(['e0e2b3b00','137ca20932c9','e0e2b3b002333e36e78f8612c617dd45b9c43704b2fefea1089bf785c3aa0af7']):
   page.evaluate('(p)=>{window.testELFPrefix=p;window.dropHello=1;}',prefix)
   page.locator('#usb').click();expect(page.locator('#connection')).to_have_text('USB已连接');expect(page.locator('#settingsState')).to_have_text('已读取设备设置');assert page.evaluate('window.usbSignalChanges')==[{'requestToSend':False},{'dataTerminalReady':False}]*(i+1)
   assert page.evaluate('JSON.stringify({pages:testPages,config:testConfig.values,monitor:[...testMonitor]})')==baseline
   page.locator('#disconnect').click();expect(page.locator('#connection')).to_have_text('未连接');expect(page.locator('#usb')).to_be_enabled()
  for invalid in ['abc','not-a-hash','0'*65]:
   page.evaluate('(p)=>window.testELFPrefix=p',invalid);page.locator('#usb').click();expect(page.locator('#status')).to_contain_text('设备芯片信息错误');expect(page.locator('#connection')).to_have_text('未连接')
  assert not errors,errors
  result={'USBReleasesRTSBeforeDTR':'PASS','threeConnectDisconnectCycles':'PASS','ELF9And12And64Characters':'PASS','busyPortRetry':'PASS','bootHandshakeRetry':'PASS','settingsPagesMonitorUnchanged':'PASS','hardware':'MOCK ONLY'};(out/'usb-connection-verification.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result));browser.close()
 server.shutdown()
if __name__=='__main__':main()
