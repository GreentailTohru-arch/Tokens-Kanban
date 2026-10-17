#pragma once
#include "esp_err.h"
#include <stddef.h>
/* App lifetime service; callback copies a completed ASCII line into the model queue. */
typedef size_t (*token_ble_receive_t)(const char *line, char *reply, size_t capacity);
esp_err_t token_ble_start(token_ble_receive_t receive);
/* Queue a refresh from committed device settings; does not drop a connection. */
void token_ble_name_changed(void);
