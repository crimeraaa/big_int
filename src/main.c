#include <setjmp.h>
#include "arena.h"

// #define DEBUG_USE_LONGJMP

static void test_arena(int argc, const char *argv[], Arena *arena)
{
    // Arena   *arena = arena_rawinit(64, ARENA_DEFAULT, nullptr, nullptr);
    LString *args = arena_alloc(LString, arena, argc);
    arena_print(arena);
    for (int i = 0; i < argc; i++) {
        size  len  = strlen(argv[i]);
        char *data = arena_alloc(char, arena, len + 1);
        data[len]  = '\0';
        args[i].data   = memcpy(data, argv[i], len);
        args[i].length = len;
    }
    arena_print(arena);
    arena_main(args, argc, arena);
}

#ifdef DEBUG_USE_LONGJMP

static void handle_error(const char *msg, size sz, void *ctx)
{
    jmp_buf *e = cast(jmp_buf*, ctx);
    fprintf(stderr, "[FATAL]: %s (requested %td bytes)\n", msg, sz);
    longjmp(*e, 1);
}

#endif  // DEBUG_USE_LONGJMP

int main(int argc, const char *argv[])
{
    Arena *arena;
#ifdef DEBUG_USE_LONGJMP
    jmp_buf err;
    if (setjmp(err) == 0) {
        arena = arena_new(128, &handle_error, &err);
#else   // DEBUG_USE_LONGJMP not defined.
        arena = arena_new(128);
#endif  // DEBUG_USE_LONGJMP

        test_arena(argc, argv, arena);
        arena_free(&arena);

#ifdef DEBUG_USE_LONGJMP
    } else {
        return EXIT_FAILURE;
    }
#endif  // DEBUG_USE_LONGJMP
    return 0;
}
