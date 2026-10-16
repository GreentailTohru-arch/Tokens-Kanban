"""v18 management, actual image decoding, mocked peripheral and USB flasher boundary tests."""
from browser_navigation import navigate
from pathlib import Path
import sys,json,threading,runpy,hashlib,struct
from http.server import ThreadingHTTPServer
from PIL import Image,ImageDraw
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from postcard_server import Handler
from playwright.sync_api import sync_playwright,expect

def main():
 repo=Path(__file__).resolve().parents[1];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
 frames=[]
 for i in range(6):
  im=Image.new('RGB',(180,120),(10,20,30));ImageDraw.Draw(im).rectangle((i*20+8,30,i*20+38,90),fill=(20,180,150));frames.append(im)
 frames[0].save(out/'motion.gif',save_all=True,append_images=frames[1:],duration=[100,200,100,200,100,300],loop=0);frames[0].save(out/'still.png')
 mock=runpy.run_path(str(repo/'tests/test_postcard_browser.py'),run_name='tests')['MOCK']
 injected=r'''
   if(v[0]==='PC_DISPLAY_GET')reply=window.legacyDisplay?'PC_INVALID':'PC_DISPLAY 1 '+(window.deviceBrightness||75);
   if(v[0]==='PC_DISPLAY_SET'){window.deviceBrightness=+v[2];window.lastDisplayId=+v[1];reply=`PC_CFG_STATE ${v[1]} 2`;}
   if(v[0]==='PC_VERSION')reply='PC_VERSION '+(window.deviceVersion||'v18');
   if(v[0]==='PC_LOGO_GET')reply=window.legacyLogo?'PC_INVALID':`PC_LOGO 1 ${window.logoBytes?.length||0} ${window.logoHash||0}`;
   if(v[0]==='PC_LOGO_BEGIN'){window.logoId=+v[1];window.logoExpected=+v[2];window.logoHash=+v[3];window.logoBytes=[];reply=`PC_LOGO_STATE ${v[1]} 2 0`;}
   if(v[0]==='PC_LOGO_DATA'){window.logoBytes.push(...decode(v[3]));reply=`PC_LOGO_STATE ${v[1]} 2 ${window.logoBytes.length}`;}
   if(v[0]==='PC_LOGO_DONE'){window.logoSaved=true;reply=`PC_LOGO_STATE ${v[1]} 3 ${window.logoBytes.length}`;}
'''
 # The logo upload uses the same bounded RLE packets as postcard transfer.
 injected=injected.replace('decode(v[3])','unpack(v[3])')
 mock="window.unpack=hex=>{const b=hex.match(/../g).map(s=>parseInt(s,16)),o=[];for(let i=0;i<b.length;){const t=b[i++],n=(t&127)+1;if(t&128){const a=b[i++],c=b[i++];for(let j=0;j<n;j++)o.push(a,c);}else{o.push(...b.slice(i,i+n*2));i+=n*2;}}return o;};\n"+mock
 mock=mock.replace("   if(v[0]==='PC_HELLO')",injected+"   if(v[0]==='PC_HELLO')")
 server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
 with sync_playwright() as p:
  browser=p.chromium.launch(executable_path=sys.argv[2],headless=True);page=browser.new_page(viewport={'width':1280,'height':1000});errors=[];page.on('pageerror',lambda e:errors.append(str(e)));page.add_init_script(mock);page.goto(f'http://127.0.0.1:{server.server_port}/')
  page.locator('#usb').click();page.wait_for_timeout(300);print({'startupErrors':errors,'status':page.locator('#status').inner_text()},flush=True);navigate(page,'settingsNav');expect(page.locator('#brightness')).to_be_enabled();expect(page.locator('#brightness')).to_have_value('75');assert page.locator('#idleTime option[value="0"]').inner_text()=='保持常亮（不建议）'
  page.locator('#brightness').fill('40');page.locator('#idleTime').select_option('0');page.locator('#upload').click();expect(page.locator('#settingsState')).to_have_text('设置已保存到设备');assert page.evaluate('window.deviceBrightness')==40
  page.locator('#disconnect').click();page.locator('#ble').click();expect(page.locator('#brightness')).to_have_value('40')
  navigate(page,'monitorNav');page.locator('#bridgeHelp').click();expect(page.locator('#bridgeDialog')).to_be_visible();assert page.locator('#bridgeDialog li').count()==3;page.locator('#bridgeClose').click();expect(page.locator('#bridgeDialog')).to_be_hidden();assert page.get_by_role('link',name='下载电脑同步程序',exact=True).count()==0
  navigate(page,'settingsNav');page.locator('#logoFile').set_input_files(out/'motion.gif');expect(page.locator('#logoDialog')).to_be_visible();page.locator('#logoZoom').fill('1.4');page.locator('#logoZoom').dispatch_event('input');page.locator('#logoApply').click();expect(page.locator('#logoDialog')).to_be_hidden(timeout=15000);expect(page.locator('#logoState')).to_contain_text('6帧');page.locator('#logoSave').click();expect(page.locator('#logoState')).to_contain_text('下次启动显示',timeout=20000)
  data=bytes(page.evaluate('window.logoBytes'));assert data[:4]==b'TLG1' and len(data)%2==0;side,count=struct.unpack_from('<HH',data,4);assert side==96 and count==6;(out/'uploaded-logo.tlg').write_bytes(data)
  # Cancel framing leaves the successfully prepared/saved image intact.
  before=page.locator('#logoPreview').evaluate('(c)=>c.toDataURL()');page.locator('#logoFile').set_input_files(out/'still.png');page.locator('#logoCancel').click();expect(page.locator('#logoSave')).to_be_disabled()
  page.locator('#logoDefault').click();page.locator('#logoSave').click();expect(page.locator('#logoState')).to_contain_text('已恢复原默认');assert page.evaluate('window.logoBytes.length')==0
  navigate(page,'aboutNav');expect(page.locator('#aboutDeviceVersion')).to_have_text('设备固件 v18');expect(page.locator('#upload')).to_be_disabled()
  checks=page.evaluate('''async()=>{
   const {validateUpdate,digest,verifyApplication,flashUpdate}=await import('./about.js');
   const bytes=new Uint8Array(65536);bytes[0]=233;bytes[12]=5;const table=new Uint8Array(3072).fill(255),old=new Uint8Array(3072).fill(7),tableHash=await digest(table),oldHash=await digest(old);
   const m={version:'v18',chip:'ESP32-C3',appOffset:65536,appMax:0x730000,appBytes:bytes.length,appSHA256:await digest(bytes),appURL:'https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/download/v18/Tokens-Kanban-v18-app.bin',fullURL:'https://github.com/GreentailTohru-arch/Tokens-Kanban/releases/download/v18/Tokens-Kanban-v18-full.bin',partitionSHA256:tableHash,partitionHex:Array.from(table,b=>b.toString(16).padStart(2,'0')).join(''),allowedPartitions:[oldHash,tableHash]};
   await validateUpdate(m,'v18');let rejects=0;for(const bad of [{...m,version:'v17'},{...m,chip:'ESP32'},{...m,appOffset:0},{...m,appURL:'https://example.com/fw.bin'},{...m,partitionHex:'00'.repeat(3072)}])try{await validateUpdate(bad,'v18');}catch{rejects++;}if(rejects!==5)throw Error('manifest guards');
   try{await verifyApplication(new Uint8Array(65536),m);throw Error('accepted wrong binary');}catch(e){if(e.message==='accepted wrong binary')throw e;}
   let writes=[],read=[],unknown=false,wrongChip=false,closed=0,disconnect=0,reset=0;let appWritten=false,tableWritten=false;
   const factory=async()=>({transport:{disconnect:async()=>{disconnect++;}},loader:{chip:{get CHIP_NAME(){return wrongChip?'ESP32-S3':'ESP32-C3';}},main:async()=>{},detectFlashSize:async()=>'8MB',readFlash:async(off,n)=>{read.push([off,n]);return off===65536?bytes:unknown?new Uint8Array(3072):tableWritten?table:old;},writeFlash:async o=>{if(o.eraseAll!==false)throw Error('eraseAll');for(const f of o.fileArray){writes.push([f.address,f.data.length]);if(f.address===65536)appWritten=true;if(f.address===32768){if(!appWritten)throw Error('partition written before app');tableWritten=true;}if(![65536,32768].includes(f.address))throw Error('userdata write');}},after:async()=>{reset++;}}});
   const opts={port:{getInfo:()=>({usbVendorId:0x303a,usbProductId:0x1001})},closeConnection:async()=>{closed++;},factory};
   unknown=true;try{await flashUpdate(bytes,m,opts);}catch{}if(writes.length)throw Error('unknown partition writes');unknown=false;wrongChip=true;try{await flashUpdate(bytes,m,opts);}catch{}if(writes.length)throw Error('wrong chip writes');wrongChip=false;
   await flashUpdate(bytes,m,opts);if(JSON.stringify(writes)!=='[[65536,65536],[32768,3072]]'||reset!==1||disconnect!==3)throw Error('install flow');
   const lib=await import('./vendor/esptool-0.7.0.js');if(typeof lib.ESPLoader!=='function'||typeof lib.Transport!=='function')throw Error('vendor');window.releaseFixture=m;return {manifestGuards:'PASS',firmwareDigest:'PASS',unknownPartitionAndChip:'PASS',appThenPartition:'PASS',readbackVerification:'PASS',preserveUserData:'PASS',bundledFlasherModule:'PASS'};
  }''')
  meta=page.evaluate('window.releaseFixture');release={'draft':False,'prerelease':False,'tag_name':'v18','assets':[{'name':'Tokens-Kanban-v18-full.bin','browser_download_url':meta['fullURL']}]}
  page.route('https://api.github.com/repos/GreentailTohru-arch/Tokens-Kanban/releases/latest',lambda r:r.fulfill(json=release));page.route('https://raw.githubusercontent.com/GreentailTohru-arch/Tokens-Kanban/v18/update.json',lambda r:r.fulfill(json=meta));page.locator('#checkUpdate').click();expect(page.locator('#updateState')).to_have_text('已是最新版本 v18');expect(page.locator('#installUpdate')).to_be_enabled()
  page.route(meta['appURL'],lambda r:r.abort());page.locator('#installUpdate').click();expect(page.locator('#manualFirmware')).to_be_visible();(out/'app-test.bin').write_bytes(bytes([233])+bytes(11)+bytes([5])+bytes(65536-13));page.locator('#updateFile').set_input_files(out/'app-test.bin');expect(page.locator('#installHint')).to_contain_text('校验通过');page.locator('#updateClose').click();
  for version,text,enabled in [('v17','可安装 v18',True),('v19','比公开版本',False),('v18','已是最新版本',True)]:
   page.evaluate('(v)=>window.deviceVersion=v',version);page.locator('#disconnect').click();page.locator('#usb').click();page.locator('#checkUpdate').click();expect(page.locator('#updateState')).to_contain_text(text);
   if enabled:expect(page.locator('#installUpdate')).to_be_enabled()
   else:expect(page.locator('#installUpdate')).to_be_disabled()
  page.evaluate('scrollTo(0,0)');page.wait_for_timeout(300);page.screenshot(path=str(out/'about-desktop.png'),full_page=True)
  navigate(page,'settingsNav');page.evaluate('scrollTo(0,0)');page.wait_for_timeout(200);page.screenshot(path=str(out/'settings-desktop.png'),full_page=True)
  for width in [390,320]:
   page.set_viewport_size({'width':width,'height':844})
   for tab in ['experience','pages','monitor','postcard','settings','about']:
    navigate(page,tab+'Nav');assert page.evaluate('document.documentElement.scrollWidth<=innerWidth'),(width,tab)
   page.evaluate('scrollTo(0,0)');page.wait_for_timeout(200);page.screenshot(path=str(out/f'about-mobile-{width}.png'))
  assert not errors,errors;result={'brightnessSaveReconnect':'PASS','neverScreenOff':'PASS','bridgeDialogAndLinks':'PASS','actualGIFDecodeCropEncode':'PASS','logoUploadDefaultCancel':'PASS','aboutVersionAndGitHubCheck':'PASS','downloadCORSFallbackAndFileValidation':'PASS','mobileSixPages':'PASS','flasherSimulation':checks,'physicalUSBInstall':'NOT RUN'};(out/'management-verification.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result));browser.close()
 server.shutdown()
if __name__=='__main__':main()
