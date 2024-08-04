/// local
#define ANSI_INCLUDE_IMPLEMENTATION
#define LOG_INCLUDE_IMPLEMENTATION
#define BIGINT_INCLUDE_IMPLEMENTATION
#include "ansi.h"
#include "bigint.h"

static void test_iadd(int addend1, int addend2)
{
    BigInt bi = bigint_from_int(addend1);
    log_tracef("%s(addend1=%i, addend2=%i)", __func__, addend1, addend2);
    bigint_print(bigint_iadd(&bi, addend2));
    printfln("expected: %i + %i = %i", addend1, addend2, addend1 + addend2);
}

static void test_isub(int minuend, int subtrahend)
{
    BigInt bi = bigint_from_int(minuend);
    log_tracef("%s(minued=%i, subtrahend=%i)", __func__, minuend, subtrahend);
    bigint_print(bigint_isub(&bi, subtrahend));
    printfln("expected: %i - %i = %i", minuend, subtrahend, minuend - subtrahend);
}

int main(void)
{
    log_traceln("trace!");
    log_debugln("debug!");
    log_warnln("warning!");
    log_fatalln("fatal!");

    // iadd_tests();
    test_iadd(1, 4);        //    5
    test_iadd(9, 9);        //   18
    test_iadd(13, 14);      //   27
    test_iadd(33, 99);      //  132
    test_iadd(1234, 5678);  // 6912
    test_iadd(5678, 1234);  // 6912

    test_isub(8, 4);      //  4
    test_isub(4, 8);      // -4
    test_isub(10, 4);     //  6
    test_isub(95, 24);    // 71
    test_isub(153, 78);   // 75
    test_isub(9004, 297); // 8707
    
    // Breakage starts here, because we don't handle the cases where the minuend
    // is smaller than the subtrahend.
    test_isub(11, 109);   // -98
    test_isub(251, 367);  // -116
    test_isub(251, 3670); // -3419
    return 0;
}

#if false

/// local
#define ARENA_INCLUDE_IMPLEMENTATION
#include "arena.h"
#include "log.h"

/// standard
#include <setjmp.h>

#define DEBUG_USE_LONGJMP

// https://github.com/tsoding/arena/tree/master?tab=readme-ov-file#quick-start
static Arena  main_arena    = {0};
static Arena  scratch_arena = {0};
static Arena *context_arena = &main_arena;

#define context_alloc(T, ...)   arena_alloc(T, context_arena, __VA_ARGS__)
#define context_realloc(T, ...) arena_realloc(T, context_arena, __VA_ARGS__)
#define context_reset()         arena_reset(context_arena)
#define push(T, da, v)          dynarray_push(T, da, v, context_arena)

static void lstring_test(int argc, const char *argv[])
{
    // 1D array of `StringView`.
    StringView *args;
    log_tracecall();
    arena_print(context_arena);
    args = context_alloc(StringView, argc);
    arena_print(context_arena);
    for (int i = 0; i < argc; i++) {
        size  len  = strlen(argv[i]);
        char *data = context_alloc(char, len + 1);
        data[len]  = '\0';
        args[i].data   = cast(char*, memcpy(data, argv[i], array_sizeof(data[0], len)));
        args[i].length = len;
    }
    
    for (int i = 0; i < argc; i++)
        printf("StringView(length=%td,data=\"%s\")\n", args[i].length, args[i].data);
    arena_print(context_arena);
    arena_main(args, argc, context_arena);
}

static void famstring_test(int argc, const char *argv[])
{
    FamString **args;
    log_tracecall();
    arena_print(context_arena);
    // 1D array of `FamString*`.
    args = context_alloc(FamString*, argc);
    arena_print(context_arena);
    for (int i = 0; i < argc; i++) {
        FamString *buf;
        size       len = strlen(argv[i]);
        size       ex  = array_sizeof(buf->data[0], len + 1);

        buf            = context_alloc(FamString, 1, ex);
        buf->length    = len;
        buf->data[len] = '\0';
        memcpy(buf->data, argv[i], array_sizeof(buf->data[0], len));
        args[i] = buf;
    }
    
    for (int i = 0; i < argc; i++)
        printf("FamString(length=%td, data=\"%s\")\n", args[i]->length, args[i]->data);
    arena_print(context_arena);
}

#define TEST_FILENAME   "all-star.txt"

static const char *read_line(StringView *out, FILE *stream)
{
    // Allow us to view empty strings until we have user input to allocate.
    static char init[] = "";
    Buffer b;

    log_tracecall();
    // It's better to use a singular arena for a single dynamic buffer so that
    // we can guarantee resizes will always just be incrementing counts.
    context_arena = &scratch_arena;
    context_reset();
    
    b.data     = init;
    b.length   = 0;
    b.capacity = array_countof(init);
    for (;;) {
        int ch = fgetc(stream);
        if (ch == EOF || ch == '\n')
            break;
        push(char, &b, cast(char, ch));
    }
    // Only allocate to the main arena after we're done with the dynamic stuff.
    context_arena = &main_arena;

    if (ferror(stream)) {
        *out = sv_literal("(ERROR)");
        return nullptr;
    } else if (feof(stream)) {
        // TODO: May break for files w/o newline terminating very last line
        *out = sv_literal("(EOF)");
        return nullptr;
    } else {
        out->data   = b.data;
        out->length = b.length;
        return b.data;
    }
}

static StringList *read_file(const char *name)
{ 
    FILE        *fp;
    StringList  *head = nullptr;
    StringList **tail = &head;
    StringView   line;
    log_tracecall();

    fp = fopen(name, "r");
    if (!fp) {
        eprintfln("Failed to open file '%s'", name);
        return nullptr;
    }
    // Forward iteration using only a singly-linked list!
    while (read_line(&line, fp)) {
        StringList *next;
        size        ex = array_sizeof(next->data[0], line.length + 1);

        next         = context_alloc(StringList, 1, ex);
        next->next   = *tail;
        next->length = line.length;

        memcpy(next->data, line.data, array_sizeof(next->data[0], line.length));
        next->data[line.length] = '\0';
        
        // Iteration
        *tail = next;
        tail  = &next->next;
    }
    fclose(fp);
    return head;
}

// https://nullprogram.com/blog/2023/10/05/
I32Array fibonacci(i32 max)
{
    static i32 init[] = {0, 1};
    I32Array   fib;

    log_tracecall();
    fib.data     = init;
    fib.length   = array_countof(init); 
    fib.capacity = fib.length;
    for (;;) {
        i32 a = fib.data[fib.length - 2];
        i32 b = fib.data[fib.length - 1];
        if (a + b > max)
            break;
        push(i32, &fib, a + b);
        
    }
    return fib;
}

#define DEBUG_MARK(expr)                                                       \
do {                                                                           \
    log_debugln("===BEGIN===");                                                \
    expr;                                                                      \
    log_debugln("===END===\n");                                                \
} while (false)

#ifdef DEBUG_USE_LONGJMP

static void handle_error(Arena *self, const char *msg, size sz, void *ctx)
{
    jmp_buf *e = cast(jmp_buf*, ctx);
    unused(self);
    log_fatalf("%s (requested %td bytes)", msg, sz);
    longjmp(*e, 1);
}

#endif  // DEBUG_USE_LONGJMP

static void cleanup_arenas(void)
{
    arena_free(&scratch_arena);
    arena_free(&main_arena);    
}

int main(int argc, const char *argv[])
{
    ArenaArgs scratch_init = arena_defaultargs();
    scratch_init.capacity  = 256;
    atexit(&cleanup_arenas);
    log_tracecall();
#ifdef DEBUG_USE_LONGJMP
    jmp_buf err;
    if (setjmp(err) == 0) {
        ArenaArgs main_init  = arena_defaultargs();
        scratch_init.handler = main_init.handler = &handle_error;
        scratch_init.context = main_init.context = &err;
        scratch_init.flags   = main_init.flags  ^= ARENA_FZEROINIT | ARENA_FALIGN;
#endif
        arena_init(&main_arena, nullptr);
        arena_init(&scratch_arena, &scratch_init);

        DEBUG_MARK(lstring_test(argc, argv));
        DEBUG_MARK({
            context_reset();
            famstring_test(argc, argv);
        });
        DEBUG_MARK({
            int i = 1;
            context_reset();
            for (const StringList *it = read_file(TEST_FILENAME);
                 it != nullptr;
                 it = it->next)
            {
                printfln("%02i: %s", i++, it->data);
            }
            arena_print(&scratch_arena);
            arena_print(&main_arena);
        });
        
        DEBUG_MARK({
            context_arena = &scratch_arena;
            context_reset();
            // If we pass INT32_MAX itself, a + b > max may overflow!
            const I32Array ia = fibonacci(INT32_MAX / 2);
            for (size i = 0; 0 <= i && i < ia.length; i++) {
                if (0 < i && i < ia.length) {
                    printf(", ");
                }
                printf("%i", ia.data[i]);
            }
            context_arena = &main_arena;
            println("");
        });
#ifdef DEBUG_USE_LONGJMP
    } else {
        return EXIT_FAILURE;
    }
#endif  // DEBUG_USE_LONGJMP
    return 0;
}

#endif // false
