export const modifiers=['Ctrl','Shift','Alt','RightAlt','Win'];
export function normalizeShortcut(value){return value.split('+').map(k=>k==='LeftAlt'?'Alt':k).filter(Boolean).join('+');}
export function selectKey(value,key){
 const keys=normalizeShortcut(value).split('+').filter(Boolean);key=normalizeShortcut(key);
 if(keys.includes(key))return keys.filter(k=>k!==key).join('+');
 const next=modifiers.includes(key)?[...keys,key]:[...keys.filter(k=>modifiers.includes(k)),key];
 if(next.length>4)return null;
 return [...modifiers.filter(k=>next.includes(k)),...next.filter(k=>!modifiers.includes(k))].join('+');
}
// Reference keyboard geometry: a fixed six-row desktop layout, scaled as one unit.
const keys=[];
function row(y,items){let x=0;for(const [key,label,width=61] of items){keys.push({key,label,x,y,width,height:52,group:'主键盘'});x+=width+8;}}
keys.push({key:'Esc',label:'Esc',x:0,y:0,width:61,height:52,group:'主键盘'});
[101,170,239,308,409,478,547,616,717,786,855,924].forEach((x,i)=>keys.push({key:'F'+(i+1),label:'F'+(i+1),x,y:0,width:61,height:52,group:'主键盘'}));
row(62,[['Backquote','~\n`'],...['!','@','#','$','%','^','&','*','(',')'].map((s,i)=>[String((i+1)%10),s+'\n'+((i+1)%10)]),['Minus','—\n-'],['Equal','+\n='],['Backspace','Backspace',88]]);
row(124,[['Tab','Tab',75],...'QWERTYUIOP'.split('').map(k=>[k,k]),['BracketLeft','{\n['],['BracketRight','}\n]'],['Backslash','|\n\\',75]]);
row(187,[['CapsLock','Caps\nLock',110],...'ASDFGHJKL'.split('').map(k=>[k,k]),['Semicolon',':\n;'],['Quote','"\n\''],['Enter','Enter',110]]);
row(249,[['Shift','Shift',145],...'ZXCVBNM'.split('').map(k=>[k,k]),['Comma','<\n,'],['Period','>\n.'],['Slash','?\n/'],['Shift','Shift',145]]);
row(312,[['Ctrl','Ctrl',91],['Win','Win'],['Alt','Alt'],['Space','Space',477],['RightAlt','Alt'],['Win','Win'],['ContextMenu','Menu'],['Ctrl','Ctrl']]);
function auxiliary(group,x,y,key,label,width=61,height=52){keys.push({group,x,y,key,label,width,height});}
[['PrintScreen','Print\nScrn'],['ScrollLock','Scroll\nLock'],['Pause','Pause\nBreak'],['Insert','Insert'],['Home','Home'],['PageUp','Page\nUp'],['Delete','Delete'],['End','End'],['PageDown','Page\nDown']].forEach(([k,l],i)=>auxiliary('导航键',1016+(i%3)*69,Math.floor(i/3)*62,k,l));
auxiliary('导航键',1085,249,'Up','↑');
[['Left','←'],['Down','↓'],['Right','→']].forEach(([k,l],i)=>auxiliary('导航键',1016+i*69,312,k,l));
[['NumLock','Num\nLock'],['NumpadDivide','/'],['NumpadMultiply','*'],['NumpadSubtract','-']].forEach(([k,l],i)=>auxiliary('数字键盘',1241+i*69,62,k,l));
[['Numpad7','7\nHome'],['Numpad8','8\n↑'],['Numpad9','9\nPg Up'],['Numpad4','4\n←'],['Numpad5','5'],['Numpad6','6\n→'],['Numpad1','1\nEnd'],['Numpad2','2\n↓'],['Numpad3','3\nPg Dn']].forEach(([k,l],i)=>auxiliary('数字键盘',1241+i%3*69,124+Math.floor(i/3)*62.5,k,l));
auxiliary('数字键盘',1448,124,'NumpadAdd','+',61,115);
auxiliary('数字键盘',1448,249,'Enter','Enter',61,115);
auxiliary('数字键盘',1241,312,'Numpad0','0\nIns',130);
auxiliary('数字键盘',1379,312,'NumpadDecimal','.\nDel');
export function createShortcutPicker(onConfirm){
 const dialog=document.createElement('dialog');dialog.className='shortcut-dialog';dialog.setAttribute('aria-labelledby','shortcutPickerTitle');
 dialog.innerHTML=`<div class="shortcut-heading"><h2 id="shortcutPickerTitle">选择自定义快捷键</h2><button type="button" class="secondary" data-close aria-label="关闭快捷键选择">×</button></div><p class="setting-help">点击按键组合，再点一次取消选中。最多四键，其中最多一个普通键。</p><div class="keyboard-scroll" tabindex="0" aria-label="完整键盘，可横向滚动"><div class="keyboard-layout"></div></div><div class="shortcut-result"><span>当前快捷键</span><output aria-live="polite"></output></div><p class="shortcut-error" role="status"></p><div class="shortcut-actions"><button type="button" class="secondary" data-clear>清空</button><span></span><button type="button" class="secondary" data-cancel>取消</button><button type="button" data-confirm>确认</button></div>`;
 document.body.append(dialog);let value='',origin=null;const output=dialog.querySelector('output'),error=dialog.querySelector('.shortcut-error');
 const refresh=()=>{output.textContent=value||'未设置';for(const b of dialog.querySelectorAll('[data-key]'))b.setAttribute('aria-pressed',String(value.split('+').includes(b.dataset.key)));};
 for(const {group,key,label,x,y,width,height} of keys){
  const b=document.createElement('button');b.type='button';b.className='keyboard-key';b.dataset.key=key;b.textContent=label;
  Object.assign(b.style,{left:(x/1509*100)+'%',top:(y/364*100)+'%',width:(width/1509*100)+'%',height:(height/364*100)+'%'});
  b.setAttribute('aria-label',group+' '+key);
  b.onclick=()=>{const next=selectKey(value,key);if(next===null){error.textContent='最多选择四个按键。';return;}value=next;error.textContent='';refresh();};
  dialog.querySelector('.keyboard-layout').append(b);
 }
 const close=()=>{dialog.close();origin?.focus();};dialog.querySelector('[data-close]').onclick=dialog.querySelector('[data-cancel]').onclick=close;
 dialog.querySelector('[data-clear]').onclick=()=>{value='';refresh();};
 dialog.querySelector('[data-confirm]').onclick=()=>{onConfirm(value);close();};
 return {open(initial,source=document.activeElement){origin=source;value=normalizeShortcut(initial);error.textContent='';refresh();if(!dialog.open)dialog.showModal();}};
}
