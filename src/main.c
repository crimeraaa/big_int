#include <setjmp.h>
#include "arena.h"

#define DEBUG_USE_LONGJMP

// https://github.com/tsoding/arena/tree/master?tab=readme-ov-file#quick-start
static Arena  main_arena    = {0};
static Arena  scratch_arena = {0};
static Arena *context_arena = &main_arena;

#define context_alloc(T, ...)   arena_alloc(T, context_arena, __VA_ARGS__)
#define context_realloc(T, ...) arena_realloc(T, context_arena, __VA_ARGS__)
#define context_reset()         arena_reset(context_arena)

static void lstring_test(int argc, const char *argv[])
{
    arena_print(context_arena);
    StringView *args = context_alloc(StringView, argc);
    arena_print(context_arena);
    for (int i = 0; i < argc; i++) {
        size  len  = strlen(argv[i]);
        char *data = context_alloc(char, len + 1);
        data[len]  = '\0';
        args[i].data   = cast_ptr(char, memcpy(data, argv[i], len));
        args[i].length = len;
    }
    
    for (int i = 0; i < argc; i++)
        printf("StringView(length=%td,data=\"%s\")\n", args[i].length, args[i].data);
    arena_print(context_arena);
    arena_main(args, argc, context_arena);
}

static void famstring_test(int argc, const char *argv[])
{
    arena_print(context_arena);
    // 1D array of `FamString*`.
    FamString **args = context_alloc(FamString*, argc);
    arena_print(context_arena);
    for (int i = 0; i < argc; i++) {
        FamString *buf;
        size       len = strlen(argv[i]);
        size       ex  = array_sizeof(buf->data[0], len + 1);

        buf            = context_alloc(FamString, 1, ex);
        buf->length    = len;
        buf->data[len] = '\0';
        memcpy(buf->data, argv[i], len); // NOTE: Return `FamString::data` not self
        args[i] = buf;
    }
    
    for (int i = 0; i < argc; i++)
        printf("FamString(length=%td, data=\"%s\")\n", args[i]->length, args[i]->data);
    arena_print(context_arena);
}

#define TEST_FILENAME   "all-star.txt"

typedef struct StringList StringList;
struct StringList {
    StringList *next;
    size        length;
    char        data[];
};

typedef struct Buffer {
    char *data;
    size  length;
    size  capacity;
} Buffer;

static const char *context_readline(StringView *out, FILE *stream)
{
    static char init[] = {'\0'};
    Buffer b;

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
        // Always save at least the last valid spot for the nul char.
        if (b.length >= b.capacity - 1) {
            arena_grow_array(context_arena, &b);
        }
        b.data[b.length++] = cast(char, ch);
    }
    // Only allocate non-growable items of a known size to the main arena.
    context_arena = &main_arena;

    // NOTE: Assumes that the very last line has newline right before EOF.
    if (feof(stream)) {
        *out = lstr_literal("(EOF)");
        return nullptr;
    } else {
        if (b.data) {
            b.data[b.length] = '\0';
        }
        out->data   = b.data;
        out->length = b.length;
        return b.data;
    }
}

static StringList *context_readfile(const char *name)
{ 
    FILE *fp = fopen(name, "r");
    if (!fp) {
        eprintfln("Failed to open file '%s'", name);
        return nullptr;
    }
    // Forward iteration using only a singly-linked list!
    StringList  *head = nullptr;
    StringList **tail = &head;
    StringView   line;
    while (context_readline(&line, fp)) {
        const size  ex   = array_sizeof(head->data[0], line.length + 1);
        StringList *next = context_alloc(StringList, 1, ex);
        next->next       = *tail;
        next->length     = line.length;

        memcpy(next->data, line.data, line.length);
        next->data[line.length] = '\0';
        
        // Iteration
        *tail = next;
        tail  = &next->next;
    }
    fclose(fp);
    return head;
}

// https://nullprogram.com/blog/2023/10/05/
I32Array fibonacci(i32 max, Arena *scratch)
{
    static i32 init[] = {0, 1};
    I32Array   fib    = {0};

    fib.data     = init;
    fib.length   = array_countof(init); 
    fib.capacity = fib.length;
    
    for (;;) {
        i32 a = fib.data[fib.length - 2];
        i32 b = fib.data[fib.length - 1];
        if (a + b > max)
            break;
        if (fib.length >= fib.capacity) {
            arena_grow_array(scratch, &fib);
        }
        fib.data[fib.length++] = a + b;
        
    }
    return fib;
}

#define DEBUG_MARK(expr)                                                       \
do {                                                                           \
    dprintln("===BEGIN===");                                                   \
    expr;                                                                      \
    dprintln("===END==="); \
} while (false)

#ifdef DEBUG_USE_LONGJMP

static void handle_error(const char *msg, size sz, void *ctx)
{
    jmp_buf *e = cast_ptr(jmp_buf, ctx);
    fprintf(stderr, "[FATAL]: %s (requested %td bytes)\n", msg, sz);
    longjmp(*e, 1);
}

#endif  // DEBUG_USE_LONGJMP

int main(int argc, const char *argv[])
{
#ifdef DEBUG_USE_LONGJMP
    jmp_buf err;
    if (setjmp(err) == 0) {
        arena_init(/* self  */  &main_arena,
                   /* cap   */  REGION_DEFAULTSIZE,
                   /* fn    */  &handle_error,
                   /* ctx   */  &err);

        arena_init(/* self  */  &scratch_arena,
                   /* cap   */  256,
                   /* fn    */  &handle_error,
                   /* ctx   */  &err);
#else   // DEBUG_USE_LONGJMP not defined.
        arena_init(&main_arena, REGION_DEFAULTSIZE);
        arena_init(&scratch_arena, 256);
#endif  // DEBUG_USE_LONGJMP

        DEBUG_MARK(lstring_test(argc, argv));
        DEBUG_MARK(famstring_test(argc, argv));
        DEBUG_MARK({
            int i = 1;
            for (const StringList *it = context_readfile(TEST_FILENAME);
                 it != nullptr;
                 it = it->next)
            {
                printfln("%02i: %s", i++, it->data);
            }
        });

        arena_free(&scratch_arena);
        arena_free(&main_arena);

#ifdef DEBUG_USE_LONGJMP
    } else {
        return EXIT_FAILURE;
    }
#endif  // DEBUG_USE_LONGJMP
    return 0;
}
