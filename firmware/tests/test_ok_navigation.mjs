import assert from 'node:assert/strict';
import {ExperienceModel} from '../companion/postcard/simulator.js';
const config={mask:63,order:[0,1,4,2,3,5],volume:30,idle:0,petBindings:[1,2,3,0,4,0,0,23,5]};
function taps(m,n,t=100){for(let i=0;i<n;i++){m.press('ok',t+i*120);m.release('ok',t+i*120+40);}m.tick(t+(n-1)*120+400);}
for(let mask=1;mask<64;mask++){
 const m=new ExperienceModel({...config,mask});const pages=m.enabled();
 for(let i=0;i<pages.length;i++){
  m.page=pages[i];taps(m,3);assert.equal(m.page,pages[(i+pages.length-1)%pages.length]);
  taps(m,2,1000);assert.equal(m.page,pages[i]);
 }
}
const m=new ExperienceModel(config);m.press('ok',100);m.release('ok',140);m.press('ok',220);m.release('ok',260);assert.equal(m.page,0);m.tick(400);assert.equal(m.page,0);m.press('ok',340);m.release('ok',380);assert.equal(m.page,5);assert.equal(m.details,false);
m.page=0;taps(m,1,2000);assert.equal(m.details,true);
m.page=3;taps(m,1,3000);assert.equal(m.contact,true);
m.page=4;m.press('ok',4000);m.tick(4501);assert.equal(m.petVoice,true);m.release('ok',4600);m.tick(5000);assert.equal(m.petVoice,false);assert.equal(m.page,4);
m.screen(6000);taps(m,3,6100);assert.equal(m.page,4);assert.equal(m.awake,true);
console.log('Global OK navigation, disabled pages, triple suppression, single, hold and wake: PASS');
