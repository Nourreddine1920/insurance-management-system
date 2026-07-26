#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static FILE* log_file = NULL;

void logger_init(const char* log_file_path) {
    if (log_file_path) {
        log_file = fopen(log_file_path, "a");
        if (!log_file) {
            fprintf(stderr, "Warning: Failed to open log file %s. Using standard console.\n",
                    log_file_path);
        }
    }
}

void logger_log(log_level_t level, const char* file, int line, const char* fmt, ...) {
    const char* level_str = "INFO";
    FILE* fallback_out = stdout;

    switch (level) {
        case LOG_LEVEL_DEBUG:
            level_str = "DEBUG";
            fallback_out = stdout;
            break;
        case LOG_LEVEL_INFO:
            level_str = "INFO";
            fallback_out = stdout;
            break;
        case LOG_LEVEL_WARN:
            level_str = "WARN";
            fallback_out = stderr;
            break;
        case LOG_LEVEL_ERROR:
            level_str = "ERROR";
            fallback_out = stderr;
            break;
    }

    FILE* target = log_file ? log_file : fallback_out;

    // Get formatted timestamp
    time_t raw_time;
    time(&raw_time);
    struct tm* time_info = localtime(&raw_time);
    char time_buf[20];
    if (time_info) {
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", time_info);
    } else {
        time_buf[0] = '\0';
    }

    // Print headers
    fprintf(target, "[%s] [%s] (%s:%d): ", time_buf, level_str, file, line);

    // Print message
    va_list args;
    va_start(args, fmt);
    vfprintf(target, fmt, args);
    va_end(args);

    fprintf(target, "\n");
    fflush(target);
}

void logger_close(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}
