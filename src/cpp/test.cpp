#include <cstdarg>
#include <cstdio>

#define STRING_INCLUDE_IMPLEMENTATION
#include "string.hpp"
#include "allocator.hpp"

const Allocator& allocator = allocator_default;

static String read_line(String_Builder* bd, FILE* stream)
{
    int ch;
    for (;;) {
        ch = fgetc(stream);
        if (ch == EOF || ch == '\r' || ch == '\n') {
            break;
        }
        string_builder_write_char(bd, char(ch));
    }
    if (ch == EOF) {
        return {nullptr, 0};
    }
    string_builder_write_char(bd, '\0');
    return string_builder_to_string(*bd);
}

static String get_string(String_Builder* bd, const char* prompt, ...)
{
    std::va_list args;
    va_start(args, prompt);
    std::vfprintf(stdout, prompt, args);
    va_end(args);
    return read_line(bd, stdin);
}

int main()
{
    String_Builder bd = string_builder_make(8, allocator);
    int i = 0;
    for (;;) {
        string_builder_reset(&bd);
        String line = get_string(&bd, "[%i]>>> ", i++);
        // Got EOF?
        if (!line.start) {
            break;
        }
        std::printf("%s\n", line.start);
    }
    string_builder_destroy(&bd);
    return 0;
}
