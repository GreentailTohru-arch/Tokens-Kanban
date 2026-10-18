#pragma once
#include <stdbool.h>
#include <stddef.h>
bool wallpaper_start(void);
size_t wallpaper_line(const char *line,char *reply,size_t capacity);
