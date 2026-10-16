#pragma once
#include "token_model.h"
#include "page_model.h"
#include "system_model.h"
#include "monitor_model.h"
void token_ui_pages(const page_config_t *config,unsigned current);
void token_ui_system(const system_snapshot_t *snapshot,bool live,const monitor_config_t *config);
void token_ui_create(void);
/* Called with the same LVGL lock as every other UI operation. */
void token_ui_startup(bool show);
void token_ui_volume(bool show,unsigned percent);
void token_ui_postcard_image(const uint8_t *data);
void token_ui_postcard_clock(uint64_t seconds, bool dated, bool contact);
void token_ui_update(const token_snapshot_t *s, unsigned period, bool show_cost,
                     bool received, uint64_t elapsed_ms, int battery, unsigned page);
