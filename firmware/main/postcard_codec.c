#include "postcard_codec.h"
#include <string.h>
static int digit(char c) {
    if(c>='0'&&c<='9') return c-'0';
    if(c>='a'&&c<='f') return c-'a'+10;
    return -1;
}
static bool byte(const char **p,uint8_t *b) {
    if(!(*p)[0]||!(*p)[1]) return false;
    int a=digit((*p)[0]),d=digit((*p)[1]);
    if(a<0||d<0) return false;
    *b=(uint8_t)(a*16+d);*p+=2;return true;
}
bool postcard_decode(const char *hex,uint8_t *out,size_t capacity,size_t *written) {
    size_t n=0;
    while(*hex) {
        uint8_t tag,a,b;
        if(!byte(&hex,&tag)) return false;
        size_t count=(tag&127)+1u;
        if(count>(capacity-n)/2) return false;
        if(tag&128) {
            if(!byte(&hex,&a)||!byte(&hex,&b)) return false;
            for(size_t i=0;i<count;++i) { out[n++]=a;out[n++]=b; }
        } else for(size_t i=0;i<count;++i) {
            if(!byte(&hex,&a)||!byte(&hex,&b)) return false;
            out[n++]=a;out[n++]=b;
        }
    }
    *written=n;return n>0;
}
uint32_t postcard_hash(uint32_t hash,const uint8_t *bytes,size_t count) {
    for(size_t i=0;i<count;++i) { hash^=bytes[i];hash*=16777619u; }
    return hash;
}
bool postcard_calendar(uint64_t seconds,unsigned *month,unsigned *day,unsigned *weekday) {
    /* Bounded 1970..2199 conversion, including the non-leap century 2100. */
    if(seconds>=7258118400ULL) return false;
    unsigned days=(unsigned)(seconds/86400),year=1970;
    *weekday=(days+4)%7;
    for(;;++year) {
        unsigned length=365+(year%4==0 && (year%100!=0 || year%400==0));
        if(days<length) break;
        days-=length;
    }
    static const unsigned lengths[]={31,28,31,30,31,30,31,31,30,31,30,31};
    unsigned m=0;
    for(;m<11;++m) {
        unsigned length=lengths[m]+(m==1 && year%4==0 && (year%100!=0 || year%400==0));
        if(days<length) break;
        days-=length;
    }
    *month=m+1;*day=days+1;return true;
}
