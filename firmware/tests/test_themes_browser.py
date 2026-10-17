"""Device palettes, WinUI chrome separation, interactive preview and mocked USB/BLE."""
from pathlib import Path
import sys,json,threading,runpy
from http.server import ThreadingHTTPServer
from playwright.sync_api import sync_playwright,expect
from browser_navigation import navigate,set_web_mode
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R/'companion'))
from postcard_server import Handler

def main():
 out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
 mock=runpy.run_path(str(R/'tests/test_postcard_browser.py'),run_name='tests')['MOCK']
 mock="window.themeValues=[0,6866123,7389183,3095113,15857652,11385027];window.themeWrites=0;\n"+mock
 injection="""
   if(v[0]==='PC_THEME_GET'&&!window.oldTheme)reply='PC_THEME 1 '+window.themeValues.join(' ');
   if(v[0]==='PC_THEME_SET'){window.themeWrites++;const c=window.testConfig;c.id=+v[1];c.state=window.failTheme?3:2;if(!window.failTheme)window.themeValues=v.slice(2).map(Number);reply=`PC_CFG_STATE ${c.id} 1`;}
   if(v[0]==='PC_VERSION')reply='PC_VERSION v21';
"""
 mock=mock.replace("   if(v[0]==='PC_HELLO')",injection+"   if(v[0]==='PC_HELLO')")
 server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start();url=f'http://127.0.0.1:{server.server_port}/'
 with sync_playwright() as p:
  browser=p.chromium.launch(executable_path=sys.argv[2],headless=True);context=browser.new_context(viewport={'width':1440,'height':1060});context.route('https://api.github.com/**',lambda r:r.fulfill(status=503,json={}));context.add_init_script(mock)
  page=context.new_page();errors=[];page.on('pageerror',lambda e:errors.append(str(e)));page.goto(url);expect(page.locator('#fields')).to_be_enabled();navigate(page,'themeNav')
  assert page.locator('[name=deviceTheme]').count()==4;assert page.locator('[value=github],[value=claude]').count()==0
  expect(page.locator('#themeMachineHost #simDevice')).to_be_visible();expect(page.locator('#upload')).to_be_disabled();expect(page.locator('#draftNotice')).to_be_hidden()
  default=page.evaluate('DeviceTheme.values()');assert default==[0,0x68c4cb,0x70bfff,0x2f3a49,0xf1f7f4,0xadb8c3],default
  expect(page.locator('#webModeToggle')).to_have_attribute('role','switch')
  expect(page.locator('theme-button .components')).to_have_css('width','108px')
  assert page.locator('theme-button .moon').count()==3
  assert page.locator('theme-button .star').count()==6
  bg=page.evaluate('getComputedStyle(document.body).backgroundColor')
  for name in ['default','obsidian','porcelain','ascii']:
   page.locator(f'.theme-choice:has([value={name}])').click();assert page.evaluate('getComputedStyle(document.body).backgroundColor')==bg
   expected=page.evaluate("DeviceTheme.colors().background.toLowerCase()")
   for back in [False,True]:
    navigate(page,'postcardNav');page.locator('#backTab' if back else '#frontTab').click()
    pixel=page.locator('#preview').evaluate("c=>'#'+Array.from(c.getContext('2d').getImageData(0,0,1,1).data).slice(0,3).map(v=>v.toString(16).padStart(2,'0')).join('')")
    assert pixel==expected,(name,pixel,expected)
    sample=page.locator('#preview').evaluate("c=>Array.from(c.getContext('2d').getImageData(80,60,60,60).data).join(',')")
    if name=='default':
     if back:qr_sample=sample
     else:photo_sample=sample
    else:assert sample==(qr_sample if back else photo_sample)
   if name=='porcelain':page.screenshot(path=str(out/'postcard-porcelain.png'),full_page=True)
   navigate(page,'pagesNav')
   for image in page.locator('.page-card img').all():
    image.evaluate("e=>e.decode()")
    pixel=image.evaluate("e=>{const c=document.createElement('canvas');c.width=240;c.height=320;const x=c.getContext('2d');x.drawImage(e,0,0);return '#'+Array.from(x.getImageData(0,0,1,1).data).slice(0,3).map(v=>v.toString(16).padStart(2,'0')).join('')}")
    assert pixel==expected,(name,pixel,expected)
   if name=='porcelain':page.screenshot(path=str(out/'pages-porcelain.png'),full_page=True)
   navigate(page,'themeNav')

   assert page.locator('#simScreen').evaluate("e=>{const p=e.getContext('2d').getImageData(0,0,1,1).data;return '#'+[...p].slice(0,3).map(v=>v.toString(16).padStart(2,'0')).join('')}")==page.evaluate('DeviceTheme.colors().background.toLowerCase()')
   page.screenshot(path=str(out/f'{name}-desktop.png'),full_page=True)
  # Reuses the real simulator: single, double, hold, overlay consumption and power.
  screen=page.locator('#simScreen');page.locator('#simOk').click();expect(screen).to_have_attribute('data-details','true');page.locator('#simOk').dblclick(delay=80);expect(screen).to_have_attribute('data-page','1')
  page.locator('#simDown').click();expect(screen).to_have_attribute('data-period','1');page.locator('#simUp').hover();page.mouse.down();page.wait_for_timeout(850);page.mouse.up();expect(screen).to_have_attribute('data-overlay','true');page.locator('#simOk').click();expect(screen).to_have_attribute('data-overlay','false');expect(screen).to_have_attribute('data-page','1')
  page.wait_for_timeout(500);page.locator('#simPower').click();expect(screen).to_have_attribute('data-awake','false');period=screen.get_attribute('data-period');page.locator('#simDown').click();expect(screen).to_have_attribute('data-awake','true');expect(screen).to_have_attribute('data-period',period)
  navigate(page,'experienceNav');expect(page.locator('#experienceStage #simDevice')).to_be_visible();navigate(page,'themeNav');expect(page.locator('#themeMachineHost #simDevice')).to_be_visible()
  # Advanced control, recovery and invalid input.
  page.locator('.theme-advanced summary').click();page.locator('#theme-hex-chart').fill('#A030BF');page.locator('#theme-hex-chart').press('Tab');expect(page.locator('#theme-chart')).to_have_value('#a030bf');expect(page.locator('#draftNotice')).to_be_visible()
  page.locator('#theme-hex-background').fill('invalid');expect(page.locator('#theme-hex-background')).to_have_attribute('aria-invalid','true');page.locator('#theme-hex-background').press('Tab');expect(page.locator('#theme-background')).to_have_value('#030703')
  page.locator('#theme-reset-chart').click();expect(page.locator('#theme-chart')).to_have_value('#6eff92')
  # Reconnect reads actual peripheral palette; save waits for completion and verifies readback.
  page.locator('#usb').click();expect(page.locator('#connection')).to_contain_text('已连接',timeout=30000);expect(page.locator('#upload')).to_be_enabled();expect(page.locator('#draftNotice')).to_be_hidden()
  page.locator('.theme-choice:has([value=porcelain])').click();page.locator('#theme-hex-accent').fill('#4167AB');page.locator('#theme-hex-accent').press('Tab');desired=page.evaluate('DeviceTheme.values()');page.locator('#upload').click();expect(page.locator('#themeStorage')).to_contain_text('已保存');expect(page.locator('#draftNotice')).to_be_hidden();assert page.evaluate('themeValues')==desired
  page.locator('.theme-choice:has([value=ascii])').click();page.evaluate('window.failTheme=true');page.locator('#upload').click();expect(page.locator('#themeStorage')).to_contain_text('保存未确认');expect(page.locator('#draftNotice')).to_be_visible();assert page.evaluate('themeValues')==desired
  page.evaluate('window.failTheme=false');page.locator('#saveAll').click();expect(page.locator('#draftNotice')).to_be_hidden();saved=page.evaluate('themeValues');writes=page.evaluate('themeWrites')
  # Web day/night never touches device values or creates a draft; persisted independently.
  for mode in ['light','dark']:
   set_web_mode(page,mode);expect(page.locator('html')).to_have_attribute('data-web-mode',mode);assert page.evaluate('themeValues')==saved;assert page.evaluate('themeWrites')==writes;expect(page.locator('#draftNotice')).to_be_hidden()
   for nav in ['experience','pages','monitor','postcard','settings','about','theme']:
    navigate(page,nav+'Nav');assert page.evaluate('document.documentElement.scrollWidth<=innerWidth'),(mode,nav)
   page.screenshot(path=str(out/f'winui-{mode}-desktop.png'),full_page=True)
  page.reload();expect(page.locator('html')).to_have_attribute('data-web-mode','dark');expect(page.locator('#fields')).to_be_enabled();navigate(page,'themeNav')
  # Actual reconnect uses BLE fragmentation and does not pretend old firmware supports saving.
  page.locator('#ble').click();expect(page.locator('#connection')).to_contain_text('已连接',timeout=30000);page.locator('.theme-choice:has([value=obsidian])').click();page.locator('#upload').click();expect(page.locator('#themeStorage')).to_contain_text('已保存');page.locator('#disconnect').click();page.evaluate('window.oldTheme=true');page.locator('#usb').click();expect(page.locator('#connection')).to_contain_text('已连接',timeout=30000);expect(page.locator('#upload')).to_be_disabled();expect(page.locator('#themeStorage')).to_contain_text('升级 v21')
  for width in [390,320,768]:
   page.set_viewport_size({'width':width,'height':844});page.wait_for_timeout(150);assert page.evaluate('document.documentElement.scrollWidth<=innerWidth'),width;page.screenshot(path=str(out/f'mobile-{width}.png'),full_page=True);navigate(page,'experienceNav');navigate(page,'themeNav')
  assert not errors,errors
  result={'postcardAndPageThumbnails':'PASS','photoQrColorsPreserved':'PASS','fourDeviceThemes':'PASS','webDayNightIndependent':'PASS','defaultExactPalette':'PASS','interactiveMachine':'PASS','advancedResetValidation':'PASS','USBAndBLEMockSaveReadback':'PASS','NvsWorkerRebootTest':'separate C harness','failedSaveRetainsDraft':'PASS','oldFirmwareGuard':'PASS','desktopAndMobile320390768':'PASS','realDeviceWrite':'NOT RUN'}
  (out/'verification.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));browser.close()
 server.shutdown()
if __name__=='__main__':main()
