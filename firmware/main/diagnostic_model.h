#pragma once
#include <stdbool.h>
#include <stdint.h>
#define DEVICE_NAME_BYTES 32
#define DEVICE_DEFAULT_NAME "You's AI Passport"
int diagnostic_cpu_load(uint64_t before_us,uint64_t before_idle,uint64_t now_us,uint64_t now_idle);
bool diagnostic_name_valid(const char *text);
bool diagnostic_name_decode(const char *hex,char out[DEVICE_NAME_BYTES+1]);
void diagnostic_name_encode(const char *text,char out[DEVICE_NAME_BYTES*2+1]);
