#include "win_compat.h"
#include <stdio.h>

#ifdef _WIN32

int my_dprintf(int fd, const char *format, ...) {
  va_list args;
  va_start(args, format);
  char buffer[1024];
  int len = vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  send(fd, buffer, len, 0);
  return len;
}

#endif // _WIN32
