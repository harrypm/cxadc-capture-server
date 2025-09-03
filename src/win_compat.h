#ifndef WIN_COMPAT_H
#define WIN_COMPAT_H

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <stdarg.h>

// Windows compatibility functions
int my_dprintf(int fd, const char *format, ...);

// Windows compatibility macros
#define dprintf my_dprintf
#define usleep(x) Sleep((x)/1000)
#define MAP_FAILED NULL
typedef HANDLE pthread_t;

#endif // _WIN32

#endif // WIN_COMPAT_H
