import {renderFace} from './codec.js?v=30';
const $=id=>document.getElementById(id),names=['用量首页','统计分析','系统检测','明信片','虚拟宠物','全屏壁纸'],periods=['今日','本周','累计'];
const petNames=['未连接','待命中','运行中','额度不足','听写中','操作中'];
const petKeys=['offline','online','thinking','limited','listening','computer'];
const bg='#2f3a49',ink='#f1f7f4',muted='#adb8c3',teal='#68c4cb';
const usage=[[18800000,15000000,15200,27.47],[215584794,209747072,506820,150.22],[640773677,623715584,1746255,351.87]];
const tones={1:'codex-notification.wav',3:'codex-error.wav',4:'opencode-complete.wav',5:'opencode-error.wav',6:'antigravity-complete.wav',7:'antigravity-error.wav'};
export class ExperienceModel {
  constructor(config,now=0){this.reset(config,now);}
  enabled(){return this.config.order.filter(id=>this.config.mask&(1<<id));}
  reset(config,now){this.config=config;this.on=true;this.awake=true;this.page=this.enabled()[0];this.period=0;this.details=false;this.contact=false;this.volume=config.volume;this.baseVolume=config.volume;this.overlay=false;this.held=null;this.pendingOK=null;this.quietUntil=0;this.okUntil=0;this.bootUntil=0;this.lastAction=now;this.lastVolume=now;this.preview=false;this.running=false;this.exhausted=false;this.petState=1;this.petStarted=now;this.petClicks=null;this.petVoice=false;this.petAction="";}
  sync(config){if(config.volume!==this.baseVolume){this.baseVolume=config.volume;this.volume=config.volume;}this.config=config;if(!this.enabled().includes(this.page))this.page=this.enabled()[0];}
  cancel(){this.held=null;this.pendingOK=null;this.petClicks=null;if(this.petVoice){this.petState=1;this.petVoice=false;}}
  power(now){this.cancel();this.overlay=false;this.on=!this.on;this.awake=this.on;this.lastAction=now;if(this.on){this.page=this.enabled()[0];this.period=0;this.details=this.contact=false;this.bootUntil=now+Math.max(this.config.startup&&this.volume?3000:1200,Math.min(6000,this.config.bootDuration||0));}}
  screen(now){if(!this.on)return;this.cancel();this.overlay=false;this.awake=!this.awake;this.lastAction=now;}
  adjust(key,now){const old=this.volume;this.volume=Math.max(0,Math.min(100,this.volume+(key==='up'?5:-5)));this.lastVolume=this.lastAction=now;this.preview ||=old!==this.volume;}
  press(key,now){
    if(!this.on||now<this.bootUntil||this.held)return;
    if(now<this.quietUntil||(key==='ok'&&now<this.okUntil))return;
    this.lastAction=now;this.held={key,at:now,consumed:false,long:false,next:0};
    if(!this.awake){this.awake=true;this.overlay=false;this.pendingOK=null;this.held.consumed=true;return;}
    if(this.overlay){this.held.consumed=true;if(key==='ok'){this.overlay=false;this.okUntil=now+500;this.pendingOK=null;}else this.adjust(key,now);}
  }
  release(key,now){
    const h=this.held;if(!h||h.key!==key)return;this.held=null;this.lastAction=now;
    if(this.petVoice){this.petVoice=false;this.petState=1;this.petAction="语音文字已保留（模拟，不发送）";}
    if(h.consumed){if(!h.long&&!this.overlay)this.quietUntil=now+400;return;}
    if(h.long)return;
    if(this.page===4){
      const previous=this.petClicks;this.petClicks={key,count:previous?.key===key&&now-previous.at<350?previous.count+1:1,at:now};
      if(this.petClicks.count===3)this.finishPetClicks(now);return;
    }
    if(key==='ok'){
      if(this.pendingOK!==null&&now-this.pendingOK<=300){this.pendingOK=null;const pages=this.enabled();this.page=pages[(pages.indexOf(this.page)+1)%pages.length];}
      else this.pendingOK=now;
    }else if(this.page<2)this.period=(this.period+(key==='up'?2:1))%3;
  }
  finishPetClicks(now){
    const p=this.petClicks;this.petClicks=null;if(!p)return;
    if(p.key==='ok'&&p.count===2){const pages=this.enabled();this.page=pages[(pages.indexOf(this.page)+1)%pages.length];this.petAction='';return;}
    const slot=p.key==='up'?p.count-1:p.key==='down'?p.count+2:p.count===1?6:7;
    const action=(this.config.petBindings||[1,2,3,0,4,0,0,0,5])[slot];
    this.petAction=['无操作','发送消息（模拟）','撤销输入（模拟）','停止／继续（模拟）','自定义快捷键（模拟）'][action]||'';
    if(action===3){this.running=!this.running;this.petState=this.running?2:1;this.petStarted=now;}
  }
  tick(now){
    if(this.petClicks&&!this.held&&now-this.petClicks.at>=350)this.finishPetClicks(now);
    const h=this.held;
    if(h&&now-h.at>=20000)this.cancel();
    else if(h&&h.key==='ok'&&!h.consumed&&now-h.at>=500){h.long=true;h.consumed=true;this.petClicks=null;if(this.page===4&&(this.config.petBindings||[1,2,3,0,4,0,0,0,5])[8]===5){this.petState=4;this.petVoice=true;this.petAction='正在听写（模拟）';}}
    else if(h&&!h.consumed&&h.key!=='ok'&&now-h.at>=500){h.long=true;h.consumed=true;h.next=now+180;this.overlay=true;this.petClicks=null;this.pendingOK=null;this.adjust(h.key,now);}
    else if(h&&h.key!=='ok'&&h.consumed&&this.overlay){
      if(!h.long&&now-h.at>=500){h.long=true;h.next=now+180;}
      if(h.long&&now>=h.next){this.adjust(h.key,now);h.next=now+180;}
    }
    if(this.pendingOK!==null&&now-this.pendingOK>300){this.pendingOK=null;if(this.page===0)this.details=!this.details;if(this.page===3)this.contact=!this.contact;}
    if(this.overlay&&!this.held&&now-this.lastAction>=3000)this.overlay=false;
    if(this.on&&this.awake&&!this.held&&now>=this.bootUntil&&this.config.idle>0&&now-this.lastAction>=this.config.idle*1000){this.awake=false;this.overlay=false;this.pendingOK=null;}
  }
}
export class ExperienceSimulator {
  constructor(getConfig){
    this.getConfig=getConfig;this.active=false;this.model=new ExperienceModel(getConfig(),performance.now());this.canvas=$('simScreen');this.screenCtx=this.canvas.getContext('2d');this.frame=document.createElement('canvas');this.frame.width=240;this.frame.height=320;this.ctx=this.frame.getContext('2d');this.face=document.createElement('canvas');this.face.width=240;this.face.height=264;this.audio=null;this.audioNext=0;this.audioTimer=null;this.images={};
    this.petAssets=[];
    fetch(new URL('pet/manifest.json',import.meta.url)).then(r=>r.json()).then(items=>{this.petAssets=items;for(const a of items){a.image=new Image();a.image.onload=()=>{this.render();document.dispatchEvent(new Event('petassetsready'));};a.image.src=new URL('pet/'+a.key+'.png',import.meta.url);}}).catch(()=>{});
    const select=document.createElement('select');select.id='simPetState';select.setAttribute('aria-label','宠物状态预览');
    petNames.forEach((name,i)=>{const o=document.createElement('option');o.value=i;o.textContent=name;select.append(o);});select.value='1';
    const row=document.createElement('label');row.className='pet-preview-control';row.textContent='宠物状态 · 示例预览';row.append(select);$('simTask').parentElement.append(row);
    select.onchange=()=>{this.model.petState=Number(select.value);this.model.petStarted=performance.now();this.model.page=4;this.model.awake=this.model.on=true;this.model.lastAction=performance.now();this.render();};
    for(const [id,file] of [['startup','sim-startup.png']]){const im=new Image();im.onload=()=>this.render();im.src=file;this.images[id]=im;}
    for(const key of ['up','down','ok'])this.bind($('sim'+key[0].toUpperCase()+key.slice(1)),key);
    const power=()=>{this.sync();this.model.power(performance.now());this.stopAudio();if(this.model.on&&this.model.bootUntil>performance.now()&&this.model.config.startup)this.play('startup.wav');this.render();};
    let powerHeld=false,powerFired=false;
    $('simPower').onpointerdown=e=>{powerHeld=true;powerFired=false;if(e.button!==0)return;e.preventDefault();$('simPower').setPointerCapture(e.pointerId);$('simPower').classList.add('pressed');clearTimeout(this.powerTimer);this.powerTimer=setTimeout(()=>{powerFired=true;power();},this.model.on?2000:500);};
    const releasePower=e=>{e.preventDefault();if(powerHeld&&!powerFired&&e.type==='pointerup'){this.model.screen(performance.now());this.stopAudio();this.render();}powerHeld=false;clearTimeout(this.powerTimer);$('simPower').classList.remove('pressed');if($('simPower').hasPointerCapture(e.pointerId))$('simPower').releasePointerCapture(e.pointerId);};
    $('simPower').onpointerup=releasePower;$('simPower').onpointercancel=releasePower;$('simPower').onlostpointercapture=()=>{clearTimeout(this.powerTimer);$('simPower').classList.remove('pressed');};$('simPower').onclick=e=>{if(e.detail===0){this.model.screen(performance.now());this.stopAudio();this.render();}};
    $('simReset').onclick=()=>{this.stopAudio();this.model.reset(getConfig(),performance.now());this.render();};
    $('simTask').onclick=()=>{const m=this.model;if(!m.on)return;m.running=!m.running;m.lastAction=performance.now();m.petState=m.running?2:1;m.petStarted=performance.now();if(!m.running)this.play(tones[m.config.done]);this.render();};
    $('simQuota').onclick=()=>{const m=this.model;if(!m.on)return;m.exhausted=!m.exhausted;m.lastAction=performance.now();m.petState=m.exhausted?3:1;m.petStarted=performance.now();if(m.exhausted)this.play(tones[m.config.quota]);this.render();};
    $('simAudio').onchange=()=>{if(!$('simAudio').checked)this.stopAudio();};
    window.addEventListener('blur',()=>this.cancel());document.addEventListener('visibilitychange',()=>{if(document.hidden){this.cancel();this.stopAudio();}});
    $('simDevice').addEventListener('keydown',e=>{if(e.target.id==='simPower')return;const key={ArrowUp:'up',ArrowDown:'down',Enter:'ok'}[e.key];if(key){e.preventDefault();if(!e.repeat)this.press(key);}});
    $('simDevice').addEventListener('keyup',e=>{if(e.target.id==='simPower')return;const key={ArrowUp:'up',ArrowDown:'down',Enter:'ok'}[e.key];if(key){e.preventDefault();this.release(key);}});
    this.timer=setInterval(()=>{if(!this.active||document.hidden)return;this.sync();this.model.tick(performance.now());this.consumePreview();this.render();},50);this.render();
  }
  bind(button,key){
    button.onpointerdown=e=>{if(e.button!==0)return;e.preventDefault();button.setPointerCapture(e.pointerId);$('simDevice').focus({preventScroll:true});this.press(key);button.classList.add('pressed');};
    button.onpointerup=e=>{e.preventDefault();this.release(key);button.classList.remove('pressed');if(button.hasPointerCapture(e.pointerId))button.releasePointerCapture(e.pointerId);};
    button.onpointercancel=()=>this.cancel();button.onlostpointercapture=()=>{button.classList.remove('pressed');if(this.model.held?.key===key)this.cancel();};
    // Native click from assistive technology has no preceding pointer/key events.
    button.onclick=e=>{if(e.detail===0){this.press(key);this.release(key);}};button.oncontextmenu=e=>e.preventDefault();
  }
  press(key){this.sync();this.model.press(key,performance.now());this.consumePreview();this.render();}
  release(key){this.model.release(key,performance.now());this.consumePreview();this.render();}
  cancel(){clearTimeout(this.powerTimer);this.model.cancel();for(const b of document.querySelectorAll('.sim-key'))b.classList.remove('pressed');}
  setActive(active){this.active=active;if(!active){this.cancel();this.stopAudio();}else{this.sync();this.model.lastAction=performance.now();this.render();}}
  sync(){this.model.sync(this.getConfig());}
  stopAudio(){clearTimeout(this.audioTimer);this.audioTimer=null;if(this.audio){this.audio.pause();this.audio=null;}}
  play(file){
    if(!file||!$('simAudio').checked||!this.model.volume||!this.active)return;
    this.stopAudio();const audio=new Audio(new URL('sim-audio/'+file,import.meta.url));this.audio=audio;audio.volume=this.model.volume/100;audio.onended=()=>{if(this.audio===audio)this.audio=null;};audio.play().catch(()=>{if(this.audio===audio)this.audio=null;$('simAudioHint').textContent='浏览器暂未允许播放声音，请再次操作按钮。';});
  }
  consumePreview(){
    if(this.audio)this.audio.volume=this.model.volume/100;if(!this.model.volume){this.model.preview=false;this.stopAudio();return;}
    if(!this.model.preview)return;this.model.preview=false;clearTimeout(this.audioTimer);const requested=performance.now();
    this.audioTimer=setTimeout(()=>{this.audioTimer=null;if(!this.active||!this.model.on||performance.now()-requested>1600)return;if(this.audio&&!this.audio.paused)return;this.audioNext=performance.now()+1200;this.play(tones[this.model.config.done||this.model.config.quota||1]);},Math.max(80,this.audioNext-requested));
  }
  color(value){return window.DeviceTheme?.map(value,this.previewPalette)||value;}
  text(text,x,y,size=14,color=ink,align='left'){const c=this.ctx;c.font=`${size}px "Microsoft YaHei UI",sans-serif`;c.fillStyle=this.color(color);c.textAlign=align;c.textBaseline='top';c.fillText(text,x,y);c.textAlign='left';}
  box(x,y,w,h,color='#374453',r=9){const c=this.ctx;c.fillStyle=this.color(color);c.beginPath();c.roundRect(x,y,w,h,r);c.fill();}
  header(clock=false){const now=new Date();if(clock)this.text([now.getHours(),now.getMinutes()].map(n=>String(n).padStart(2,'0')).join(':'),18,14);else{this.ctx.fillStyle=this.model.exhausted?'#ff626c':this.model.running?'#4cdd91':'#5ba7ff';this.ctx.beginPath();this.ctx.arc(26,23,6,0,7);this.ctx.fill();this.text('Codex / '+(this.model.exhausted?'额度不足':this.model.running?'运行中':'待命中'),39,14);}this.text('86%',222,14,14,muted,'right');}
  marks(){const enabled=this.model.enabled(),start=(240-(enabled.length*22+(enabled.length-1)*6))/2;this.ctx.fillStyle=this.color(bg);this.ctx.fillRect(0,303,240,17);enabled.forEach((id,i)=>this.box(start+i*28,309,22,3,id===this.model.page?teal:'#52616e',2));}
  home(){
    const m=this.model,p=m.period,[input,cached,output,cost]=usage[p],total=input+output;this.header();periods.forEach((v,i)=>{this.box(16+i*70,43,66,27,p===i?'#516471':'#374453',7);this.text(v,49+i*70,48,14,p===i?'#bdecd8':muted,'center');});
    this.text('TOTAL TOKENS',18,82,14,muted);this.text(total>=1e8?(total/1e6).toFixed(2)+'M':total.toLocaleString('en-US'),16,104,32);this.text(m.details?'估算 $'+cost.toFixed(2):total.toLocaleString('en-US')+' tokens',18,143,14,muted);this.box(14,172,212,104);
    if(m.details){for(const [i,[label,value]] of [['输入',input],['缓存',cached],['输出',output]].entries())this.text(label+'  '+(value/1e6).toFixed(2)+'M',24,182+i*28);}
    else{this.text('每周额度',24,179,14,muted);this.text(m.exhausted?'剩余 0%':'剩余 87%',216,179,14,ink,'right');this.box(24,200,192,5,'#475b69',3);if(!m.exhausted)this.box(24,200,167,5,teal,3);this.text('6天 3小时后重置',24,207,14,muted);this.text('账户额度',24,227,14,muted);this.text('--',216,227,14,ink,'right');this.box(24,248,192,5,'#475b69',3);this.text('暂无数据',24,255,14,muted);}
    this.text('Pro',18,280,14,muted);this.text('重置券 1',222,280,14,muted,'right');
  }
  analytics(){
    const c=this.ctx,p=this.model.period;this.header();this.text('模型',18,42);this.text(periods[p],222,42,14,muted,'right');this.text('gpt-6-astra',18,65,12);this.text(['18.6M','211.4M','631.2M'][p],183,65,12,ink,'right');this.text('99%',222,65,12,muted,'right');this.text('gpt-6.1-sol',18,87,12);this.text(['215K','4.7M','11.3M'][p],183,87,12,ink,'right');this.text('1%',222,87,12,muted,'right');this.box(18,113,204,1,'#455361',0);this.text('活动',18,121);this.text('活跃 55 天',222,121,14,muted,'right');
    const colors=['#344352','#3b5267','#456985','#528db5','#9edbfa'];for(let col=0;col<26;col++)for(let row=0;row<7;row++){const level=col<16?0:((col*7+row*3)%5);this.box(17+col*8,145+row*8,6,6,colors[level],1);}for(const [i,label] of ['5月','8月','10月'].entries())this.text(label,18+i*86,202,12,muted);
    this.text('趋势',18,225);this.text('峰值 537.3M',222,225,14,muted,'right');c.strokeStyle=this.color('#70bfff');c.lineWidth=2;c.beginPath();for(let i=0;i<60;i++){const x=19+i*202/59,y=279-(Math.abs(Math.sin(i*.7))*12+(i===37?20:0));if(i)c.lineTo(x,y);else c.moveTo(x,y);}c.stroke();for(const [i,label] of ['8/30','9/21','10/16'].entries())this.text(label,18+i*81,286,12,muted);
  }
  render(){
    // Publish one completed frame. Never reveal intermediate clear/draw operations.
    this.drawFrame();this.screenCtx.drawImage(this.frame,0,0);
  }
  drawFrame(){
    const m=this.model,c=this.ctx;this.canvas.dataset.page=m.page;this.canvas.dataset.period=m.period;this.canvas.dataset.volume=m.volume;this.canvas.dataset.awake=String(m.awake);this.canvas.dataset.on=String(m.on);this.canvas.dataset.details=String(m.details);this.canvas.dataset.contact=String(m.contact);this.canvas.dataset.overlay=String(m.overlay);
    c.fillStyle=this.color(bg);c.fillRect(0,0,240,320);$('simVolumeBar').style.setProperty('--volume-accent',window.DeviceTheme?.colors().accent||teal);$('simVolumeBar').hidden=!(m.overlay&&m.on&&m.awake);$('simVolumeBar').style.setProperty('--volume',m.volume+'%');$('simVolumeBar').setAttribute('aria-valuenow',m.volume);
    $('simTask').disabled=$('simQuota').disabled=!m.on;$('simTask').textContent=m.running?'结束思考':'开始思考';$('simQuota').textContent=m.exhausted?'恢复额度':'额度耗尽';$('simPower').setAttribute('aria-pressed',String(m.on));
    const state=!m.on?'已关机':!m.awake?'已熄屏':`${names[m.page]}${m.page<2?' · '+periods[m.period]:''} · 音量 ${m.volume}%${m.page===4&&m.petAction?' · '+m.petAction:''}`;if($('simState').textContent!==state)$('simState').textContent=state;if($('themeSimState'))$('themeSimState').textContent=state;
    if(!m.on||!m.awake){c.fillStyle='#050b10';c.fillRect(0,0,240,320);return;}
    if(performance.now()<m.bootUntil){c.fillStyle='#000';c.fillRect(0,0,240,320);if(m.config.drawBoot)m.config.drawBoot(c);else if(this.images.startup.complete&&this.images.startup.naturalWidth)c.drawImage(this.images.startup,0,0,240,320);return;}
    this.drawPage();
  }
  previewURL(page){
    const model=this.model,ctx=this.ctx,c=document.createElement('canvas');c.width=240;c.height=320;
    try{this.ctx=c.getContext('2d');this.model=Object.assign(Object.create(Object.getPrototypeOf(model)),model,{page,contact:false});this.drawPage();return c.toDataURL();}
    finally{this.model=model;this.ctx=ctx;}
  }
  pet(){
    const state=this.model.petState??1,a=this.petAssets?.[state];
    this.ctx.fillStyle=state===0?'#788391':state===3?'#ff626c':state===2||state===5?'#4cdd91':'#5ba7ff';this.ctx.beginPath();this.ctx.arc(26,23,6,0,7);this.ctx.fill();
    const short=['未连接','待命中','运行中','额度不足','听写中','操作中'];
    this.text('Codex / '+short[state],39,14);this.text('86%',222,14,14,muted,'right');
    if(a?.image.complete&&a.image.naturalWidth){let t=(performance.now()-(this.model.petStarted||0))%a.durations.reduce((x,y)=>x+y,0),frame=0;while(frame<a.durations.length-1&&t>=a.durations[frame])t-=a.durations[frame++];this.ctx.imageSmoothingEnabled=false;this.ctx.drawImage(a.image,frame*192,0,192,208,24,65,192,208);}
  }
  drawPage(){const m=this.model,c=this.ctx;c.fillStyle=this.color(bg);c.fillRect(0,0,240,320);
    if(m.page===0)this.home();if(m.page===1)this.analytics();if(m.page===2){c.drawImage($('monitorPreview'),0,0);c.fillStyle=this.color(bg);c.fillRect(0,0,240,38);this.header(true);}
    if(m.page===3){renderFace(this.face,m.config.content,m.contact,true);c.drawImage(this.face,0,38);if(m.config.postcardDate!==false)this.header(true);c.fillStyle=this.color(bg);c.fillRect(70,0,170,38);const now=new Date();if(m.config.postcardDate!==false)this.text(`${now.getMonth()+1}月${now.getDate()}日, 星期${'日一二三四五六'[now.getDay()]}`,222,14,14,ink,'right');}
    if(m.page===4)this.pet();
    if(m.page===5){m.config.wallpaper?.(c);return;}this.marks();
  }
}

// Palette thumbnails share the screen renderer without changing the interactive simulator.
export function renderThemePreviews(){
 for(const [id,palette] of Object.entries(window.DeviceTheme.previews())){
  const canvas=document.querySelector(`[data-theme-preview="${id}"]`);if(!canvas)continue;
  const preview=Object.create(ExperienceSimulator.prototype);
  preview.ctx=canvas.getContext('2d');preview.previewPalette=palette;
  preview.model=new ExperienceModel({mask:15,order:[0,1,2,3],volume:35},0);
  preview.drawPage();
 }
}
