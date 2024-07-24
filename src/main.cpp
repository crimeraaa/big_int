#include "arena.hpp"

// Test to ensure that linking separate translation units works fine
int main(int argc, const char* argv[])
{
    Arena*   arena = arena_new(64);
    LString* args  = arena_alloc_n(LString, arena, argc);
    arena_print(arena);
    for (int i = 0; i < argc; i++) {
        size_t len  = std::strlen(argv[i]);
        char*  data = arena_alloc_n(char, arena, len + 1);
        std::memcpy(data, argv[i], len);
        data[len] = '\0';
        args[i] = {data, len};
    }
    arena_print(arena);
    arena_main(args, argc, arena);
    arena_free(arena);
    return 0;
}
