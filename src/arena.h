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

// Disable zero initialization.
#define ARENA_NOZERO            0x1

// Disable calling the error handler. Allows [re]alloc to return `nullptr`.
#define ARENA_NOTHROW           0x2

// Disable determining if a given pointer does not need to be alloc'd again,
#define ARENA_NOSMARTREALLOC    0x4
#define ARENA_NODEFAULTFLAGS    (ARENA_NOZERO | ARENA_NOTHROW | ARENA_NOSMARTREALLOC)
#define ARENA_DEFAULTFLAGS      0x0
#define BITFLAG_IS_ON(n, flag)  ((n) & (flag))
#define BITFLAG_IS_OFF(n, flag) (!BITFLAG_IS_ON(n, flag))

#define DYNARRAY_INIT           32
#define DYNARRAY_GROW(cap)      ((cap) < DYNARRAY_INIT) ? DYNARRAY_INIT : (cap) * 2

struct Arena {
    Arena  *next;
    ErrorFn handler;
    void   *context;  // The sole argument to `handler`.
    uint8_t flags;    // Don't put right above `buffer` to ensure alignment.
    size    active;   // Number of bytes currently being used.
    size    capacity; // Number of bytes we allocated in total.
    byte    buffer[]; // C99 Flexible array member. NOT valid in standard C++!
};

Arena  *arena_rawnew(size cap, uint8_t flags, ErrorFn fn, void *ctx);
void    arena_set_flags(Arena *self, uint8_t flags);
void    arena_clear_flags(Arena *self, uint8_t flags);
void    arena_reset(Arena *self);
void    arena_free(Arena **self);
size    arena_count_active(const Arena *self);
size    arena_count_capacity(const Arena *self);
void    arena_print(const Arena *self);
void   *arena_rawalloc(Arena *self, size rawsz, size align);
void   *arena_rawrealloc(Arena *self, void *ptr, size oldsz, size newsz, size align);
void    arena_main(const LString args[], size count, Arena *arena);

#define arena__xselect5(a, b, c, d, e, ...)     e
#define arena__xnew4(n, flags, fn, ctx) arena_rawnew(n, flags, fn, ctx)
#define arena__xnew3(n, fn, ctx)        arena_rawnew(n, ARENA_DEFAULTFLAGS, fn, ctx)
#define arena__xnew2(n, flags)          arena_rawnew(n, flags, nullptr, nullptr)
#define arena__xnew1(n)                 arena__xnew3(n, nullptr, nullptr)
#define arena_default()                 arena__xnew1(ARENA_DEFAULTSIZE)

// ... = len [, flags [, fn, ctx ] ] OR
// ... = len [, fn, ctx ]
#define arena_new(...)                                                         \
    arena__xselect5(                                                           \
        __VA_ARGS__,                                                           \
        arena__xnew4,                                                          \
        arena__xnew3,                                                          \
        arena__xnew2,                                                          \
        arena__xnew1,                                                          \
    )(__VA_ARGS__)


#define arena__xselect4(a, b, c, d, ...)    d
#define arena__xrawalloc(T, a, sz)  cast(T*, arena_rawalloc(a, sz, alignof(T)))
#define arena__xalloc4(T, a, n, ex) arena__xrawalloc(T, a, sizeof(T) * (n) + (ex))
#define arena__xalloc3(T, a, n)     arena__xalloc4(T, a, n, 0)
#define arena__xalloc2(T, a)        arena__xalloc3(T, a, 1)

// ... = self [, len [, extra] ]
#define arena_alloc(T, ...)                                                    \
    arena__xselect4(                                                           \
        __VA_ARGS__,                                                           \
        arena__xalloc4,                                                        \
        arena__xalloc3,                                                        \
        arena__xalloc2                                                         \
    )(T, __VA_ARGS__)

#define arena__xrawrealloc(T, a, p, oldsz, newsz) \
    cast(T*, arena_rawrealloc(a, p, oldsz, newsz, alignof(T)))
#define arena_realloc(T, a, ptr, oldn, newn) \
    arena__xrawrealloc(T, a, ptr, sizeof(T) * (oldn), sizeof(T) * (newn))
