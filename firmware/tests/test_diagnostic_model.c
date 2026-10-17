#include "diagnostic_model.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    assert(diagnostic_cpu_load(0,0,1000,500)==-1);
    assert(diagnostic_cpu_load(1000,200,2000,1200)==0);
    assert(diagnostic_cpu_load(1000,200,2000,200)==1000);
    assert(diagnostic_cpu_load(1000,200,2000,950)==250);
    assert(diagnostic_cpu_load(1000,200,1000,300)==-1);
    assert(diagnostic_cpu_load(1000,200,2000,1201)==-1);
    assert(diagnostic_cpu_load(1000,200,2000,199)==-1);
    assert(diagnostic_cpu_load(4294967295ULL,3000,4295967295ULL,503000)==500);
    const char *valid[]={DEVICE_DEFAULT_NAME,"小张的护照","AI Passport 01","😊 的设备"};char hex[65],text[33];
    for(unsigned i=0;i<sizeof(valid)/sizeof(*valid);++i){assert(diagnostic_name_valid(valid[i]));diagnostic_name_encode(valid[i],hex);assert(diagnostic_name_decode(hex,text)&&!strcmp(text,valid[i]));}
    const char *bad[]={""," leading","trailing ","\n","a\x7F","\xC0\x80","\xED\xA0\x80","\xF4\x90\x80\x80","\xF0\x80\x80\x80","\xE2\x80\xA8","123456789012345678901234567890123"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i)assert(!diagnostic_name_valid(bad[i]));
    const char *badhex[]={"00","ff","0","zz","c280","61ff","e4b8","e4b88000"};
    for(unsigned i=0;i<sizeof(badhex)/sizeof(*badhex);++i)assert(!diagnostic_name_decode(badhex[i],text));
    assert(diagnostic_name_advertised_length(DEVICE_DEFAULT_NAME)==strlen(DEVICE_DEFAULT_NAME));
    assert(diagnostic_name_advertised_length("12345678901234567890123456789012")==29);
    assert(diagnostic_name_advertised_length("一二三四五六七八九十")==27);
    assert(diagnostic_name_advertised_length("1234567890123456789012345678😊")==28);
    assert(diagnostic_name_advertised_length("1234567890123456789012345😊")==29);
    assert(diagnostic_name_advertised_length("")==0);
    puts("Device CPU interval/unknown/64-bit counters and UTF-8 persistent name validation: PASS");return 0;
}
