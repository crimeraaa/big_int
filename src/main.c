#include <setjmp.h>
#include "arena.h"

// #define DEBUG_USE_LONGJMP

static void lstring_test(int argc, const char *argv[], Arena *arena)
{
    arena_print(arena);
    LString *args = arena_alloc(LString, arena, argc);
    arena_print(arena);
    for (int i = 0; i < argc; i++) {
        size  len  = strlen(argv[i]);
        char *data = arena_alloc(char, arena, len + 1);
        data[len]  = '\0';
        args[i].data   = cast(char*, memcpy(data, argv[i], len));
        args[i].length = len;
    }
    
    for (int i = 0; i < argc; i++)
        printf("LString(length=%td,data=\"%s\")\n", args[i].length, args[i].data);
    arena_print(arena);
}

static void buffer_test(int argc, const char *argv[], Arena *arena)
{
    arena_print(arena);
    // 1D array of `Buffer*`.
    Buffer **args = arena_alloc(Buffer*, arena, argc);
    arena_print(arena);
    for (int i = 0; i < argc; i++) {
        Buffer *buf;
        size    len = strlen(argv[i]);
        size    ex  = array_sizeof(buf->data, len + 1);

        buf            = arena_alloc(Buffer, arena, 1, ex);
        buf->length    = len;
        buf->data[len] = '\0';
        memcpy(buf->data, argv[i], len); // NOTE: Return `Buffer::data` not self
        args[i] = buf;
    }
    
    for (int i = 0; i < argc; i++)
        printf("Buffer(length=%td, data=\"%s\")\n", args[i]->length, args[i]->data);
    arena_print(arena);
}

static LString read_line(FILE *stream, Arena *arena)
{
    size  len = 0;
    size  cap = 1;
    char *buf = arena_alloc(char, arena, cap); // is at least nul char.
    int   ch;
    for (;;) {
        ch = fgetc(stream);
        if (ch == EOF || ch == '\n')
            break;
        // Always save buf[cap - 1] for the nul char.
        if (len >= cap - 1) {
            size newcap = DYNARRAY_GROW(cap);

            // We assume that `ARENA_NOTHROW` is not toggled on.
            buf = arena_realloc(char, arena, buf, cap, newcap);
            cap = newcap;
        }
        buf[len++] = cast(char, ch);
    }
    buf[len] = '\0';
    return lstr_make(buf, len);
}

#define TEST_FILENAME   "all-star.txt"

typedef struct FileLines {
    LString *lines;
    size     count;
} FileLines;

static FileLines read_file(const char *name, Arena *arena)
{ 
    FILE     *fp = fopen(name, "r");
    FileLines out;
    size      cap = 0;

    out.lines = nullptr;
    out.count = 0;
    if (fp == nullptr) {
        eprintfln("Failed to open file '%s'", name);
        return out;
    }
    while (!feof(fp)) {
        LString ln = read_line(fp, arena);
        if (out.count >= cap) {
            size newcap = DYNARRAY_GROW(cap);

            // We assume this will never throw.
            out.lines = arena_realloc(LString, arena, out.lines, cap, newcap);
            cap       = newcap;
        }
        out.lines[out.count++] = ln;
    }
    fclose(fp);
    return out;
}

#define MARK_BEGIN          DEBUG_PRINTLN("===BEGIN===")
#define MARK_END            DEBUG_PRINTLN("===END===")
#define DEBUG_MARK(expr)    MARK_BEGIN; expr; MARK_END;

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
        arena = arena_default();
#endif  // DEBUG_USE_LONGJMP

        DEBUG_MARK(lstring_test(argc, argv, arena));
        DEBUG_MARK(buffer_test(argc, argv, arena));
        
        MARK_BEGIN;
        FileLines fl = read_file(TEST_FILENAME, arena);
        for (size i = 0; i < fl.count; i++)
            printf("[%02td] %s\n", i, fl.lines[i].data);
        MARK_END;

        arena_free(&arena);

#ifdef DEBUG_USE_LONGJMP
    } else {
        return EXIT_FAILURE;
    }
#endif  // DEBUG_USE_LONGJMP
    return 0;
}
