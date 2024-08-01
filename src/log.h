#pragma once

enum LogLevel {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_WARN,
    LOG_FATAL,
};

// TODO: Allow writing to streams other than stderr? Allow for quiet?
void log_writer(enum LogLevel lvl, const char *file, int line, const char *fmt, ...);

#define log_debugf(fmt, ...)    log_writer(LOG_DEBUG, __FILE__, __LINE__, fmt "\n", __VA_ARGS__)
#define log_tracef(fmt, ...)    log_writer(LOG_TRACE, __FILE__, __LINE__, fmt "\n", __VA_ARGS__)
#define log_warnf(fmt, ...)     log_writer(LOG_WARN,  __FILE__, __LINE__, fmt "\n", __VA_ARGS__)
#define log_fatalf(fmt, ...)    log_writer(LOG_FATAL, __FILE__, __LINE__, fmt "\n", __VA_ARGS__)
#define log_debugln(s)          log_debugf("%s", s)
#define log_traceln(s)          log_tracef("%s", s)
#define log_tracecall()         log_tracef("%s()", __func__)
#define log_warnln(s)           log_warnf("%s", s)
#define log_fatalln(s)          log_fatalf("%s", s)

#ifdef LOG_INCLUDE_IMPLEMENTATION

/// local
#include "common.h"

/// standard
#include <stdarg.h>

// Maps `enum LogLevel` to `const char*`.
static const char *const LOG_LEVEL_STRINGS[] = {
    [LOG_TRACE] = "TRACE",
    [LOG_DEBUG] = "DEBUG",
    [LOG_WARN]  = "WARN",
    [LOG_FATAL] = "FATAL",
};

static const char *get_filename_only(const char *path)
{
    const char *name = strrchr(path, '/');
    // Don't have UNIX-style separator, try Windows-style separator.
    if (!name)
        name = strrchr(path, '\\');
    // Point to character AFTER the separator if we have one.
    return (name) ? name + 1 : path;
}

void log_writer(enum LogLevel lvl, const char *file, int line, const char *fmt, ...)
{
    va_list args;
    char    hdr[16];
    int     len = sprintf(hdr, "[%s]", LOG_LEVEL_STRINGS[lvl]);
    hdr[len] = '\0';
    
    va_start(args, fmt);
    // May be wasteful
    fprintf(stderr, "%-8s %s(%i): ", hdr, get_filename_only(file), line);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

#endif
