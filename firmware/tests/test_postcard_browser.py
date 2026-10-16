"""Optional local browser integration: pip install playwright pillow qrcode zxing-cpp.
Run with output directory, C codec probe path, and installed Chromium executable.
Uses a simulated serial peripheral; never searches for or opens real devices.
"""
from pathlib import Path
import sys,threading,subprocess,json
from http.server import ThreadingHTTPServer
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from postcard_server import Handler
from playwright.sync_api import sync_playwright,expect
from PIL import Image,ImageDraw
import qrcode
import zxingcpp

MOCK=r'''
window.testCard={offset:0,data:[],saved:false};
window.testConfig={values:[0,1,3,35],state:0,id:0,sounds:[]};
Object.defineProperty(navigator,'serial',{value:{requestPort:async()=>{
 let feed,buffer='',id=0,checksum=0;
 const card=window.testCard;
 const readable=new ReadableStream({start(c){feed=c;}});
 const writable=new WritableStream({write(bytes){buffer+=new TextDecoder().decode(bytes);
  while(buffer.includes('\n')){const p=buffer.indexOf('\n'),line=buffer.slice(0,p);buffer=buffer.slice(p+1);const v=line.split(' ');let reply='PC_INVALID';
   if(v[0]==='PC_HELLO')reply='PC_READY 1';
   if(v[0]==='PC_TIME')reply='PC_TIME_OK';
   if(v[0]==='PC_CLOCK'){card.clock=+v[1];reply=window.legacyClock?'PC_INVALID':'PC_CLOCK_OK';}
   if(v[0]==='PC_CFG_GET'&&!window.legacySettings)reply='PC_CFG 1 '+window.testConfig.values.join(' ');
   if(v[0]==='PC_CFG_SET'){const c=window.testConfig;c.id=+v[1];c.state=window.failConfig?3:2;if(!window.failConfig)c.values=v.slice(2).map(Number);reply=`PC_CFG_STATE ${c.id} 1`;}
   if(v[0]==='PC_CFG_STATUS'){const c=window.testConfig;reply=`PC_CFG_STATE ${c.id} ${c.state}`;}
   if(v[0]==='PC_SOUND'){window.testConfig.sounds.push(v.slice(1).map(Number));reply='PC_SOUND_QUEUED';}
   if(v[0]==='PC_BEGIN'){id=+v[1];checksum=+v[2];card.offset=0;card.data=[];reply=`PC_STATE ${id} 2 0`;}
   if(v[0]==='PC_DATA'){
    if(+v[1]!==id||+v[2]!==card.offset)throw Error('Bad sequence');
    const hex=v[3],bytes=hex.match(/../g).map(h=>parseInt(h,16));let out=[];
    for(let i=0;i<bytes.length;){let t=bytes[i++],n=(t&127)+1;if(t&128){const a=bytes[i++],b=bytes[i++];for(let j=0;j<n;j++)out.push(a,b);}else{out.push(...bytes.slice(i,i+n*2));i+=n*2;}}
    if(out.length>256)throw Error('Block overflow');card.data.push(...out);card.offset+=out.length;
    reply=`PC_STATE ${id} 2 ${card.offset}`;
   }
   if(v[0]==='PC_DONE'){let hash=2166136261;for(const b of card.data)hash=Math.imul(hash^b,16777619)>>>0;
    if(card.offset!==253440||hash!==checksum)throw Error('Checksum mismatch');card.saved=true;reply=`PC_STATE ${id} 3 ${card.offset}`;}
   feed.enqueue(new TextEncoder().encode(reply+'\n'));
  }
 }});
 return {open:async()=>{},setSignals:async()=>{},close:async()=>{},readable,writable};
}}});
Object.defineProperty(navigator,'bluetooth',{value:{requestDevice:async options=>{
 if(!options.filters[0].services[0].startsWith('9e7a0001'))throw Error('Wrong BLE filter');
 const port=await navigator.serial.requestPort();let reader,writer,notify;
 const device={addEventListener(){},gatt:{connect:async()=>({getPrimaryService:async()=>({getCharacteristic:async uuid=>uuid.startsWith('9e7a0002')?{
  writeValueWithResponse:async bytes=>{if(bytes.length>20)throw Error('MTU exceeded');await writer.write(bytes);}
 }:{
  addEventListener(type,cb){notify=cb;},removeEventListener(){},
  async startNotifications(){reader=port.readable.getReader();writer=port.writable.getWriter();(async()=>{for(;;){const {done,value}=await reader.read();if(done)break;notify({target:{value:new DataView(value.buffer)}});}})();},
  async stopNotifications(){await reader.cancel();reader.releaseLock();writer.releaseLock();}
 }})}),disconnect(){}}};return device;
}}});
'''

def main():
    out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
    probe=Path(sys.argv[2]).resolve();browser_exe=sys.argv[3]
    image=Image.new('RGB',(640,480));draw=ImageDraw.Draw(image)
    for y in range(480):draw.line((0,y,640,y),fill=(42+y//8,78+y//9,104+y//7))
    draw.ellipse((410,70,470,130),fill=(232,226,198));draw.polygon([(0,350),(180,180),(340,360),(440,260),(640,430),(640,480),(0,480)],fill=(49,76,84))
    image.save(out/'fixture-photo.png')
    qr=qrcode.make('https://example.com/contact').convert('RGB');qr.save(out/'fixture-qr.png')
    poster=Image.new('RGB',(500,800),'white');poster.paste(qr,(65,350));poster.save(out/'fixture-qr-poster.png')
    server=ThreadingHTTPServer(('127.0.0.1',0),Handler)
    thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    with sync_playwright() as p:
        browser=p.chromium.launch(executable_path=browser_exe,headless=True)
        page=browser.new_page(viewport={'width':1200,'height':1000},device_scale_factor=1,timezone_id="UTC")
        errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
        page.add_init_script(MOCK)
        page.clock.install(time=__import__('datetime').datetime(2026,10,16,9,41,tzinfo=__import__('datetime').timezone.utc))
        page.goto(f'http://127.0.0.1:{server.server_port}/')
        expect(page.locator('#fields')).to_be_enabled()
        expect(page.locator('#title')).to_have_value('标题')
        expect(page.locator('#subtitle')).to_have_value('副标题')
        expect(page.locator('#name')).to_have_value('扫码QR Code')
        assert page.locator('.toolbar button').all_text_contents()==['搜索蓝牙设备','连接 USB','断开','保存到设备']
        assert page.locator('h1:visible').count()==0
        assert page.locator('#status').is_hidden()
        page.screenshot(path=str(out/'browser-default.png'),full_page=True)
        page.locator('#frontTab').click()
        import base64,io
        default_front=Image.open(io.BytesIO(base64.b64decode(page.locator('#preview').evaluate('(c)=>c.toDataURL()').split(',')[1])))
        assert default_front.getpixel((33,6))[:3]==(47,58,73),'Rounded corner missing'
        assert default_front.getpixel((120,6))[:3]!=(47,58,73),'Default avatar missing'
        page.locator('#backTab').click()
        default_contact=Image.open(io.BytesIO(base64.b64decode(page.locator('#preview').evaluate('(c)=>c.toDataURL()').split(',')[1])))
        default_qr=zxingcpp.read_barcode(default_contact)
        assert default_qr and default_qr.text=='https://greentailtohru-arch.github.io/Tokens-Kanban/'
        page.screenshot(path=str(out/'browser-default-contact.png'),full_page=True)
        page.locator('#frontTab').click()
        page.locator('#title').fill('山海之间')
        page.locator('#subtitle').fill('把日常，寄给远方。')
        page.locator('#name').fill('林间来信')
        expect(page.locator('#date')).to_have_text('10月16日, 星期五')
        before=page.locator('#preview').evaluate('(c)=>c.toDataURL()')
        page.locator('#photo').set_input_files(out/'fixture-photo.png')
        expect(page.locator('#cropDialog')).to_be_visible()
        expect(page.locator('#cropTitle')).to_have_text('头像取景')
        initial=page.locator('#crop').evaluate('(c)=>c.toDataURL()')
        page.locator('#zoom').fill('2')
        zoomed=page.locator('#crop').evaluate('(c)=>c.toDataURL()');assert zoomed!=initial
        crop=page.locator('#crop').bounding_box()
        page.mouse.move(crop['x']+120,crop['y']+120);page.mouse.down()
        page.mouse.move(crop['x']+160,crop['y']+150,steps=4);page.mouse.up()
        assert page.locator('#crop').evaluate('(c)=>c.toDataURL()')!=zoomed
        assert page.locator('#preview').evaluate('(c)=>c.toDataURL()')==before
        page.locator('#resetCrop').click()
        reset=page.locator('#crop').evaluate('(c)=>c.toDataURL()')
        if reset!=initial:
            import base64
            (out/'crop-initial.png').write_bytes(base64.b64decode(initial.split(',')[1]));(out/'crop-reset.png').write_bytes(base64.b64decode(reset.split(',')[1]))
        assert reset==initial
        page.locator('#zoom').fill('1.3')
        page.locator('#crop').focus();page.keyboard.press('ArrowLeft')
        page.screenshot(path=str(out/'browser-photo-crop.png'),full_page=True)
        page.locator('#applyCrop').click();expect(page.locator('#cropDialog')).to_be_hidden()
        selected=page.locator('#preview').evaluate('(c)=>c.toDataURL()');assert selected!=before
        page.locator('#photoEdit').click();page.locator('#zoom').fill('2');page.keyboard.press('Escape')
        expect(page.locator('#cropDialog')).to_be_hidden()
        assert page.locator('#preview').evaluate('(c)=>c.toDataURL()')==selected
        # Replacing a photo then cancelling must keep both the old bitmap and its framing.
        page.locator('#photo').set_input_files(out/'fixture-qr.png');page.locator('#cancelCrop').click()
        assert page.locator('#preview').evaluate('(c)=>c.toDataURL()')==selected
        assert page.locator('#photo').evaluate('(input)=>input.files[0].name')=='fixture-photo.png'
        page.locator('#photoEdit').click();expect(page.locator('#zoom')).to_have_value('1.3');page.locator('#applyCrop').click()
        page.locator('#qr').set_input_files(out/'fixture-qr-poster.png')
        expect(page.locator('#cropDialog')).to_be_visible();expect(page.locator('#cropTitle')).to_have_text('二维码取景')
        page.locator('#zoom').fill('2')
        crop=page.locator('#crop').bounding_box()
        page.mouse.move(crop['x']+120,crop['y']+120);page.mouse.down()
        page.mouse.move(crop['x']+120,crop['y']+39,steps=5);page.mouse.up()
        page.screenshot(path=str(out/'browser-qr-crop.png'),full_page=True)
        page.locator('#applyCrop').click();expect(page.locator('#cropDialog')).to_be_hidden()
        qr_selected=page.locator('#preview').evaluate('(c)=>c.toDataURL()')
        page.locator('#qrEdit').click();page.locator('#resetCrop').click();page.keyboard.press('Escape')
        assert page.locator('#preview').evaluate('(c)=>c.toDataURL()')==qr_selected
        page.locator('#qrEdit').click();expect(page.locator('#zoom')).to_have_value('2');page.locator('#applyCrop').click()
        page.locator('#frontTab').click()
        # Avatar always covers the square; QR starts fully contained with white padding.
        assert page.evaluate("""async()=>{const {cropRect}=await import('./codec.js');for(const [w,h] of [[640,480],[200,800],[500,500]])for(const zoom of [1,2,4])for(const x of [-1,.5,2])for(const y of [-1,.5,2]){const r=cropRect(w,h,{x,y,zoom});if(r.x<0||r.y<0||r.x+r.size>w||r.y+r.size>h)return false;}const q=cropRect(500,800,{},true);return q.x<=0&&q.y<=0&&q.x+q.size>=500&&q.y+q.size>=800;}""")
        page.clock.fast_forward(16*3600*1000)
        expect(page.locator('#date')).to_have_text('10月17日, 星期六')
        page.clock.set_system_time(__import__('datetime').datetime(2026,10,16,9,41,tzinfo=__import__('datetime').timezone.utc))
        page.clock.run_for(1000)
        page.screenshot(path=str(out/'browser-editor.png'),full_page=True)
        page.locator('#usb').click();page.wait_for_function("document.getElementById('connection').textContent.includes('已连接')")
        page.locator('#settingsNav').click();expect(page.locator('#idleTime')).to_be_enabled()
        expect(page.locator('#settingsState')).to_have_text('已读取设备设置')
        page.locator('#idleTime').select_option('60');page.locator('#doneTone').select_option('2');page.locator('#quotaTone').select_option('3');page.locator('#soundVolume').fill('45')
        page.locator('#previewDone').click();expect(page.locator('#status')).to_contain_text('试听已发送')
        assert page.evaluate('window.testConfig.sounds')==[[2,45]]
        page.locator('#previewQuota').click();page.wait_for_function('window.testConfig.sounds.length===2')
        page.locator('#upload').click();expect(page.locator('#settingsState')).to_have_text('设置已保存到设备')
        assert page.evaluate('window.testConfig.values')==[60,2,3,45]
        page.screenshot(path=str(out/'browser-settings.png'),full_page=True)
        page.evaluate('window.failConfig=true');page.locator('#idleTime').select_option('120')
        page.locator('#upload').click();expect(page.locator('#status')).to_contain_text('原设置仍保留')
        assert page.evaluate('window.testConfig.values')==[60,2,3,45]
        page.locator('#disconnect').click();expect(page.locator('#idleTime')).to_be_disabled()
        page.evaluate('window.failConfig=false');page.locator('#usb').click();expect(page.locator('#idleTime')).to_have_value('60')
        page.locator('#postcardNav').click()
        page.locator('#upload').click();page.wait_for_function('window.testCard.saved',timeout=60000)
        page.wait_for_function("document.getElementById('status').textContent.startsWith('已保存到设备')")
        raw=bytes(page.evaluate('window.testCard.data'));assert len(raw)==253440
        assert 1792143661<=page.evaluate('window.testCard.clock')<1792143721
        (out/'postcard-fixture.rgb565').write_bytes(raw)
        # Scan the uploaded RGB565 contact face, not the pristine source QR.
        rgb=bytearray()
        for i in range(126720,len(raw),2):
            c=raw[i]|raw[i+1]<<8;rgb.extend((((c>>11)&31)*255//31,((c>>5)&63)*255//63,(c&31)*255//31))
        contact=Image.frombytes('RGB',(240,264),bytes(rgb))
        decoded_qr=zxingcpp.read_barcode(contact)
        assert decoded_qr and decoded_qr.text=='https://example.com/contact'
        contact.save(out/'decoded-contact.png')
        # Browser compressor round-trips every actual uploaded pixel through production C decoder.
        packets=page.evaluate('''async data=>{const {pack}=await import('./codec.js');const b=new Uint8Array(data),out=[];for(let i=0;i<b.length;i+=256)out.push(pack(b.slice(i,i+256)));return out;}''',list(raw))
        result=subprocess.run([str(probe)],input='\n'.join(packets)+'\n',text=True,capture_output=True,check=True)
        decoded=b''.join(bytes.fromhex(s) for s in result.stdout.splitlines());assert decoded==raw
        page.locator('#backTab').click();page.screenshot(path=str(out/'browser-contact.png'),full_page=True)
        page.locator('#disconnect').click();page.wait_for_function("document.getElementById('connection').textContent==='未连接'")
        page.locator('#ble').click();page.wait_for_function("document.getElementById('connection').textContent.includes('蓝牙')")
        page.locator('#settingsNav').click();expect(page.locator('#idleTime')).to_have_value('60')
        page.locator('#idleTime').select_option('300');page.locator('#upload').click();expect(page.locator('#settingsState')).to_have_text('设置已保存到设备')
        assert page.evaluate('window.testConfig.values')[0]==300
        page.locator('#postcardNav').click()
        page.evaluate('window.testCard.saved=false')
        page.locator('#upload').click();page.wait_for_function('window.testCard.saved',timeout=60000)
        page.wait_for_function("document.getElementById('status').textContent.startsWith('已保存到设备')")
        assert bytes(page.evaluate('window.testCard.data'))==raw
        page.locator('#disconnect').click();page.wait_for_function("document.getElementById('connection').textContent==='未连接'")
        page.evaluate('window.legacyClock=true;window.legacySettings=true')
        page.locator('#usb').click();expect(page.locator('#status')).to_contain_text('需要更新设备固件')
        page.locator('#settingsNav').click();expect(page.locator('#idleTime')).to_be_disabled();expect(page.locator('#upload')).to_be_disabled()
        expect(page.locator('#settingsState')).to_contain_text('更新至 v7')
        page.locator('#postcardNav').click();expect(page.locator('#upload')).to_be_enabled()
        page.locator('#disconnect').click()
        page.set_viewport_size({'width':390,'height':844});page.screenshot(path=str(out/'browser-mobile.png'),full_page=True)
        assert page.evaluate('document.documentElement.scrollWidth<=innerWidth')
        page.locator('#settingsNav').click();page.screenshot(path=str(out/'browser-mobile-settings.png'),full_page=True)
        assert page.evaluate('document.documentElement.scrollWidth<=innerWidth')
        page.locator('#postcardNav').click()
        page.locator('#qrEdit').click();expect(page.locator('#cropDialog')).to_be_visible()
        box=page.locator('#cropDialog').bounding_box();assert 0<=box['x'] and box['x']+box['width']<=390
        page.screenshot(path=str(out/'browser-mobile-crop.png'))
        page.locator('#applyCrop').click();expect(page.locator('#cropDialog')).to_be_hidden()
        assert not errors,errors
        print(json.dumps({'browser':'PASS','simulatedUSBUpload':'PASS','simulatedBLEUpload':'PASS','CCodecRoundTrip':'PASS','cropZoomDragReset':'PASS','modalConfirmCancelReopen':'PASS','croppedQRDecode':'PASS','defaultQRDecode':'PASS','toolbarDefaultsRoundedAvatar':'PASS','dateRollover':'PASS','v4ClockFallback':'PASS','pixels':len(raw)//2,'wireCharacters':sum(map(len,packets)),'blocks':len(packets),'realDevice':'NOT RUN'}))
        browser.close()
    server.shutdown()

if __name__=='__main__':main()
