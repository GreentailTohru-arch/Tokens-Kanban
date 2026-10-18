#include "pet_model.h"
#include "pet_assets.h"
bool pet_parse(const char *text,unsigned *sequence,unsigned *state){
    unsigned values[2];
    for(unsigned i=0;i<2;++i){
        uint64_t value=0;if(*text<'0'||*text>'9')return false;
        do{value=value*10+(unsigned)(*text++-'0');if(value>UINT32_MAX)return false;}while(*text>='0'&&*text<='9');
        values[i]=(unsigned)value;
        if(i==0 && *text++!=' ')return false;
    }
    if(*text || !values[0] || values[1]>=PET_COUNT)return false;
    *sequence=values[0];*state=values[1];return true;
}
unsigned pet_resolve(bool live,int signal,unsigned state,uint64_t age){
    if(!live)return PET_OFFLINE;
    if(signal==TOKEN_LIMITED)return PET_LIMITED;
    if(state<PET_COUNT && age<15000)return state;
    return signal==TOKEN_BUSY?PET_THINKING:PET_ONLINE;
}
unsigned pet_frame_at(unsigned state,uint64_t elapsed){
    if(state>=PET_COUNT)state=PET_OFFLINE;
    const pet_animation_t *a=&pet_animations[state];
    unsigned time=(unsigned)(elapsed%a->duration);
    for(unsigned i=0;i<a->count;++i){unsigned f=a->first+i;if(time<pet_frames[f].duration)return f;time-=pet_frames[f].duration;}
    return a->first;
}
void pet_decode_row(unsigned frame,unsigned y,unsigned x,unsigned width,uint16_t bg,uint16_t *out){
    if(y>=208 || x+width>192)return;
    const uint8_t *p=pet_bytes+pet_rows[pet_frames[frame].row+y];unsigned pos=0,end=x+width;
    while(pos<end){unsigned run=*p++;uint16_t color=bg;unsigned count=run&127;
        if(!(run&128)){color=p[0]|((uint16_t)p[1]<<8);p+=2;}
        for(unsigned i=0;i<count && pos<end;++i,++pos)if(pos>=x)out[pos-x]=color;
    }
}
