#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
bool device_diagnostics_start(bool battery_ready,bool buttons_ready);
size_t device_diagnostics_line(const char *line,char *reply,size_t capacity);
/* Bounded callback bookkeeping only; no I2C, audio, NVS or LVGL access. */
void device_diagnostics_key(unsigned key,unsigned event);
void device_diagnostics_test_complete(unsigned id,bool ok,int peak,int average,unsigned samples);
int device_diagnostics_screen(uint64_t now_ms);
bool device_diagnostics_screen_stop(void);
