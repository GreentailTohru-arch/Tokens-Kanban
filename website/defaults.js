export const DEFAULT_URL="https://greentailtohru-arch.github.io/Tokens-Kanban/";
export async function loadDefaults(){
  const names=['default-avatar.png','default-qr.png'];
  const [photo,qr]=await Promise.all(names.map(async name=>{
    const response=await fetch(new URL(name,import.meta.url));
    if(!response.ok)throw new Error('默认图片加载失败');
    return createImageBitmap(await response.blob());
  }));
  return {photo,qr,title:'标题',subtitle:'副标题',name:'扫码QR Code',crop:{x:.5,y:.5,zoom:1},qrCrop:{x:.5,y:.5,zoom:1}};
}
