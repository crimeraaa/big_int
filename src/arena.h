#pragma once

#include "common.h"

#define ARENA_FMTSTR        "Arena(active=%td, capacity=%td)"
#define ARENA_FMTSTRLN      ARENA_FMTSTR "\n"
#define ARENA_FMTARGS(a)    (a)->active, (a)->capacity
#define ARENA_DEFAULTSIZE   (1024 * 8)

// https://en.wikipedia.org/wiki/Data_structure_alignment#Computing_padding
#define ARENA_GETPADDING(align, offset)                                        \
(                                                                              \
    ( (align) - ( (offset) % (align) ) ) % (align)                             \
)

typedef struct Arena Arena;
typedef void (*ErrorFn)(const char *msg, size sz, void *ctx);

#define ARENA_NOZERO            0x1
#define ARENA_NOTHROW           0x2
#define ARENA_NODEFAULTFLAGS    (ARENA_NOZERO | ARENA_NOTHROW)
#define ARENA_DEFAULTFLAGS      0x0

struct Arena {
    Arena  *next;
    uint8_t flags;
    ErrorFn handler;
    void   *context;  // The sole argument to `handler`.
    size    active;   // Number of bytes currently being used.
    size    capacity; // Number of bytes we allocated in total.
    byte    buffer[]; // C99 Flexible array member.
};

Arena  *arena_new(size cap, uint8_t flags, ErrorFn fn, void *ctx);
void    arena_set_flags(Arena *self, uint8_t flags);
void    arena_clear_flags(Arena *self, uint8_t flags);
void    arena_reset(Arena *self);
void    arena_free(Arena **self);
void    arena_print(const Arena *self);
void   *arena_alloc(Arena *self, size sz, size align);
void    arena_main(const LString args[], size count, Arena *arena);

// NOTE: ... = self [, len [, align [, extra] ] ]
#define arena__xselect5(a, b, c, d, e, ...)     e
#define arena__xnew4(n, flags, fn, ctx) arena_new(n, flags, fn, ctx)
#define arena__xnew3(n, fn, ctx)        arena_new(n, ARENA_DEFAULTFLAGS, fn, ctx)
#define arena__xnew2(n, flags)          arena_new(n, flags, nullptr, nullptr)
#define arena__xnew1(n)                 arena__xnew3(n, nullptr, nullptr)
#define arena_xnew(...)                                                        \
    arena__xselect5(                                                           \
        __VA_ARGS__,                                                           \
        arena__xnew4,                                                          \
        arena__xnew3,                                                          \
        arena__xnew2,                                                          \
        arena__xnew1,                                                          \
    )(__VA_ARGS__)

#define arena__xselect4(a, b, c, d, ...)    d
#define arena__xalloc4(T, a, n, ex) cast(T*, arena_alloc(a, sizeof(T) * (n) + (ex), alignof(T)))
#define arena__xalloc3(T, a, n)     arena__xalloc4(T, a, n, 0)
#define arena__xalloc2(T, a)        arena__xalloc3(T, a, 1)
#define arena_xalloc(T, ...)                                                   \
    arena__xselect4(                                                           \
        __VA_ARGS__,                                                           \
        arena__xalloc4,                                                        \
        arena__xalloc3,                                                        \
        arena__xalloc2                                                         \
    )(T, __VA_ARGS__)
