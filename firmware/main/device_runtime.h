#pragma once
#include "device_model.h"
#include "page_model.h"
#include "monitor_model.h"
monitor_config_t device_runtime_monitor(void);
page_config_t device_runtime_pages(void);
unsigned device_runtime_volume(void);
unsigned device_runtime_brightness(void);
void device_runtime_volume_set(unsigned value,uint64_t now_ms);
bool device_runtime_start(void);
/* The single startup request is owned by the audio worker, never by LVGL. */
bool device_runtime_booting(void);
size_t device_runtime_line(const char *line,char *reply,size_t capacity);
/* Main-task only; hardware and Flash operations are dispatched to a worker. */
bool device_runtime_key(uint64_t now_ms);
void device_runtime_tick(uint64_t now_ms,const token_snapshot_t *new_snapshot);

/* Explicit web tests share the same serialized audio worker. kind: 2 mic, 3 speaker. */
bool device_runtime_test(unsigned id,unsigned kind);
