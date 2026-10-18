import {rgb565,hash,pack} from './codec.js?v=30';
const $=id=>document.getElementById(id),wait=ms=>new Promise(r=>setTimeout(r,ms));
export function normalizeWallFlags(flags){return (flags&12)|((flags&3)?3:0);}
export function wallpaperCrop(width,height,{x=.5,y=.5,zoom=1}={}){
  zoom=Math.max(1,Math.min(8,Number(zoom)||1));
  const w=Math.min(width,height*.75)/zoom,h=w/.75;
  const cx=Math.max(w/2,Math.min(width-w/2,x*width)),cy=Math.max(h/2,Math.min(height-h/2,y*height));
  return {sx:cx-w/2,sy:cy-h/2,w,h,x:cx/width,y:cy/height,zoom};
}
export class WallpaperEditor{
 constructor(api){
  this.api=api;this.flags=4;this.bytes=null;this.image=null;this.savedLength=0;this.savedHash=hash(new Uint8Array());this.revision=0;this.supported=false;this.imageDirty=false;this.pending=null;
  this.canvas=document.createElement('canvas');this.canvas.width=240;this.canvas.height=320;
  $('wallFile').onchange=()=>this.choose($('wallFile').files[0]);
  
  $('wallClear').onclick=()=>{this.bytes=null;this.image=null;this.original?.close?.();this.original=null;this.savedLength=0;this.imageDirty=true;this.revision++;this.changed();};
  $('wallNav').onchange=$('wallTime').onchange=$('postcardDate').onchange=()=>{this.flags=($('wallTime').checked?3:0)|($('postcardDate').checked?4:0)|($('wallNav').checked?8:0);this.changed();};
  $('wallCropZoom').oninput=()=>{this.pending.crop.zoom=+$('wallCropZoom').value;this.paintCrop();};
  $('wallCropReset').onclick=()=>{this.pending.crop={x:.5,y:.5,zoom:1};this.paintCrop();};
  $('wallCropCancel').onclick=()=>this.cancel();
  $('wallCropDialog').addEventListener('cancel',e=>{e.preventDefault();this.cancel();});
  $('wallCropApply').onclick=()=>this.apply();
  const c=$('wallCropCanvas');
  c.onpointerdown=e=>{if(!this.pending)return;const p=this.pending;this.drag={id:e.pointerId,x:e.clientX,y:e.clientY,r:wallpaperCrop(p.image.width,p.image.height,p.crop)};c.setPointerCapture(e.pointerId);};
  c.onpointermove=e=>{if(!this.drag||this.drag.id!==e.pointerId)return;const d=this.drag,p=this.pending,scale=d.r.w/c.getBoundingClientRect().width;p.crop.x=d.r.x-(e.clientX-d.x)*scale/p.image.width;p.crop.y=d.r.y-(e.clientY-d.y)*scale/p.image.height;this.paintCrop();};
  c.onpointerup=c.onpointercancel=c.onlostpointercapture=()=>{this.drag=null;};
  c.onkeydown=e=>{if(!this.pending)return;const d={ArrowLeft:[-1,0],ArrowRight:[1,0],ArrowUp:[0,-1],ArrowDown:[0,1]}[e.key];if(!d)return;e.preventDefault();this.pending.crop.x+=d[0]*.02;this.pending.crop.y+=d[1]*.02;this.paintCrop();};
  $('wallRead').onclick=()=>this.loadImage();$('wallForm').onsubmit=e=>{e.preventDefault();void this.save();};this.render();
 }
 key(){return JSON.stringify([this.flags,this.revision]);}
 changed(){this.render();this.api.changed();}
 controls(){const locked=this.api.locked();$('wallControls').inert=locked;$('wallNav').disabled=locked||(!!this.api.link()&&!this.navSupported);$('wallNav').title=this.api.link()&&!this.navSupported?'请更新设备固件以保存导航条设置':'';$('wallClear').disabled=locked;$('postcardDate').disabled=locked;$('wallRead').disabled=locked||!this.supported||!this.savedLength;}
 render(){
  $('wallNav').checked=!!(this.flags&8);$('wallTime').checked=!!(this.flags&3);$('postcardDate').checked=!!(this.flags&4);$('date').hidden=$('time').hidden=!(this.flags&4);
  this.draw($('wallPreview').getContext('2d'));this.controls();
 }
 draw(ctx){
  ctx.fillStyle=window.DeviceTheme?.map('#2f3a49')||'#2f3a49';ctx.fillRect(0,0,240,320);
  if(this.image)ctx.drawImage(this.image,0,0,240,320);
  const now=new Date();ctx.font='14px "Microsoft YaHei UI",sans-serif';ctx.textBaseline='top';ctx.fillStyle=window.DeviceTheme?.map('#f1f7f4')||'#f1f7f4';
  ctx.save();if(this.image){ctx.shadowColor='#000';ctx.shadowBlur=4;ctx.fillStyle='#fff';}
  if(this.flags&1){ctx.textAlign='left';ctx.fillText(`${String(now.getHours()).padStart(2,'0')}:${String(now.getMinutes()).padStart(2,'0')}`,14,14);}
  if(this.flags&2){ctx.textAlign='right';ctx.fillText(`${now.getMonth()+1}月${now.getDate()}日, 星期${'日一二三四五六'[now.getDay()]}`,226,14);}ctx.restore();
  if(this.flags&8){const pages=this.api.pages?.()||[0,1,4,2,3,5],start=(240-(pages.length*28-6))/2;pages.forEach((id,i)=>{ctx.fillStyle=window.DeviceTheme?.map(id===5?'#68c4cb':'#52616e')||(id===5?'#68c4cb':'#52616e');ctx.beginPath();ctx.roundRect(start+i*28,309,22,3,2);ctx.fill();});}
 }
 async choose(file){
  if(!file)return;try{if(file.size>30*1024*1024)throw Error('请选择小于 30 MB 的图片');const image=await createImageBitmap(file);if(image.width*image.height>50000000){image.close();throw Error('图片像素过大，请缩小后重试');}this.open(image,{x:.5,y:.5,zoom:1});}catch(e){this.api.status(e.message,'error');}finally{$('wallFile').value='';}
 }
 open(image,crop){this.pending={image,crop:{...crop}};$('wallCropDialog').showModal();this.paintCrop();$('wallCropCanvas').focus();}
 paintCrop(guides=true){const p=this.pending;if(!p)return;const r=wallpaperCrop(p.image.width,p.image.height,p.crop);Object.assign(p.crop,{x:r.x,y:r.y,zoom:r.zoom});$('wallCropZoom').value=r.zoom;const c=$('wallCropCanvas').getContext('2d');c.imageSmoothingEnabled=true;c.imageSmoothingQuality='high';c.fillStyle=window.DeviceTheme?.map('#2f3a49')||'#2f3a49';c.fillRect(0,0,240,320);c.drawImage(p.image,r.sx,r.sy,r.w,r.h,0,0,240,320);if(guides){c.save();c.strokeStyle='#77b9b855';c.lineWidth=1;for(const x of [80,160]){c.beginPath();c.moveTo(x,0);c.lineTo(x,320);c.stroke();}for(const y of [320/3,640/3]){c.beginPath();c.moveTo(0,y);c.lineTo(240,y);c.stroke();}c.restore();}}
 cancel(){if(this.pending?.image!==this.original)this.pending?.image.close?.();this.pending=null;this.drag=null;$('wallCropDialog').close();}
 apply(){if(!this.pending)return;this.paintCrop(false);this.original?.close&&this.original!==this.pending.image&&this.original.close();this.original=this.pending.image;this.crop={...this.pending.crop};this.bytes=rgb565($('wallCropCanvas').getContext('2d').getImageData(0,0,240,320).data);this.decode(this.bytes);this.imageDirty=true;this.revision++;this.pending=null;$('wallCropDialog').close();$('wallState').textContent='已裁剪为 240 × 320 · 150 KiB · 保存后应用到设备';this.changed();}
 decode(bytes){const ctx=this.canvas.getContext('2d'),im=ctx.createImageData(240,320);for(let i=0,j=0;i<bytes.length;i+=2,j+=4){const n=bytes[i]|bytes[i+1]<<8;im.data[j]=(n>>11)*255/31;im.data[j+1]=((n>>5)&63)*255/63;im.data[j+2]=(n&31)*255/31;im.data[j+3]=255;}ctx.putImageData(im,0,0);this.image=this.canvas;}
 async read(link){const p=(await link.rpc('PC_WALL_GET')).split(' ');this.supported=p[0]==='PC_WALL'&&['1','2'].includes(p[1])&&p.length===5;this.navSupported=this.supported&&p[1]==='2';if(this.supported){const [n,h,f]=p.slice(2).map(Number);if(![0,153600].includes(n)||!Number.isInteger(h)||h<0||h>4294967295||!Number.isInteger(f)||f<0||f>15)throw Error('壁纸设置格式错误');this.flags=normalizeWallFlags(f);this.savedLength=n;this.savedHash=h;this.bytes=null;this.image=null;this.imageDirty=false;this.revision++;this.original?.close?.();this.original=null;$('wallState').textContent=n?'设备已有壁纸 · 点击读取图片可预览；修改开关不会替换图片':'设备使用主题背景';}else $('wallState').textContent='当前固件不支持壁纸存储；需安装含壁纸分区的完整固件。可先取景预览。';this.render();}
 async loadImage(){const link=this.api.link();if(!link||this.api.locked()||!this.savedLength)return;this.api.busy(true);$('progress').value=0;$('progress').hidden=false;try{const bytes=new Uint8Array(this.savedLength);for(let at=0;at<bytes.length;){const p=(await link.rpc(`PC_WALL_READ ${at}`)).split(' ');if(p[0]!=='PC_WALL_DATA'||+p[1]!==at||!p[2]||!/^(?:[0-9a-f]{2}){1,128}$/i.test(p[2]))throw Error('读取壁纸失败');const b=Uint8Array.from(p[2].match(/../g),v=>parseInt(v,16));if(at+b.length>bytes.length)throw Error('壁纸数据长度错误');bytes.set(b,at);at+=b.length;$('progress').value=at/bytes.length*100;}if(hash(bytes)!==this.savedHash)throw Error('壁纸校验失败，请重新连接');this.bytes=bytes;this.decode(bytes);$('wallState').textContent='已读取设备壁纸';this.changed();}catch(e){this.api.status(e.message,'error');}finally{$('progress').hidden=true;this.api.busy(false);}}
 async save(){
  const link=this.api.link();if(!link||this.api.locked())return;if(!this.supported){this.api.status('请先安装含壁纸分区的完整固件','error');return;}if(this.pending){this.api.status('请先确定或取消壁纸取景','error');return;}
  this.api.busy(true);$('progress').value=0;$('progress').hidden=false;const id=(crypto.getRandomValues(new Uint32Array(1))[0]>>>1)||1,bytes=this.bytes||new Uint8Array(),changed=this.imageDirty;
  const poll=async(reply,desired,count)=>{for(let i=0;i<400;i++){const p=reply.split(' ');if(p[0]!=='PC_WALL_STATE'||+p[1]!==id||p[2]==='4')throw Error('壁纸保存失败，原图未替换');if(+p[2]===desired&&(count===undefined||+p[3]===count))return;await wait(35);reply=await link.rpc('PC_WALL_STATUS');}throw Error('壁纸保存超时，请重新连接确认');};
  try{
   if(changed){await poll(await link.rpc(`PC_WALL_BEGIN ${id} ${bytes.length} ${hash(bytes)} ${this.flags}`),2,0);for(let at=0;at<bytes.length;at+=256){const b=bytes.subarray(at,at+256);await poll(await link.rpc(`PC_WALL_DATA ${id} ${at} ${pack(b)}`),2,at+b.length);$('progress').value=(at+b.length)/bytes.length*100;}await poll(await link.rpc(`PC_WALL_DONE ${id}`),3,bytes.length);}
   else await poll(await link.rpc(`PC_WALL_OPTIONS ${id} ${this.flags}`),3);
   const p=(await link.rpc('PC_WALL_GET')).split(' '),n=changed?bytes.length:this.savedLength,h=changed?hash(bytes):this.savedHash;
   if(p[0]!=='PC_WALL'||+p[2]!==n||+p[3]!==h||+p[4]!==this.flags)throw Error('保存读回不一致，请重新连接确认');this.savedLength=n;this.savedHash=h;this.imageDirty=false;this.api.saved();$('wallState').textContent='壁纸与显示选项已保存到设备';this.api.status('壁纸与显示选项已保存');
  }catch(e){this.api.status(e.message,'error');}finally{$('progress').hidden=true;this.api.busy(false);}
 }
}
