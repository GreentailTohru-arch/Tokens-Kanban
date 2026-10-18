#pragma once
#include <stdint.h>
typedef struct { uint32_t row; uint16_t duration; } pet_frame_t;
typedef struct { uint16_t first,count; uint32_t duration; } pet_animation_t;
extern const uint8_t pet_bytes[];
extern const uint32_t pet_rows[];
extern const pet_frame_t pet_frames[];
extern const pet_animation_t pet_animations[6];
