#include <iostream>
#include <fstream>
#include <iomanip>
#include <mutex>

#include <cstdarg>

#define LINE_BUF_SIZE 256

static std::fstream file("../logs/server.log", std::fstream::app);
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

void cull_log_file(int max_size_bytes, std::string file_path) {
  std::lock_guard<std::mutex> lock(log_mutex);

  std::fstream file(file_path, std::fstream::in | std::fstream::out | std::fstream::app);
  file.seekg(0, std::ios::end);
  std::streampos file_size = file.tellg();

  if (file_size <= max_size_bytes) {
    return; // No need to cull
  }

  // Read the entire log file into memory
  file.seekg(0, std::ios::beg);
  std::string log_contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  // Find the position to start retaining logs
  size_t start_pos = log_contents.size() - max_size_bytes;
  size_t new_line_pos = log_contents.find('\n', start_pos);

  if (new_line_pos != std::string::npos) {
    // Retain logs from the next line
    log_contents = log_contents.substr(new_line_pos + 1);
  } else {
    // If no newline found, retain the last max_size_bytes
    log_contents = log_contents.substr(start_pos);
  }

  // Rewrite the log file with the retained logs
  file.close();
  file.open("../logs/server.log", std::fstream::out | std::fstream::trunc);
  file << log_contents;
  file.flush();
}