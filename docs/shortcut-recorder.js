// Physical key names are shared with the companion parser, independent of IME text.
export function keyName(code){
 const names={ControlLeft:'Ctrl',ControlRight:'Ctrl',ShiftLeft:'Shift',ShiftRight:'Shift',AltLeft:'LeftAlt',AltRight:'RightAlt',MetaLeft:'Win',MetaRight:'Win',Escape:'Esc',ArrowUp:'Up',ArrowDown:'Down',ArrowLeft:'Left',ArrowRight:'Right',NumpadEnter:'Enter'};
 if(names[code])return names[code];
 if(/^Key[A-Z]$/.test(code))return code.slice(3);
 if(/^Digit[0-9]$/.test(code))return code.slice(5);
 if(/^(F([1-9]|1[0-9]|2[0-4])|Numpad([0-9]|Add|Subtract|Multiply|Divide|Decimal)|Enter|Space|Tab|Backspace|Delete|Insert|Home|End|PageUp|PageDown|CapsLock|NumLock|ScrollLock|PrintScreen|Pause|ContextMenu|Equal|Minus|Comma|Period|Slash|Semicolon|Quote|BracketLeft|BracketRight|Backslash|Backquote)$/.test(code))return code;
 return null;
}
const modifiers=['Ctrl','Shift','LeftAlt','RightAlt','Win'];
export function chordFor(codes){
 const names=[...new Set([...codes].map(keyName))];
 if(names.includes(null)||names.length>4)return null;
 const regular=names.filter(n=>!modifiers.includes(n));
 if(regular.length>1)return null;
 return [...modifiers.filter(n=>names.includes(n)),...regular].join('+');
}
export function attachRecorder(input,onChange,onError){
 const held=new Set();let peak=0;
 input.addEventListener('keydown',e=>{
  e.preventDefault();e.stopPropagation();
  if(e.repeat)return;
  if(!held.size)peak=0;
  held.add(e.code);
  // Include modifiers held before the field received focus.
  if(e.ctrlKey&&!held.has('ControlLeft')&&!held.has('ControlRight'))held.add('ControlLeft');
  if(e.shiftKey&&!held.has('ShiftLeft')&&!held.has('ShiftRight'))held.add('ShiftLeft');
  if(e.altKey&&!held.has('AltLeft')&&!held.has('AltRight'))held.add('AltLeft');
  if(e.metaKey&&!held.has('MetaLeft')&&!held.has('MetaRight'))held.add('MetaLeft');
  const value=chordFor(held);
  if(value===null){onError('不支持这组按键；请使用最多四键、最多一个非修饰键的组合。');return;}
  if(held.size>=peak){peak=held.size;input.value=value;onChange(value);if(e.code==='Tab'||e.code==='Escape')input.blur();}
 });
 input.addEventListener('keyup',e=>{e.preventDefault();e.stopPropagation();held.delete(e.code);if(!e.ctrlKey){held.delete('ControlLeft');held.delete('ControlRight');}if(!e.altKey){held.delete('AltLeft');held.delete('AltRight');}if(!e.shiftKey){held.delete('ShiftLeft');held.delete('ShiftRight');}if(!e.metaKey){held.delete('MetaLeft');held.delete('MetaRight');}});
 input.addEventListener('blur',()=>{held.clear();peak=0;});
}
