#pragma once
#include <stddef.h>
#include <stdint.h>
#define STARTUP_WIDTH 240
#define STARTUP_HEIGHT 320
extern const uint8_t startup_logo_rgb565[STARTUP_WIDTH*STARTUP_HEIGHT*2];
extern const int16_t startup_sound_pcm[];
extern const size_t startup_sound_samples;
