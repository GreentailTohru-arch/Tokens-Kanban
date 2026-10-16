import {loadDefaults} from './defaults.js';
import {WIDTH,HEIGHT,FACE_BYTES,hash,pack,renderFace,cropRect} from './codec.js';
const $=id=>document.getElementById(id);
const service='9e7a0001-4b51-4a4c-9b50-2f53f077c301';
const rxId='9e7a0002-4b51-4a4c-9b50-2f53f077c301',txId='9e7a0003-4b51-4a4c-9b50-2f53f077c301';
const content={photo:null,qr:null,title:'',subtitle:'',name:'',crop:{x:.5,y:.5,zoom:1},qrCrop:{x:.5,y:.5,zoom:1}};
let back=false,link=null,busy=false,imageLoading=true,activePage='postcard',settingsSupported=false,startupSupported=false;
const delay=ms=>new Promise(resolve=>setTimeout(resolve,ms));
let toastTimer;
function status(text){clearTimeout(toastTimer);$('status').textContent=text;$('status').hidden=!text;if(text)toastTimer=setTimeout(()=>{$('status').hidden=true;},10000);}
function repaint(){renderFace($('preview'),content,back);$('frontTab').classList.toggle('selected',!back);$('backTab').classList.toggle('selected',back);}
function controls(){const locked=busy||imageLoading;for(const id of ['ble','usb'])$(id).disabled=locked||!!link;$('disconnect').disabled=locked||!link;$('upload').disabled=locked||!link||(activePage==='settings'&&!settingsSupported);$('fields').disabled=locked;$('settingsFields').disabled=locked||!link||!settingsSupported;$('previewDone').disabled=locked||!link||!settingsSupported||!Number($('doneTone').value)||!Number($('soundVolume').value);$('previewQuota').disabled=locked||!link||!settingsSupported||!Number($('quotaTone').value)||!Number($('soundVolume').value);$('connection').textContent=link?link.kind+'已连接':'未连接';$('connection').classList.toggle('connected',!!link);}
function localClock(now=new Date()){return Math.floor(Date.UTC(now.getFullYear(),now.getMonth(),now.getDate(),now.getHours(),now.getMinutes(),now.getSeconds())/1000);}
function updateClock(){const now=new Date();$('time').textContent=[now.getHours(),now.getMinutes()].map(n=>String(n).padStart(2,'0')).join(':');$('date').textContent=`${now.getMonth()+1}月${now.getDate()}日, 星期${'日一二三四五六'[now.getDay()]}`;}
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
    if(draft.fresh){content[draft.field]?.close();content[draft.field]=draft.image;selectedFiles[draft.field]=draft.file;}
    content[cropKey(draft.field)]={...draft.crop};
    $(draft.field+'Edit').hidden=false;back=draft.field==='qr';repaint();
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
for(const field of ['photo','qr'])$(field+'Edit').onclick=()=>openCrop(field);
class Connection{
  constructor(kind){this.kind=kind;this.text='';this.pending=null;this.closed=false;}
  feed(bytes){this.text+=new TextDecoder().decode(bytes);let pos;while((pos=this.text.indexOf('\n'))>=0){const line=this.text.slice(0,pos).trim();this.text=this.text.slice(pos+1);if(line.startsWith('PC_')&&this.pending){this.pending.resolve(line);this.pending=null;}}if(this.text.length>2048)this.text='';}
  async rpc(line){
    if(this.closed)throw new Error('连接已断开');
    if(this.pending)throw new Error('上一个命令尚未完成');
    let timer;const response=new Promise((resolve,reject)=>{this.pending={resolve,reject};timer=setTimeout(()=>{this.pending=null;reject(new Error('设备响应超时，请断开重连'));},12000);});
    // Attach a rejection handler immediately while BLE fragments are being written.
    response.catch(()=>{});
    try{await this.send(new TextEncoder().encode(line+'\n'));return await response;}
    finally{clearTimeout(timer);if(this.pending){this.pending.reject(new Error('发送未完成'));this.pending=null;}}
  }
  lost(){this.closed=true;if(this.pending){this.pending.reject(new Error('连接已断开，旧明信片仍保留'));this.pending=null;}if(link===this){link=null;settingsSupported=false;$('settingsState').textContent='连接设备后读取设置';controls();status('连接已断开');}}
  async close(){this.lost();}
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
  async close(){if(this.tx){try{await this.tx.stopNotifications();}catch{}this.tx.removeEventListener('characteristicvaluechanged',this.notify);}this.device?.gatt.disconnect();this.lost();}
}
class SerialConnection extends Connection{
  constructor(){super('USB');}
  async open(){
    if(!navigator.serial)throw new Error('此浏览器不支持 USB 串口，请使用 Windows Chrome / Edge');
    this.port=await navigator.serial.requestPort({filters:[{usbVendorId:0x303a,usbProductId:0x1001}]});
    await this.port.open({baudRate:115200,bufferSize:4096});await this.port.setSignals({dataTerminalReady:false,requestToSend:false});
    this.writer=this.port.writable.getWriter();this.reader=this.port.readable.getReader();
    this.readTask=this.readLoop();
  }
  async readLoop(){try{while(!this.closed){const {done,value}=await this.reader.read();if(done)break;this.feed(value);}}catch{}finally{this.lost();}}
  async send(bytes){await this.writer.write(bytes);}
  async close(){this.closed=true;if(this.reader){try{await this.reader.cancel();await this.readTask;}catch{}this.reader.releaseLock();}if(this.writer)this.writer.releaseLock();if(this.port){try{await this.port.close();}catch{}}this.lost();}
}
async function connect(kind){
  busy=true;controls();let candidate=kind==='ble'?new BluetoothConnection():new SerialConnection();
  try{await candidate.open();if(await candidate.rpc('PC_HELLO')!=='PC_READY 1')throw new Error('设备尚未安装明信片固件');
    const dated=await syncClock(candidate);await readSettings(candidate);link=candidate;status(dated?'已连接':'已连接，日期显示需要更新设备固件');
  }catch(error){await candidate.close();status(error.message);}finally{busy=false;controls();}
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
  if(!content.photo||!content.qr||!content.title.trim()||!content.name.trim()){status('请导入图片、二维码，并填写标题和联系名字。');return;}
  busy=true;controls();$('progress').hidden=false;$('progress').value=0;
  try{
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
    state=await stateFor(id,await link.rpc(`PC_DONE ${id}`));if(state.state!==3)throw new Error('设备尚未确认保存');
    const dated=await syncClock(link);status('已保存到设备'+(dated?'':'，日期显示需要更新设备固件'));
  }catch(error){status(error.message);}finally{busy=false;controls();}
}
function setPage(page){
  activePage=page;$('postcardPage').hidden=page!=='postcard';$('settingsPage').hidden=page!=='settings';
  for(const name of ['postcard','settings']){const selected=page===name;$(name+'Nav').classList.toggle('selected',selected);$(name+'Nav').setAttribute('aria-pressed',String(selected));}
  $('upload').setAttribute('form',page==='settings'?'settingsForm':'form');controls();
}
function settingsValues(){const values=['idleTime','doneTone','quotaTone','soundVolume'].map(id=>Number($(id).value));if(startupSupported)values.push(Number($('startupSound').checked));return values;}
function putSettings(values){
  const [idle,done,quota,volume]=values;
  if(!$('idleTime').querySelector(`option[value="${idle}"]`)){const option=new Option(idle+' 秒',String(idle));$('idleTime').add(option);}
  for(const [i,id] of ['idleTime','doneTone','quotaTone','soundVolume'].entries())$(id).value=values[i];
  $('soundVolumeValue').textContent=volume+'%';
  $('startupSound').checked=values.length===5?values[4]===1:true;
}
async function readSettings(connection){
  let parts=(await connection.rpc('PC_CFG_GET2')).split(' ');
  startupSupported=parts.length===7&&parts[0]==='PC_CFG'&&parts[1]==='2';
  if(!startupSupported)parts=(await connection.rpc('PC_CFG_GET')).split(' ');
  settingsSupported=parts[0]==='PC_CFG'&&((parts.length===6&&parts[1]==='1')||startupSupported);
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
  busy=true;controls();$('settingsState').textContent='正在保存设置…';
  try{
    const values=settingsValues(),id=(crypto.getRandomValues(new Uint32Array(1))[0]>>>1)||1;
    let reply=await link.rpc(`PC_CFG_SET${startupSupported?'2':''} ${id} ${values.join(' ')}`),saved=false;
    for(let i=0;i<300;i++){
      const parts=reply.split(' ');
      if(parts[0]!=='PC_CFG_STATE'||Number(parts[1])!==id)throw new Error('设备暂忙，请稍后保存');
      if(parts[2]==='3')throw new Error('设备保存失败，原设置仍保留');
      if(parts[2]==='2'){saved=true;break;}
      await delay(50);reply=await link.rpc('PC_CFG_STATUS');
    }
    if(!saved)throw new Error('保存超时，请重新连接');
    await readSettings(link);
    if(!settingsSupported||settingsValues().some((v,i)=>v!==values[i]))throw new Error('设置校验不一致，请重新读取');
    $('settingsState').textContent='设置已保存到设备';status('设置已保存到设备');
  }catch(error){$('settingsState').textContent='保存未确认，请重新连接读取';status(error.message);}finally{busy=false;controls();}
}
async function previewTone(field){
  if(!link||busy||!settingsSupported)return;
  const tone=Number($(field).value),volume=Number($('soundVolume').value);if(!tone||!volume)return;
  busy=true;controls();
  try{const reply=await link.rpc(`PC_SOUND ${tone} ${volume}`);if(reply!=='PC_SOUND_QUEUED')throw new Error('设备暂忙，请稍后试听');status('试听已发送到设备');}
  catch(error){status(error.message);}finally{busy=false;controls();}
}
$('postcardNav').onclick=()=>setPage('postcard');$('settingsNav').onclick=()=>setPage('settings');
$('settingsForm').addEventListener('submit',saveSettings);
for(const id of ['idleTime','doneTone','quotaTone','soundVolume','startupSound'])$(id).addEventListener('input',()=>{$('soundVolumeValue').textContent=$('soundVolume').value+'%';$('settingsState').textContent='有未保存的设置';controls();});
$('previewDone').onclick=()=>previewTone('doneTone');$('previewQuota').onclick=()=>previewTone('quotaTone');
for(const field of ['title','subtitle','name'])$(field).addEventListener('input',e=>{content[field]=e.target.value;repaint();});
for(const field of ['photo','qr'])$(field).addEventListener('change',async e=>{
  const file=e.target.files[0];e.target.value='';if(!file||busy||imageLoading){restoreFile(field);return;}
  if(file.size>25*1024*1024){status('请选择小于 25 MB 的图片');restoreFile(field);return;}
  imageLoading=true;controls();
  try{
    const next=await createImageBitmap(file,{imageOrientation:'from-image'});
    if(next.width*next.height>50000000){next.close();throw new Error('图片像素过大，请选 5000 万像素以内的图片');}
    openCrop(field,next,true,file);
  }catch(error){status('无法读取图片：'+error.message);}
  finally{imageLoading=false;controls();if(!draft)restoreFile(field);}
});
$('frontTab').onclick=()=>{back=false;repaint();};$('backTab').onclick=()=>{back=true;repaint();};
$('ble').onclick=()=>connect('ble');$('usb').onclick=()=>connect('usb');
$('disconnect').onclick=async()=>{busy=true;controls();try{await link?.close();}finally{link=null;busy=false;controls();}};
$('form').addEventListener('submit',upload);
window.addEventListener('beforeunload',()=>{if(link instanceof BluetoothConnection)link.device?.gatt.disconnect();});
repaint();controls();updateClock();setInterval(updateClock,1000);
try{
  Object.assign(content,await loadDefaults());
  for(const field of ['title','subtitle','name'])$(field).value=content[field];
  repaint();
}catch(error){status(error.message);}
finally{imageLoading=false;controls();}
