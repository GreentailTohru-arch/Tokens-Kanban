#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define BOOT_LOGO_MAX_BYTES (0x20000u-0x1000u)
#define BOOT_LOGO_MAX_FRAMES 40u
typedef struct {uint32_t offset,length;uint16_t duration;} boot_frame_t;
typedef struct {unsigned side,count,duration,height;boot_frame_t frames[BOOT_LOGO_MAX_FRAMES];} boot_logo_index_t;
bool boot_logo_validate(const uint8_t *data,size_t length,boot_logo_index_t *index);
bool boot_logo_decode(const uint8_t *data,size_t length,uint8_t *pixels,size_t bytes);
