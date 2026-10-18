import assert from 'node:assert/strict';
import {defaultMonitor,values,layout,contentLayout,fitMonitorText} from '../companion/postcard/monitor.js';
const cfg=defaultMonitor();assert.equal(values(cfg,3)[0],'57.0°C');assert.equal(values(cfg,4)[0],'51.0°C');
let count=0;
for(let n=1;n<=6;n++)for(let i=0;i<n;i++)for(const pair of [false,true]){
 const p=layout(n,i),b=contentLayout(p.w,p.h,pair),line={14:18,20:22,32:35}[b.fontSize];
 assert(b.top>=b.titleY+18);assert(b.top+b.height<=p.h);assert(line*(pair?2:1)+(pair?b.gap:0)<=b.height);count++;
}
// Exercise fit fallback with an intentionally wide unit string measurement.
const ctx={font:'',letterSpacing:'0px',measureText(t){return {width:t.length*(parseFloat(this.font)*.65+parseFloat(this.letterSpacing))};}};
for(const n of [1,2,3,4,5,6])for(let i=0;i<n;i++)for(let metric=0;metric<12;metric++){
 const p=layout(n,i),f=fitMonitorText(ctx,cfg,metric,p.w,p.h);assert(f.y>=f.titleY+18);assert(f.y+f.lineHeight*(metric===11?2:1)+(metric===11?f.gap:0)<=p.h);
}
console.log(`Monitor: °C and ${count} content layouts / fitted text heights PASS`);
