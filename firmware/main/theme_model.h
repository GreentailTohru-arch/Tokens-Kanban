#pragma once
#include <stdbool.h>
#include <stdint.h>
/* Versioned independently from all existing settings and postcard storage. */
typedef struct { uint32_t preset,accent,chart,background,text,muted; } theme_config_t;
theme_config_t theme_default(void);
bool theme_valid(const theme_config_t *config);
uint32_t theme_color(const theme_config_t *config,uint32_t original);

uint16_t theme_postcard_pixel(const theme_config_t *config,uint16_t pixel,unsigned x,unsigned y,bool back);
