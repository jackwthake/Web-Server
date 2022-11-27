#include "log.hpp"

#include <iostream>
#include <iomanip>

#include <cstdio>
#include <cstdarg>

static int log_fd = 0;

void init_log_file(void) {
  // unimplemented
}

void log_info(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);

  std::cout << "[" << COLOR_Yellow << std::put_time(&tm, "%d/%m/%Y %H:%M:%S") << COLOR_Reset << "]: " << COLOR_Cyan;
  vprintf(fmt, args);
  std::cout << COLOR_Reset << std::endl;

  va_end(args);
}

void log_warning(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);

  std::cout << "[" << COLOR_Yellow << std::put_time(&tm, "%d/%m/%Y %H:%M:%S") << COLOR_Reset << "]: " << COLOR_Magenta;
  vprintf(fmt, args);
  std::cout << COLOR_Reset << std::endl;

  va_end(args);
}

void log_fatal_and_exit(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);

  std::cout << "[" << COLOR_Yellow << std::put_time(&tm, "%d/%m/%Y %H:%M:%S") << COLOR_Reset << "]: " << COLOR_Red;
  vprintf(fmt, args);
  std::cout << COLOR_Reset << std::endl;

  va_end(args);
  exit(EXIT_FAILURE);
}