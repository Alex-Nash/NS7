#ifndef LOG_H
#define LOG_H

int open_log();
void close_log();

void log(const char *format, ...);

#endif
