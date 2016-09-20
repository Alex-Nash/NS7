#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#define LOG_FILE "/var/log/ns7_daemon.log"

// log function
void write_log(const char *format, ...)
{
    FILE *file;

    file = fopen(LOG_FILE, "a");

    if (file != NULL)
    {
        time_t cur_time = time(NULL);
        struct tm *tm = localtime(&cur_time);
        char time_buf[64];
        strftime(time_buf, sizeof(time_buf), "%d-%m-%y %H:%M", tm);

        va_list args;
        va_start(args, format );
        fprintf(file,"(%s) ", time_buf);
        vfprintf(file, format, args );
        va_end( args );
        fclose(file);
    }
}
