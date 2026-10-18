#include "pet_model.h"
#include "pet_assets.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    unsigned seq=0,state=0;assert(pet_parse("4294967295 5",&seq,&state)&&seq==UINT32_MAX&&state==5);
    const char *bad[]={"0 1","-1 1","+1 1","4294967296 1","1 6","1 -1","1 2 ","1 2 junk","1  2","1"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i)assert(!pet_parse(bad[i],&seq,&state));
    assert(pet_resolve(false,TOKEN_BUSY,PET_COMPUTER,0)==PET_OFFLINE);
    assert(pet_resolve(true,TOKEN_LIMITED,PET_THINKING,0)==PET_LIMITED);
    for(unsigned s=0;s<PET_COUNT;++s)assert(pet_resolve(true,TOKEN_IDLE,s,0)==s);
    assert(pet_resolve(true,TOKEN_BUSY,PET_COMPUTER,15000)==PET_THINKING);
    for(unsigned s=0;s<PET_COUNT;++s){const pet_animation_t *a=&pet_animations[s];unsigned time=0;
        for(unsigned i=0;i<a->count;++i){unsigned f=a->first+i;assert(pet_frame_at(s,time)==f);time+=pet_frames[f].duration;
            for(unsigned y=0;y<208;++y){uint16_t whole[192],part[192];pet_decode_row(f,y,0,192,0x1234,whole);
                for(unsigned x=0;x<192;x+=16){pet_decode_row(f,y,x,16,0x1234,part);for(unsigned j=0;j<16;++j)assert(part[j]==whole[x+j]);}
            }
        }assert(time==a->duration);assert(pet_frame_at(s,time)==a->first);
    }
    puts("Pet priority/expiry, all 38 frame timings and clipped row decoding PASS");return 0;
}
