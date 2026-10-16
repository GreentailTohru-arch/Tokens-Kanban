#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
enum { SYS_SEQ,SYS_CPU,SYS_GPU,SYS_RAM,SYS_CPU_TEMP,SYS_GPU_TEMP,SYS_DOWN,SYS_UP,SYS_CPU_FREQ,SYS_HDD,SYS_TODAY_UP,SYS_TODAY_DOWN,SYS_FIELDS };
/* Percent integers, temperatures in tenths Celsius, network bytes/second; -1 unknown. */
typedef struct { int64_t v[SYS_FIELDS]; } system_snapshot_t;
bool system_parse(const char *line,system_snapshot_t *out);
void system_rate(char *out,size_t size,int64_t bytes);
