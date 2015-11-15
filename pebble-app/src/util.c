#include "util.h"

// http://blog.liw.fi/posts/strlcpy/
void classy_strlcpy(char *dst, const char *src, size_t len) {
  snprintf(dst, len, "%s", src);
  dst[len - 1] = '\0';
}

// https://stackoverflow.com/questions/5117393/utf-8-strings-length-in-linux-c
size_t classy_utflen(const char *s) {
   size_t i = 0, j = 0;
   while (s[i]) {
     if ((s[i] & 0xc0) != 0x80) j++;
     i++;
   }
   return j;
}

// The goal is to copy a number of UTF-8 characters without breaking sequences...
// Seems to work somehow ¯\_(ツ)_/¯
size_t classy_utfcpy(char *dst, char *s, size_t utf_len, size_t len) {
  size_t i = 0, j = 0, k = 0;
  while (s[i] && j < utf_len) {
    if (i < len)
      dst[i] = s[i];
    if ((s[i] & 0xc0) != 0x80) { // Not a continuation byte
      j++;
      k = 1;
    } else k++;
    i++;
  }
  i -= k;
  dst[i] = '\0';
  dst[len - 1] = '\0';
  return i;
}

struct tm* current_time() {
  time_t now = time(NULL);
  return localtime(&now);
}
