import qrcode from './vendor/qrcode.js';
const $=id=>document.getElementById(id),wait=ms=>new Promise(r=>setTimeout(r,ms));
const states=['已关闭','等待重连','正在连接','已连接','密码或认证失败','配网热点已开启','网络暂不可用'];
const hex=text=>Array.from(new TextEncoder().encode(text),b=>b.toString(16).padStart(2,'0')).join('');
const unhex=text=>new TextDecoder('utf-8',{fatal:true}).decode(Uint8Array.from(text.match(/../g)||[],v=>parseInt(v,16)));
export class HTTPConnection {
 constructor(){this.commands=Promise.resolve();this.kind='Wi-Fi';this.closed=false;this.timeout=12000;this.token=location.hash.slice(1)||document.querySelector('meta[name="passport-token"]')?.content||'';}
 async info(){const response=await fetch('/api/info',{headers:{Authorization:'Bearer '+this.token},cache:'no-store',signal:AbortSignal.timeout(5000)});if(!response.ok)throw Error('管理入口暂不可用');return response.json();}
 async open(){if(!/^[0-9a-f]{32}$/.test(this.token))throw Error('请扫描设备上的管理二维码进入');const info=await this.info();if(!info.connected)throw Error('电脑程序正在等待设备连接');this.kind=info.kind;this.revision=info.revision;this.timer=setInterval(async()=>{if(this.checking||this.closed)return;this.checking=true;try{const state=await this.info();if(!state.connected||state.revision!==this.revision){this.onlost?.();clearInterval(this.timer);}}catch{this.onlost?.();clearInterval(this.timer);}finally{this.checking=false;}},3000);}
 rpc(line){const task=this.commands.then(async()=>{if(this.closed)throw Error('连接已断开');const response=await fetch('/api/rpc',{method:'POST',headers:{Authorization:'Bearer '+this.token,'Content-Type':'text/plain'},body:line,signal:AbortSignal.timeout(this.timeout)});if(!response.ok)throw Error(response.status===401?'管理授权已失效，请重新扫码':'设备暂不可用，请等待电脑程序重连');const text=(await response.text()).trim();if(!text.startsWith('PC_')||text.length>255)throw Error('设备响应无效');return text;});this.commands=task.catch(()=>{});return task;}
 async close(){this.intentional=true;this.closed=true;clearInterval(this.timer);this.onlost?.();}
}
export class NetworkPanel {
 constructor({getLink,isBusy,setBusy,status}){Object.assign(this,{getLink,isBusy,setBusy,status});this.supported=false;this.url='';
  $('wifiForm').onsubmit=e=>{e.preventDefault();this.change('SET');};$('wifiToggle').onclick=()=>this.change('ENABLE');$('wifiForget').onclick=()=>this.change('FORGET');$('wifiSetup').onclick=()=>this.change('SETUP');$('wifiRefresh').onclick=()=>this.refresh();
 }
 controls(){const locked=this.isBusy()||!this.getLink()||!this.supported;for(const id of ['wifiSave','wifiToggle','wifiForget','wifiSetup','wifiRefresh'])$(id).disabled=locked;$('wifiSSID').disabled=$('wifiPassword').disabled=locked;}
 async read(link){
  const p=(await link.rpc('PC_WIFI_GET')).split(' ');this.supported=p.length===8&&p[0]==='PC_WIFI'&&p[1]==='1';
  if(!this.supported){$('wifiState').textContent='此固件尚不支持 Wi-Fi，请更新到 v19 或更高版本';$('wifiShare').hidden=true;this.controls();return;}
  if(p.slice(2,6).some(v=>!/^\d+$/.test(v))||+p[2]>6)throw Error('网络状态无效');
  this.enabled=p[3]==='1';this.configured=p[4]==='1';this.ap=p[5]==='1';this.ip=p[6];
  $('wifiState').textContent=states[+p[2]]+(this.ip!=='-'?' · '+this.ip:'');$('wifiToggle').textContent=this.enabled?'关闭 Wi-Fi':'开启 Wi-Fi';
  const ssid=(await link.rpc('PC_WIFI_SSID')).split(' ');if(ssid.length!==2||ssid[0]!=='PC_WIFI_SSID')throw Error('网络名称响应无效');this.ssid=ssid[1]==='-'?'':unhex(ssid[1]);$('wifiSavedSSID').textContent=this.ssid?'已保存：'+this.ssid:'尚未保存网络';
  const access=(await link.rpc('PC_WIFI_ACCESS')).split(' ');if(access.length!==3||!/^\w{12}$/.test(access[1])||! /^[0-9a-f]{32}$/.test(access[2]))throw Error('管理授权响应无效');this.token=access[2];
  this.url=/^\d+\.\d+\.\d+\.\d+$/.test(this.ip)?'http://'+this.ip+'/#'+this.token:this.ap?'http://192.168.4.1/#'+this.token:'';
  $('wifiShare').hidden=!this.url;$('wifiURL').href=this.url;$('wifiURL').textContent=this.ap?'打开配网管理页':'打开设备管理页';
  if(this.url)this.draw('wifiQR',this.url);
  $('wifiAPInfo').hidden=!this.ap;
  if(this.ap){const a=(await link.rpc('PC_WIFI_AP')).split(' ');if(a.length!==3||a[0]!=='PC_WIFI_AP'||! /^[0-9a-f]{12}$/.test(a[2]))throw Error('热点响应无效');const name=unhex(a[1]);$('wifiAPName').textContent=name;$('wifiAPPassword').textContent=a[2];this.draw('wifiJoinQR',`WIFI:T:WPA;S:${name};P:${a[2]};;`);}
  this.controls();
 }
 draw(id,text){const qr=qrcode(0,'M');qr.addData(text);qr.make();const n=qr.getModuleCount(),scale=Math.floor(240/(n+8)),canvas=$(id);canvas.width=canvas.height=(n+8)*scale;const ctx=canvas.getContext('2d');ctx.fillStyle='#fff';ctx.fillRect(0,0,canvas.width,canvas.height);ctx.fillStyle='#17212a';for(let y=0;y<n;y++)for(let x=0;x<n;x++)if(qr.isDark(y,x))ctx.fillRect((x+4)*scale,(y+4)*scale,scale,scale);}
 async refresh(){if(!this.getLink()||this.isBusy())return;this.setBusy(true);try{await this.read(this.getLink());this.status('网络状态已刷新');}catch(e){this.status(e.message,'error');}finally{this.setBusy(false);}}
 async change(kind){if(!this.getLink()||this.isBusy())return;const link=this.getLink(),id=(crypto.getRandomValues(new Uint32Array(1))[0]>>>1)||1;let command=`PC_WIFI_${kind} ${id}`;
  if(kind==='SET'){
   const ssid=$('wifiSSID').value,password=$('wifiPassword').value,bytes=new TextEncoder();
   if(!ssid||bytes.encode(ssid).length>32||password.includes('\0')||ssid.includes('\0')||(password&&(bytes.encode(password).length<8||bytes.encode(password).length>63))){this.status('网络名称最多32字节，密码为8～63字节；开放网络密码留空','error');return;}
   if(link.kind.includes('蓝牙')){this.status('蓝牙连接请先开启配网热点，用手机扫码连接后填写 Wi-Fi 密码；也可改用 USB 配网','error');return;}
   command+=' '+hex(ssid)+' '+(password?hex(password):'-');
  }else if(kind==='ENABLE')command+=' '+(this.enabled?0:1);
  this.setBusy(true);
  try{let reply=await link.rpc(command),done=false;for(let n=0;n<80;n++){const p=reply.split(' ');if(p[0]!=='PC_WIFI_STATE'||+p[1]!==id)throw Error('设备正忙或网络设置被拒绝');if(p[2]==='3')throw Error('网络设置失败，请刷新状态后重试');if(p[2]==='2'){done=true;break;}await wait(100);reply=await link.rpc('PC_WIFI_STATUS');}if(!done)throw Error('网络设置仍在处理中，请稍后刷新');$('wifiPassword').value='';$('wifiSSID').value='';await this.read(link);this.status(kind==='SET'?'网络已保存，设备会自动连接':'网络设置已更新');}
  catch(e){this.status(e.message+(link instanceof HTTPConnection?'；切换网络会中断网页，请用设备二维码重新进入':''),'error');}finally{this.setBusy(false);}
 }
}
