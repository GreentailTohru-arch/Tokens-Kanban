const $=id=>document.getElementById(id),defaultName="You's AI Passport";
const fields=['Cpu','Frequency','Heap','HeapDetail','Flash','StorageDetail','Battery','Voltage','Capacity','Chip','Firmware','Uptime','Keys'];
const bytes=n=>n>0?(n>=1048576?(n/1048576).toFixed(n%1048576?2:0)+' MiB':(n/1024).toFixed(0)+' KiB'):'--';
const numeric=(reply,prefix,count)=>{const p=reply.split(' ');if(p.length!==count+1||p[0]!==prefix||p.slice(1).some(v=>! /^-?\d+$/.test(v)||!Number.isSafeInteger(Number(v))))throw Error('设备信息格式错误');return p.slice(1).map(Number);};
export class DeviceInfoPanel {
  constructor({getLink,isBusy,setBusy,status,onSaved}){
    Object.assign(this,{getLink,isBusy,setBusy,status,onSaved});this.supported=false;this.active=false;this.polling=false;this.freshAt=0;this.lastUptime=-1;this.keyCounts=null;this.testBusy=false;
    $('deviceName').value=defaultName;$('deviceName').oninput=()=>{this.validateName();$('deviceNameHint').textContent='名称有未保存的修改';};$('deviceNameForm').onsubmit=e=>this.saveName(e);
    for(const [id,kind] of [['diagBattery',1],['diagMic',2],['diagSpeaker',3]])$(id).onclick=()=>this.test(kind);
    $('diagKeys').onclick=()=>{this.keyCounts=null;$('diagKeysState').textContent='请按实物的上／下／确定键，收到按下事件后会亮起';this.poll();};
    for(const b of document.querySelectorAll('[data-screen-color]'))b.onclick=()=>this.screen(+b.dataset.screenColor);
    this.interval=setInterval(()=>{if(!this.active||document.hidden)return;if(this.freshAt&&performance.now()-this.freshAt>6500)this.unavailable('数据已过期');this.poll();},2000);this.unavailable('未连接设备');
  }
  setActive(active){this.active=active;if(active)this.poll();}
  canUse(){return !!this.getLink()&&this.supported;}
  controls(){const disabled=!this.canUse()||this.isBusy()||this.testBusy;$('deviceName').disabled=this.isBusy()||this.testBusy;$('deviceNameSave').disabled=disabled||!this.validateName();$('diagFields').disabled=disabled;for(const b of document.querySelectorAll('[data-screen-color]'))b.disabled=disabled;}
  validateName(){const value=$('deviceName').value.trim();let error='';if(!value)error='请输入设备名称';else if(new TextEncoder().encode(value).length>32)error='设备名称最多 32 个 UTF-8 字节（中文通常每字 3 字节）';else if(/[\u0000-\u001f\u007f-\u009f\u2028\u2029]/u.test(value))error='名称不能包含换行或控制字符';$('deviceName').setCustomValidity(error);$('deviceNameError').textContent=error;$('deviceNameError').hidden=!error;return !error;}
  unavailable(text){for(const f of fields)$('device'+f).textContent='--';$('deviceConnectionState').textContent=text;$('deviceConnectionState').classList.remove('live');for(const el of document.querySelectorAll('.diag-key-indicator'))el.classList.remove('active');}
  lost(){this.supported=false;this.freshAt=0;this.lastUptime=-1;this.keyCounts=null;this.unavailable('未连接设备');for(const id of ['diagBatteryState','diagMicState','diagSpeakerState','diagKeysState','diagScreenState'])$(id).textContent='未连接，等待检测';this.controls();}
  async read(connection){
    this.supported=(await connection.rpc('PC_INFO'))==='PC_INFO 1';
    if(!this.supported){this.unavailable('当前固件暂不支持设备检测');$('deviceNameHint').textContent='名称保存与实机检测需升级配套固件';this.controls();return;}
    await this.readName(connection);await this.snapshot(connection);this.controls();
  }
  async readName(connection){
    const p=(await connection.rpc('PC_INFO_NAME')).split(' ');
    if(p.length!==2||p[0]!=='PC_NAME'||!/^(?:[0-9a-f]{2}){1,32}$/.test(p[1]))throw Error('设备名称编码错误');
    const name=new TextDecoder('utf-8',{fatal:true}).decode(Uint8Array.from(p[1].match(/../g),h=>parseInt(h,16)));
    $('deviceName').value=name;this.validateName();
    this.nameBluetooth=false;
    try{this.nameBluetooth=(await connection.rpc('PC_NAME_BLE'))==='PC_NAME_BLE 1 29';}catch{}
    $('deviceNameHint').textContent=this.nameBluetooth
      ?'已读取设备名称 · 同步蓝牙名称，重新搜索可刷新'+(new TextEncoder().encode(name).length>29?'；长名称在蓝牙列表中缩短显示':'')
      :'已读取设备名称 · 当前固件仅修改设备名称，蓝牙联动需升级固件';
  }
  async snapshot(connection){
    const system=numeric(await connection.rpc('PC_INFO_SYSTEM'),'PC_SYSTEM',7),storage=numeric(await connection.rpc('PC_INFO_STORAGE'),'PC_STORAGE',5),battery=numeric(await connection.rpc('PC_INFO_BATTERY'),'PC_BATTERY',3),keys=numeric(await connection.rpc('PC_INFO_KEYS'),'PC_KEYS',6);
    const chip=(await connection.rpc('PC_INFO_CHIP')).split(' ');if(chip.length!==7||chip[0]!=='PC_CHIP'||!/^\d+$/.test(chip[2])||!/^\d+$/.test(chip[3])||!/^[0-9a-f]{12}$/.test(chip[4])||!/^[0-9a-f]{8,64}$/.test(chip[5]))throw Error('设备芯片信息错误');
    if(connection.closed||(this.getLink()&&connection!==this.getLink()))return;
    const [up,cpu,mhz,total,free,min,largest]=system,[flash,app,card,usedEntries,freeEntries]=storage,[soc,mv,design]=battery;
    if(up!==this.lastUptime){this.lastUptime=up;this.freshAt=performance.now();}if(performance.now()-this.freshAt>6500){this.unavailable('数据已过期');return;}
    $('deviceCpu').textContent=cpu>=0&&cpu<=1000?(cpu/10).toFixed(1)+'%':'--';$('deviceFrequency').textContent=mhz>0?mhz+' MHz':'--';$('deviceHeap').textContent=total>0&&free>=0&&free<=total?((total-free)/total*100).toFixed(1)+'%':'--';$('deviceHeapDetail').textContent=total>0?`可用 ${bytes(free)} / ${bytes(total)} · 最大连续 ${bytes(largest)} · 最低 ${bytes(min)}`:'--';
    $('deviceFlash').textContent=bytes(flash);$('deviceStorageDetail').textContent=flash>0?`应用分区 ${bytes(app)} · 明信片 ${bytes(card)}${usedEntries>=0&&freeEntries>=0?` · NVS ${usedEntries} / ${usedEntries+freeEntries} 条目`:''}`:'--';
    $('deviceBattery').textContent=soc>=0&&soc<=100?soc+'%':'--';$('deviceVoltage').textContent=mv>=0?(mv/1000).toFixed(3)+' V':'--';$('deviceCapacity').textContent=design>0?design+' mAh · 额定值':'--';$('deviceChip').textContent=chip[1]+' · '+chip[2]+' 核';$('deviceFirmware').textContent=chip[6]+' · '+chip[5];$('deviceUptime').textContent=up>=0?`${Math.floor(up/3600000)}时 ${Math.floor(up/60000)%60}分 ${Math.floor(up/1000)%60}秒`:'--';$('deviceKeys').textContent=keys[0]>=0?keys[0]+' mV':'--';
    const counts=keys.slice(1,4);for(let i=0;i<3;i++){const el=$('diagKey'+i);el.textContent=['↑ 上键','↓ 下键','✓ 确定'][i]+' · '+counts[i];if(this.keyCounts&&counts[i]>this.keyCounts[i]){el.classList.add('active');setTimeout(()=>el.classList.remove('active'),1800);}}this.keyCounts=counts;
    $('deviceConnectionState').textContent='设备实时 · '+connection.kind;$('deviceConnectionState').classList.add('live');this.controls();
  }
  async poll(){const connection=this.getLink();if(!connection||!this.supported||!this.active||this.polling||this.isBusy()||this.testBusy)return;this.polling=true;try{await this.snapshot(connection);}catch{if(this.getLink()===connection)this.unavailable('读取失败，请重连');}finally{this.polling=false;}}
  async saveName(event){
    event.preventDefault();if(!this.canUse()||this.isBusy()||!this.validateName())return;const connection=this.getLink(),name=$('deviceName').value.trim(),hex=Array.from(new TextEncoder().encode(name),n=>n.toString(16).padStart(2,'0')).join(''),id=(crypto.getRandomValues(new Uint32Array(1))[0]>>>1)||1;this.setBusy(true);
    try{let reply=await connection.rpc(`PC_NAME_SET ${id} ${hex}`),saved=false;for(let i=0;i<160;i++){const p=reply.split(' ');if(p[0]!=='PC_NAME_STATE'||+p[1]!==id)throw Error('设备暂忙，请稍后保存');if(p[2]==='3')throw Error('名称保存失败，原名称仍保留');if(p[2]==='2'){saved=true;break;}await new Promise(r=>setTimeout(r,100));reply=await connection.rpc('PC_NAME_STATE');}if(!saved)throw Error('名称保存超时，请重连读取');await this.readName(connection);if($('deviceName').value!==name)throw Error('名称读回不一致');this.onSaved?.();this.status('设备名称已保存');}catch(e){$('deviceName').value=name;$('deviceNameHint').textContent='保存未确认，请重新连接读取';this.status(e.message,'error');}finally{this.setBusy(false);this.controls();}
  }
  async test(kind){
    if(!this.canUse()||this.isBusy()||this.testBusy)return;this.testBusy=true;this.controls();const id=(crypto.getRandomValues(new Uint32Array(1))[0]>>>1)||1,connection=this.getLink(),target=$(kind===1?'diagBatteryState':kind===2?'diagMicState':'diagSpeakerState');target.textContent=kind===2?'正在采样约 0.4 秒…':'正在检测…';
    try{let reply=await connection.rpc(`PC_TEST ${id} ${kind}`),done=false;for(let i=0;i<200;i++){const p=numeric(reply,'PC_TEST_STATE',6);if(p[0]!==id||p[2]!==kind)throw Error('测试被其他请求占用');if(p[1]===3)throw Error(kind===3?'未能播放，请检查设备音量或稍后重试':'传感器未响应或测试失败');if(p[1]===2){done=true;target.textContent=kind===2?`采样完成 · 峰值 ${p[3]} / 32768 · 平均幅度 ${p[4]} · ${p[5]} 采样`:kind===3?'播放完成，请以实机听感确认':'电量计读取成功';break;}await new Promise(r=>setTimeout(r,100));reply=await connection.rpc('PC_TEST_STATE');}if(!done)throw Error('测试超时，请稍后重试');await this.snapshot(connection);}catch(e){target.textContent=e.message;this.status(e.message,'error');}finally{this.testBusy=false;this.controls();}
  }
  async screen(color){if(!this.canUse()){$('diagScreenState').textContent='请先连接设备';return;}if(this.isBusy()||this.testBusy)return;this.testBusy=true;this.controls();try{const reply=await this.getLink().rpc('PC_SCREEN '+color);if(reply!=='PC_SCREEN_OK')throw Error(color>4?'彩条与灰度实机测试需 v22 或更新固件':'设备暂未接受屏幕测试');$('diagScreenState').textContent=color<0?'已结束屏幕测试':'实机显示测试色，按任意键退出，或约15秒自动恢复';}catch(e){$('diagScreenState').textContent=e.message;this.status(e.message,'error');}finally{this.testBusy=false;this.controls();}}
}
