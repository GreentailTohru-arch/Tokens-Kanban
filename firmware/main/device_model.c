#include "device_model.h"
device_config_t device_config_default(void) { return (device_config_t){0,1,3,35}; }
bool device_config_valid(const device_config_t *c) {
    return (c->idle_seconds==0 || (c->idle_seconds>=15 && c->idle_seconds<=3600)) &&
        c->done_tone<=3 && c->quota_tone<=3 && c->volume<=100;
}
bool device_key(device_behavior_t *s,uint64_t now) {
    bool consumed=s->off;s->off=false;s->last_touch=now;return consumed;
}
bool device_screen_off(device_behavior_t *s,const device_config_t *c,uint64_t now) {
    s->off=c->idle_seconds && now>=s->last_touch && now-s->last_touch>=c->idle_seconds*1000ULL;
    return s->off;
}
bool device_quota_alert(device_behavior_t *s,const token_snapshot_t *snapshot) {
    if(snapshot->v[T_QUOTA_AGE]<0 || snapshot->v[T_QUOTA_AGE]>=180) return false;
    unsigned before=s->exhausted;
    for(unsigned i=0;i<2;++i) {
        unsigned f=T_USED1+i*3,bit=1u<<i;
        if(snapshot->v[f]<0 || snapshot->v[f]>100 || snapshot->v[f+1]<=0 || snapshot->v[f+2]<-1) continue;
        if(snapshot->v[f]==100 && snapshot->v[f+2]!=0) s->exhausted|=bit;
        else s->exhausted&=~bit;
    }
    return !before && s->exhausted;
}
