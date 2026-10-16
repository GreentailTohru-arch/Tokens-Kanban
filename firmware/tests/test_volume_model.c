#include "volume_model.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    volume_controls_t v={.value=35};
    assert(!volume_controls_key(&v,TOKEN_KEY_UP,VOLUME_PRESS,0));
    assert(volume_controls_key(&v,TOKEN_KEY_UP,VOLUME_RELEASE,100));
    assert(!volume_controls_key(&v,TOKEN_KEY_UP,VOLUME_CLICK,280));assert(v.value==35&&!v.visible);
    volume_controls_key(&v,TOKEN_KEY_UP,VOLUME_PRESS,500);
    assert(volume_controls_key(&v,TOKEN_KEY_UP,VOLUME_LONG,1000));assert(v.value==40&&v.visible);
    volume_controls_tick(&v,1179);assert(v.value==40);volume_controls_tick(&v,1180);assert(v.value==45);
    for(unsigned i=0;i<30;++i)volume_controls_tick(&v,1360+i*180);
    assert(v.value==100);
    volume_controls_key(&v,TOKEN_KEY_UP,VOLUME_RELEASE,7000);
    assert(volume_controls_key(&v,TOKEN_KEY_UP,VOLUME_CLICK,7180));assert(v.value==100);
    volume_controls_key(&v,TOKEN_KEY_DOWN,VOLUME_PRESS,7300);assert(v.value==95);
    volume_controls_key(&v,TOKEN_KEY_DOWN,VOLUME_RELEASE,7400);
    volume_controls_key(&v,TOKEN_KEY_DOWN,VOLUME_CLICK,7580);assert(v.value==95);
    volume_controls_tick(&v,10399);assert(v.visible);volume_controls_tick(&v,10400);assert(!v.visible);
    v=(volume_controls_t){.value=3};volume_controls_key(&v,TOKEN_KEY_DOWN,VOLUME_PRESS,11000);volume_controls_key(&v,TOKEN_KEY_DOWN,VOLUME_LONG,11500);assert(v.value==0);
    volume_controls_key(&v,TOKEN_KEY_DOWN,VOLUME_RELEASE,12000);
    assert(volume_controls_key(&v,TOKEN_KEY_OK,VOLUME_PRESS,12100));assert(!v.visible);
    assert(volume_controls_key(&v,TOKEN_KEY_OK,VOLUME_RELEASE,12200));
    assert(volume_controls_key(&v,TOKEN_KEY_OK,VOLUME_DOUBLE,12300));
    assert(!volume_controls_key(&v,TOKEN_KEY_OK,VOLUME_PRESS,12700));
    assert(!volume_controls_key(&v,TOKEN_KEY_OK,VOLUME_CLICK,12900));
    /* An entire wake hold, release and delayed click are consumed. */
    volume_controls_wake(&v,TOKEN_KEY_UP);assert(volume_controls_key(&v,TOKEN_KEY_UP,VOLUME_PRESS,13000));
    assert(volume_controls_key(&v,TOKEN_KEY_UP,VOLUME_LONG,13500));volume_controls_tick(&v,13900);assert(v.value==0&&!v.visible);
    volume_controls_key(&v,TOKEN_KEY_UP,VOLUME_RELEASE,14000);assert(volume_controls_key(&v,TOKEN_KEY_UP,VOLUME_CLICK,14180));
    assert(!volume_controls_key(&v,TOKEN_KEY_UP,VOLUME_PRESS,14500));volume_controls_key(&v,TOKEN_KEY_UP,VOLUME_LONG,15000);assert(v.value==5&&v.visible);
    volume_controls_cancel(&v,15100);volume_controls_tick(&v,16000);assert(v.value==5&&!v.repeat);
    puts("Volume long/short conflict, repeat, bounds, dismissal, timeout and wake guard: PASS");return 0;
}
