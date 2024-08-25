#include <cstdarg>
#include <cstdio>

#include "common.hpp"
#include "pointer.hpp"
#include "slice.hpp"

#define ALLOCATOR_INCLUDE_IMPLEMENTATION
#include "allocator.hpp"
#include "dynamic_array.hpp"

const Allocator& allocator = allocator_default;

using String_Buffer = Dynamic_Array<char>;

static String read_line(String_Buffer* buf, FILE* stream)
{
    int ch;
    for (;;) {
        ch = fgetc(stream);
        if (ch == EOF || ch == '\r' || ch == '\n') {
            break;
        }
        append(buf, char(ch));
    }
    append(buf, '\0');
    String str;
    str.start  = (ch == EOF) ? nullptr : &(*buf)[0];
    str.length = (ch == EOF) ? 0       : len(buf);
    return str;
}

static String get_string(String_Buffer *buf, const char* prompt, ...)
{
    std::va_list args;
    va_start(args, prompt);
    std::vfprintf(stdout, prompt, args);
    va_end(args);
    return read_line(buf, stdin);
}

int main()
{
    String_Buffer buf = dynamic_array_make<char>(4, allocator);
    int i = 0;
    for (;;) {
        clear(&buf);
        String line = get_string(&buf, "[%i]>>> ", i++);
        // Got EOF?
        if (!line.start) {
            break;
        }
        std::printf("%s\n", line.start);
    }
    destroy(&buf);
    return 0;
}
