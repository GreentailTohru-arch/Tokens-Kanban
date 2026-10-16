#include <assert.h>
#include <stdio.h>
#include "device_model.h"
int main(void){
    device_config_t c=device_config_default();assert(device_config_valid(&c));device_behavior_t s={0};
    assert(!device_screen_off(&s,&c,9999999));c.idle_seconds=14;assert(!device_config_valid(&c));c.idle_seconds=15;assert(device_config_valid(&c));
    assert(!device_screen_off(&s,&c,14999));assert(device_screen_off(&s,&c,15000));assert(device_key(&s,16000));assert(!device_key(&s,16001));
    assert(!device_screen_off(&s,&c,30000));assert(device_screen_off(&s,&c,31001));
    c.idle_seconds=3601;assert(!device_config_valid(&c));
    token_snapshot_t p={0};p.v[T_QUOTA_AGE]=0;p.v[T_USED1]=100;p.v[T_WINDOW1]=300;p.v[T_RESET1]=500;
    assert(device_quota_alert(&s,&p));assert(!device_quota_alert(&s,&p));
    p.v[T_USED1]=-1;assert(!device_quota_alert(&s,&p)&&s.exhausted==1);
    p.v[T_USED1]=20;p.v[T_QUOTA_AGE]=180;assert(!device_quota_alert(&s,&p)&&s.exhausted==1);
    p.v[T_QUOTA_AGE]=0;assert(!device_quota_alert(&s,&p)&&s.exhausted==0);
    p.v[T_USED1]=100;assert(device_quota_alert(&s,&p));p.v[T_RESET1]=0;assert(!device_quota_alert(&s,&p)&&s.exhausted==0);
    puts("Idle boundaries, wake key and fresh quota edge detection: PASS");return 0;
}
