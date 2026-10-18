import {readCardText,readCardPicture,writeCardText,encodeCardText} from './card-storage.js?v=30';
import {completeSave,cancelSaveCompletion} from './save-progress.js';
import {WallpaperEditor} from './wallpaper.js?v=30';
import {NoticeCenter} from './notices.js?v=30';
import {LocalConnection} from './local-connection.js?v=30';
import {DraftState} from './draft-state.js?v=30';
import {LogoEditor} from './logo-editor.js?v=30';
import {AboutPanel} from './about.js?v=30';
import {DeviceInfoPanel} from './device-info.js?v=30';
import {ExperienceSimulator,renderThemePreviews} from './simulator.js?v=30';
import {MonitorEditor} from './monitor.js?v=30-readback';
import {loadDefaults} from './defaults.js';
import {WIDTH,HEIGHT,FACE_BYTES,hash,pack,renderFace,cropRect} from './codec.js?v=30';
const $=id=>document.getElementById(id);
const drawerMedia=matchMedia('(max-width:760px)');
function setSidebar(open,restoreFocus=false){
 if(!drawerMedia.matches){open=true;restoreFocus=false;}
 document.querySelector('.app-shell').classList.toggle('sidebar-open',open);
 $('menuToggle').setAttribute('aria-expanded',String(open));
 $('menuToggle').setAttribute('aria-label',open?'收起页面导航':'打开页面导航');
 $('menuToggle').title=open?'收起页面导航':'打开页面导航';
 $('pageSidebar').inert=!open;$('sidebarBackdrop').hidden=!open||!drawerMedia.matches;
 $('pageContent').inert=open&&drawerMedia.matches;
 document.body.classList.toggle('drawer-open',open&&drawerMedia.matches);
 if(restoreFocus)$('menuToggle').focus({preventScroll:true});
}
const service='9e7a0001-4b51-4a4c-9b50-2f53f077c301';
const rxId='9e7a0002-4b51-4a4c-9b50-2f53f077c301',txId='9e7a0003-4b51-4a4c-9b50-2f53f077c301';
const content={photo:null,qr:null,title:'',subtitle:'',name:'',crop:{x:.5,y:.5,zoom:1},qrCrop:{x:.5,y:.5,zoom:1}};
let back=false,link=null,busy=false,imageLoading=true,activePage='experience',settingsSupported=false,startupSupported=false,brightnessSupported=false,autoLocalEnabled=true;
let themeSupported=false,cardInfo=null;
const petKeyDefaults=[1,2,3,0,4,0,0,0,5];
let petKeys=[...petKeyDefaults],petKeysSupported=false;
let pagesSupported=false,pagesVersion=3,pageEnabled=63,pageOrder=[0,1,4,2,3,5];
const pageNames=['用量首页','统计分析','系统检测','明信片','虚拟宠物','全屏壁纸'];
const pageDescriptions=['Token 数量与额度','模型、活动与趋势','按检测卡片配置显示','头像、文字与联系方式','六种动画，陪伴每一次思考','你的图片，铺满整块屏幕'];
const delay=ms=>new Promise(resolve=>setTimeout(resolve,ms));
let wallEditor=null,simulator=null,devicePanel=null,logoEditor=null,aboutPanel=null,drafts=null,savingAll=false;
const notices=new NoticeCenter($('status'));
function status(text,kind='success'){if(kind==='error')cancelSaveCompletion();else if(text.includes('已保存'))completeSave();notices.show(text,kind);}

function repaint(){renderFace($('preview'),content,back,true);refreshPreviewColors();refreshThumbnails();$('frontTab').classList.toggle('selected',!back);$('backTab').classList.toggle('selected',back);}
function controls(){const progress=$('progress');if(!busy)progress.hidden=true;else if(progress.hidden){progress.removeAttribute('value');progress.hidden=false;}wallEditor?.controls();const locked=busy||imageLoading;$("petKeysSave").disabled=locked||!link||!petKeysSupported;$("petKeysReset").disabled=locked;$("petKeyRows").inert=locked;document.querySelector('.theme-controls').inert=locked;for(const id of ['ble','usb'])$(id).disabled=locked||!!link;$('disconnect').disabled=locked||!link;$('upload').disabled=(activePage==='experience'||activePage==='device'||activePage==='about')||locked||!link||(activePage==='postcard'&&!cardInfo?.supported)||(activePage==='pet'&&!petKeysSupported)||(activePage==='wallpaper'&&!wallEditor?.supported)||(activePage==='theme'&&!themeSupported)||(activePage==='settings'&&!settingsSupported)||(activePage==='pages'&&!pagesSupported)||(activePage==='monitor'&&!monitorEditor.canSave());monitorEditor.setLocked(locked);devicePanel?.controls();logoEditor?.controls();aboutPanel?.controls();$('pagesFields').disabled=locked||!link||!pagesSupported;$('fields').disabled=locked;$('settingsFields').disabled=locked||!link||!settingsSupported;$('previewDone').disabled=locked||!link||!settingsSupported||!Number($('doneTone').value)||!Number($('soundVolume').value);$('previewQuota').disabled=locked||!link||!settingsSupported||!Number($('quotaTone').value)||!Number($('soundVolume').value);$('connection').textContent=link?link.kind+'已连接':'未连接';$('connection').classList.toggle('connected',!!link);$('connection').title=$('connection').textContent;}
function localClock(now=new Date()){return Math.floor(Date.UTC(now.getFullYear(),now.getMonth(),now.getDate(),now.getHours(),now.getMinutes(),now.getSeconds())/1000);}
function updateClock(){if(activePage==='wallpaper'||activePage==='pages'){wallEditor?.render();if(activePage==='pages'){const thumbnail=document.querySelector('.page-card[data-page="5"] img');if(thumbnail)thumbnail.src=$('wallPreview').toDataURL();}}const now=new Date();$('time').textContent=[now.getHours(),now.getMinutes()].map(n=>String(n).padStart(2,'0')).join(':');$('date').textContent=`${now.getMonth()+1}月${now.getDate()}日, 星期${'日一二三四五六'[now.getDay()]}`;}
async function syncClock(connection){
  const now=new Date(),reply=await connection.rpc('PC_CLOCK '+localClock(now));
  // Keep the new crop editor usable with v4 while v5 awaits flashing approval.
  if(reply==='PC_INVALID'){
    if(await connection.rpc('PC_TIME '+(now.getHours()*3600+now.getMinutes()*60+now.getSeconds()))!=='PC_TIME_OK')throw new Error('设备时间同步失败');
    return false;
  }
  if(reply!=='PC_CLOCK_OK')throw new Error('设备日期同步失败：'+reply);
  return true;
}
// A draft owns a newly selected bitmap until confirmed. Cancel never changes the saved framing.
let draft=null,drag=null;
const selectedFiles={photo:null,qr:null};
function restoreFile(field){const transfer=new DataTransfer();if(selectedFiles[field])transfer.items.add(selectedFiles[field]);$(field).files=transfer.files;}
function cropKey(field){return field==='qr'?'qrCrop':'crop';}
function openCrop(field,image=content[field],fresh=false,file=null){
  if(!image||busy)return;
  if(draft)closeCrop(false);
  draft={field,image,fresh,file,crop:fresh?{x:.5,y:.5,zoom:1}:{...content[cropKey(field)]}};
  $('cropTitle').textContent=field==='qr'?'二维码取景':'头像取景';
  $('cropHint').textContent=field==='qr'?'拖动和缩放，保留完整二维码及四周白边。':'拖动或用方向键移动画面，缩放调整构图。';
  $('zoom').max=field==='qr'?8:4;
  repaintCrop();$('cropDialog').showModal();$('crop').focus();
}
function closeCrop(apply){
  if(!draft)return;
  if(apply){
    if(draft.fresh){content[draft.field]?.close?.();content[draft.field]=draft.image;selectedFiles[draft.field]=draft.file;}
    content[cropKey(draft.field)]={...draft.crop};
    back=draft.field==='qr';repaint();
  }else if(draft.fresh)draft.image.close();
  restoreFile(draft.field);draft=null;drag=null;$('cropDialog').close();
}
function repaintCrop(){
  if(!draft)return;const im=draft.image;
  const r=cropRect(im.width,im.height,draft.crop,draft.field==='qr');draft.crop={x:r.cx,y:r.cy,zoom:r.zoom};
  const ctx=$('crop').getContext('2d',{willReadFrequently:true});ctx.fillStyle=draft.field==='qr'?'#fff':'#2f3a49';ctx.fillRect(0,0,240,240);
  ctx.imageSmoothingEnabled=draft.field!=='qr';ctx.drawImage(im,r.x,r.y,r.size,r.size,0,0,240,240);
  ctx.strokeStyle=draft.field==='qr'?'#68aab055':'#ffffff65';ctx.lineWidth=1;
  for(const p of [80,160]){ctx.beginPath();ctx.moveTo(p,0);ctx.lineTo(p,240);ctx.moveTo(0,p);ctx.lineTo(240,p);ctx.stroke();}
  $('zoom').value=r.zoom;$('zoomValue').textContent=r.zoom.toFixed(2)+'×';
}
$('crop').onpointerdown=e=>{if(!draft)return;const r=cropRect(draft.image.width,draft.image.height,draft.crop,draft.field==='qr');drag={id:e.pointerId,x:e.clientX,y:e.clientY,cx:r.cx,cy:r.cy,size:r.size};$('crop').setPointerCapture(e.pointerId);};
$('crop').onpointermove=e=>{if(!draft||!drag||drag.id!==e.pointerId)return;const scale=drag.size/$('crop').getBoundingClientRect().width;draft.crop.x=drag.cx-(e.clientX-drag.x)*scale/draft.image.width;draft.crop.y=drag.cy-(e.clientY-drag.y)*scale/draft.image.height;repaintCrop();};
$('crop').onpointerup=$('crop').onpointercancel=$('crop').onlostpointercapture=()=>{drag=null;};
$('crop').onkeydown=e=>{if(!draft)return;const shifts={ArrowLeft:[1,0],ArrowRight:[-1,0],ArrowUp:[0,1],ArrowDown:[0,-1]};if(!shifts[e.key])return;e.preventDefault();const r=cropRect(draft.image.width,draft.image.height,draft.crop,draft.field==='qr'),d=shifts[e.key];draft.crop.x+=d[0]*r.size*.025/draft.image.width;draft.crop.y+=d[1]*r.size*.025/draft.image.height;repaintCrop();};
$('zoom').oninput=()=>{if(draft){draft.crop.zoom=Number($('zoom').value);repaintCrop();}};
$('resetCrop').onclick=()=>{if(draft){draft.crop={x:.5,y:.5,zoom:1};repaintCrop();}};
$('applyCrop').onclick=()=>closeCrop(true);$('cancelCrop').onclick=()=>closeCrop(false);
$('cropDialog').addEventListener('cancel',e=>{e.preventDefault();closeCrop(false);});

class Connection{
  constructor(kind){this.commands=Promise.resolve();this.kind=kind;this.text='';this.pending=null;this.closed=false;this.timeout=12000;}
  feed(bytes){this.text+=new TextDecoder().decode(bytes);let pos;while((pos=this.text.indexOf('\n'))>=0){const line=this.text.slice(0,pos).trim();this.text=this.text.slice(pos+1);if(line.startsWith('PC_')&&this.pending){this.pending.resolve(line);this.pending=null;}}if(this.text.length>2048)this.text='';}
  rpc(line){const command=this.commands.then(()=>this.exchange(line));this.commands=command.catch(()=>{});return command;}
  async exchange(line){
    if(this.closed)throw new Error('连接已断开');
    if(this.pending)throw new Error('上一个命令尚未完成');
    let timer;const response=new Promise((resolve,reject)=>{this.pending={resolve,reject};timer=setTimeout(()=>{this.pending=null;reject(new Error('设备响应超时，请断开重连'));},this.timeout);});
    // Attach a rejection handler immediately while BLE fragments are being written.
    response.catch(()=>{});
    try{await this.send(new TextEncoder().encode(line+'\n'));return await response;}
    finally{clearTimeout(timer);if(this.pending){this.pending.reject(new Error('发送未完成'));this.pending=null;}}
  }
  lost(){this.closed=true;if(this.pending){this.pending.reject(new Error('连接已断开，旧明信片仍保留'));this.pending=null;}if(link===this){link=null;devicePanel?.lost();logoEditor?.lost();aboutPanel?.lost();settingsSupported=false;pagesSupported=false;monitorEditor.supported=false;$('monitorState').textContent='连接设备后读取检测配置';$('pagesState').textContent='连接设备后读取页面设置';$('settingsState').textContent='连接设备后读取设置';controls();status('连接已断开',this.intentional?'success':'error');}}
  async close(){this.intentional=true;this.lost();}
}
class BluetoothConnection extends Connection{
  constructor(){super('蓝牙');}
  async open(){
    if(!navigator.bluetooth)throw new Error('此浏览器不支持蓝牙，请使用 Windows Chrome / Edge');
    this.device=await navigator.bluetooth.requestDevice({filters:[{services:[service]}]});
    this.device.addEventListener('gattserverdisconnected',()=>this.lost());
    const server=await this.device.gatt.connect();const svc=await server.getPrimaryService(service);
    this.rx=await svc.getCharacteristic(rxId);this.tx=await svc.getCharacteristic(txId);
    this.notify=e=>this.feed(new Uint8Array(e.target.value.buffer,e.target.value.byteOffset,e.target.value.byteLength));
    this.tx.addEventListener('characteristicvaluechanged',this.notify);await this.tx.startNotifications();
  }
  async send(bytes){for(let i=0;i<bytes.length;i+=20)await this.rx.writeValueWithResponse(bytes.slice(i,i+20));}
  async close(){this.intentional=true;if(this.tx){try{await this.tx.stopNotifications();}catch{}this.tx.removeEventListener('characteristicvaluechanged',this.notify);}this.device?.gatt.disconnect();this.lost();}
}
class SerialConnection extends Connection{
  constructor(){super('USB');}
  async open(){
    if(!navigator.serial)throw new Error('此浏览器不支持 USB 串口，请使用 Windows Chrome / Edge');
    this.port=await navigator.serial.requestPort({filters:[{usbVendorId:0x303a,usbProductId:0x1001}]});
    // ESP32-C3 USB Serial/JTAG uses DTR/RTS edges for reset/download control.
    // Release RTS before DTR: the reverse order passes through the reset state.
    await this.port.open({baudRate:115200,bufferSize:4096});
    await this.port.setSignals({requestToSend:false});
    await this.port.setSignals({dataTerminalReady:false});
    this.writer=this.port.writable.getWriter();this.reader=this.port.readable.getReader();
    this.readTask=this.readLoop();
  }
  async readLoop(){try{while(!this.closed){const {done,value}=await this.reader.read();if(done)break;this.feed(value);}}catch{}finally{this.lost();}}
  async send(bytes){await this.writer.write(bytes);}
  async close(){this.intentional=true;this.closed=true;if(this.reader){try{await this.reader.cancel();await this.readTask;}catch{}this.reader.releaseLock();}if(this.writer)this.writer.releaseLock();if(this.port){try{await this.port.close();}catch{}}this.lost();}
}
async function connect(kind){
  busy=true;controls();let candidate=kind==='local'?new LocalConnection():kind==='ble'?new BluetoothConnection():new SerialConnection();
  try{if(kind==='local')candidate.onlost=()=>Connection.prototype.lost.call(candidate);await candidate.open();const deadline=performance.now()+10000;candidate.timeout=kind==='usb'?1000:12000;
    for(;;){try{if(await candidate.rpc('PC_HELLO')!=='PC_READY 1')throw new Error('设备尚未安装明信片固件');
    const dated=await syncClock(candidate);await readSettings(candidate);drafts?.mark('settings');await readPages(candidate);drafts?.mark('pages');await wallEditor.read(candidate);drafts?.mark('wallpaper');await readPostcard(candidate);await readPetKeys(candidate);drafts?.mark('petkeys');await monitorEditor.read(candidate);drafts?.mark('monitor');await devicePanel.read(candidate);drafts?.mark('name');await logoEditor.read(candidate);await readTheme(candidate);drafts?.mark('theme');await aboutPanel.read(candidate);candidate.timeout=12000;link=candidate;for(const key of ['settings','pages','monitor','name'])drafts?.mark(key);status(dated?'已连接':'已连接，日期显示需要更新设备固件');break;
    }catch(error){if(kind!=='usb'||candidate.closed||error.message!=='设备响应超时，请断开重连'||performance.now()>=deadline)throw error;candidate.text='';await delay(350);}}
  }catch(error){await candidate.close();status(error.message,'error');}finally{busy=false;controls();}
}
async function readPostcard(connection){
 cardInfo=null;const info=await readCardText(connection);cardInfo=info;
 if(!info.supported||info.length){
  for(const field of ['photo','qr']){content[field]?.close?.();content[field]=null;selectedFiles[field]=null;$(field).value='';}
  content.crop={x:.5,y:.5,zoom:1};content.qrCrop={x:.5,y:.5,zoom:1};
  for(const field of ['title','subtitle','name'])content[field]=info.text?.[field]||'';
 }
 if(info.supported&&!info.length)Object.assign(content,await loadDefaults());
 for(const field of ['title','subtitle','name'])$(field).value=content[field];
 $('cardReadState').textContent=!info.supported?'请升级固件以自动读取明信片。为保护设备内容，当前不能保存明信片。':info.text?'已读取设备文字；未上传新图片时，保存会保留设备头像和二维码。':info.length?'旧明信片只保存了合成图片，无法读回文字；请重新填写完整文字，头像和二维码会保留。':'设备尚未保存明信片';
 repaint();drafts?.mark('postcard');
}
async function stateFor(id,reply){
  for(let i=0;i<240;i++){
    const parts=reply.split(' ');if(parts[0]!=='PC_STATE')throw new Error('设备拒绝请求：'+reply);
    if(Number(parts[1])!==id)throw new Error('设备有其他上传，请等待一分钟后重试');
    const state=Number(parts[2]),offset=Number(parts[3]);
    if(state===4)throw new Error('设备校验或存储失败，旧明信片仍保留');
    if(state===2||state===3)return {state,offset};
    await delay(30);reply=await link.rpc('PC_STATUS');
  }
  throw new Error('设备保存超时，请断开重连');
}
async function upload(event){
  event.preventDefault();if(!link||busy||imageLoading||draft)return;
  if(!cardInfo?.supported){status('请先连接支持文字读取的新版固件，避免覆盖设备明信片','error');return;}
  if(((!content.photo||!content.qr)&&!cardInfo.length)||!content.title.trim()||!content.name.trim()){status('请导入图片、二维码，并填写标题和联系名字。','error');return;}
  busy=true;controls();$('progress').hidden=false;$('progress').value=0;
  try{
    encodeCardText(content);
    const currentCard=await readCardText(link);if(currentCard.generation!==cardInfo.generation)throw Error('设备明信片已改变，请重新连接后再编辑');
    if(!content.photo||!content.qr){status('正在读取设备原有图片，读取完成后保存…');
      if(!content.photo)content.photo=await readCardPicture(link,cardInfo,false);
      if(!content.qr)content.qr=await readCardPicture(link,cardInfo,true);
      repaint();
    }
    const canvas=document.createElement('canvas');canvas.width=WIDTH;canvas.height=HEIGHT;
    const data=new Uint8Array(FACE_BYTES*2);data.set(renderFace(canvas,content,false));data.set(renderFace(canvas,content,true),FACE_BYTES);
    const id=crypto.getRandomValues(new Uint32Array(1))[0]>>>1;
    let state=await stateFor(id,await link.rpc(`PC_BEGIN ${id} ${hash(data)}`));
    let sent=0;const started=performance.now();
    while(sent<data.length){
      const block=data.slice(sent,sent+256);state=await stateFor(id,await link.rpc(`PC_DATA ${id} ${sent} ${pack(block)}`));
      if(state.state!==2||state.offset!==sent+block.length)throw new Error('传输顺序不一致，请重新保存');
      sent=state.offset;$('progress').value=sent/data.length*100;
      const left=Math.max(0,Math.ceil((performance.now()-started)/1000*(data.length-sent)/sent));
      status(`正在保存 ${Math.floor(sent/data.length*100)}%${sent>4096?' · 预计剩余 '+left+' 秒':''}`);
    }
    await writeCardText(link,id,content,stateFor);
    state=await stateFor(id,await link.rpc(`PC_DONE ${id}`));if(state.state!==3)throw new Error('设备尚未确认保存');
    cardInfo=await readCardText(link);drafts?.mark('postcard');const dated=await syncClock(link);status('已保存到设备'+(dated?'':'，日期显示需要更新设备固件'));
  }catch(error){status(error.message,'error');}finally{$('progress').hidden=true;busy=false;controls();}
}
function setPage(page){
  const changed=activePage!==page;
  if(drawerMedia.matches)setSidebar(false,document.querySelector('.app-shell').classList.contains('sidebar-open'));
  activePage=page;if(page==='pages')refreshThumbnails();$('petPage').hidden=page!=='pet';$('wallpaperPage').hidden=page!=='wallpaper';wallEditor?.render();$('experiencePage').hidden=page!=='experience';if(page==='theme')$('themeMachineHost').append($('simDevice'));else if(page==='experience')$('experienceStage').prepend($('simDevice'));simulator?.setActive(page==='experience'||page==='theme');devicePanel?.setActive(page==='device');$('devicePage').hidden=page!=='device';$('monitorPage').hidden=page!=='monitor';$('pagesPage').hidden=page!=='pages';$('postcardPage').hidden=page!=='postcard';$('settingsPage').hidden=page!=='settings';$('aboutPage').hidden=page!=='about';$('themePage').hidden=page!=='theme';
  for(const name of ['experience','pages','monitor','postcard','wallpaper','pet','theme','settings','device','about']){const selected=page===name;$(name+'Nav').classList.toggle('selected',selected);$(name+'Nav').setAttribute('aria-pressed',String(selected));if(selected)$(name+'Nav').setAttribute('aria-current','page');else $(name+'Nav').removeAttribute('aria-current');}
  $('upload').setAttribute('form',page==='pet'?'petKeysForm':page==='wallpaper'?'wallForm':page==='theme'?'themeForm':page==='settings'?'settingsForm':page==='pages'?'pagesForm':page==='monitor'?'monitorForm':'form');controls();if(changed)window.scrollTo({top:0,behavior:'instant'});
}
function settingsValues(){const values=['idleTime','doneTone','quotaTone','soundVolume'].map(id=>Number($(id).value));if(startupSupported)values.push(Number($('startupSound').checked));return values;}
function putSettings(values){
  const [idle,done,quota,volume]=values;
  if(!$('idleTime').querySelector(`option[value="${idle}"]`)){const option=new Option(idle+' 秒',String(idle));$('idleTime').add(option,$('idleTime').querySelector('option[value="0"]'));}
  for(const [i,id] of ['idleTime','doneTone','quotaTone','soundVolume'].entries())$(id).value=values[i];
  $('soundVolumeValue').textContent=volume+'%';
  $('startupSound').checked=values.length===5?values[4]===1:true;
}
async function readSettings(connection){
  let parts=(await connection.rpc('PC_CFG_GET2')).split(' ');
  startupSupported=parts.length===7&&parts[0]==='PC_CFG'&&parts[1]==='2';
  if(!startupSupported)parts=(await connection.rpc('PC_CFG_GET')).split(' ');
  settingsSupported=parts[0]==='PC_CFG'&&((parts.length===6&&parts[1]==='1')||startupSupported);
  const display=(await connection.rpc('PC_DISPLAY_GET')).split(' ');brightnessSupported=display.length===3&&display[0]==='PC_DISPLAY'&&display[1]==='1'&&Number(display[2])>=5&&Number(display[2])<=100;
  $('brightness').disabled=!brightnessSupported;if(brightnessSupported)$('brightness').value=Number(display[2]);$('brightnessValue').textContent=$('brightness').value+'%';$('brightnessHint').textContent=brightnessSupported?'保存后应用，熄屏唤醒沿用此亮度。':'当前固件不支持亮度设置，请在关于页更新。';
  let migrated=false,banks=1;
  if(settingsSupported){
    const values=parts.slice(2).map(Number);
    if(values.some(v=>!Number.isInteger(v))||values[0]<0||(values[0]>0&&(values[0]<15||values[0]>3600))||values[1]<0||values[1]>7||values[2]<0||values[2]>7||values[3]<0||values[3]>100||(startupSupported&&values[4]!==0&&values[4]!==1))throw new Error('设备设置格式错误');
    const capability=await connection.rpc('PC_SOUND_BANKS');
    if(capability==='PC_SOUND_BANKS 3')banks=3;else if(capability==='PC_SOUND_BANKS 2')banks=2;
    const done=values[1]===0?0:values[1]===4||values[1]===6?values[1]:1;
    const quota=values[2]===0?0:values[2]===5||values[2]===7?values[2]:3;
    migrated=done!==values[1]||quota!==values[2];values[1]=done;values[2]=quota;
    if((banks<2&&(done===4||quota===5))||(banks<3&&(done===6||quota===7)))throw new Error('设备音效信息不一致，请重新连接');
    putSettings(values);
  }
  for(const [id,value,needed] of [['doneTone','4',2],['quotaTone','5',2],['doneTone','6',3],['quotaTone','7',3]])$(id).querySelector(`option[value="${value}"]`).disabled=banks<needed;
  $('startupSound').disabled=!startupSupported;
  $('startupHint').textContent=startupSupported?'开机显示 Logo 时播放，使用下方音量。':'升级 v10 固件后可设置开机音效。';
  $('settingsState').textContent=!settingsSupported?'设备固件需更新至 v7 才能使用设置':migrated?'旧版音效已改为 Codex，保存后应用':banks<3?'已读取设备设置 · 升级 v9 可用全部音效':'已读取设备设置';
}
async function saveSettings(event){
  event.preventDefault();if(!link||busy||!settingsSupported)return;
  const desired=settingsValues(),desiredBrightness=$('brightness').value;
  busy=true;controls();$('settingsState').textContent='正在保存设置…';
  try{
    const values=desired,id=(crypto.getRandomValues(new Uint32Array(1))[0]>>>1)||1;
    let reply=await link.rpc(`PC_CFG_SET${startupSupported?'2':''} ${id} ${values.join(' ')}`),saved=false;
    for(let i=0;i<300;i++){
      const parts=reply.split(' ');
      if(parts[0]!=='PC_CFG_STATE'||Number(parts[1])!==id)throw new Error('设备暂忙，请稍后保存');
      if(parts[2]==='3')throw new Error('设备保存失败，原设置仍保留');
      if(parts[2]==='2'){saved=true;break;}
      await delay(50);reply=await link.rpc('PC_CFG_STATUS');
    }
    if(!saved)throw new Error('保存超时，请重新连接');
    if(brightnessSupported)await saveBrightness();
    await readSettings(link);
    if(!settingsSupported||settingsValues().some((v,i)=>v!==values[i]))throw new Error('设置校验不一致，请重新读取');
    drafts?.mark('settings');$('settingsState').textContent='设置已保存到设备';status('设置已保存到设备');
  }catch(error){putSettings(desired);$('brightness').value=desiredBrightness;$('brightnessValue').textContent=desiredBrightness+'%';$('settingsState').textContent='保存未确认，请重新连接读取';status(error.message,'error');}finally{busy=false;controls();}
}
async function saveBrightness(){
 const id=(crypto.getRandomValues(new Uint32Array(1))[0]>>>1)||1,level=Number($('brightness').value);let reply=await link.rpc(`PC_DISPLAY_SET ${id} ${level}`),saved=false;
 for(let i=0;i<200;i++){const p=reply.split(' ');if(p[0]!=='PC_CFG_STATE'||+p[1]!==id||p[2]==='3')throw Error('亮度保存失败，请重新读取');if(p[2]==='2'){saved=true;break;}await delay(50);reply=await link.rpc('PC_CFG_STATUS');}
 if(!saved)throw Error('亮度保存超时');const actual=(await link.rpc('PC_DISPLAY_GET')).split(' ');if(+actual[2]!==level)throw Error('亮度读回不一致');
}
async function previewTone(field){
  if(!link||busy||!settingsSupported)return;
  const tone=Number($(field).value),volume=Number($('soundVolume').value);if(!tone||!volume)return;
  busy=true;controls();
  try{const reply=await link.rpc(`PC_SOUND ${tone} ${volume}`);if(reply!=='PC_SOUND_QUEUED')throw new Error('设备暂忙，请稍后试听');status('试听已发送到设备');}
  catch(error){status(error.message,'error');}finally{busy=false;controls();}
}
async function readTheme(connection){
 const reply=await connection.rpc('PC_THEME_GET'),parts=reply.split(' ');
 themeSupported=parts[0]==='PC_THEME'&&parts[1]==='1'&&parts.length===8;
 if(themeSupported){window.DeviceTheme.load(parts.slice(2).map(Number));monitorEditor.draw();repaint();$('themeStorage').textContent='已读取设备配色 · 修改后点击顶部保存到设备';}
 else $('themeStorage').textContent='当前固件不支持设备主题，请升级 v21；仍可预览配色';
}
async function saveTheme(event){
 event.preventDefault();if(!link||busy||!themeSupported)return;
 const desired=window.DeviceTheme.values();busy=true;controls();
 try{
  const id=(crypto.getRandomValues(new Uint32Array(1))[0]>>>1)||1;let reply=await link.rpc(`PC_THEME_SET ${id} ${desired.join(' ')}`),saved=false;
  for(let i=0;i<300;i++){const p=reply.split(' ');if(p[0]!=='PC_CFG_STATE'||+p[1]!==id||p[2]==='3')throw Error('主题保存失败，请重试');if(p[2]==='2'){saved=true;break;}await delay(50);reply=await link.rpc('PC_CFG_STATUS');}
  if(!saved)throw Error('主题保存超时，请重新读取');
  const actual=(await link.rpc('PC_THEME_GET')).split(' ');if(actual[0]!=='PC_THEME'||actual[1]!=='1'||actual.length!==8||desired.some((v,i)=>v!==Number(actual[i+2])))throw Error('主题读回不一致，保存尚未确认');
  drafts?.saved.set('theme',JSON.stringify(desired));$('themeStorage').textContent='主题已保存到设备 · 重启后保留';status('主题已保存到设备');
 }catch(error){$('themeStorage').textContent='保存未确认，修改仍保留';status(error.message,'error');}finally{busy=false;controls();}
}
const monitorEditor=new MonitorEditor({onSaved:()=>drafts?.mark('monitor'),status,getLink:()=>link,setBusy:value=>{busy=value;controls();},controls,onRender:url=>{document.querySelector('.page-card[data-page="2"] img')?.setAttribute('src',url);}});
devicePanel=new DeviceInfoPanel({onSaved:()=>drafts?.mark('name'),getLink:()=>link,isBusy:()=>busy||imageLoading,setBusy:value=>{busy=value;controls();},status});
wallEditor=new WallpaperEditor({pages:()=>pageOrder.filter(id=>pageEnabled&(1<<id)),link:()=>link,locked:()=>busy||imageLoading,busy:value=>{busy=value;controls();},status,changed:()=>{refreshThumbnails();},saved:()=>drafts?.mark('wallpaper')});
simulator=new ExperienceSimulator(()=>({wallpaper:ctx=>wallEditor.draw(ctx),postcardDate:!!(wallEditor.flags&4),petBindings:[...petKeys],mask:pageEnabled,order:[...pageOrder],content,volume:Number($('soundVolume').value),idle:Number($('idleTime').value),done:Number($('doneTone').value),quota:Number($('quotaTone').value),startup:$('startupSound').checked,drawBoot:ctx=>logoEditor.draw(ctx),bootDuration:logoEditor?.frames?.reduce((n,f)=>n+f.duration,0)||0}));
logoEditor=new LogoEditor({getLink:()=>link,isBusy:()=>busy||imageLoading,setBusy:value=>{busy=value;controls();},status});
aboutPanel=new AboutPanel({getLink:()=>link,isBusy:()=>busy||imageLoading,setBusy:value=>{busy=value;controls();},status});
$('themeForm').addEventListener('submit',saveTheme);
renderThemePreviews();
window.addEventListener('device-theme-preview-change',renderThemePreviews);
window.addEventListener('device-theme-change',()=>{monitorEditor.draw();repaint();simulator?.render();refreshDraftNotice();});
$('aboutNav').onclick=()=>setPage('about');$('themeNav').onclick=()=>setPage('theme');
$('bridgeHelp').onclick=()=>$('bridgeDialog').showModal();$('bridgeClose').onclick=()=>$('bridgeDialog').close();
$('brightness').oninput=()=>{$('brightnessValue').textContent=$('brightness').value+'%';$('settingsState').textContent='有未保存的设置';};
$('experienceNav').onclick=()=>setPage('experience');$('deviceNav').onclick=()=>setPage('device');
$('monitorNav').onclick=()=>setPage('monitor');
$('pagesNav').onclick=()=>setPage('pages');$('pagesForm').addEventListener('submit',savePages);
$('petNav').onclick=()=>setPage('pet');$('wallpaperNav').onclick=()=>setPage('wallpaper');$('postcardNav').onclick=()=>setPage('postcard');$('settingsNav').onclick=()=>setPage('settings');
$('settingsForm').addEventListener('submit',saveSettings);
for(const id of ['idleTime','doneTone','quotaTone','soundVolume','startupSound'])$(id).addEventListener('input',()=>{$('soundVolumeValue').textContent=$('soundVolume').value+'%';$('settingsState').textContent='有未保存的设置';controls();});
$('previewDone').onclick=()=>previewTone('doneTone');$('previewQuota').onclick=()=>previewTone('quotaTone');
for(const field of ['title','subtitle','name'])$(field).addEventListener('input',e=>{content[field]=e.target.value;repaint();});
for(const field of ['photo','qr'])$(field).addEventListener('change',async e=>{
  const file=e.target.files[0];e.target.value='';if(!file||busy||imageLoading){restoreFile(field);return;}
  if(file.size>25*1024*1024){status('请选择小于 25 MB 的图片','error');restoreFile(field);return;}
  imageLoading=true;controls();
  try{
    const next=await createImageBitmap(file,{imageOrientation:'from-image'});
    if(next.width*next.height>50000000){next.close();throw new Error('图片像素过大，请选 5000 万像素以内的图片');}
    openCrop(field,next,true,file);
  }catch(error){status('无法读取图片：'+error.message,'error');}
  finally{imageLoading=false;controls();if(!draft)restoreFile(field);}
});
$('frontTab').onclick=()=>{back=false;repaint();};$('backTab').onclick=()=>{back=true;repaint();};
for(const kind of ['ble','usb'])$(kind).onclick=()=>{const local=!!document.querySelector('meta[name=passport-device]');if(local)autoLocalEnabled=true;connect(local?'local':kind);};
$('disconnect').onclick=async()=>{autoLocalEnabled=false;busy=true;controls();try{await link?.close();}finally{link=null;busy=false;controls();}};
$('form').addEventListener('submit',async e=>{e.preventDefault();if(drafts?.changed().some(c=>c.key==='postcard')||!drafts?.changed().some(c=>c.key==='wallpaper'))await upload(e);if(drafts?.changed().some(c=>c.key==='wallpaper'))await wallEditor.save();});
window.addEventListener('beforeunload',event=>{if(unsaved().length||aboutPanel?.installing||savingAll){event.preventDefault();event.returnValue='';}});
window.addEventListener('pagehide',()=>{if(link instanceof BluetoothConnection)link.device?.gatt.disconnect();});
drafts=new DraftState();
const bitmapIds=new WeakMap();let nextBitmap=1;
const bitmapId=image=>{if(!image)return 0;if(!bitmapIds.has(image))bitmapIds.set(image,nextBitmap++);return bitmapIds.get(image);};
const cardKey=()=>JSON.stringify([content.title,content.subtitle,content.name,bitmapId(content.photo),bitmapId(content.qr),content.crop,content.qrCrop]);
drafts.add('wallpaper','壁纸显示设置',()=>wallEditor.key());
drafts.add('petkeys','宠物按键',()=>JSON.stringify(petKeys));
drafts.add('theme','设备主题',()=>JSON.stringify(window.DeviceTheme.values()));
drafts.add('settings','显示与声音',()=>JSON.stringify([settingsValues(),Number($('brightness').value)]));
drafts.add('pages','页面管理',()=>JSON.stringify([pageEnabled,pageOrder]));drafts.add('monitor','系统检测',()=>JSON.stringify(monitorEditor.config));drafts.add('name','设备名称',()=>$('deviceName').value.trim());drafts.add('postcard','明信片',cardKey);
function unsaved(){
 const changes=drafts?.changed()||[];if(wallEditor?.pending&&!changes.some(c=>c.key==='wallpaper'))changes.push({key:'wallpaper',label:'壁纸取景'});
 if(draft&&(draft.fresh||JSON.stringify(draft.crop)!==JSON.stringify(content[cropKey(draft.field)]))&&!changes.some(c=>c.key==='postcard'))changes.push({key:'postcard',label:'明信片'});
 if(logoEditor?.dirty||(logoEditor?.pending&&(logoEditor.pending.file!==logoEditor.file||JSON.stringify(logoEditor.draftCrop)!==JSON.stringify(logoEditor.crop))))changes.push({key:'logo',label:'开机 Logo'});
 return changes;
}
function refreshDraftNotice(){const changes=unsaved();$('draftNotice').hidden=!changes.length;$('draftSummary').textContent='未保存：'+changes.map(c=>c.label).join('、');$('draftNotice').title=$('draftSummary').textContent;$('saveAll').disabled=busy||imageLoading||savingAll;}
async function saveAll(){
 if(busy||imageLoading||savingAll)return;if(!link){status('请先连接设备，再保存修改','error');return;}
 if(wallEditor.pending){status('请先确定或取消壁纸取景','error');return;}if(draft)closeCrop(true);if(logoEditor.pending){status('请先在小窗中确定 Logo 取景','error');return;}
 savingAll=true;refreshDraftNotice();const event={preventDefault(){}};
 const jobs={wallpaper:()=>wallEditor.save(),petkeys:()=>savePetKeys(event),theme:()=>saveTheme(event),settings:()=>saveSettings(event),pages:()=>savePages(event),monitor:()=>monitorEditor.save(event),name:()=>devicePanel.saveName(event),postcard:()=>upload(event),logo:()=>logoEditor.save()};
 try{for(const {key,label} of unsaved()){await jobs[key]();if(unsaved().some(c=>c.key===key)){status(label+'保存未确认，修改仍保留，请检查后重试','error');return;}}status('全部修改已保存到设备');}
 catch(e){status(e.message,'error');}finally{savingAll=false;refreshDraftNotice();}
}
$('menuToggle').onclick=()=>{const open=$('menuToggle').getAttribute('aria-expanded')!=='true';setSidebar(open);if(open&&drawerMedia.matches){$('pageSidebar').getBoundingClientRect();$('pageSidebar').querySelector('.selected')?.focus();}};
$('sidebarBackdrop').onclick=()=>setSidebar(false,true);
drawerMedia.addEventListener('change',()=>setSidebar(!drawerMedia.matches));
document.addEventListener('keydown',event=>{
 if($('menuToggle').getAttribute('aria-expanded')!=='true'||document.querySelector('dialog[open]'))return;
 if(event.key==='Escape'&&drawerMedia.matches){event.preventDefault();setSidebar(false,true);return;}
 if(event.key==='Tab'&&drawerMedia.matches){
  const items=[$('menuToggle'),...$('pageSidebar').querySelectorAll('button')],index=items.indexOf(document.activeElement);
  event.preventDefault();items[(index+(event.shiftKey?-1:1)+items.length)%items.length].focus();
 }
});
setSidebar(!drawerMedia.matches);
new ResizeObserver(()=>document.documentElement.style.setProperty('--topbar-height',document.querySelector('.topbar').offsetHeight+'px')).observe(document.querySelector('.topbar'));
$('saveAll').onclick=saveAll;setInterval(refreshDraftNotice,150);document.addEventListener('input',refreshDraftNotice);document.addEventListener('change',refreshDraftNotice);
renderPages();setPage('experience');repaint();controls();updateClock();setInterval(updateClock,1000);
try{
  Object.assign(content,await loadDefaults());
  for(const field of ['title','subtitle','name'])$(field).value=content[field];
  repaint();drafts?.mark('postcard');
}catch(error){status(error.message,'error');}
finally{imageLoading=false;controls();}
void aboutPanel.check({automatic:true});

function refreshPreviewColors(){
 wallEditor?.render();const map=window.DeviceTheme.map;for(const el of document.querySelectorAll('.device,.page-thumbnail')){
  el.style.setProperty('--device-bg',map('#2f3a49'));el.style.setProperty('--device-ink',map('#f1f7f4'));el.style.setProperty('--device-mark',map('#52616e'));el.style.setProperty('--device-accent',map('#68c4cb'));
 }
}
document.addEventListener('petassetsready',()=>refreshThumbnails());
function refreshThumbnails(){if(!simulator)return;simulator.sync();for(const image of document.querySelectorAll('.page-card img'))image.src=simulator.previewURL(+image.closest('.page-card').dataset.page);}
function validPages(mask,order){return Number.isInteger(mask)&&mask>0&&mask<(1<<order.length)&&[4,5,6].includes(order.length)&&order.every(n=>Number.isInteger(n)&&n>=0&&n<order.length)&&new Set(order).size===order.length;}
function renderPages(){
  const list=$('pageList');list.replaceChildren();
  const postcardMarks=document.querySelector('.marks');postcardMarks.replaceChildren();for(const id of pageOrder.filter(n=>pageEnabled&(1<<n))){const mark=document.createElement('i');if(id===3)mark.className='active';postcardMarks.append(mark);}
  const count=pageOrder.filter(id=>pageEnabled&(1<<id)).length;
  pageOrder.forEach((id,index)=>{
    const enabled=!!(pageEnabled&(1<<id)),card=document.createElement('article');card.className='page-card'+(enabled?'':' page-off');card.dataset.page=id;
    const thumb=document.createElement('div');thumb.className='page-thumbnail';
    const image=document.createElement('img');image.src=simulator?simulator.previewURL(id):(id===2?monitorEditor.previewURL():`page-${id}.png`);image.width=240;image.height=320;image.alt=pageNames[id]+'布局预览';thumb.append(image);
    const marks=document.createElement('div');marks.className='preview-marks';
    for(const other of pageOrder.filter(n=>pageEnabled&(1<<n))){const mark=document.createElement('i');if(other===id)mark.className='active';marks.append(mark);}if(id!==5)thumb.append(marks);card.append(thumb);
    const info=document.createElement('div');info.className='page-info';
    const number=document.createElement('span');number.className='page-number';number.textContent=String(index+1).padStart(2,'0');info.append(number);
    const heading=document.createElement('h2');heading.textContent=pageNames[id];info.append(heading);
    const description=document.createElement('p');description.textContent=pageDescriptions[id];info.append(description);
    const label=document.createElement('label');label.className='toggle-row';const text=document.createElement('span');text.textContent=enabled?'已开启':'已关闭';
    const toggle=document.createElement('input');toggle.type='checkbox';toggle.setAttribute('role','switch');toggle.setAttribute('aria-label',pageNames[id]);toggle.checked=enabled;toggle.disabled=enabled&&count===1;
    toggle.onchange=()=>{const next=pageEnabled^(1<<id);if(!next){status('至少保留一个页面','error');toggle.checked=true;return;}pageEnabled=next;changedPages();};label.append(text,toggle);info.append(label);
    const actions=document.createElement('div');actions.className='page-actions';
    for(const [delta,name] of [[-1,'上移'],[1,'下移']]){const button=document.createElement('button');button.type='button';button.className='secondary';button.textContent=name;button.setAttribute('aria-label',pageNames[id]+name);button.disabled=index+delta<0||index+delta>=pageOrder.length;button.onclick=()=>{const target=index+delta;[pageOrder[index],pageOrder[target]]=[pageOrder[target],pageOrder[index]];changedPages();$('pageList').querySelector(`[data-page="${id}"] button[aria-label="${pageNames[id]+name}"]`)?.focus();};actions.append(button);}info.append(actions);card.append(info);list.append(card);
  });
  refreshPreviewColors();
}
function changedPages(){simulator?.sync();renderPages();refreshPreviewColors();$('pagesState').textContent='有未保存的页面设置';controls();}
async function readPages(connection){
  let parts=[];try{parts=(await connection.rpc('PC_PAGES_GET3')).split(' ');}catch{}
  if(parts[0]!=='PC_PAGES'||parts[1]!=='3')try{parts=(await connection.rpc('PC_PAGES_GET2')).split(' ');}catch{}
  if(parts[0]!=='PC_PAGES'||!['2','3'].includes(parts[1]))parts=(await connection.rpc('PC_PAGES_GET')).split(' ');
  pagesVersion=Number(parts[1]);pagesSupported=parts[0]==='PC_PAGES'&&((pagesVersion===3&&parts.length===9)||(pagesVersion===2&&parts.length===8)||(pagesVersion===1&&parts.length===7));
  if(pagesSupported){const values=parts.slice(2).map(Number);if(!validPages(values[0],values.slice(1)))throw new Error('设备页面设置格式错误');pageEnabled=values[0];pageOrder=values.slice(1);}
  $('pagesState').textContent=pagesSupported?'已读取设备页面设置':'升级 v13 固件后可管理页面';renderPages();
}
async function savePages(event){
  event.preventDefault();if(!link||busy||!pagesSupported)return;
  if(!validPages(pageEnabled,pageOrder)){status('至少保留一个页面，页面顺序不可重复','error');return;}
  const mask=pageEnabled,order=[...pageOrder],id=(crypto.getRandomValues(new Uint32Array(1))[0]>>>1)||1;
  busy=true;controls();$('pagesState').textContent='正在保存页面设置…';
  try{
    let reply=await link.rpc(`PC_PAGES_SET${pagesVersion>=2?String(pagesVersion):''} ${id} ${mask} ${order.join(' ')}`),saved=false;
    for(let i=0;i<300;i++){
      const parts=reply.split(' ');if(parts[0]!=='PC_CFG_STATE'||Number(parts[1])!==id)throw new Error('设备暂忙，请稍后保存');
      if(parts[2]==='3')throw new Error('设备保存失败，原页面设置仍保留');if(parts[2]==='2'){saved=true;break;}
      await delay(50);reply=await link.rpc('PC_CFG_STATUS');
    }
    if(!saved)throw new Error('保存超时，请重新连接');
    await readPages(link);if(!pagesSupported||pageEnabled!==mask||order.some((v,i)=>v!==pageOrder[i]))throw new Error('页面设置校验不一致，请重新连接');
    drafts?.mark('pages');$('pagesState').textContent='页面设置已保存到设备';status('页面设置已保存到设备');
  }catch(error){pageEnabled=mask;pageOrder=[...order];renderPages();$('pagesState').textContent='保存未确认，请重新连接读取';status(error.message,'error');}finally{busy=false;controls();}
}

function renderPetKeys(){
 const labels=['上 · 单击','上 · 双击','上 · 三击','下 · 单击','下 · 双击','下 · 三击','OK · 单击','OK · 三击','OK · 长按'];
 const actions=['无','发送消息','撤销输入（Ctrl+Z）','停止／继续思考','自定义快捷键','按住语音输入'];
 $('petKeyRows').replaceChildren(...labels.map((label,i)=>{const row=document.createElement('label');row.className='field';const title=document.createElement('span');title.textContent=label;const select=document.createElement('select');select.id='petKey'+i;select.setAttribute('aria-label',label);for(const v of i===8?[0,5]:[0,1,2,3,4]){const shortcut=$('petCustomHotkey').value.replaceAll('LeftAlt','Alt');const option=new Option(v===4&&shortcut?'自定义快捷键 · '+shortcut:actions[v],v);select.add(option);}select.value=petKeys[i];const commit=value=>{petKeys[i]=value;$('petKeysState').textContent='有未保存的宠物按键设置';renderPetKeys();refreshDraftNotice();};
 select.onchange=()=>{const value=Number(select.value);if(value===4){select.value=petKeys[i];document.dispatchEvent(new CustomEvent('pickpetshortcut',{detail:{source:select,confirm:()=>commit(4)}}));}else commit(value);};row.append(title,select);
 if(petKeys[i]===4){const edit=document.createElement('button');edit.type='button';edit.className='secondary shortcut-edit-link';edit.textContent='选择快捷键';edit.onclick=()=>document.dispatchEvent(new CustomEvent('pickpetshortcut',{detail:{source:edit}}));row.append(edit);}return row;}));
}
document.addEventListener('petshortcutchange',()=>renderPetKeys());
async function readPetKeys(connection){
 let parts=[];try{parts=(await connection.rpc('PC_KEYS_GET')).split(' ');}catch{}
 petKeysSupported=parts[0]==='PC_KEYS'&&parts[1]==='1'&&parts.length===11;
 if(petKeysSupported){const values=parts.slice(2).map(Number);if(values.some((v,i)=>!Number.isInteger(v)||(i===8?![0,5].includes(v):v<0||v>4)))throw new Error('宠物按键设置格式错误');petKeys=values;}
 $('petKeysState').textContent=petKeysSupported?'已读取设备宠物按键':'升级支持宠物遥控的固件后可保存';renderPetKeys();
}
async function savePetKeys(event){
 event.preventDefault();if(!link||busy||!petKeysSupported){status('请先连接支持宠物遥控的固件','error');return;}
 const desired=[...petKeys],id=(crypto.getRandomValues(new Uint32Array(1))[0]>>>1)||1;busy=true;controls();
 try{let reply=await link.rpc(`PC_KEYS_SET ${id} ${desired.join(' ')}`),saved=false;
  for(let i=0;i<300;i++){const p=reply.split(' ');if(p[0]!=='PC_CFG_STATE'||Number(p[1])!==id)throw new Error('设备暂忙，请重试');if(p[2]==='3')throw new Error('按键保存失败');if(p[2]==='2'){saved=true;break;}await delay(50);reply=await link.rpc('PC_CFG_STATUS');}
  if(!saved)throw new Error('保存超时');await readPetKeys(link);if(!petKeysSupported||desired.some((v,i)=>v!==petKeys[i]))throw new Error('按键校验不一致');drafts.mark('petkeys');$('petKeysState').textContent='宠物按键已保存到设备';status('宠物按键已保存到设备');
 }catch(e){petKeys=desired;renderPetKeys();status(e.message,'error');}finally{busy=false;controls();refreshDraftNotice();}
}
$('petKeysForm').addEventListener('submit',savePetKeys);$('petKeysReset').onclick=()=>{petKeys=[...petKeyDefaults];renderPetKeys();$('petKeysState').textContent='已恢复默认，保存后生效';refreshDraftNotice();};renderPetKeys();

if(document.querySelector('meta[name="passport-device"]')){
 const connectLocal=async()=>{if(autoLocalEnabled&&!link&&!busy&&!imageLoading&&!unsaved().length)await connect('local');};
 await connectLocal();setInterval(connectLocal,5000);
}
