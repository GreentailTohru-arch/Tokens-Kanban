#pragma once
#include "token_model.h"
enum { VOLUME_PRESS,VOLUME_CLICK,VOLUME_DOUBLE,VOLUME_LONG,VOLUME_RELEASE };
typedef struct {
    unsigned value,key,suppressed;
    bool visible,down,repeat,adjusted,waking;
    uint64_t pressed_at,last_active,next_repeat,quiet_until,ok_until;
} volume_controls_t;
void volume_controls_wake(volume_controls_t *state,unsigned key);
/* True consumes the gesture; false permits the existing click/double navigation. */
bool volume_controls_key(volume_controls_t *state,unsigned key,unsigned event,uint64_t now);
void volume_controls_tick(volume_controls_t *state,uint64_t now);
void volume_controls_cancel(volume_controls_t *state,uint64_t now);
