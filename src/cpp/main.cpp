#define ODIN_IMPLEMENTATION
#include "odin.hpp"
#include "strings.hpp"

#include <cstdio>

int main(int argc, cstring argv[])
{
    String_Builder bd;
    unused(argc);
    unused(argv);
    string_builder_init(&bd, heap_allocator, 0, 64);
    string_builder_append_cstring(&bd, "Hi mom!");

    cstring cs = string_builder_to_cstring(&bd);
    std::printf("cstring(bd) = '%s', len(bd) = %ti, cap(bd) = %ti\n",
                cs, string_builder_len(bd), string_builder_cap(bd));
    std::printf("alignof(bd) = %ti\n", align_of(bd));
    
    string_builder_free(&bd);
    return 0;
}
