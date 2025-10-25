#include "server.hpp"
#include "util/log.hpp"
#include <exception>

int main(void) {
  try {
    https_server server;
    return EXIT_SUCCESS;
  } catch (const std::exception &e) {
    log_info("FATAL ERROR: %s", e.what());
    return EXIT_FAILURE;
  }
}