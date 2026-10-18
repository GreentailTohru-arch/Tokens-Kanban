"""Preserve user GIF frames/timing; emit flash row RLE and web sprite sheets."""
from pathlib import Path
import json, shutil, hashlib
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
NAMES=['未连接','在线','思考','额度不够','正在听你说话或者正在看你打字','正在使用电脑']
KEYS=['offline','online','thinking','limited','listening','computer']

def main():
    target=ROOT/'assets/images/codex-pet';target.mkdir(exist_ok=True)
    web=ROOT/'companion/postcard/pet';web.mkdir(exist_ok=True)
    source=Path.home()/'Desktop'
    blob=bytearray();rows=[];frames=[];animations=[];manifest=[]
    for name,key in zip(NAMES,KEYS):
        dest=target/(key+'.gif')
        if not dest.exists():shutil.copyfile(source/(name+'.gif'),dest)
        gif=Image.open(dest);assert gif.size==(192,208)
        sheet=Image.new('RGBA',(192*gif.n_frames,208))
        durations=[];first=len(frames)
        for i in range(gif.n_frames):
            gif.seek(i);im=gif.convert('RGBA');sheet.paste(im,(192*i,0));duration=max(20,gif.info.get('duration',100));durations.append(duration)
            start=len(rows);pixels=list(im.getdata())
            for y in range(208):
                rows.append(len(blob));line=[]
                for r,g,b,a in pixels[y*192:(y+1)*192]:line.append(None if a<128 else ((r>>3)<<11)|((g>>2)<<5)|(b>>3))
                x=0
                while x<192:
                    v=line[x];end=x+1
                    while end<192 and line[end]==v and end-x<127:end+=1
                    blob.append((end-x)|(128 if v is None else 0))
                    if v is not None:blob.extend(v.to_bytes(2,'little'))
                    x=end
            frames.append((start,duration))
        sheet.save(web/(key+'.png'))
        animations.append((first,gif.n_frames,sum(durations)))
        manifest.append(dict(key=key,source=name+'.gif',sha256=hashlib.sha256(dest.read_bytes()).hexdigest(),width=192,height=208,durations=durations))
    text='#include "pet_assets.h"\nconst uint8_t pet_bytes[]={\n'+''.join(','.join(str(x) for x in blob[i:i+32])+',\n' for i in range(0,len(blob),32))+'};\n'
    text+='const uint32_t pet_rows[]={'+','.join(map(str,rows))+'};\n'
    text+='const pet_frame_t pet_frames[]={'+','.join('{%d,%d}'%v for v in frames)+'};\n'
    text+='const pet_animation_t pet_animations[6]={'+','.join('{%d,%d,%d}'%v for v in animations)+'};\n'
    (target/'pet_assets.c').write_text(text,encoding='utf-8')
    (target/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    (web/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False),encoding='utf-8')
    print('Pet assets:',len(blob),'RLE bytes;',len(frames),'frames;',len(rows)*4,'row index bytes')
if __name__=='__main__':main()
