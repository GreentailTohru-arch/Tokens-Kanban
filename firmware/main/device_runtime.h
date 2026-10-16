#pragma once
#include "device_model.h"
bool device_runtime_start(void);
size_t device_runtime_line(const char *line,char *reply,size_t capacity);
/* Main-task only; hardware and Flash operations are dispatched to a worker. */
bool device_runtime_key(uint64_t now_ms);
void device_runtime_tick(uint64_t now_ms,const token_snapshot_t *new_snapshot);
