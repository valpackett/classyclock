#pragma once

#include <pebble.h>

void classy_strlcpy(char *dst, const char *src, size_t len);

size_t classy_utflen(const char *s);

size_t classy_utfcpy(char *dst, char *s, size_t utf_len, size_t len);

struct tm* current_time();
