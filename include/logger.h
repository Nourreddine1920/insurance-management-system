#ifndef LOGGER_H
#define LOGGER_H

/**
 * @brief Logging severity levels.
 */
typedef enum { LOG_LEVEL_DEBUG, LOG_LEVEL_INFO, LOG_LEVEL_WARN, LOG_LEVEL_ERROR } log_level_t;

/**
 * @brief Initializes the logging subsystem.
 * @param log_file_path Optional file path to output logs. If NULL, logs to stdout/stderr.
 * @return void
 */
void logger_init(const char* log_file_path);

/**
 * @brief Core logging function. Usually invoked via macros.
 */
void logger_log(log_level_t level, const char* file, int line, const char* fmt, ...);

/**
 * @brief Shuts down the logging subsystem and closes open file descriptors.
 */
void logger_close(void);

/*
 * Helper macros to automatically populate file and line information.
 * The __VA_OPT__(,) trick is a C23 feature. For C11+GCC, we use
 * the well-established GNU ##__VA_ARGS__ extension. To suppress
 * -Wpedantic warnings on strict builds, we disable that specific
 * warning around these macros.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#define LOG_DEBUG(...) logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) logger_log(LOG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#pragma GCC diagnostic pop

#endif  // LOGGER_H
