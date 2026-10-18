#include "monitor_model.h"
#include <stdio.h>
#include <string.h>
extern bool token_text_supported(uint32_t cp);
static const char *names[]={"CPU 占用","GPU 占用","RAM 占用","CPU 温度","GPU 温度","CPU 频率","硬盘占用","上传速率","下载速率","总网速","今日流量","NET 速率"};
monitor_config_t monitor_default(void) {
    monitor_config_t c={.version=1,.show_unit=1,.show_percent=1,.decimals=1,.up_label="↑",.down_label="↓"};
    const unsigned metrics[]={M_CPU,M_GPU,M_RAM,M_CPU_TEMP,M_GPU_TEMP,M_PAIR};
    for(unsigned i=0;i<MONITOR_SLOTS;++i){c.cards[i].metric=(uint8_t)metrics[i];c.cards[i].enabled=1;}
    return c;
}
static bool text_valid(const char *text,unsigned capacity,unsigned limit) {
    unsigned n=0,i=0;
    while(i<capacity && text[i]) {
        uint32_t cp=(uint8_t)text[i++];
        if(cp>=0xC2 && cp<=0xDF){if(i>=capacity || ((uint8_t)text[i]&0xC0)!=0x80)return false;cp=((cp&31)<<6)|((uint8_t)text[i++]&63);}
        else if(cp>=0xE0 && cp<=0xEF){
            if(i+1>=capacity || ((uint8_t)text[i]&0xC0)!=0x80 || ((uint8_t)text[i+1]&0xC0)!=0x80)return false;
            cp=((cp&15)<<12)|(((uint8_t)text[i]&63)<<6)|((uint8_t)text[i+1]&63);i+=2;
            if(cp<0x800 || (cp>=0xD800 && cp<=0xDFFF))return false;
        } else if(cp>=128)return false;
        if(cp<32 || !token_text_supported(cp) || ++n>limit)return false;
    }
    if(i>=capacity)return false;
    /* Canonical zero padding makes equality and stored hashes deterministic. */
    for(;i<capacity;++i)if(text[i])return false;
    return true;
}
bool monitor_valid(const monitor_config_t *c) {
    if(c->version!=1 || c->bits>1 || c->unit>4 || c->show_unit>1 || c->show_percent>1 || c->space>1 || c->decimals>2 || c->compact>1)return false;
    if(!text_valid(c->up_label,7,2) || !text_valid(c->down_label,7,2))return false;
    for(unsigned i=0;i<MONITOR_SLOTS;++i) {
        const monitor_card_t *card=&c->cards[i];
        if(card->enabled>1 || (card->metric>=M_METRICS && card->metric!=M_NONE) || (card->metric==M_NONE && card->enabled))return false;
        if(!text_valid(card->title,19,6))return false;
    }
    return true;
}
static int hex_digit(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
bool monitor_decode(const char *hex,monitor_config_t *out) {
    if(strlen(hex)!=MONITOR_CONFIG_BYTES*2)return false;
    monitor_config_t next;uint8_t *p=(uint8_t*)&next;
    for(unsigned i=0;i<MONITOR_CONFIG_BYTES;++i){int a=hex_digit(hex[i*2]),b=hex_digit(hex[i*2+1]);if(a<0||b<0)return false;p[i]=(uint8_t)(a*16+b);}
    if(!monitor_valid(&next))return false;
    *out=next;return true;
}
void monitor_encode(const monitor_config_t *c,char *out) {
    static const char digits[]="0123456789abcdef";const uint8_t *p=(const uint8_t*)c;
    for(unsigned i=0;i<MONITOR_CONFIG_BYTES;++i){out[i*2]=digits[p[i]>>4];out[i*2+1]=digits[p[i]&15];}out[MONITOR_CONFIG_BYTES*2]=0;
}
uint32_t monitor_hash(const monitor_config_t *c){const uint8_t *p=(const uint8_t*)c;uint32_t hash=2166136261u;for(unsigned i=0;i<sizeof(*c);++i)hash=(hash^p[i])*16777619u;return hash;}
unsigned monitor_count(const monitor_config_t *c){unsigned n=0;for(unsigned i=0;i<MONITOR_SLOTS;++i)n+=c->cards[i].enabled;return n;}
const char *monitor_title(const monitor_card_t *c){return c->title[0]?c->title:c->metric<M_METRICS?names[c->metric]:"";}
static void quantity(const monitor_config_t *c,int64_t bytes,bool rate,char *out,size_t size) {
    if(bytes<0){snprintf(out,size,"--");return;}
    double value=(double)bytes*(rate&&c->bits?8:1);unsigned unit=rate?c->unit:0;
    if(!unit){unit=1;while(value>=1024 && unit<4){value/=1024;++unit;}}
    else for(unsigned i=1;i<unit;++i)value/=1024;
    unsigned decimals=c->decimals;if(c->compact && value>=100)decimals=0;
    const char *units_b[]={"","B","KB","MB","GB"},*units_bit[]={"","b","Kb","Mb","Gb"};
    const char *suffix=rate&&c->bits?units_bit[unit]:units_b[unit];
    if(value>=100000)snprintf(out,size,"%.0e%s%s%s",value,c->space&&(!rate||c->show_unit)?" ":"",!rate||c->show_unit?suffix:"",rate&&c->show_unit?"/s":"");
    else snprintf(out,size,"%.*f%s%s%s",unit==1?0:(int)decimals,value,c->space&&(!rate||c->show_unit)?" ":"",!rate||c->show_unit?suffix:"",rate&&c->show_unit?"/s":"");
}
void monitor_format(const monitor_config_t *c,unsigned metric,const system_snapshot_t *s,bool live,char *first,char *second,size_t size) {
    second[0]=0;int64_t value=-1;unsigned field=SYS_CPU;
    switch(metric){case M_CPU:field=SYS_CPU;break;case M_GPU:field=SYS_GPU;break;case M_RAM:field=SYS_RAM;break;case M_CPU_TEMP:field=SYS_CPU_TEMP;break;case M_GPU_TEMP:field=SYS_GPU_TEMP;break;case M_CPU_FREQ:field=SYS_CPU_FREQ;break;case M_HDD:field=SYS_HDD;break;case M_UP:field=SYS_UP;break;case M_DOWN:field=SYS_DOWN;break;default:break;}
    if(live)value=s->v[field];
    if(metric==M_PAIR) {
        char rate[40];quantity(c,live?s->v[SYS_DOWN]:-1,true,rate,sizeof(rate));snprintf(first,size,"%s%s%s",c->down_label,c->down_label[0]?" ":"",rate);
        quantity(c,live?s->v[SYS_UP]:-1,true,rate,sizeof(rate));snprintf(second,size,"%s%s%s",c->up_label,c->up_label[0]?" ":"",rate);return;
    }
    if(metric==M_SPEED || metric==M_TODAY){unsigned a=metric==M_SPEED?SYS_DOWN:SYS_TODAY_DOWN,b=metric==M_SPEED?SYS_UP:SYS_TODAY_UP;value=live&&s->v[a]>=0&&s->v[b]>=0?s->v[a]+s->v[b]:-1;quantity(c,value,metric==M_SPEED,first,size);return;}
    if(metric==M_UP || metric==M_DOWN){quantity(c,value,true,first,size);return;}
    if(value<0){snprintf(first,size,"--");return;}
    if(metric==M_CPU_TEMP || metric==M_GPU_TEMP)snprintf(first,size,"%.*f%s°C",(int)c->decimals,(double)value/10,c->space?" ":"");
    else if(metric==M_CPU_FREQ)snprintf(first,size,"%.*f%sGHz",(int)c->decimals,(double)value/1000,c->space?" ":"");
    else snprintf(first,size,"%lld%s%s",(long long)value,c->space&&c->show_percent?" ":"",c->show_percent?"%":"");
}
void monitor_layout(unsigned n,unsigned i,int *x,int *y,int *w,int *h) {
    unsigned rows=n<=2?n:(n+1)/2;bool full=n<=2 || (n%2 && i==n-1);
    unsigned row=n<=2?i:i/2,col=n<=2?0:i%2;
    *x=14+(full?0:(int)col*110);*w=full?212:102;*h=rows?(196-(int)(rows-1)*8)/(int)rows:196;
    *y=90+(int)row*(*h+8);
}

monitor_content_t monitor_content_layout(int w,int h,bool pair) {
    int pad=h>=150?12:h>=90?8:2;
    monitor_content_t out={.x=w>=200?14:9,.title_y=pad,.top=pad+18+(h>=90?8:2),.gap=h>=150?8:h>=90?4:0};
    out.height=h-pad-out.top;
    out.font_size=h>=150?32:h>=90?(pair?20:32):(pair?14:20);
    return out;
}
