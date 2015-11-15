#include "util.h"

// http://blog.liw.fi/posts/strlcpy/
void classy_strlcpy(char *dst, const char *src, size_t len) {
  snprintf(dst, len, "%s", src);
  dst[len - 1] = '\0';
}

struct tm* current_time() {
  time_t now = time(NULL);
  return localtime(&now);
}
