#define ODIN_IMPLEMENTATION
#include "odin.hpp"
#include "strings.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#define printfln(fmt, ...)  std::fprintf(stdout, fmt "\n", __VA_ARGS__)
#define println(c_str)      std::fprintf(stdout, "%s\n", c_str)
 
#define eprintf(fmt, ...)   std::fprintf(stderr, fmt, __VA_ARGS__)
#define eprintfln(fmt, ...) std::fprintf(stderr, fmt "\n", __VA_ARGS__)
#define eprintln(c_str)     std::fprintf(stderr, "%s\n", c_str)

static cstring read_line(String_Builder *bd, FILE *stream)
{
    char _buf_[16];
    Slice<char> buf{_buf_, size_of(_buf_)};

    // char buf[16];
    for (;;) {
        /**
         * @brief
         *      Handle EOF.
         *
         * @note
         *      At least with my installation of the UCRT, sending EOF after
         *      some nonzero amount of characters results in fgets() asking for
         *      more characters. I'm unsure why that is based on reading the
         *      source code.
         * 
         * @see
         *      https://github.com/huangqinjin/ucrt/blob/master/stdio/fgets.cpp
         */
        if (!std::fgets(buf.data, len(buf), stream)) {
            break;
        }
        isize end_index   = cstring_find_first_index_any(buf.data, "\r\n");
        bool  has_newline = (end_index != -1);
        if (!has_newline) {
            end_index = len(buf) - 1;
        }
        string_builder_append_string(bd, string_from_slice(buf, 0, end_index));
        if (has_newline) {
            break;
        }
    }
    if (std::ferror(stream) || std::feof(stream)) {
        return nullptr;
    }
    return string_builder_to_cstring(bd);
}

int main(int argc, cstring argv[])
{
    if (argc != 1) {
        if (argc != 3) {
            eprintfln("Usage: %s [pattern <text>]", argv[0]);
            return -1;
        }
        cstring needle   = argv[1];
        cstring haystack = argv[2];
        
        printfln("needle:   cstring(\"%s\")", needle);
        printfln("haystack: cstring(\"%s\")", haystack);
        
        isize i = cstring_find_first_index_any(haystack, needle);
        if (i != -1) {
            printfln("haystack[%ti]: '%c'", i, haystack[i]);
        } else {
            printfln("no character in '%s' found", needle);
        }
    }

    String_Builder bd;
    string_builder_init(&bd, heap_allocator, 0, 32);
    defer(string_builder_free(&bd));

    for (;;) {
        string_builder_reset(&bd);
        std::printf(">>> ");
        cstring c_str = read_line(&bd, stdin);
        if (!c_str) {
            break;
        }
        std::printf("'%s'\n", c_str);
        std::printf("len=%ti, cap=%ti\n", string_builder_len(bd), string_builder_cap(bd));
    }
    return 0;
}
