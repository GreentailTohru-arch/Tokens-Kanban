#include "diagnostic_model.h"
#include <string.h>
int diagnostic_cpu_load(uint64_t before,uint64_t idle,uint64_t now,uint64_t next_idle) {
    if(!before || now<=before || next_idle<idle || next_idle-idle>now-before)return -1;
    return 1000-(int)((next_idle-idle)*1000/(now-before));
}
bool diagnostic_name_valid(const char *text) {
    size_t n=strlen(text);if(!n||n>DEVICE_NAME_BYTES||text[0]==' '||text[n-1]==' ')return false;
    const unsigned char *p=(const unsigned char *)text;
    while(*p) {
        uint32_t cp=*p++;unsigned continuation=0;
        if(cp<128){if(cp<32||cp==127)return false;continue;}
        if(cp>=0xC2&&cp<=0xDF){continuation=1;cp&=31;}
        else if(cp>=0xE0&&cp<=0xEF){continuation=2;cp&=15;}
        else if(cp>=0xF0&&cp<=0xF4){continuation=3;cp&=7;}else return false;
        unsigned count=continuation;
        while(continuation--){if((*p&0xC0)!=0x80)return false;cp=(cp<<6)|(*p++&63);}
        if((count==2&&cp<0x800)||(count==3&&cp<0x10000)||cp>0x10FFFF||(cp>=0xD800&&cp<=0xDFFF)||(cp>=0x80&&cp<=0x9F)||cp==0x2028||cp==0x2029)return false;
    }
    return true;
}
static int digit(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
bool diagnostic_name_decode(const char *hex,char out[DEVICE_NAME_BYTES+1]) {
    size_t n=strlen(hex);if(!n||n%2||n>DEVICE_NAME_BYTES*2)return false;
    char next[DEVICE_NAME_BYTES+1];
    for(size_t i=0;i<n/2;i++){int a=digit(hex[i*2]),b=digit(hex[i*2+1]);if(a<0||b<0||!(a||b))return false;next[i]=(char)(a*16+b);}
    next[n/2]=0;if(!diagnostic_name_valid(next))return false;memcpy(out,next,n/2+1);return true;
}
void diagnostic_name_encode(const char *text,char out[DEVICE_NAME_BYTES*2+1]) {
    static const char hex[]="0123456789abcdef";size_t i;
    for(i=0;text[i];++i){unsigned c=(unsigned char)text[i];out[i*2]=hex[c>>4];out[i*2+1]=hex[c&15];}out[i*2]=0;
}
