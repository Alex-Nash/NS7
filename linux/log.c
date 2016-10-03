#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <sys/file.h>
#include <unistd.h>
#include <string.h>

#include "log.h"

int log_file;

// open log file
int open_log()
{
    log_file = open(log_filename, O_RDWR | O_CREAT | O_APPEND);

    if(log_file == -1) return -1;

    return 0;
}

// close log
void close_log()
{
    close(log_file);
}

// log function !!! do not use DEBUG_MSG, ERROR_MSG, USER_MSG FUNC HERE !!!
void log(const char *format, ...)
{
    char time_buf[64];
    char log_buf[256];

    time_t cur_time = time(NULL);
    struct tm *tm = localtime(&cur_time);

    strftime(time_buf, sizeof(time_buf), "%d-%m-%y %H:%M", tm);

    va_list args;
    va_start(args, format);

    // log time
    sprintf(log_buf, "(%s) ", time_buf);
    if(log_file > 0) write(log_file, log_buf, strlen(log_buf));

    // log message
    vsprintf(log_buf, format, args);
    if(log_file > 0) write(log_file, log_buf, strlen(log_buf));
    if(!daemonized) printf(log_buf);

    va_end(args);
}
