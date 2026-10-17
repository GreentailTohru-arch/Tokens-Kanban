/* Device palette and management chrome have separate state and storage. */
(()=>{
 'use strict';
 const root=document.documentElement,modeKey='tokens-kanban.manager-mode.v1';
 const palettes={
  default:{name:'默认',note:'原始 · 青绿',accent:'#68C4CB',chart:'#70BFFF',background:'#2F3A49',text:'#F1F7F4',muted:'#ADB8C3'},
  obsidian:{name:'黑曜',note:'墨黑 · 冷蓝',accent:'#D6D9DE',chart:'#6AB9F0',background:'#0D0F11',text:'#ECEEF1',muted:'#9CA3AD'},
  porcelain:{name:'瓷白',note:'柔白 · 雾蓝',accent:'#2761A8',chart:'#2F8EC8',background:'#E9EDF1',text:'#242A32',muted:'#586777'},
  ascii:{name:'ASCII 极客',note:'黑底 · 绿色',accent:'#45FF7B',chart:'#6EFF92',background:'#030703',text:'#A3FFB7',muted:'#63BF7D'}
 };
 const fields={accent:'强调色',chart:'图表色',background:'背景',text:'文字',muted:'次要文字'};
 const valid=v=>typeof v==='string'&&/^#[0-9a-f]{6}$/i.test(v);
 const names=Object.keys(palettes),rgb=c=>[1,3,5].map(i=>parseInt(c.slice(i,i+2),16));
 const mix=(a,b,t)=>'#'+rgb(a).map((v,i)=>Math.round(v*(1-t)+rgb(b)[i]*t).toString(16).padStart(2,'0')).join('');
 const luminance=c=>rgb(c).map(v=>{v/=255;return v<=.04045?v/12.92:((v+.055)/1.055)**2.4;}).reduce((s,v,i)=>s+v*[.2126,.7152,.0722][i],0);
 let state={theme:'default',overrides:Object.fromEntries(names.map(n=>[n,{}]))};
 const colors=()=>({...palettes[state.theme],...state.overrides[state.theme]});
 function changed(){window.dispatchEvent(new Event('device-theme-change'));}
 function apply(){if(document.getElementById('themeOptions'))render();}
 function render(){const c=colors();document.querySelectorAll('[name=deviceTheme]').forEach(e=>{e.checked=e.value===state.theme;});for(const f of Object.keys(fields)){document.getElementById('theme-'+f).value=c[f];const hex=document.getElementById('theme-hex-'+f);if(document.activeElement!==hex){hex.value=c[f];hex.setCustomValidity('');hex.removeAttribute('aria-invalid');}document.getElementById('theme-reset-'+f).disabled=!Object.hasOwn(state.overrides[state.theme],f);}
  document.getElementById('themeCurrent').textContent=c.name+(Object.keys(state.overrides[state.theme]).length?' · 自定义':'');const a=luminance(c.background),b=luminance(c.text);document.getElementById('themeContrast').hidden=(Math.max(a,b)+.05)/(Math.min(a,b)+.05)>=4.5;window.dispatchEvent(new Event('device-theme-preview-change'));
 }
 function customize(f,value){state.overrides[state.theme][f]=value.toUpperCase();apply();changed();}
 function values(){const c=colors();return [names.indexOf(state.theme),...Object.keys(fields).map(f=>parseInt(c[f].slice(1),16))];}
 function load(values){if(!Array.isArray(values)||values.length!==6||!Number.isInteger(values[0])||values[0]<0||values[0]>=names.length||values.slice(1).some(v=>!Number.isInteger(v)||v<0||v>0xffffff))throw Error('设备主题数据无效');state.theme=names[values[0]];state.overrides[state.theme]={};Object.keys(fields).forEach((f,i)=>{const hex='#'+values[i+1].toString(16).padStart(6,'0').toUpperCase();if(hex!==palettes[state.theme][f])state.overrides[state.theme][f]=hex;});apply();}
 function map(original,c=colors()){const d=palettes.default,o=original.toUpperCase();if(Object.keys(fields).every(k=>c[k]===d[k]))return original;
  const direct={'#2F3A49':c.background,'#F1F7F4':c.text,'#ADB8C3':c.muted,'#68C4CB':c.accent,'#70BFFF':c.chart};if(direct[o])return direct[o];
  const blend={'#374453':[c.text,.05],'#516471':[c.accent,.27],'#475B69':[c.text,.16],'#40555E':[c.text,.16],'#52616E':[c.text,.22],'#455361':[c.text,.22],'#344352':[c.chart,.08],'#3B5267':[c.chart,.25],'#456985':[c.chart,.45],'#528DB5':[c.chart,.70]};if(o==='#BDECD8')return mix(c.accent,c.text,.55);if(o==='#9EDBFA')return c.chart;return blend[o]?mix(c.background,...blend[o]):original;
 }
 window.DeviceTheme={values,load,colors,map,previews:()=>Object.fromEntries(names.map(id=>[id,{...palettes[id],...state.overrides[id]}]))};
 let mode='dark';try{const saved=localStorage.getItem(modeKey);mode=saved==='light'||saved==='dark'?saved:matchMedia('(prefers-color-scheme:light)').matches?'light':'dark';}catch{}
 function webMode(next){mode=next;root.dataset.webMode=mode;root.dataset.themeStyled='true';root.dataset.themeLight=String(mode==='light');root.style.colorScheme=mode;const favicon=document.getElementById('brandFavicon');if(favicon)favicon.href='brand-'+mode+'.svg';
  const light=mode==='light',c=light?{background:'#F3F3F3',surface:'#FAFAFA',raised:'#FFFFFF',text:'#1B1B1B',muted:'#616161',line:'#DCDCDC',accent:'#0067C0',chart:'#0067C0',soft:'#E4EFF9',buttonText:'#FFFFFF'}:{background:'#202020',surface:'#272727',raised:'#2D2D2D',text:'#F4F4F4',muted:'#B6B6B6',line:'#414141',accent:'#60CDFF',chart:'#60CDFF',soft:'#293D48',buttonText:'#10232D'};
  for(const [k,v]of Object.entries({...c,font:'"Segoe UI Variable","Segoe UI","Microsoft YaHei UI",sans-serif',radius:'8px'}))root.style.setProperty('--theme-'+k,v);
  const toggle=document.getElementById('webModeToggle');if(toggle){toggle.setAttribute('aria-checked',String(mode==='dark'));toggle.title=mode==='dark'?'切换为日间模式':'切换为夜间模式';document.getElementById('webModeVisual').setAttribute('value',mode);document.getElementById('webModeLabel').textContent=mode==='dark'?'夜间':'日间';}
 }
 webMode(mode);
 function mountMode(){document.getElementById('webModeToggle').onclick=()=>{webMode(mode==='dark'?'light':'dark');try{localStorage.setItem(modeKey,mode);}catch{}};webMode(mode);}
 function mount(){const host=document.getElementById('themeOptions');if(!host)return;
  for(const [id,c] of Object.entries(palettes)){const label=document.createElement('label');label.className='theme-choice';label.innerHTML=`<input type="radio" name="deviceTheme" value="${id}" aria-label="${c.name}"><span class="theme-mini"><canvas width="240" height="320" data-theme-preview="${id}" aria-label="${c.name}设备首页配色预览，示例数据"></canvas></span><span class="theme-caption"><strong>${c.name}</strong><small>${c.note}</small><span class="theme-check" aria-hidden="true">✓</span></span>`;
   for(const f of Object.keys(fields))label.style.setProperty('--sample-'+f,c[f]);label.querySelector('input').onchange=()=>{state.theme=id;apply();changed();};host.append(label);
  }
  const rows=document.getElementById('themeColors');for(const [f,name] of Object.entries(fields)){const row=document.createElement('div');row.className='theme-color-row';row.innerHTML=`<label for="theme-${f}">${name}</label><input id="theme-hex-${f}" class="theme-hex" type="text" maxlength="7" spellcheck="false" aria-label="${name}十六进制颜色" autocomplete="off"><input id="theme-${f}" type="color" aria-label="${name}"><button id="theme-reset-${f}" type="button" class="secondary theme-reset" aria-label="恢复${name}" title="恢复${name}">↺</button>`;
   const picker=row.querySelector('[type=color]'),hex=row.querySelector('[type=text]');picker.oninput=()=>customize(f,picker.value);picker.onchange=changed;hex.oninput=()=>{const ok=valid(hex.value);hex.setCustomValidity(ok?'':'请输入 #RRGGBB 格式的颜色');hex.setAttribute('aria-invalid',String(!ok));if(ok)customize(f,hex.value);};hex.onblur=()=>{render();changed();};row.querySelector('button').onclick=()=>{delete state.overrides[state.theme][f];apply();changed();};rows.append(row);
  }
  document.getElementById('themeResetAll').onclick=()=>{state.overrides[state.theme]={};apply();changed();};render();
 }
 function ready(){mount();mountMode();}
 if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',ready,{once:true});else ready();
 window.addEventListener('storage',e=>{if(e.key===modeKey&&(e.newValue==='light'||e.newValue==='dark'))webMode(e.newValue);});
})();
