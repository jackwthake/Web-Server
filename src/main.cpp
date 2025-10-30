#include "server.hpp"
#include "util/log.hpp"
#include <exception>

int main(void) {
  log_info("SERVER: Starting secure-serve HTTPS server...");
  try {
    https_server server;
    log_info("SERVER: Server shutting down gracefully");
    return EXIT_SUCCESS;
  } catch (const std::exception &e) {
    log_info("FATAL ERROR: %s", e.what());
    log_info("SERVER: Server exiting due to fatal error");
    return EXIT_FAILURE;
  }
}