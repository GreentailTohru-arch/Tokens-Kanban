import {FACE_BYTES,pack} from './codec.js?v=31';
export function encodeCardText(content){
 const bytes=new TextEncoder().encode(JSON.stringify([content.title,content.subtitle,content.name]));
 if(bytes.length>510)throw Error('明信片文字过长');
 const out=new Uint8Array(bytes.length+(bytes.length%2));out.set(bytes);return out;
}
export function decodeCardText(bytes){
 const text=new TextDecoder('utf-8',{fatal:true}).decode(bytes).replace(/\0+$/,'');
 const values=JSON.parse(text);
 if(!Array.isArray(values)||values.length!==3||values.some(v=>typeof v!=='string'))throw Error('明信片文字格式错误');
 return Object.fromEntries(['title','subtitle','name'].map((k,i)=>[k,values[i]]));
}
async function readBlock(link,command,generation,offset){
 const p=(await link.rpc(`${command} ${generation} ${offset}`)).split(' ');
 if(p[0]!=='PC_CARD_DATA'||+p[1]!==offset||!p[2]||!/^(?:[0-9a-f]{2}){1,96}$/i.test(p[2]))throw Error('读取明信片失败，请重新连接');
 return Uint8Array.from(p[2].match(/../g),v=>parseInt(v,16));
}
export async function readCardText(link){
 const p=(await link.rpc('PC_CARD_INFO')).split(' ');
 if(p[0]==='PC_INVALID')return {supported:false};
 if(p[0]!=='PC_CARD_INFO'||p.length!==6||p[1]!=='1')throw Error('无法读取明信片，请重新连接');
 const [generation,length,hash,metaLength]=p.slice(2).map(Number);
 if(![generation,length,hash,metaLength].every(Number.isInteger)||generation<0||![0,FACE_BYTES*2].includes(length)||metaLength<0||metaLength>512)throw Error('明信片信息格式错误');
 const info={supported:true,generation,length,hash,metaLength};
 if(metaLength){const bytes=new Uint8Array(metaLength);for(let off=0;off<metaLength;){const b=await readBlock(link,'PC_CARD_META',generation,off);if(off+b.length>metaLength)throw Error('明信片文字长度错误');bytes.set(b,off);off+=b.length;}info.text=decodeCardText(bytes);}
 return info;
}
export async function readCardPicture(link,info,back){
 const size=back?164:174,x=back?38:33,y=back?20:6;
 const canvas=document.createElement('canvas');canvas.width=canvas.height=size;const ctx=canvas.getContext('2d'),image=ctx.createImageData(size,size);
 for(let row=0;row<size;row++){
  const start=(back?FACE_BYTES:0)+((y+row)*240+x)*2,bytes=new Uint8Array(size*2);
  for(let at=0;at<bytes.length;){const b=await readBlock(link,'PC_CARD_READ',info.generation,start+at),n=Math.min(b.length,bytes.length-at);bytes.set(b.subarray(0,n),at);at+=n;}
  for(let col=0;col<size;col++){const n=bytes[col*2]|bytes[col*2+1]<<8,i=(row*size+col)*4;image.data.set([Math.round((n>>11)*255/31),Math.round(((n>>5)&63)*255/63),Math.round((n&31)*255/31),255],i);}
 }
 ctx.putImageData(image,0,0);return canvas;
}
export async function writeCardText(link,id,content,poll){const bytes=encodeCardText(content);for(let off=0;off<bytes.length;off+=256)await poll(id,await link.rpc(`PC_META ${id} ${off} ${pack(bytes.subarray(off,off+256))}`));}
