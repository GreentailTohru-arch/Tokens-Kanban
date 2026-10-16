#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
bool postcard_start(void);
size_t postcard_line(const char *line,char *reply,size_t capacity);
/* Call only under the BSP LVGL lock; publishes committed data without copying. */
void postcard_update_ui(bool contact);
