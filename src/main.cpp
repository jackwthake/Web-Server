#include "net/serve.hpp"

#include <iostream>
#include <unistd.h>

int main(void) {
  int server_fd = server_init();
  server_listen_loop(server_fd);

  close(server_fd);

  return EXIT_SUCCESS;
}