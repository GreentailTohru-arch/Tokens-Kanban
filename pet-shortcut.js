import {completeSave,cancelSaveCompletion} from './save-progress.js';
import {createShortcutPicker,normalizeShortcut} from './shortcut-picker.js?v=31';
const button=document.getElementById('petCustomSave'),help=document.getElementById('petCustomHelp');
let values=Array(8).fill(''),pending=null;
const changed=()=>document.dispatchEvent(new CustomEvent('petshortcutchange'));
window.PetShortcuts={get:slot=>values[slot]||''};
const picker=createShortcutPicker(value=>{
 if(!pending)return;const {slot,confirm}=pending;pending=null;
 values[slot]=value;help.textContent='已修改此手势的快捷键，请保存本机快捷键；其他手势保持不变。';confirm?.();changed();
});
document.addEventListener('pickpetshortcut',event=>{
 const {slot,source,confirm}=event.detail;if(!Number.isInteger(slot)||slot<0||slot>7)return;
 pending={slot,confirm};picker.open(values[slot],source);
 document.getElementById('shortcutPickerTitle').textContent=['上键单击','上键双击','上键三击','下键单击','下键双击','下键三击','OK 单击','OK 三击'][slot]+' · 自定义快捷键';
});
const token=document.querySelector('meta[name="passport-token"]')?.content;
if(token){
 const headers={'Authorization':'Bearer '+token,'Content-Type':'application/json'};
 const request=async(options)=>{const r=await fetch('/api/pet-shortcut',{headers,...options});if(!r.ok)throw new Error(await r.text());return r;};
 try{const data=await (await request()).json();if(!Array.isArray(data.shortcuts)||data.shortcuts.length!==8)throw Error('请更新电脑同步程序以分别保存快捷键');values=data.shortcuts.map(normalizeShortcut);changed();button.disabled=false;}catch(e){help.textContent=e.message;}
 button.onclick=async()=>{button.disabled=true;try{const r=await request({method:'POST',body:JSON.stringify({shortcuts:values})});help.textContent=await r.text();completeSave();}catch(e){cancelSaveCompletion();help.textContent=e.message;}finally{button.disabled=false;}};
}else help.textContent='每个手势可单独选择快捷键；从新版电脑同步程序打开此页后，可保存到本机。';
changed();
