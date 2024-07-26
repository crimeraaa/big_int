#pragma once

#include "common.h"

#define REGION_FMTSTR       "Region(active=%td, capacity=%td)"
#define REGION_FMTSTRLN     REGION_FMTSTR "\n"
#define REGION_ACTIVE(r)    ((r)->free - (r)->buffer)
#define REGION_CAPACITY(r)  ((r)->end - (r)->buffer)
#define REGION_FMTARGS(r)   REGION_ACTIVE(r), REGION_CAPACITY(r)
#define REGION_DEFAULTSIZE  (1024 * 8)

// https://en.wikipedia.org/wiki/Data_structure_alignment#Computing_padding
#define ARENA_GETPADDING(align, offset)                                        \
(                                                                              \
    ( (align) - ( (offset) % (align) ) ) % (align)                             \
)

// Disable zero initialization upon allocation of a new region.
#define ARENA_NOZERO            0x1

// Disable calling the error handler. 
// This will allows arena_[re]alloc to return `nullptr`.
#define ARENA_NOTHROW           0x2

// Disable trying to align pointers. This may help reduce active memory usage,
// at the cost of increasing CPU cycles taken to read things into memory.
#define ARENA_NOPADDING         0x4

// Disable determining if a given pointer does not need to be alloc'd again,
#define ARENA_NOSMARTREALLOC    0x8
#define ARENA_NODEFAULTFLAGS    (ARENA_NOZERO \
                                | ARENA_NOTHROW \
                                | ARENA_NOPADDING \
                                | ARENA_NOSMARTREALLOC)

#define ARENA_DEFAULTFLAGS      0x0
#define BITFLAG_ON(n, flag)     ((n) & (flag))
#define BITFLAG_OFF(n, flag)    (!BITFLAG_ON(n, flag))

#define DYNARRAY_STARTCAP       32
#define DYNARRAY_GROWCAP(cap)   ((cap) < DYNARRAY_STARTCAP) ? DYNARRAY_STARTCAP : (cap) * 2

// https://github.com/tsoding/arena/blob/master/arena.h
typedef struct Region Region;
typedef struct Arena Arena;
typedef void (*ErrorFn)(const char *msg, size sz, void *ctx);

struct Region {
    Region *next;     // Regions can link to each other.
    byte   *free;     // Points to the first free slot.
    byte   *end;      // &buffer[capacity]. NOT dereferencable!
    byte    buffer[]; // C99 Flexible array member. NOT valid in standard C++!
};

struct Arena {
    ErrorFn handler;
    void   *context;  // The 3rd argument to `handler`.
    Region *begin;
    Region *end;
    uint8_t flags;
};

bool  arena_rawinit(Arena *self, size cap, uint8_t flags, ErrorFn fn, void *ctx);
void  arena_set_flags(Arena *self, uint8_t flags);
void  arena_clear_flags(Arena *self, uint8_t flags);
void  arena_reset(Arena *self);
void  arena_free(Arena *self);
size  arena_active(const Arena *self);
size  arena_capacity(const Arena *self);
void  arena_print(const Arena *self);
void *arena_rawalloc(Arena *self, size rawsz, size align);
void *arena_rawrealloc(Arena *self, void *ptr, size oldsz, size newsz, size align);
void  arena_main(const LString args[], size count, Arena *arena);

#define arena__xselect6(a, b, c, d, e, f, ...)     f
#define arena__xinit5(a, n, flg, fn, ctx)   arena_rawinit(a, n, flg, fn, ctx)
#define arena__xinit4(a, n, fn, ctx)        arena_rawinit(a, n, ARENA_DEFAULTFLAGS, fn, ctx)
#define arena__xinit3(a, n, flg)            arena_rawinit(a, n, flg, nullptr, nullptr)
#define arena__xinit2(a, n)                 arena__xinit4(a, n, nullptr, nullptr)
#define arena__xinit1(a)                    arena__xinit2(a, REGION_DEFAULTSIZE)

// ... = self [, len [, flags [, fn, ctx ] ] ] OR
// ... = self [, len [, fn, ctx ] ]
#define arena_init(...)                                                        \
    arena__xselect6(                                                           \
        __VA_ARGS__,                                                           \
        arena__xinit5,                                                         \
        arena__xinit4,                                                         \
        arena__xinit3,                                                         \
        arena__xinit2,                                                         \
        arena__xinit1,                                                         \
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
