#include <iostream>
#include <fstream>
#include <iomanip>
#include <mutex>

#include <cstdarg>

#define LINE_BUF_SIZE 256

static std::fstream file("server.log", std::fstream::app);
static std::mutex log_mutex;


// Close the open file descriptor
void close_log_file(void) {
  file.close();
}


// Log information to both the console and a log file
void log_info(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  const size_t length = std::size("[mm/dd/yy hh:mm:ss]: "); // get length of time string
  char line_buf[LINE_BUF_SIZE];
  auto t = std::time(nullptr);

  std::strftime(line_buf, length, "[%D %T]: ", std::localtime(&t)); // put date and time into string
  vsnprintf(line_buf + (length - 1), 256 - (length - 1), fmt, args); // add the message

  {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << line_buf << std::endl; // print message
    file << line_buf << std::endl; // add message to log file
    file.flush(); // ensure immediate write
  }

  va_end(args);
}
