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

/// standard
#include <stdarg.h>

/// local
#include "common.h"
#include "ansi.h"

struct LogHeader {
    const char    *text;
    enum AnsiColor color;
};

// Maps `enum LogLevel` to `const char*`.
static const struct LogHeader LOGHEADERS[] = {
    [LOG_TRACE] = {"[TRACE]",   ANSI256_PLUM3},
    [LOG_DEBUG] = {"[DEBUG]",   ANSI256_LIGHTCYAN3},
    [LOG_WARN]  = {"[WARN]",    ANSI256_NAVAJOWHITE1},
    [LOG_FATAL] = {"[FATAL]",   ANSI256_SALMON1},
};

static const char *get_filename_only(const char *path)
{
    const char *name = strrchr(path, '/');
#ifdef _WIN32
    // If on Windows and don't have UNIX-style separator try Windows-style separator.
    if (!name)
        name = strrchr(path, '\\');
#endif
    // Point to character AFTER the separator if we have one.
    return (name) ? name + 1 : path;
}

void log_writer(enum LogLevel lvl, const char *file, int line, const char *fmt, ...)
{
    va_list     args;
    const char *name = get_filename_only(file);
    va_start(args, fmt);

    // Silly but it works
    ansi_printfg_256color(stderr, LOGHEADERS[lvl].color, "%-8s", LOGHEADERS[lvl].text);
    ansi_printfg_256color(stderr, ANSI256_PALETURQUOISE1, "%s(%i): ", name, line);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

#endif  // LOG_INCLUDE_IMPLEMENTATION
