#include <setjmp.h>
#include "arena.h"

// #define DEBUG_USE_LONGJMP

static void test_arena(int argc, const char *argv[], Arena *arena)
{
    // Arena   *arena = arena_new(64, ARENA_DEFAULT, nullptr, nullptr);
    LString *args  = arena_xalloc(LString, arena, argc);
    arena_print(arena);
    for (int i = 0; i < argc; i++) {
        Size  len  = strlen(argv[i]);
        char *data = arena_xalloc(char, arena, len + 1);
        data[len]  = '\0';
        args[i].data   = memcpy(data, argv[i], len);
        args[i].length = len;
    }
    arena_print(arena);
    arena_main(args, argc, arena);
}

#ifdef DEBUG_USE_LONGJMP
typedef struct ErrorInfo {
    jmp_buf          buffer;
    volatile LString message;
    volatile Size    requested;
} ErrorInfo;

static void handle_error(const char *msg, size_t sz, void *ctx)
{
    ErrorInfo *err = cast(ErrorInfo*, ctx);
    err->message   = lstr_make(msg, strlen(msg));
    err->requested = sz;
    longjmp(err->buffer, 1);
}

#endif  // DEBUG_USE_LONGJMP

int main(int argc, const char *argv[])
{
    Arena *arena;
#ifdef DEBUG_USE_LONGJMP
    ErrorInfo err;
    if (setjmp(err.buffer) == 0) {
        arena = arena_new(64, ARENA_DEFAULT, &handle_error, &err);
#else   // DEBUG_USE_LONGJMP not defined.
        arena = arena_new_default(64);
#endif  // DEBUG_USE_LONGJMP

        test_arena(argc, argv, arena);
        arena_free(&arena);

#ifdef DEBUG_USE_LONGJMP
    } else {
        fprintf(stderr, "[FATAL]: %s (requested %zu bytes)\n",
                err.message.data, err.requested);
        return EXIT_FAILURE;
    }
#endif  // DEBUG_USE_LONGJMP
    return 0;
}
