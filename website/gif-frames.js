import {GifReader} from './vendor/omggif.js';
// Compositing fallback for the device's HTTP origin, where ImageDecoder is unavailable.
export async function* gifFrames(file){
 const reader=new GifReader(new Uint8Array(await file.arrayBuffer()));
 if(reader.numFrames()>300||reader.width*reader.height>16000000)throw Error('GIF最多300帧、1600万像素');
 const canvas=document.createElement('canvas');canvas.width=reader.width;canvas.height=reader.height;const ctx=canvas.getContext('2d');let previous=null,backup=null,elapsed=0;
 for(let i=0;i<reader.numFrames()&&elapsed<6000;i++){
  if(previous?.disposal===2)ctx.clearRect(previous.x,previous.y,previous.width,previous.height);
  if(previous?.disposal===3&&backup)ctx.putImageData(backup,0,0);
  const info=reader.frameInfo(i);backup=info.disposal===3?ctx.getImageData(0,0,canvas.width,canvas.height):null;
  const raw=new Uint8ClampedArray(canvas.width*canvas.height*4);reader.decodeAndBlitFrameRGBA(i,raw);
  const layer=document.createElement('canvas');layer.width=canvas.width;layer.height=canvas.height;layer.getContext('2d').putImageData(new ImageData(raw,canvas.width,canvas.height),0,0);ctx.drawImage(layer,0,0);
  const duration=Math.min(6000-elapsed,Math.max(20,Math.min(1000,info.delay*10||100)));elapsed+=duration;previous=info;
  yield {image:canvas,duration};await new Promise(r=>setTimeout(r,0));
 }
}
