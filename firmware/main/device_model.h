#pragma once
#include "token_model.h"
typedef struct { uint32_t idle_seconds,done_tone,quota_tone,volume; } device_config_t;
typedef struct { uint64_t last_touch; bool off; unsigned exhausted; } device_behavior_t;
device_config_t device_config_default(void);
bool device_config_valid(const device_config_t *config);
/* The first key wakes and is consumed. Incoming snapshots do not reset idle time. */
bool device_key(device_behavior_t *state,uint64_t now);
bool device_screen_off(device_behavior_t *state,const device_config_t *config,uint64_t now);
bool device_quota_alert(device_behavior_t *state,const token_snapshot_t *snapshot);
