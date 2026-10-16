#include "monitor_model.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
    monitor_config_t c=monitor_default(),copy;assert(monitor_valid(&c)&&monitor_count(&c)==6);
    char hex[MONITOR_CONFIG_BYTES*2+1];monitor_encode(&c,hex);assert(strlen(hex)==296&&monitor_decode(hex,&copy)&&!memcmp(&c,&copy,sizeof(c)));
    strcpy(c.cards[0].title,"春夏秋冬天地");assert(monitor_valid(&c));strcpy(c.cards[0].title,"龘");assert(!monitor_valid(&c));
    strcpy(c.cards[0].title,"1234567");assert(!monitor_valid(&c));strcpy(c.cards[0].title,"CPU");c.cards[0].title[5]='X';assert(!monitor_valid(&c));c=monitor_default();
    c.bits=2;assert(!monitor_valid(&c));c=monitor_default();c.unit=5;assert(!monitor_valid(&c));c=monitor_default();
    c.cards[0].metric=255;c.cards[0].enabled=1;assert(!monitor_valid(&c));c=monitor_default();
    for(unsigned i=0;i<6;++i){c.cards[i].metric=255;c.cards[i].enabled=0;}assert(monitor_valid(&c)&&monitor_count(&c)==0);
    for(unsigned n=1;n<=6;++n)for(unsigned i=0;i<n;++i){int x,y,w,h;monitor_layout(n,i,&x,&y,&w,&h);assert(x>=14&&x+w<=226&&y>=90&&y+h<=286&&h>=60);}
    c=monitor_default();system_snapshot_t s={.v={1,8,9,64,570,510,9910,1000,3380,12,1073741824,2147483648}};char first[64],second[64];
    monitor_format(&c,M_DOWN,&s,true,first,second,sizeof(first));assert(!strcmp(first,"9.7KB/s"));
    c.bits=1;monitor_format(&c,M_DOWN,&s,true,first,second,sizeof(first));assert(!strcmp(first,"77.4Kb/s"));
    c.show_unit=0;monitor_format(&c,M_DOWN,&s,true,first,second,sizeof(first));assert(!strcmp(first,"77.4"));
    c.show_percent=0;monitor_format(&c,M_RAM,&s,true,first,second,sizeof(first));assert(!strcmp(first,"64"));
    c=monitor_default();c.decimals=2;monitor_format(&c,M_CPU_FREQ,&s,true,first,second,sizeof(first));assert(!strcmp(first,"3.38GHz"));
    c.space=1;monitor_format(&c,M_CPU_TEMP,&s,true,first,second,sizeof(first));assert(!strcmp(first,"57.00 C"));
    c=monitor_default();monitor_format(&c,M_TODAY,&s,true,first,second,sizeof(first));assert(!strcmp(first,"3.0GB"));
    s.v[SYS_UP]=-1;monitor_format(&c,M_SPEED,&s,true,first,second,sizeof(first));assert(!strcmp(first,"--"));
    s.v[SYS_TODAY_UP]=-1;monitor_format(&c,M_TODAY,&s,true,first,second,sizeof(first));assert(!strcmp(first,"--"));
    for(unsigned metric=0;metric<M_METRICS;++metric){monitor_format(&c,metric,&s,false,first,second,sizeof(first));assert(strstr(first,"--"));}
    hex[0]='x';assert(!monitor_decode(hex,&copy));
    puts("Monitor card encoding, UTF-8/glyph rejection, layouts, bits/units and daily-only totals: PASS");return 0;
}
