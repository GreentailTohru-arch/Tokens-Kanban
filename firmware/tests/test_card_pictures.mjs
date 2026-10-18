import assert from 'node:assert/strict';
import {readCardPicture} from '../companion/postcard/card-storage.js';
let output;
globalThis.document={createElement(){return {getContext(){return {createImageData(w,h){return {data:new Uint8ClampedArray(w*h*4)};},putImageData(v){output=v.data;}};}};}};
let reads=0;
const link={async rpc(cmd){const [op,gen,off]=cmd.split(' ');assert.equal(op,'PC_CARD_READ');assert.equal(gen,'3');assert(+off>=0&&+off+96<=253440);reads++;return `PC_CARD_DATA ${off} `+'00f8'.repeat(48);}};
for(const back of [false,true]){const pic=await readCardPicture(link,{generation:3},back);assert.equal(pic.width,back?164:174);for(let i=0;i<output.length;i+=4)assert.deepEqual(Array.from(output.slice(i,i+4)),[255,0,0,255]);}
assert(reads>0);console.log('Stored portrait and QR range reads / RGB565 decode PASS');
