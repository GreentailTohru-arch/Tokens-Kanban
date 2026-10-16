#pragma once
#include "token_model.h"
void token_ui_create(void);
void token_ui_postcard_image(const uint8_t *data);
void token_ui_postcard_clock(uint64_t seconds, bool dated, bool contact);
void token_ui_update(const token_snapshot_t *s, unsigned period, bool show_cost,
                     bool received, uint64_t elapsed_ms, int battery, unsigned page);
