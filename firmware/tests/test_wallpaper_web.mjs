import assert from 'node:assert/strict';
import {WallpaperEditor,wallpaperCrop,normalizeWallFlags} from '../companion/postcard/wallpaper.js';
import {hash} from '../companion/postcard/codec.js';
assert.deepEqual(Array.from({length:8},(_,n)=>normalizeWallFlags(n)),[0,3,3,3,4,7,7,7]);
let cases=0;
for(const [w,h] of [[240,320],[4000,1000],[1000,4000],[1,1]])for(const zoom of [1,2,8])for(const x of [-10,.5,10])for(const y of [-10,.5,10]){
 const r=wallpaperCrop(w,h,{x,y,zoom});assert(r.sx>=-.0001&&r.sy>=-.0001&&r.sx+r.w<=w+.0001&&r.sy+r.h<=h+.0001);assert(Math.abs(r.w/r.h-.75)<.0001);cases++;
}
globalThis.document={getElementById:()=>({textContent:''})};
const e=Object.create(WallpaperEditor.prototype);let stored=new Uint8Array(),flags=4,expected=0,declared=0,saves=0,uploads=0,error='',failReadback=false;
const link={async rpc(cmd){const p=cmd.split(' ');switch(p[0]){
 case 'PC_WALL_BEGIN':uploads++;declared=+p[2];expected=+p[3];flags=+p[4];stored=new Uint8Array();return `PC_WALL_STATE ${p[1]} 2 0`;
 case 'PC_WALL_DATA':{assert.equal(+p[2],stored.length);const a=p[3].match(/../g).map(n=>parseInt(n,16)),out=[];for(let i=0;i<a.length;){let t=a[i++],n=(t&127)+1;if(t&128){const x=a[i++],y=a[i++];for(let j=0;j<n;j++)out.push(x,y);}else{out.push(...a.slice(i,i+n*2));i+=n*2;}}const next=new Uint8Array(stored.length+out.length);next.set(stored);next.set(out,stored.length);stored=next;return `PC_WALL_STATE ${p[1]} 2 ${stored.length}`;}
 case 'PC_WALL_DONE':assert.equal(stored.length,declared);assert.equal(hash(stored),expected);return `PC_WALL_STATE ${p[1]} 3 ${stored.length}`;
 case 'PC_WALL_OPTIONS':flags=+p[2];return `PC_WALL_STATE ${p[1]} 3 ${stored.length}`;
 case 'PC_WALL_GET':return `PC_WALL 1 ${stored.length} ${failReadback?0:hash(stored)} ${flags}`;
 default:throw Error(cmd);
}}};
e.api={link:()=>link,locked:()=>false,busy(){},saved(){saves++;},status(message,kind){if(kind==='error')error=message;}};
Object.assign(e,{supported:true,pending:null,flags:7,bytes:Uint8Array.from({length:153600},(_,i)=>i%255),imageDirty:true});
await e.save();assert.equal(saves,1);assert.equal(error,'');assert.deepEqual(stored,e.bytes);assert.equal(e.imageDirty,false);
const prior=stored;e.flags=0;await e.save();assert.equal(saves,2);assert.equal(uploads,1);assert.equal(stored,prior);assert.equal(flags,0);
e.imageDirty=true;failReadback=true;await e.save();assert.equal(saves,2);assert(e.imageDirty&&error.includes('不一致'));
failReadback=false;e.bytes=null;e.flags=4;await e.save();assert.equal(stored.length,0);assert.equal(saves,3);
console.log(`Wallpaper web: ${cases} crop bounds, full compressed upload/hash, flags-only preservation, failed readback, clear PASS`);
