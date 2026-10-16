#include "boot_logo_model.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv){
    if(argc!=2)return 2;
    FILE *file=fopen(argv[1],"rb");if(!file)return 2;
    uint8_t *data=malloc(BOOT_LOGO_MAX_BYTES+1);if(!data){fclose(file);return 2;}
    size_t n=fread(data,1,BOOT_LOGO_MAX_BYTES+1,file);fclose(file);boot_logo_index_t index;
    bool ok=boot_logo_validate(data,n,&index);free(data);
    if(!ok)return 1;
    printf("%u %u %u\n",index.side,index.count,index.duration);return 0;
}
