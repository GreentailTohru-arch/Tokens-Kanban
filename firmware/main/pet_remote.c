#include "pet_remote.h"
#include "pet_controls.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
static bool pair(const char *p,unsigned *a,unsigned *b){
    unsigned *dst[]={a,b};
    for(unsigned i=0;i<2;++i){if(*p<'0'||*p>'9')return false;char *end;errno=0;unsigned long n=strtoul(p,&end,10);if(errno||n>UINT32_MAX)return false;*dst[i]=(unsigned)n;p=end;if(i==0){if(*p++!=' ')return false;}}
    return !*p && *a;
}
void pet_remote_update(pet_remote_t *r,unsigned action,bool voice,uint64_t now){
    if(!r->session || now-r->polled>1500){r->action=0;r->voice=false;return;}
    r->voice=voice;
    if(action && (action<PET_ACTION_VOICE || (action>=PET_ACTION_CUSTOM_FIRST && action<=PET_ACTION_CUSTOM_LAST)) && !r->action){r->seq=r->seq==UINT32_MAX?1:r->seq+1;r->action=action;r->created=now;}
}
size_t pet_remote_line(pet_remote_t *r,const char *line,char *out,size_t cap,unsigned transport,uint64_t now){
    if(!strcmp(line,"PC_CTRL_HELLO"))return snprintf(out,cap,"PC_CTRL_READY 1\n");
    if(strncmp(line,"PC_CTRL_POLL ",13))return 0;
    unsigned session,ack;if(!pair(line+13,&session,&ack))return snprintf(out,cap,"PC_CTRL_INVALID\n");
    if(r->session!=session || r->transport!=transport || now-r->polled>1500){*r=(pet_remote_t){.session=session,.transport=transport};}
    if(ack==r->seq || (r->action && now-r->created>1500))r->action=0;
    r->polled=now;
    return snprintf(out,cap,"PC_CTRL 1 %u %u %u\n",r->seq,r->action,(unsigned)r->voice);
}
