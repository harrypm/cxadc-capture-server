#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #define close closesocket
  #define SHUT_RDWR SD_BOTH
  typedef int socklen_t;
#else
  #include <arpa/inet.h>
  #include <sys/socket.h>
  #include <sys/un.h>
  #include <unistd.h>
#endif

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http.h"
#include "version.h"

static void usage(const char* name) {
  fprintf(stderr, "Usage: %s version|<port>|unix:<socket>\n", name);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }

#ifdef _WIN32
  // Initialize Winsock
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
    fprintf(stderr, "WSAStartup failed\n");
    exit(EXIT_FAILURE);
  }
#else
  signal(SIGPIPE, SIG_IGN);
#endif

  int server_fd;

  if (0 == strcmp(argv[1], "version")) {
    puts(CXADC_CAPTURE_SERVER_VERSION);
#ifdef _WIN32
    WSACleanup();
#endif
    exit(EXIT_SUCCESS);
  } else if (0 == strncmp(argv[1], "unix:", 5)) {
#ifdef _WIN32
    fprintf(stderr, "Unix sockets not supported on Windows\n");
    WSACleanup();
    exit(EXIT_FAILURE);
#else
    const char* path = argv[1] + 5;
    const int length = strlen(path);
    if (length == 0 || length >= 108) {
      errno = EINVAL;
      perror(NULL);
      exit(EXIT_FAILURE);
    }

    // create server socket
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
      perror("socket failed");
      exit(EXIT_FAILURE);
    }

    int reuseaddr = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr))) {
      perror("setsockopt failed");
      exit(EXIT_FAILURE);
    }

    // config socket
    struct sockaddr_un server_addr;
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, path);

    // bind socket to port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
      perror("bind failed");
      exit(EXIT_FAILURE);
    }
#endif
  } else {
    long lport;
    if ((lport = atol(argv[1])) <= 0 || lport > 0xffff) {
      errno = EINVAL;
      perror(NULL);
#ifdef _WIN32
      WSACleanup();
#endif
      exit(EXIT_FAILURE);
    }

    const uint16_t port = (uint16_t)lport;

    // create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
      perror("socket failed");
#ifdef _WIN32
      WSACleanup();
#endif
      exit(EXIT_FAILURE);
    }

    int reuseaddr = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseaddr, sizeof(reuseaddr))) {
      perror("setsockopt failed");
#ifdef _WIN32
      WSACleanup();
#endif
      exit(EXIT_FAILURE);
    }

    // config socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // bind socket to port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
      perror("bind failed");
#ifdef _WIN32
      WSACleanup();
#endif
      exit(EXIT_FAILURE);
    }
  }

  // listen for connections
  if (listen(server_fd, 10) < 0) {
    perror("listen failed");
#ifdef _WIN32
    WSACleanup();
#endif
    exit(EXIT_FAILURE);
  }

  printf("server listening on %s\n", argv[1]);
  while (1) {
    // client info
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // accept client connection
    const int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);

    if (client_fd < 0) {
      perror("accept failed");
      continue;
    }

    // Handle request directly in main thread for Windows simplicity
    // In a production version, you'd want proper threading here
    http_handle_request(client_fd);
    close(client_fd);
  }

  close(server_fd);
#ifdef _WIN32
  WSACleanup();
#endif
  return 0;
}
