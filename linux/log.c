#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#define LOG_FILE "/var/log/ns7_daemon.log"

FILE *log_file;

// open log file
int open_log()
{
    log_file = fopen(LOG_FILE, "a");

    if(log_file != NULL) return -1;

    return 0;
}

// close log
void close_log()
{
    fclose(log_file);
}

// log function
void log(const char *format, ...)
{
    time_t cur_time = time(NULL);
    struct tm *tm = localtime(&cur_time);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%d-%m-%y %H:%M", tm);

    va_list args;
    va_start(args, format);
    fprintf(log_file,"(%s) ", time_buf);
    vfprintf(log_file, format, args);
    va_end(args);
    fclose(log_file);
}
