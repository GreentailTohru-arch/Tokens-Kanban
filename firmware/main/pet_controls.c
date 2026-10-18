#include "pet_controls.h"
#include <string.h>
/* Wait after release: double/triple presses must never first send a single. */
#define MULTI_MS 350
pet_bindings_t pet_bindings_default(void) {return (pet_bindings_t){{1,2,3,0,4,0,0,0,5}};}
bool pet_bindings_valid(const pet_bindings_t *b) {
    if(!b)return false;
    for(unsigned i=0;i<8;++i)if(b->action[i]>PET_ACTION_APPSHOT && (b->action[i]<PET_ACTION_CUSTOM_FIRST || b->action[i]>PET_ACTION_CUSTOM_LAST))return false;
    return b->action[8]==PET_ACTION_NONE || b->action[8]==PET_ACTION_VOICE;
}
void pet_controls_cancel(pet_controls_t *s) {memset(s,0,sizeof(*s));}
static unsigned finish(pet_controls_t *s,const pet_bindings_t *b) {
    unsigned n=s->clicks,key=s->key;s->clicks=0;
    if(!n)return PET_ACTION_NONE;
    if(key==2 && n==2)return PET_ACTION_NEXT;
    unsigned slot=key<2?key*3+n-1:n==1?6:7;
    return b->action[slot];
}
unsigned pet_controls_tick(pet_controls_t *s,const pet_bindings_t *b,uint64_t now) {
    if(s->down && now>=s->pressed && now-s->pressed>=20000){pet_controls_cancel(s);return 0;}
    if(!s->down && s->clicks && now>=s->released && now-s->released>=MULTI_MS)return finish(s,b);
    return 0;
}
unsigned pet_controls_event(pet_controls_t *s,const pet_bindings_t *b,unsigned key,unsigned event,uint64_t now) {
    if(key>2)return 0;
    if(event==PET_CONTROL_PRESS){
        /* Changing keys cancels ambiguous input rather than sending stale text. */
        if(s->key!=key || (s->clicks && now-s->released>=MULTI_MS))pet_controls_cancel(s);
        s->key=key;s->down=true;s->held=false;s->pressed=now;
    } else if(event==PET_CONTROL_LONG && s->down && s->key==key){
        s->clicks=0;s->held=true;s->voice=key==2 && b->action[8]==PET_ACTION_VOICE;
    } else if(event==PET_CONTROL_RELEASE && s->down && s->key==key){
        s->down=false;s->voice=false;s->released=now;
        if(!s->held && ++s->clicks==3)return finish(s,b);
    }
    return 0;
}
