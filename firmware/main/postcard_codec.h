#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define POSTCARD_WIDTH 240
#define POSTCARD_HEIGHT 264
#define POSTCARD_FACE_BYTES (POSTCARD_WIDTH * POSTCARD_HEIGHT * 2u)
#define POSTCARD_BYTES (POSTCARD_FACE_BYTES * 2u)
#define POSTCARD_BLOCK_BYTES 256
/* PackBits-style RGB565: tag 0..127 = literal count+1; 128..255 = run count+1. */
bool postcard_decode(const char *hex, uint8_t *out, size_t capacity, size_t *written);
uint32_t postcard_hash(uint32_t hash, const uint8_t *bytes, size_t count);
/* Local civil time encoded as UTC seconds; no timezone conversion on the board. */
bool postcard_calendar(uint64_t seconds, unsigned *month, unsigned *day, unsigned *weekday);
