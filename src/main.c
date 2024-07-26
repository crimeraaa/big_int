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
    LString *args = context_alloc(LString, argc);
    arena_print(context_arena);
    for (int i = 0; i < argc; i++) {
        size  len  = strlen(argv[i]);
        char *data = context_alloc(char, len + 1);
        data[len]  = '\0';
        args[i].data   = cast(char*, memcpy(data, argv[i], len));
        args[i].length = len;
    }
    
    for (int i = 0; i < argc; i++)
        printf("LString(length=%td,data=\"%s\")\n", args[i].length, args[i].data);
    arena_print(context_arena);
    arena_main(args, argc, context_arena);
}

static void buffer_test(int argc, const char *argv[])
{
    arena_print(context_arena);
    // 1D array of `Buffer*`.
    Buffer **args = context_alloc(Buffer*, argc);
    arena_print(context_arena);
    for (int i = 0; i < argc; i++) {
        Buffer *buf;
        size    len = strlen(argv[i]);
        size    ex  = array_sizeof(buf->data[0], len + 1);

        buf            = context_alloc(Buffer, 1, ex);
        buf->length    = len;
        buf->data[len] = '\0';
        memcpy(buf->data, argv[i], len); // NOTE: Return `Buffer::data` not self
        args[i] = buf;
    }
    
    for (int i = 0; i < argc; i++)
        printf("Buffer(length=%td, data=\"%s\")\n", args[i]->length, args[i]->data);
    arena_print(context_arena);
}

static LString read_line(FILE *stream)
{
    size  len = 0;
    size  cap = 1;
    char *buf = context_alloc(char, cap); // is at least nul char.
    int   ch;
    for (;;) {
        ch = fgetc(stream);
        if (ch == EOF || ch == '\n')
            break;
        // Always save buf[cap - 1] for the nul char.
        if (len >= cap - 1) {
            size newcap = DYNARRAY_GROWCAP(cap);

            // We assume that `ARENA_NOTHROW` is not toggled on.
            buf = context_realloc(char, buf, cap, newcap);
            cap = newcap;
        }
        buf[len++] = cast(char, ch);
    }
    buf[len] = '\0';
    return lstr_make(buf, len);
}

#define TEST_FILENAME   "all-star.txt"

typedef struct ListString ListString;
struct ListString {
    ListString *next;
    size        length;
    char        data[];
};

static ListString *read_file(const char *name)
{ 
    FILE *fp = fopen(name, "r");
    if (fp == nullptr) {
        eprintfln("Failed to open file '%s'", name);
        return nullptr;
    }
    // Forward iteration using only a singly-linked list!
    ListString  *head = nullptr;
    ListString **tail = &head;
    while (!feof(fp)) {
        context_arena = &scratch_arena;
        // It's better to use a singular arena for a single dynamic buffer so that
        // we can guarantee resizes will always just be incrementing counts.
        context_reset();
        LString ln = read_line(fp);

        // Only allocate non-growable items of a known size to the main arena.
        context_arena = &main_arena;
        const size  famsz = array_sizeof(head->data[0], ln.length + 1);
        ListString *next  = context_alloc(ListString, 1, famsz);
        next->next        = *tail;
        next->length      = ln.length;
        memcpy(next->data, ln.data, ln.length);
        
        // Iteration
        *tail = next;
        tail  = &next->next;
    }
    arena_print(&scratch_arena);
    arena_print(&main_arena);
    fclose(fp);
    return head;
}

#define MARK_BEGIN          DEBUG_PRINTLN("===BEGIN===")
#define MARK_END            DEBUG_PRINTLN("===END===")
#define DEBUG_MARK(expr)    do {MARK_BEGIN; expr; MARK_END;} while (0)

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
#ifdef DEBUG_USE_LONGJMP
    jmp_buf err;
    if (setjmp(err) == 0) {
        arena_init(&main_arena, REGION_DEFAULTSIZE, ARENA_NOPADDING, &handle_error, &err);
        arena_init(&scratch_arena, 256, &handle_error, &err);
#else   // DEBUG_USE_LONGJMP not defined.
        arena_init(&main_arena, REGION_DEFAULTSIZE);
        arena_init(&scratch_arena, 256);
#endif  // DEBUG_USE_LONGJMP

        DEBUG_MARK(lstring_test(argc, argv));
        DEBUG_MARK(buffer_test(argc, argv));
        DEBUG_MARK({
            const ListString *list = read_file(TEST_FILENAME);
            int i = 1;
            for (const ListString *it = list; it != nullptr; it = it->next) {
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
