#include "net/serve.hpp"

#include <iostream>
#include <unistd.h>

#include <openssl/ssl.h>

int main(void) {
  SSL_library_init();
  
  int server_fd = server_init();
  server_listen_loop(server_fd);

  close(server_fd);

  return EXIT_SUCCESS;
}