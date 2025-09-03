#include "files.h"
#include "win_compat.h"

#include <fcntl.h>
#ifndef _WIN32
  #include <pthread.h>
  #include <sys/mman.h>
  #include <unistd.h>
#endif

#include <ctype.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "version.h"

servefile_fn file_root;
servefile_fn file_version;
servefile_fn file_cxadc;
servefile_fn file_baseband;
servefile_fn file_start;
servefile_fn file_stop;
servefile_fn file_stats;

struct served_file SERVED_FILES[] = {
  {"/", "Content-Type: text/html; charset=utf-8\r\n", file_root},
  {"/version", "Content-Type: text/plain; charset=utf-8\r\n", file_version},
  {"/cxadc", "Content-Disposition: attachment\r\n", file_cxadc},
  {"/baseband", "Content-Disposition: attachment\r\n", file_baseband},
  {"/start", "Content-Type: text/json; charset=utf-8\r\n", file_start},
  {"/stop", "Content-Type: text/json; charset=utf-8\r\n", file_stop},
  {"/stats", "Content-Type: text/json; charset=utf-8\r\n", file_stats},
  {NULL}
};

struct atomic_ringbuffer {
  uint8_t* buf;
  size_t buf_size;
  _Atomic size_t written;
  _Atomic size_t read;
};

bool atomic_ringbuffer_init(struct atomic_ringbuffer* ctx, size_t buf_size) {
#ifdef _WIN32
  void* buf = VirtualAlloc(NULL, buf_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!buf) {
    return false;
  }
#else
  void* buf = mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (buf == MAP_FAILED) {
    return false;
  }
#endif

  ctx->buf_size = buf_size;
  ctx->read = 0;
  ctx->written = 0;
  ctx->buf = (uint8_t*)buf;
  return true;
}

void atomic_ringbuffer_free(struct atomic_ringbuffer* ctx) {
  if (ctx->buf) {
#ifdef _WIN32
    VirtualFree(ctx->buf, 0, MEM_RELEASE);
#else
    munmap(ctx->buf, ctx->buf_size);
#endif
    ctx->buf = NULL;
  }
}

enum capture_state {
  State_Idle = 0,
  State_Starting,
  State_Running,
  State_Stopping,
  State_Failed
};

const char* capture_state_to_str(enum capture_state state) {
  const char* NAMES[] = {"Idle", "Starting", "Running", "Stopping", "Failed"};
  return NAMES[(int)state];
}

struct {
  _Atomic enum capture_state cap_state;
  _Atomic size_t overflow_counter;
} g_state = {State_Idle, 0};

static void urldecode2(char* dst, const char* src) {
  char a, b;
  while (*src) {
    if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
      if (a >= 'a') a -= 'a' - 'A';
      if (a >= 'A') a -= ('A' - 10);
      else a -= '0';
      if (b >= 'a') b -= 'a' - 'A';
      if (b >= 'A') b -= ('A' - 10);
      else b -= '0';
      *dst++ = (char)(16 * a + b);
      src += 3;
    } else if (*src == '+') {
      *dst++ = ' ';
      src++;
    } else {
      *dst++ = *src++;
    }
  }
  *dst++ = '\0';
}

void file_start(int fd, int argc, char** argv) {
  (void)argc;
  (void)argv;
  
  // Windows version doesn't support audio capture (CX cards are Linux-only)
  dprintf(fd, "{\"state\": \"Failed\", \"fail_reason\": \"Audio capture not supported on Windows - CX cards require Linux\"}");
}

void file_stop(int fd, int argc, char** argv) {
  (void)argc;
  (void)argv;
  dprintf(fd, "{\"state\": \"Idle\", \"overflows\": 0}");
}

void file_root(int fd, int argc, char** argv) {
  (void)argc;
  (void)argv;
  dprintf(fd, "cxadc-capture-server Windows Edition - HTTP server running!\n");
}

void file_version(int fd, int argc, char** argv) {
  (void)argc;
  (void)argv;
  dprintf(fd, "%s\n", CXADC_CAPTURE_SERVER_VERSION);
}

void file_cxadc(int fd, int argc, char** argv) {
  (void)fd;
  (void)argc;
  (void)argv;
  // CX card streaming not supported on Windows
}

void file_baseband(int fd, int argc, char** argv) {
  (void)fd;
  (void)argc;
  (void)argv;
  // Baseband streaming not supported on Windows  
}

void file_stats(int fd, int argc, char** argv) {
  (void)argc;
  (void)argv;
  const enum capture_state state = g_state.cap_state;
  dprintf(fd, "{\"state\":\"%s\",\"note\":\"Audio capture not available on Windows\"}", capture_state_to_str(state));
}

