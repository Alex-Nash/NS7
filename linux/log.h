#ifndef LOG_H
#define LOG_H

char *log_filename;
int daemonized;

int open_log();
void close_log();

void log(const char *format, ...);

#endif
