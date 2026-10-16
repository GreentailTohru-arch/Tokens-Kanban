#include "boot_logo_model.h"
#include <string.h>
static uint32_t u32(const uint8_t *p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned u16(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
bool boot_logo_decode(const uint8_t *data,size_t length,uint8_t *pixels,size_t bytes){
    size_t pos=0,out=0;
    while(pos<length){unsigned tag=data[pos++],n=(tag&127)+1;size_t count=n*2u;
        if(count>bytes-out)return false;
        if(tag&128){if(length-pos<2)return false;for(unsigned i=0;pixels&&i<n;++i){pixels[out+i*2]=data[pos];pixels[out+i*2+1]=data[pos+1];}pos+=2;}
        else{if(length-pos<count)return false;if(pixels)memcpy(pixels+out,data+pos,count);pos+=count;}
        out+=count;
    }return out==bytes;
}
bool boot_logo_validate(const uint8_t *data,size_t length,boot_logo_index_t *index){
    if(length<8||length>BOOT_LOGO_MAX_BYTES||memcmp(data,"TLG1",4))return false;
    boot_logo_index_t result={.side=u16(data+4),.count=u16(data+6)};
    if(result.side<16||result.side>96||result.count<1||result.count>BOOT_LOGO_MAX_FRAMES)return false;
    size_t pos=8;
    for(unsigned i=0;i<result.count;++i){if(length-pos<6)return false;unsigned duration=u16(data+pos);uint32_t n=u32(data+pos+2);pos+=6;
        if(duration<50||duration>1000||n>length-pos||!boot_logo_decode(data+pos,n,NULL,result.side*result.side*2u))return false;
        result.frames[i]=(boot_frame_t){pos,n,duration};result.duration+=duration;if(result.duration>6000)return false;pos+=n;
    }
    if(pos!=length&&!(pos+1==length&&data[pos]==0))return false;
    if(index)*index=result;
    return true;
}
