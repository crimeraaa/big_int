#include <cstdarg>
#include <cstdio>

#define STRING_INCLUDE_IMPLEMENTATION
#include "string.hpp"
#include "allocator.hpp"

static const char* read_line(String_Builder* bd, FILE* stream)
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
        return nullptr;
    }
    return string_builder_to_cstring(bd);
}

static const char* get_string(String_Builder* bd, const char* prompt, ...)
{
    std::va_list args;
    va_start(args, prompt);
    std::vfprintf(stdout, prompt, args);
    va_end(args);
    return read_line(bd, stdin);
}

int main()
{
    String_Builder bd = string_builder_make(8, allocator_default);
    int i = 0;
    for (;;) {
        string_builder_reset(&bd);
        const char* line = get_string(&bd, "[%i] >>> ", i++);
        // Got EOF?
        if (!line) {
            break;
        }
        std::printf("%s\n", line);
    }
    string_builder_destroy(&bd);
    return 0;
}
