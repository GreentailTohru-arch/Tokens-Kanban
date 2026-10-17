#pragma once
#include "token_wifi.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
  const char *uri, *mime;
  const uint8_t *data;
  size_t length;
  bool compressed;
} network_asset_t;
extern const network_asset_t network_assets[];
extern const size_t network_assets_count;
bool network_http_start(token_network_receiver_t receiver);
void network_http_stop(void);
