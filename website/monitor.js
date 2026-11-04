const names=['CPU 占用','GPU 占用','RAM 占用','CPU 温度','GPU 温度','CPU 频率','硬盘占用','上传速率','下载速率','总网速','今日流量','NET 速率'];
const options=['CPU 利用率','GPU 利用率','内存利用率','CPU 温度','GPU 温度','CPU 频率','硬盘利用率','上传速率','下载速率','总网速','今日总流量','上传＋下载'];
const sample=[8,8,64,570,510,9910,1000,3380,12,1073741824,2147483648];
const $=id=>document.getElementById(id),none=()=>({metric:255,enabled:0,title:''});
export function defaultMonitor(){return {bits:0,unit:0,show_unit:1,show_percent:1,space:0,decimals:1,compact:0,up_label:'↑',down_label:'↓',cards:[0,1,2,3,4,11].map(metric=>({metric,enabled:1,title:''}))};}
export function encodeMonitor(c){
  const b=new Uint8Array(148),enc=new TextEncoder();b.set([1,c.bits,c.unit,c.show_unit,c.show_percent,c.space,c.decimals,c.compact]);
  function text(value,offset,cap){const v=enc.encode(value);if(v.length>=cap)throw Error('文字超过设备存储限制');b.set(v,offset);}
  text(c.up_label,8,7);text(c.down_label,15,7);
  c.cards.forEach((card,i)=>{const off=22+i*21;b[off]=card.metric;b[off+1]=card.enabled;text(card.title,off+2,19);});
  return Array.from(b,v=>v.toString(16).padStart(2,'0')).join('');
}
export function decodeMonitor(hex){
  if(!/^[0-9a-f]{296}$/.test(hex))throw Error('设备检测配置长度错误');const b=Uint8Array.from(hex.match(/../g),h=>parseInt(h,16));
  if(b[0]!==1||b[1]>1||b[2]>4||b[3]>1||b[4]>1||b[5]>1||b[6]>2||b[7]>1)throw Error('设备显示设置无效');
  function text(offset,cap){const v=b.slice(offset,offset+cap),end=v.indexOf(0);if(end<0||v.slice(end).some(n=>n))throw Error('设备文字编码无效');return new TextDecoder('utf-8',{fatal:true}).decode(v.slice(0,end));}
  const c={bits:b[1],unit:b[2],show_unit:b[3],show_percent:b[4],space:b[5],decimals:b[6],compact:b[7],up_label:text(8,7),down_label:text(15,7),cards:[]};
  for(let i=0;i<6;i++){const off=22+i*21,metric=b[off],enabled=b[off+1];if((metric>11&&metric!==255)||enabled>1||(metric===255&&enabled))throw Error('设备卡片配置无效');c.cards.push({metric,enabled,title:text(off+2,19)});}return c;
}
export function layout(n,i){const rows=n<=2?n:Math.ceil(n/2),full=n<=2||(n%2&&i===n-1),row=n<=2?i:Math.floor(i/2),h=rows?Math.floor((196-(rows-1)*8)/rows):196;return {x:14+(full?0:i%2*110),y:90+row*(h+8),w:full?212:102,h};}
export function contentLayout(w,h,pair){const pad=h>=150?12:h>=90?8:2,top=pad+18+(h>=90?8:2);return {x:w>=200?14:9,titleY:pad,top,height:h-pad-top,gap:h>=150?8:h>=90?4:0,fontSize:h>=150?32:h>=90?(pair?20:32):(pair?14:20)};}
export function fitMonitorText(ctx,cfg,metric,w,h){
 const pair=metric===11,b=contentLayout(w,h,pair),lineHeights={14:18,20:22,32:35};let lines=values(cfg,metric),size=14,spacing=0;
 ctx.letterSpacing='0px';
 const fits=()=>lines.every(t=>ctx.measureText(t).width<=w-b.x*2);
 for(const candidate of [32,20,14].filter(v=>v<=b.fontSize)){ctx.font=`${candidate}px "Microsoft YaHei UI",sans-serif`;if(lineHeights[candidate]*(pair?2:1)+(pair?b.gap:0)<=b.height&&fits()){size=candidate;break;}}
 ctx.font=`${size}px "Microsoft YaHei UI",sans-serif`;
 if(!fits()){lines=values({...cfg,compact:1,decimals:0},metric);while(spacing>-3&&!fits()){ctx.letterSpacing=(--spacing)+'px';}}
 const lineHeight=lineHeights[size],block=lineHeight*(pair?2:1)+(pair?b.gap:0);
 return {...b,lines,size,spacing,lineHeight,y:b.top+(b.height-block)/2};
}
function quantity(c,bytes,rate=true){
  if(bytes<0)return '--';let v=bytes*(rate&&c.bits?8:1),u=rate?c.unit:0;
  if(!u){u=1;while(v>=1024&&u<4){v/=1024;u++;}}else for(let i=1;i<u;i++)v/=1024;
  const d=c.compact&&v>=100?0:c.decimals,n=v>=100000?v.toExponential(0):v.toFixed(u===1?0:d);
  const unit=(rate&&c.bits?['','b','Kb','Mb','Gb']:['','B','KB','MB','GB'])[u];return n+((!rate||c.show_unit)?(c.space?' ':'')+unit+(rate?'/s':''):'');
}
export function values(c,metric){
  if(metric===11)return [c.down_label+(c.down_label?' ':'')+quantity(c,sample[5]),c.up_label+(c.up_label?' ':'')+quantity(c,sample[6])];
  if(metric===7||metric===8)return [quantity(c,sample[metric===7?6:5])];
  if(metric===9)return [quantity(c,sample[5]+sample[6])];if(metric===10)return [quantity(c,sample[9]+sample[10],false)];
  const field=[0,1,2,3,4,7,8][metric],v=sample[field];if(metric===3||metric===4)return [(v/10).toFixed(c.decimals)+(c.space?' ':'')+'°C'];
  if(metric===5)return [(v/1000).toFixed(c.decimals)+(c.space?' ':'')+'GHz'];return [String(v)+(c.show_percent?(c.space?' ':'')+'%':'')];
}
export class MonitorEditor {
  constructor({status,getLink,setBusy,controls,onRender,onSaved}){
    Object.assign(this,{status,getLink,setBusy,controls,onRender,onSaved});this.config=defaultMonitor();this.supported=false;this.coverage=null;this.dragIndex=-1;this.locked=false;
    for(const id of ['monBits','monUnit','monDecimals','monShowUnit','monPercent','monSpace','monCompact','monUpLabel','monDownLabel'])$(id).addEventListener('input',()=>this.changeDisplay());
    $('monAdd').onclick=()=>{const i=this.config.cards.findIndex(c=>c.metric===255);if(i<0){status('最多六张卡片，请先删除一张再新增','error');return;}this.config.cards.splice(i,1);this.config.cards.push({metric:0,enabled:1,title:''});this.changed();};
    $('monResetText').onclick=()=>{this.config.up_label='↑';this.config.down_label='↓';for(const c of this.config.cards)c.title='';this.put();this.changed();};
    $('monReset').onclick=()=>{this.config=defaultMonitor();this.put();this.changed();};$('monitorForm').addEventListener('submit',e=>this.save(e));
    this.put();this.render();fetch(new URL('font-coverage.json',import.meta.url)).then(r=>{if(!r.ok)throw Error('字体清单加载失败');return r.json();}).then(c=>{this.coverage=new Set(c);this.validate();controls();}).catch(e=>status(e.message,'error'));
  }
  canSave(){return this.supported&&!!this.coverage;}
  setLocked(value){this.locked=value;$('monitorFields').disabled=value;}
  put(){const c=this.config;$('monBits').value=c.bits;$('monUnit').value=c.unit;$('monDecimals').value=c.decimals;for(const [id,key] of [['monShowUnit','show_unit'],['monPercent','show_percent'],['monSpace','space'],['monCompact','compact']])$(id).checked=!!c[key];$('monUpLabel').value=c.up_label;$('monDownLabel').value=c.down_label;}
  changeDisplay(){const c=this.config;c.bits=+$('monBits').value;c.unit=+$('monUnit').value;c.decimals=+$('monDecimals').value;for(const [id,key] of [['monShowUnit','show_unit'],['monPercent','show_percent'],['monSpace','space'],['monCompact','compact']])c[key]=+$(id).checked;c.up_label=$('monUpLabel').value;c.down_label=$('monDownLabel').value;this.changed(false);}
  checkText(value,max){if([...value].length>max)return `最多 ${max} 个字符`;if(this.coverage){const bad=[...value].find(ch=>!this.coverage.has(ch.codePointAt(0))||ch.codePointAt(0)<32);if(bad)return `设备字体不支持“${bad}”，请使用常用中文、英文或箭头`;}return '';}
  validate(){let message=this.checkText(this.config.up_label,2)||this.checkText(this.config.down_label,2);for(const c of this.config.cards)message ||= this.checkText(c.title,6);$('monTextError').textContent=message;$('monTextError').hidden=!message;$('monUpLabel').setCustomValidity(this.checkText(this.config.up_label,2));$('monDownLabel').setCustomValidity(this.checkText(this.config.down_label,2));for(const input of $('monCards').querySelectorAll('input[data-title]'))input.setCustomValidity(this.checkText(input.value,6));return !message;}
  changed(rows=true){$('monitorState').textContent='有未保存的检测设置';if(rows)this.render();else this.draw();this.validate();this.controls();}
  move(from,to){const cards=this.config.cards,item=cards.splice(from,1)[0];cards.splice(to,0,item);this.changed();}
  render(){
    const list=$('monCards');list.replaceChildren();const present=this.config.cards.filter(c=>c.metric!==255);$('monCount').textContent=`${present.filter(c=>c.enabled).length} / 6 已开启`;
    this.config.cards=[...present,...Array.from({length:6-present.length},none)];
    present.forEach((card,index)=>{
      const row=document.createElement('article');row.className='monitor-card'+(card.enabled?'':' module-off');row.dataset.index=index;
      const head=document.createElement('div');head.className='module-heading';const drag=document.createElement('button');drag.type='button';drag.className='drag-handle secondary';drag.textContent='⠿';drag.draggable=true;drag.setAttribute('aria-label',`拖动第${index+1}张卡片`);drag.ondragstart=e=>{this.dragIndex=index;e.dataTransfer.setData('text/plain',String(index));e.dataTransfer.effectAllowed='move';};head.append(drag);
      const select=document.createElement('select');select.setAttribute('aria-label',`第${index+1}张卡片项目`);options.forEach((name,id)=>select.add(new Option(name,id)));select.value=card.metric;select.onchange=()=>{card.metric=+select.value;this.changed();};head.append(select);
      const toggle=document.createElement('input');toggle.type='checkbox';toggle.setAttribute('role','switch');toggle.setAttribute('aria-label',`第${index+1}张卡片开启`);toggle.checked=!!card.enabled;toggle.onchange=()=>{if(toggle.checked&&this.config.cards.filter(c=>c.enabled).length>=6){toggle.checked=false;this.status('最多开启六张卡片','error');return;}card.enabled=+toggle.checked;this.changed();};const switchLabel=document.createElement('label');switchLabel.className='toggle-row';switchLabel.append(toggle);head.append(switchLabel);row.append(head);
      const label=document.createElement('label');label.className='module-title-label';label.textContent='显示标题';const input=document.createElement('input');input.type='text';input.maxLength=6;input.dataset.title=index;input.setAttribute('aria-label',`第${index+1}张卡片标题`);input.placeholder=names[card.metric];input.value=card.title;input.oninput=()=>{card.title=input.value;this.changed(false);};label.append(input);row.append(label);
      const actions=document.createElement('div');actions.className='module-actions';for(const [delta,text] of [[-1,'上移'],[1,'下移']]){const b=document.createElement('button');b.type='button';b.className='secondary';b.textContent=text;b.disabled=index+delta<0||index+delta>=present.length;b.setAttribute('aria-label',`第${index+1}张卡片${text}`);b.onclick=()=>this.move(index,index+delta);actions.append(b);}const remove=document.createElement('button');remove.type='button';remove.className='secondary';remove.textContent='删除';remove.setAttribute('aria-label',`删除第${index+1}张卡片`);remove.onclick=()=>{this.config.cards.splice(index,1);this.config.cards.push(none());this.changed();};actions.append(remove);row.append(actions);
      row.ondragover=e=>{e.preventDefault();e.dataTransfer.dropEffect='move';};row.ondrop=e=>{e.preventDefault();const raw=e.dataTransfer.getData('text/plain'),from=raw===''?-1:Number(raw);if(from===this.dragIndex&&Number.isInteger(from)&&from>=0&&from<present.length&&from!==index)this.move(from,index);this.dragIndex=-1;};drag.ondragend=()=>{this.dragIndex=-1;};list.append(row);
    });this.draw();
  }
  draw(){
    const c=$('monitorPreview'),x=c.getContext('2d'),cfg=this.config;x.fillStyle=window.DeviceTheme?.map('#2f3a49')||'#2f3a49';x.fillRect(0,0,240,320);
    const font=(size=14)=>`${size}px "Microsoft YaHei UI",sans-serif`;x.font=font();x.textBaseline='top';x.fillStyle=window.DeviceTheme?.map('#f1f7f4')||'#f1f7f4';x.fillText('00:00',18,14);x.fillStyle=window.DeviceTheme?.map('#adb8c3')||'#adb8c3';x.fillText('86%',195,14);x.fillStyle=window.DeviceTheme?.map('#f1f7f4')||'#f1f7f4';x.fillText('系统检测',18,43);x.fillStyle=window.DeviceTheme?.map('#adb8c3')||'#adb8c3';x.fillText('TrafficMonitor / 示例数据',18,64);
    const cards=cfg.cards.filter(c=>c.enabled);if(!cards.length){x.textAlign='center';x.fillText('暂无检测卡片',120,156);x.textAlign='left';}
    cards.forEach((card,i)=>{
      const p=layout(cards.length,i),b=contentLayout(p.w,p.h,card.metric===11);
      x.fillStyle=window.DeviceTheme?.map('#374453')||'#374453';x.beginPath();x.roundRect(p.x,p.y,p.w,p.h,9);x.fill();
      x.save();x.beginPath();x.rect(p.x+b.x,p.y,p.w-b.x*2,p.h);x.clip();
      x.font=font();x.textAlign='left';x.fillStyle=window.DeviceTheme?.map('#adb8c3')||'#adb8c3';x.fillText(card.title||names[card.metric],p.x+b.x,p.y+b.titleY);
      const f=fitMonitorText(x,cfg,card.metric,p.w,p.h);x.textAlign='center';
      x.fillStyle=window.DeviceTheme?.map('#f1f7f4')||'#f1f7f4';x.fillText(f.lines[0],p.x+p.w/2,p.y+f.y);
      if(f.lines[1]){x.fillStyle=window.DeviceTheme?.map('#adb8c3')||'#adb8c3';x.fillText(f.lines[1],p.x+p.w/2,p.y+f.y+f.lineHeight+f.gap);}x.restore();
    });
    x.fillStyle=window.DeviceTheme?.map('#52616e')||'#52616e';for(let i=0;i<4;i++){x.beginPath();x.roundRect(67+i*28,309,22,3,2);x.fill();}x.fillStyle=window.DeviceTheme?.map('#68c4cb')||'#68c4cb';x.fillRect(123,309,22,3);this.onRender?.(this.previewURL());
  }
  previewURL(){return $('monitorPreview').toDataURL();}
  async read(connection){
    const reply=await connection.rpc('PC_MON_GET'),parts=reply.split(' ');this.supported=parts.length===3&&parts[0]==='PC_MON'&&parts[1]==='1'&&/^\d+$/.test(parts[2]);
    if(this.supported){let hex='';for(let off=0;off<148;off+=16){const block=(await connection.rpc('PC_MON_READ '+off)).split(' ');if(block.length!==4||block[0]!=='PC_MON_DATA'||block[1]!==parts[2]||+block[2]!==off||!/^[0-9a-f]+$/.test(block[3])||block[3].length!==Math.min(16,148-off)*2)throw Error('检测配置已变化，请重新连接读取');hex+=block[3];}this.config=decodeMonitor(hex);this.put();this.render();this.validate();}
    $('monitorState').textContent=this.supported?'已读取设备检测配置':'连接 v15 固件后可保存；当前为示例预览';
  }
  async save(event){
    event.preventDefault();const link=this.getLink();if(!link||this.locked||!this.canSave()||!this.validate())return;
    const draft=JSON.parse(JSON.stringify(this.config)),wanted=encodeMonitor(this.config),id=(crypto.getRandomValues(new Uint32Array(1))[0]>>>1)||1;this.setBusy(true);$('monitorState').textContent='正在保存检测设置…';
    try{let reply=await link.rpc(`PC_MON_SET ${id} ${wanted}`),saved=false;for(let i=0;i<300;i++){const p=reply.split(' ');if(p[0]!=='PC_CFG_STATE'||+p[1]!==id)throw Error('设备暂忙，请稍后保存');if(p[2]==='3')throw Error('保存失败，原检测设置仍保留');if(p[2]==='2'){saved=true;break;}await new Promise(r=>setTimeout(r,50));reply=await link.rpc('PC_CFG_STATUS');}if(!saved)throw Error('保存超时，请重新连接');await this.read(link);if(!this.supported||encodeMonitor(this.config)!==wanted)throw Error('检测设置校验不一致');this.onSaved?.();$('monitorState').textContent='检测设置已保存到设备';this.status('检测设置已保存到设备');}catch(e){this.config=draft;this.put();this.render();$('monitorState').textContent='保存未确认，请重新连接读取';this.status(e.message,'error');}finally{this.setBusy(false);}
  }
}
