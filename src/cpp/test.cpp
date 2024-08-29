#define PSEUDO_ODIN_IMPLEMENTATION
#include "cpp_odin.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

const Allocator global_allocator {
    /**
     * @note
     *      Standard C `malloc` family takes care of alignment and bookkeeping
     *      for us already.
     */
    [](isize new_sz, isize align, void *prev, isize old_sz, void *ctx) -> void *
    {
        unused(align);
        unused(old_sz);
        unused(ctx);
        if (new_sz == 0) {
            std::free(prev);            
            return nullptr;
        }
        void *next = std::realloc(prev, new_sz);
        if (!next) {
            std::fprintf(stderr, "[FATAL]: %s\n", " [Re]allocation request failure");
            std::fflush(stderr);
            std::abort();
        }
        return next;
    },
    nullptr,
};

static String read_line(String_Builder *bd, FILE *stream)
{
    String out;
    int ch;
    for (;;) {
        ch = std::fgetc(stream);
        if (ch == EOF || std::strchr("\r\n", ch)) {
            break;
        }
        string_builder_write_char(bd, cast(char)ch);
    }
    if (ch == EOF) {
        out = {nullptr, 0};
    } else {
        out = string_builder_to_string(*bd);
    }

    return out;
}

static String get_string(String_Builder *bd, FILE *stream, cstring fmt, ...)
{
    std::va_list args;
    va_start(args, fmt);
    std::vfprintf(stdout, fmt, args);
    va_end(args);
    return read_line(bd, stream);

}

int main()
{
    String_Builder bd = string_builder_make(global_allocator, 64);
    int i = 1;
    for (;;) {
        string_builder_reset(&bd);
        String line = get_string(&bd, stdin, "%i>>> ", i++);
        if (!line.data) {
            break;
        }
        std::printf("'%s'\n", line.data);
    }
    string_builder_destroy(&bd);
    return 0;
}
