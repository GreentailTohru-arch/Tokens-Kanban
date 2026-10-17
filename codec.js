export const WIDTH=240,HEIGHT=264,FACE_BYTES=WIDTH*HEIGHT*2;
export function rgb565(rgba){
  const out=new Uint8Array(rgba.length/2);
  for(let i=0,j=0;i<rgba.length;i+=4,j+=2){const n=((rgba[i]>>3)<<11)|((rgba[i+1]>>2)<<5)|(rgba[i+2]>>3);out[j]=n&255;out[j+1]=n>>8;}
  return out;
}
export function hash(bytes){let h=2166136261;for(const b of bytes)h=Math.imul(h^b,16777619)>>>0;return h;}
export function pack(bytes){
  if(bytes.length%2||!bytes.length||bytes.length>256)throw new Error('Invalid pixel block');
  const out=[];const same=(a,b)=>bytes[a]===bytes[b]&&bytes[a+1]===bytes[b+1];
  for(let i=0;i<bytes.length;){
    let run=1;while(run<128&&i+run*2<bytes.length&&same(i,i+run*2))run++;
    if(run>=2){out.push(128+run-1,bytes[i],bytes[i+1]);i+=run*2;}
    else{const start=i;i+=2;while(i<bytes.length&&(i-start)/2<128&&!(i+2<bytes.length&&same(i,i+2)))i+=2;
      out.push((i-start)/2-1,...bytes.slice(start,i));}
  }
  return out.map(b=>b.toString(16).padStart(2,'0')).join('');
}
export function fitText(ctx,text,size,width,min=12){
  const family='"Microsoft YaHei UI", "PingFang SC", sans-serif';
  while(size>min){ctx.font=`500 ${size}px ${family}`;if(ctx.measureText(text).width<=width)break;size--;}
  ctx.font=`500 ${size}px ${family}`;
  const chars=Array.from(text);while(chars.length&&ctx.measureText(chars.join('')).width>width)chars.pop();
  return chars.join('')===text?text:chars.slice(0,-1).join('')+'…';
}
// Normalized centre. Photos cover the square; QR starts fully contained with white padding.
export function cropRect(width,height,crop={},contain=false){
  const zoom=Math.max(1,Math.min(contain?8:4,Number(crop.zoom)||1)),size=(contain?Math.max(width,height):Math.min(width,height))/zoom;
  const cx=size>=width?width/2:Math.max(size/2,Math.min(width-size/2,(crop.x??.5)*width));
  const cy=size>=height?height/2:Math.max(size/2,Math.min(height-size/2,(crop.y??.5)*height));
  return {x:cx-size/2,y:cy-size/2,size,cx:cx/width,cy:cy/height,zoom};
}
export function renderFace(canvas,content,back=false,themed=false){
  const color=value=>themed?(window.DeviceTheme?.map(value)||value):value;
  const ctx=canvas.getContext('2d',{willReadFrequently:true});ctx.fillStyle=color('#2f3a49');ctx.fillRect(0,0,WIDTH,HEIGHT);
  ctx.textBaseline='top';ctx.textAlign='left';ctx.imageSmoothingEnabled=true;ctx.imageSmoothingQuality='high';
  if(back){
    if(content.qr){ctx.fillStyle='#fff';ctx.fillRect(26,8,188,188);const im=content.qr,r=cropRect(im.width,im.height,content.qrCrop,true);ctx.imageSmoothingEnabled=false;ctx.drawImage(im,r.x,r.y,r.size,r.size,38,20,164,164);}
    ctx.fillStyle=color('#f1f7f4');const name=fitText(ctx,content.name,22,204,14);ctx.textAlign='center';ctx.fillText(name,120,222);
  }else{
    const top=6;
    if(content.photo){const im=content.photo,r=cropRect(im.width,im.height,content.crop);ctx.save();ctx.beginPath();ctx.roundRect(33,top,174,174,16);ctx.clip();ctx.drawImage(im,r.x,r.y,r.size,r.size,33,top,174,174);ctx.restore();}
    ctx.textAlign='center';
    ctx.fillStyle=color('#f1f7f4');const title=fitText(ctx,content.title,24,212,15);ctx.fillText(title,120,190+top);
    ctx.fillStyle=color('#adb8c3');ctx.font='13px "Microsoft YaHei UI", "PingFang SC", sans-serif';
    const chars=Array.from(content.subtitle.replace(/\s+/g,' '));let line='';let row=0;
    while(chars.length&&row<2){const ch=chars.shift();if(ctx.measureText(line+ch).width>212){ctx.fillText(line,120,226+top+row*18);row++;line=ch;}else line+=ch;}
    if(row<2)ctx.fillText(line,120,226+top+row*18);
  }
  return rgb565(ctx.getImageData(0,0,WIDTH,HEIGHT).data);
}
