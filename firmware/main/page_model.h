#pragma once
#include "token_model.h"
enum { PAGE_HOME, PAGE_ANALYTICS, PAGE_SYSTEM, PAGE_POSTCARD, PAGE_PET, PAGE_WALLPAPER, PAGE_COUNT };
typedef struct { uint32_t enabled, order[PAGE_COUNT]; } page_config_t;
page_config_t page_config_default(void);
bool page_config_valid(const page_config_t *config);
unsigned page_count(const page_config_t *config);
unsigned page_position(const page_config_t *config,unsigned page);
void page_nav_normalize(const page_config_t *config,token_nav_t *nav);
void page_nav_step(const page_config_t *config,token_nav_t *nav,bool previous);
void page_nav_key(const page_config_t *config,token_nav_t *nav,unsigned key,bool double_click);
