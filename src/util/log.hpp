#ifndef __LOG_HPP__
#define __LOG_HPP__

#define COLOR_Black "\u001b[30m"
#define COLOR_Red "\u001b[31m"
#define COLOR_Green "\u001b[32m"
#define COLOR_Yellow "\u001b[33m"
#define COLOR_Blue "\u001b[34m"
#define COLOR_Magenta "\u001b[35m"
#define COLOR_Cyan "\u001b[36m"
#define COLOR_White "\u001b[37m"
#define COLOR_Reset "\u001b[0m"

void init_log_file(void);
void log_info(const char *fmt, ...);
void log_warning(const char *fmt, ...);
void log_fatal_and_exit(const char *fmt, ...);

#endif