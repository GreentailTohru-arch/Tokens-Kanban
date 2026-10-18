import {completeSave,cancelSaveCompletion} from './save-progress.js';
import {attachRecorder} from './shortcut-recorder.js';
const input=document.getElementById('petCustomHotkey'),button=document.getElementById('petCustomSave'),help=document.getElementById('petCustomHelp');
const clear=document.getElementById('petCustomClear');
attachRecorder(input,()=>{help.textContent='快捷键已录入，请保存到本机。系统保留快捷键可能无法被浏览器捕获。';},message=>{help.textContent=message;});
clear.onclick=()=>{input.value='';help.textContent='已清空，保存后禁用自定义快捷键。';};
const token=document.querySelector('meta[name="passport-token"]')?.content;
if(token){
 const headers={'Authorization':'Bearer '+token,'Content-Type':'application/json'};
 const request=async(options)=>{const r=await fetch('/api/pet-shortcut',{headers,...options});if(!r.ok)throw new Error(await r.text());return r;};
 try{input.value=(await (await request()).json()).shortcut;button.disabled=false;clear.disabled=false;}catch(e){help.textContent=e.message;}
 button.onclick=async()=>{button.disabled=true;try{const r=await request({method:'POST',body:JSON.stringify({shortcut:input.value})});help.textContent=await r.text();completeSave();}catch(e){cancelSaveCompletion();help.textContent=e.message;}finally{button.disabled=false;}};
}else input.disabled=true;
