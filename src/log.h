#pragma once

/// local
#include "common.h"

enum LogLevel {
    LOG_TRACE,
    LOG_DEBUG,
};

// TODO: Allow writing to streams other than stderr? Allow for quiet?
void log_writer(enum LogLevel level, const char *file, int line, const char *fmt, ...);

#define log_debugf(fmt, ...)    log_writer(LOG_DEBUG, __FILE__, __LINE__, fmt "\n", __VA_ARGS__)
#define log_debugln(s)          log_debugf("%s", s)
#define log_tracef(fmt, ...)    log_writer(LOG_TRACE, __FILE__, __LINE__, fmt "\n", __VA_ARGS__)
#define log_traceln(s)          log_tracef("%s", s)
#define log_tracecall()         log_tracef("%s()", __func__)

// #define LOG_WANT_IMPLEMENTATION
#ifdef LOG_WANT_IMPLEMENTATION

/// standard
#include <stdarg.h>

const char *const LOG_LEVEL_STRINGS[] = {
    [LOG_TRACE] = "TRACE",
    [LOG_DEBUG] = "DEBUG",
};

void log_writer(enum LogLevel level, const char *file, int line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    // May be wasteful
    fprintf(stderr, "[%s] %s(%i) ", LOG_LEVEL_STRINGS[level], file, line);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

#endif
