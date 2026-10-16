#include "postcard_codec.h"
#include <stdio.h>
int main(void){char line[600];uint8_t out[256];size_t count;
    while(fgets(line,sizeof(line),stdin)){
        for(char *p=line;*p;++p)if(*p=='\n'||*p=='\r'){*p=0;break;}
        if(!postcard_decode(line,out,sizeof(out),&count))return 1;
        for(size_t i=0;i<count;i++)printf("%02x",out[i]);
        puts("");
    }return 0;
}
