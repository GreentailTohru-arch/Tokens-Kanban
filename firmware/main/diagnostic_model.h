#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define DEVICE_NAME_BYTES 32
#define DEVICE_BLE_NAME_BYTES 29
#define DEVICE_DEFAULT_NAME "You's AI Passport"
int diagnostic_cpu_load(uint64_t before_us,uint64_t before_idle,uint64_t now_us,uint64_t now_idle);
bool diagnostic_name_valid(const char *text);
bool diagnostic_name_decode(const char *hex,char out[DEVICE_NAME_BYTES+1]);
void diagnostic_name_encode(const char *text,char out[DEVICE_NAME_BYTES*2+1]);
/* Legacy scan response has 31 bytes, including the name AD length/type bytes. */
size_t diagnostic_name_advertised_length(const char *text);
