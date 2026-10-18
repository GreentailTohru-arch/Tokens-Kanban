#include "pet_controls.h"
#include "pet_remote.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void click(pet_controls_t *s,pet_bindings_t *b,unsigned key,unsigned t){assert(!pet_controls_event(s,b,key,PET_CONTROL_PRESS,t));assert(!pet_controls_event(s,b,key,PET_CONTROL_RELEASE,t+40));}
int main(void){
    pet_bindings_t b=pet_bindings_default();assert(pet_bindings_valid(&b));pet_controls_t s={0};
    click(&s,&b,0,100);assert(!pet_controls_tick(&s,&b,400));assert(pet_controls_tick(&s,&b,490)==PET_ACTION_SEND);
    click(&s,&b,0,1000);click(&s,&b,0,1200);assert(pet_controls_tick(&s,&b,1590)==PET_ACTION_UNDO);
    click(&s,&b,0,2000);click(&s,&b,0,2200);pet_controls_event(&s,&b,0,PET_CONTROL_PRESS,2400);assert(pet_controls_event(&s,&b,0,PET_CONTROL_RELEASE,2440)==PET_ACTION_TOGGLE);assert(!pet_controls_tick(&s,&b,3000));
    click(&s,&b,2,3100);click(&s,&b,2,3300);assert(pet_controls_tick(&s,&b,3690)==PET_ACTION_NEXT);
    pet_controls_event(&s,&b,2,PET_CONTROL_PRESS,4000);pet_controls_event(&s,&b,2,PET_CONTROL_LONG,4500);assert(s.voice);pet_controls_event(&s,&b,2,PET_CONTROL_RELEASE,4600);assert(!s.voice);assert(!pet_controls_tick(&s,&b,5100));
    pet_controls_event(&s,&b,0,PET_CONTROL_PRESS,6000);pet_controls_event(&s,&b,0,PET_CONTROL_LONG,6500);pet_controls_event(&s,&b,0,PET_CONTROL_RELEASE,6600);assert(!pet_controls_tick(&s,&b,7100));
    b.action[0]=5;assert(!pet_bindings_valid(&b));b=pet_bindings_default();b.action[8]=1;assert(!pet_bindings_valid(&b));
    pet_remote_t r={0};char out[96];
    pet_remote_update(&r,1,true,100);assert(!r.action&&!r.voice);
    pet_remote_line(&r,"PC_CTRL_POLL 123 0",out,sizeof(out),1,200);pet_remote_update(&r,2,true,300);
    pet_remote_line(&r,"PC_CTRL_POLL 123 0",out,sizeof(out),1,400);assert(!strcmp(out,"PC_CTRL 1 1 2 1\n"));
    pet_remote_line(&r,"PC_CTRL_POLL 123 0",out,sizeof(out),1,500);assert(r.action==2);
    pet_remote_line(&r,"PC_CTRL_POLL 123 1",out,sizeof(out),1,600);assert(!r.action);
    pet_remote_update(&r,1,true,700);pet_remote_line(&r,"PC_CTRL_POLL 456 0",out,sizeof(out),2,800);assert(!r.action&&!r.voice);
    pet_remote_update(&r,1,true,900);pet_remote_line(&r,"PC_CTRL_POLL 456 0",out,sizeof(out),2,3000);assert(!r.action&&!r.voice);
    pet_remote_line(&r,"PC_CTRL_POLL -1 0",out,sizeof(out),2,3001);assert(strstr(out,"INVALID"));
    b=pet_bindings_default();
    for(unsigned slot=0;slot<8;slot++){b.action[slot]=PET_ACTION_CUSTOM_FIRST+slot;assert(pet_bindings_valid(&b));}
    pet_controls_cancel(&s);click(&s,&b,0,10000);assert(pet_controls_tick(&s,&b,10400)==16);
    click(&s,&b,1,11000);click(&s,&b,1,11200);assert(pet_controls_tick(&s,&b,11600)==20);
    click(&s,&b,2,12000);click(&s,&b,2,12200);assert(pet_controls_tick(&s,&b,12600)==PET_ACTION_NEXT);
    /* A legacy custom binding cannot override global OK triple navigation. */
    click(&s,&b,2,13000);click(&s,&b,2,13200);
    assert(!pet_controls_event(&s,&b,2,PET_CONTROL_PRESS,13400));
    assert(pet_controls_event(&s,&b,2,PET_CONTROL_RELEASE,13440)==PET_ACTION_PREVIOUS);
    assert(!pet_controls_tick(&s,&b,14000));
    b.action[0]=15;assert(!pet_bindings_valid(&b));b.action[0]=24;assert(!pet_bindings_valid(&b));
    pet_remote_line(&r,"PC_CTRL_POLL 789 0",out,sizeof(out),1,20000);
    pet_remote_update(&r,20,false,20100);pet_remote_line(&r,"PC_CTRL_POLL 789 0",out,sizeof(out),1,20200);
    assert(!strcmp(out,"PC_CTRL 1 1 20 0\n"));
    pet_remote_line(&r,"PC_CTRL_POLL 789 1",out,sizeof(out),1,20300);assert(!r.action);
    puts("Pet controls: gestures, hold, mapping, stale/replayed actions PASS");return 0;
}
