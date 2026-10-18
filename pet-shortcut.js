import {completeSave,cancelSaveCompletion} from './save-progress.js';
import {createShortcutPicker,normalizeShortcut} from './shortcut-picker.js?v=30-keyboard';
const input=document.getElementById('petCustomHotkey'),button=document.getElementById('petCustomSave'),help=document.getElementById('petCustomHelp');
const clear=document.getElementById('petCustomClear');
let pendingConfirm=null;
const changed=()=>document.dispatchEvent(new CustomEvent('petshortcutchange'));
const picker=createShortcutPicker(value=>{input.value=value;help.textContent='快捷键已选择，请保存到本机后使用。所有自定义按键共用此组合。';const callback=pendingConfirm;pendingConfirm=null;callback?.();changed();});
const open=(callback=null,source=document.activeElement)=>{pendingConfirm=callback;picker.open(input.value,source);};
input.onclick=()=>open(null,input);input.onkeydown=e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();open(null,input);}};
document.addEventListener('pickpetshortcut',event=>open(event.detail?.confirm,event.detail?.source));
clear.onclick=()=>{input.value='';help.textContent='已清空，保存后禁用自定义快捷键。';changed();};
const token=document.querySelector('meta[name="passport-token"]')?.content;
if(token){
 const headers={'Authorization':'Bearer '+token,'Content-Type':'application/json'};
 const request=async(options)=>{const r=await fetch('/api/pet-shortcut',{headers,...options});if(!r.ok)throw new Error(await r.text());return r;};
 try{input.value=normalizeShortcut((await (await request()).json()).shortcut);changed();button.disabled=false;clear.disabled=false;}catch(e){help.textContent=e.message;}
 button.onclick=async()=>{button.disabled=true;try{const r=await request({method:'POST',body:JSON.stringify({shortcut:input.value})});help.textContent=await r.text();completeSave();}catch(e){cancelSaveCompletion();help.textContent=e.message;}finally{button.disabled=false;}};
}else help.textContent='可预览选择快捷键；从电脑同步程序打开此页后，可保存到本机。';
