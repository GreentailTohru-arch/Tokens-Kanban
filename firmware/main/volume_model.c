#include "volume_model.h"
static void adjust(volume_controls_t *s,unsigned key,uint64_t now) {
    if(key==TOKEN_KEY_UP)s->value=s->value>=95?100:s->value+5;
    if(key==TOKEN_KEY_DOWN)s->value=s->value<=5?0:s->value-5;
    s->last_active=now;
}
void volume_controls_wake(volume_controls_t *s,unsigned key) {
    s->visible=false;s->down=false;s->repeat=false;s->waking=true;s->key=key;s->suppressed=7;
}
void volume_controls_cancel(volume_controls_t *s,uint64_t now) {
    s->down=s->repeat=s->waking=false;s->quiet_until=now+500;s->last_active=now;s->suppressed=7;
}
bool volume_controls_key(volume_controls_t *s,unsigned key,unsigned event,uint64_t now) {
    if(key>TOKEN_KEY_OK)return true;
    if(s->waking) {
        if(event==VOLUME_RELEASE && key==s->key){s->waking=false;s->quiet_until=now+400;}
        return true;
    }
    if(now<s->quiet_until)return true;
    if(key==TOKEN_KEY_OK && now<s->ok_until)return true;
    unsigned bit=1u<<key;
    if(event==VOLUME_PRESS) {
        s->suppressed&=~bit;s->key=key;s->down=true;s->repeat=false;s->adjusted=false;s->pressed_at=now;
        if(s->visible) {
            s->suppressed|=bit;s->last_active=now;
            if(key==TOKEN_KEY_OK){s->visible=false;s->down=false;s->ok_until=now+500;}
            else {adjust(s,key,now);s->adjusted=true;}
            return true;
        }
        return false;
    }
    if(event==VOLUME_RELEASE) {
        if(s->key==key){s->down=false;s->repeat=false;if(s->visible)s->last_active=now;}
        return true;
    }
    if(event==VOLUME_LONG && key!=TOKEN_KEY_OK && s->down && s->key==key) {
        if(!s->repeat) {
            s->visible=true;s->suppressed|=bit;
            if(!s->adjusted)adjust(s,key,now);
            s->repeat=true;s->next_repeat=now+180;
        }
        return true;
    }
    if(s->suppressed&bit)return true;
    if(s->visible && (event==VOLUME_CLICK || event==VOLUME_DOUBLE)) {
        if(key==TOKEN_KEY_OK){s->visible=false;s->ok_until=now+500;}
        else adjust(s,key,now);
        return true;
    }
    return false;
}
void volume_controls_tick(volume_controls_t *s,uint64_t now) {
    if(s->down && now-s->pressed_at>=20000){s->down=s->repeat=false;s->last_active=now;}
    if(s->repeat && s->down && now>=s->next_repeat) {adjust(s,s->key,now);s->next_repeat=now+180;}
    if(s->visible && !s->down && now>=s->last_active && now-s->last_active>=3000)s->visible=false;
}
