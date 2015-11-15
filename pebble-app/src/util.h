#pragma once

#include <pebble.h>

void classy_strlcpy(char *dst, const char *src, size_t len);

struct tm* current_time();
