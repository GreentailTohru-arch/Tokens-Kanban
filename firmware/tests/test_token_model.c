#include "token_model.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void frame(char *out,const char *body) { sprintf(out,"%s*%08x",body,token_checksum(body,strlen(body))); }
int main(void) {
    char line[TOKEN_FRAME_MAX],num[48]; token_snapshot_t s;
    frame(line,"CT1 1 100 60 20 200 100 40 10000000000 1 300 13 10080 86400 -1 -1 -1 1 3 0");
    assert(token_parse(line,&s)); assert(token_total(&s,0)==120);assert(token_total(&s,2)==10000000300LL);
    assert(s.v[T_DAY_CENTS]==-1 && s.v[T_DAY_COST_STATUS]==0);
    token_snapshot_t saved=s;line[8]^=1;assert(!token_parse(line,&s));assert(!memcmp(&s,&saved,sizeof(s)));
    frame(line,"CT1 1 100 101 20 200 100 40 1 0 1 13 10080 86400 -1 -1 -1 1 3 0");assert(!token_parse(line,&s));
    frame(line,"CT1 1 100 60 20 200 100 40 1 0 1 101 10080 86400 -1 -1 -1 1 3 0");assert(!token_parse(line,&s));
    frame(line,"CT1 1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 0 -1");assert(token_parse(line,&s));assert(token_total(&s,0)==-1);
    assert(!token_parse("CT1 bad",&s));
    token_stream_t stream={0};for(int i=0;i<1100;++i) assert(!token_stream_push(&stream,'a'));
    assert(!token_stream_push(&stream,'\n'));
    for(size_t i=0;i<strlen(line);++i) assert(!token_stream_push(&stream,line[i]));
    assert(token_stream_push(&stream,'\n'));assert(token_parse(stream.data,&s));
    assert(!token_online(false,0,0));assert(token_online(true,14999,0));assert(!token_online(true,15000,0));
    assert(token_remaining(2,1000)==1);assert(token_remaining(2,9999)==0);assert(token_remaining(-1,1000)==-1);
    token_number(num,sizeof(num),18815200,false);assert(!strcmp(num,"18,815,200"));
    token_number(num,sizeof(num),1731284824,true);assert(!strcmp(num,"1.73B"));
    token_number(num,sizeof(num),-1,true);assert(!strcmp(num,"--"));
    frame(line,"CT2 1 100 60 20 200 100 40 1 0 1 99 10080 86400 -1 -1 -1 1 3 0 2");
    assert(token_parse(line,&s));assert(token_signal(&s,true,0)==TOKEN_BUSY);
    s.v[T_ACTIVITY]=TOKEN_IDLE;assert(token_signal(&s,true,0)==TOKEN_IDLE);
    s.v[T_ACTIVITY]=TOKEN_BUSY;s.v[T_USED1]=100;
    assert(token_signal(&s,true,0)==TOKEN_LIMITED); /* Red overrides running. */
    s.v[T_QUOTA_AGE]=180;assert(token_signal(&s,true,0)==TOKEN_BUSY);
    s.v[T_QUOTA_AGE]=0;s.v[T_RESET1]=0;assert(token_signal(&s,true,0)==TOKEN_BUSY);
    s.v[T_USED2]=100;s.v[T_WINDOW2]=300;s.v[T_RESET2]=60;
    assert(token_signal(&s,true,0)==TOKEN_LIMITED);
    assert(token_signal(&s,true,15000)==TOKEN_UNKNOWN);
    assert(token_signal(&s,false,0)==TOKEN_UNKNOWN);
    frame(line,"CT2 1 100 60 20 200 100 40 1 0 1 99 10080 86400 -1 -1 -1 1 3 0 3");
    assert(!token_parse(line,&s));
    frame(line,"CT3 1 100 60 20 200 100 40 1 0 1 99 10080 86400 -1 -1 -1 1 3 0 2 5736 1 9999 2 -1 0");
    assert(token_parse(line,&s));assert(s.v[T_DAY_CENTS]==5736);assert(s.v[T_WEEK_COST_STATUS]==2);
    frame(line,"CT3 1 100 60 20 200 100 40 1 0 1 99 10080 86400 -1 -1 -1 1 3 0 2 -1 1 9999 2 -1 0");
    assert(!token_parse(line,&s));
    frame(line,"CT3 1 100 60 20 200 100 40 1 0 1 99 10080 86400 -1 -1 -1 1 3 0 2 0 0 9999 2 -1 0");
    assert(!token_parse(line,&s));
    frame(line,"CT3 1 100 60 20 200 100 40 1 0 1 99 10080 86400 -1 -1 -1 1 3 0 2 0 3 9999 2 -1 0");
    assert(!token_parse(line,&s));
    token_nav_t nav={0};token_nav_key(&nav,TOKEN_KEY_OK,false);assert(nav.show_cost && nav.page==0);
    token_nav_key(&nav,TOKEN_KEY_OK,true);assert(nav.page==1 && nav.show_cost);
    token_nav_key(&nav,TOKEN_KEY_OK,false);assert(nav.page==1 && nav.show_cost);
    token_nav_key(&nav,TOKEN_KEY_UP,false);assert(nav.period==2);
    token_nav_key(&nav,TOKEN_KEY_OK,true);assert(nav.page==2 && nav.show_cost && !nav.contact);
    token_nav_key(&nav,TOKEN_KEY_OK,false);assert(nav.contact && nav.show_cost);
    token_nav_key(&nav,TOKEN_KEY_DOWN,false);assert(nav.period==2);
    token_nav_key(&nav,TOKEN_KEY_OK,true);assert(nav.page==0 && nav.period==2 && nav.show_cost);
    token_nav_key(&nav,TOKEN_KEY_OK,false);assert(!nav.show_cost);
    token_nav_key(&nav,TOKEN_KEY_OK,true);assert(nav.page==1);
    token_nav_key(&nav,TOKEN_KEY_DOWN,false);assert(nav.period==0);
    token_nav_key(&nav,TOKEN_KEY_DOWN,true);assert(nav.period==0);
    puts("Token protocol/state/navigation tests: PASS");return 0;
}
