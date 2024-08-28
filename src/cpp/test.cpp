#define PSEUDO_ODIN_IMPLEMENTATION
#include "pseudo_odin.hpp"

#include <cstdarg>
#include <cstdlib>
#include <cstdio>

// Standard C allocator
static const Allocator global_allocator = {
    // AllocFn
    [](void *hint, isize sz_old, isize sz_new, void *ctx) -> void * 
    {
        unused(sz_old);
        unused(ctx);
        if (!hint && sz_new == 0) {
            std::free(hint);
            return nullptr;
        }
        return std::realloc(hint, sz_new);
    },
    // ErrorFn
    [](const char *msg, void *ctx)
    {
        unused(ctx);
        std::fprintf(stderr, "[FATAL] %s\n", msg);
        std::fflush(stderr);
        std::abort();
    },
    // context
    nullptr,
};

static String read_line(String *buf, FILE *stream)
{
    int ch;
    for (;;) {
        ch = std::fgetc(stream);
        if (ch == EOF || ch == '\r' || ch == '\n') {
            break;
        }
        string_append_char(buf, cast(char)ch);
    }
    return (ch == EOF) ? nullptr : *buf;
}

static String get_string(String *buf, const char *fmt, ...)
{
    std::va_list args;
    va_start(args, fmt);
    std::vfprintf(stdout, fmt, args);
    va_end(args);
    return read_line(buf, stdin);
}

int main()
{
    String buf = string_make_reserve(global_allocator, 8);
    for (;;) {
        String line;
        string_clear(&buf);
        line = get_string(&buf, ">>> ");
        if (!line) {
            break;
        }
        std::printf("'%s' (len=%ti, cap=%ti)\n", line, string_len(line), string_cap(line));
    }
    string_delete(&buf);
    return 0;
}
