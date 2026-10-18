#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
bool boot_logo_start(void);
size_t boot_logo_line(const char *line,char *reply,size_t capacity);
/* Main task only, inside LVGL lock. Pixels survive until finish after UI deletion. */
const uint8_t *boot_logo_frame(uint64_t elapsed,unsigned *side);
void boot_logo_finish(void);

unsigned boot_logo_duration(void);
unsigned boot_logo_height(void);
