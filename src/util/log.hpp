#ifndef __LOG_HPP__
#define __LOG_HPP__

#include <string>

void close_log_file(void);
void log_info(const char *fmt, ...);

void cull_log_file(int max_size, std::string file = std::string("../logs/server.log"));

#endif