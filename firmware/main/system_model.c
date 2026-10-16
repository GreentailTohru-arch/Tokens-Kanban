#include "system_model.h"
#include "token_model.h"
#include <stdio.h>
#include <string.h>
bool system_parse(const char *line,system_snapshot_t *out) {
    bool extended=!strncmp(line,"SM2 ",4);
    if(!extended && strncmp(line,"SM1 ",4))return false;
    const char *star=strchr(line,'*');
    if(!star || strlen(star+1)!=8)return false;
    uint32_t sum=0;
    for(unsigned i=1;i<=8;++i) {
        char c=star[i];unsigned n;
        if(c>='0' && c<='9')n=(unsigned)(c-'0');
        else if(c>='a' && c<='f')n=(unsigned)(c-'a'+10);
        else return false;
        sum=(sum<<4)|n;
    }
    if(sum!=token_checksum(line,(size_t)(star-line)))return false;
    const char *p=line+3;system_snapshot_t next;for(unsigned i=0;i<SYS_FIELDS;++i)next.v[i]=-1;
    const int64_t limits[]={UINT32_MAX,100,100,100,1500,1500,1000000000000LL,1000000000000LL,100000,100,9007199254740991LL,9007199254740991LL};
    for(unsigned i=0;i<(extended?SYS_FIELDS:8);++i) {
        if(*p++!=' ')return false;
        bool neg=*p=='-';if(neg)++p;
        if(*p<'0' || *p>'9')return false;
        int64_t n=0;
        do {n=n*10+(*p++-'0');if(n>limits[i])return false;}while(*p>='0' && *p<='9');
        if(neg && n!=1)return false;
        next.v[i]=neg?-n:n;
        if(i==SYS_SEQ && next.v[i]<1)return false;
    }
    if(p!=star)return false;
    *out=next;return true;
}
void system_rate(char *out,size_t size,int64_t bytes) {
    if(bytes<0)snprintf(out,size,"--");
    else if(bytes>=107374182400LL)snprintf(out,size,"%.0fG/s",(double)bytes/1073741824);
    else if(bytes>=1073741824)snprintf(out,size,"%.1fG/s",(double)bytes/1073741824);
    else if(bytes>=1048576)snprintf(out,size,"%.1fM/s",(double)bytes/1048576);
    else if(bytes>=1024)snprintf(out,size,"%.1fK/s",(double)bytes/1024);
    else snprintf(out,size,"%lldB/s",(long long)bytes);
}
