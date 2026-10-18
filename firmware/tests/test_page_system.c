#include "page_model.h"
#include "system_model.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void wire(char *out,const char *body) {sprintf(out,"%s*%08x",body,token_checksum(body,strlen(body)));}
int main(void) {
    page_config_t c=page_config_default();assert(page_config_valid(&c)&&page_count(&c)==6);
    token_nav_t nav={0};page_nav_key(&c,&nav,TOKEN_KEY_OK,false);assert(nav.show_cost);
    page_nav_key(&c,&nav,TOKEN_KEY_OK,true);assert(nav.page==PAGE_ANALYTICS);
    page_nav_key(&c,&nav,TOKEN_KEY_UP,false);assert(nav.period==2);
    page_nav_key(&c,&nav,TOKEN_KEY_OK,true);assert(nav.page==PAGE_PET);
    page_nav_key(&c,&nav,TOKEN_KEY_OK,true);assert(nav.page==PAGE_SYSTEM);
    page_nav_key(&c,&nav,TOKEN_KEY_DOWN,false);assert(nav.period==2);
    page_nav_key(&c,&nav,TOKEN_KEY_OK,true);assert(nav.page==PAGE_POSTCARD);
    page_nav_key(&c,&nav,TOKEN_KEY_OK,false);assert(nav.contact);
    page_nav_key(&c,&nav,TOKEN_KEY_OK,true);assert(nav.page==PAGE_WALLPAPER);
    page_nav_key(&c,&nav,TOKEN_KEY_OK,true);assert(nav.page==PAGE_HOME);
    c=(page_config_t){12,{3,2,1,0,4,5}};assert(page_config_valid(&c));
    page_nav_normalize(&c,&nav);assert(nav.page==3 && page_count(&c)==2 && page_position(&c,2)==1);
    page_nav_key(&c,&nav,TOKEN_KEY_OK,true);assert(nav.page==2);
    page_nav_key(&c,&nav,TOKEN_KEY_OK,true);assert(nav.page==3);
    /* Exhaust all masks/permutations and every current page, including hidden pages. */
    for(unsigned a=0;a<6;++a)for(unsigned b=0;b<6;++b)for(unsigned d=0;d<6;++d)for(unsigned e=0;e<6;++e)
    for(unsigned f=0;f<6;++f)for(unsigned g=0;g<6;++g)for(unsigned mask=1;mask<64;++mask) {
        c=(page_config_t){mask,{a,b,d,e,f,g}};if(!page_config_valid(&c))continue;
        for(unsigned start=0;start<7;++start) {
            nav.page=start;page_nav_normalize(&c,&nav);unsigned first=nav.page,seen=0;
            for(unsigned i=0;i<page_count(&c);++i) {assert(!(seen&(1u<<nav.page)));seen|=1u<<nav.page;page_nav_key(&c,&nav,TOKEN_KEY_OK,true);}
            assert(nav.page==first && seen==mask);
        }
    }
    c=page_config_default();c.enabled=0;assert(!page_config_valid(&c));c.enabled=64;assert(!page_config_valid(&c));
    c=page_config_default();c.order[0]=1;assert(!page_config_valid(&c));c.order[0]=6;assert(!page_config_valid(&c));
    char frame[256],text[32];system_snapshot_t s;
    wire(frame,"SM1 1 8 9 64 570 510 9910 1000");assert(system_parse(frame,&s));assert(s.v[SYS_DOWN]==9910);
    wire(frame,"SM1 4294967295 -1 100 0 -1 1500 1000000000000 0");assert(system_parse(frame,&s));
    system_snapshot_t before=s;
    const char *bad[]={"SM1 0 8 9 64 570 510 9910 1000","SM1 1 101 9 64 570 510 9910 1000","SM1 1 -2 9 64 570 510 9910 1000","SM1 1 +8 9 64 570 510 9910 1000","SM1 1 8 9 64 1501 510 9910 1000","SM1 1 8 9 64 570 510 1000000000001 1000","SM1 1 8 9 64 570 510 9910 1000 1"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i) {wire(frame,bad[i]);assert(!system_parse(frame,&s));assert(!memcmp(&s,&before,sizeof(s)));}
    wire(frame,"SM1 1 8 9 64 570 510 9910 1000");frame[8]^=1;assert(!system_parse(frame,&s));
    system_rate(text,sizeof(text),-1);assert(!strcmp(text,"--"));system_rate(text,sizeof(text),9910);assert(!strcmp(text,"9.7K/s"));
    wire(frame,"SM2 1 8 9 64 570 510 9910 1000 3380 12 1073741824 2147483648");assert(system_parse(frame,&s));assert(s.v[SYS_CPU_FREQ]==3380 && s.v[SYS_HDD]==12 && s.v[SYS_TODAY_UP]==1073741824);
    wire(frame,"SM2 1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1");assert(system_parse(frame,&s));
    const char *bad2[]={"SM2 1 8 9 64 570 510 9910 1000 100001 12 0 0","SM2 1 8 9 64 570 510 9910 1000 3380 101 0 0","SM2 1 8 9 64 570 510 9910 1000 3380 12 9007199254740992 0","SM2 1 8 9 64 570 510 9910 1000 3380 12 0"};
    for(unsigned i=0;i<sizeof(bad2)/sizeof(*bad2);++i){wire(frame,bad2[i]);assert(!system_parse(frame,&s));}
    puts("All page masks/order permutations, navigation and system telemetry bounds: PASS");
    return 0;
}
