#pragma once

#include "common.h"

#define REGION_FMTSTR       "Region(active=%td, capacity=%td)"
#define REGION_FMTARGS(r)   region_active(r), region_capacity(r)
#define REGION_DEFAULTSIZE  (1024 * 8)

/* 
Overview
    Bit flags allow us to combine multiple boolean flags into a single unsigned
    integer.

    To enable particular flags, simply use the bitwise OR operator e.g:

    (ARENA_FZEROINIT | ARENA_FTHROW)
    
    Conversely, to disable particular flags, simply use the bitwise XOR operator:
    
    (ARENA_FDEFAULT ^ ARENA_FTHROW)

 */
enum {
    ARENA_FNODEFAULT    = 0x0, // All succeeding bit flags unset.
    ARENA_FZEROINIT     = 0x1, // Enables memset of 0 on Region's buffer.
    ARENA_FTHROW        = 0x2, // Enables error handlers on malloc error.
    ARENA_FALIGN        = 0x4, // Enables determining alignment/padding.
    ARENA_FSMARTREALLOC = 0x8, // Enables trying to find last allocation.
    ARENA_FDEFAULT      = (ARENA_FZEROINIT | ARENA_FTHROW 
                          | ARENA_FALIGN   | ARENA_FSMARTREALLOC),
};

#define BITFLAG_ON(n, flag)     ((n) & (flag))
#define BITFLAG_OFF(n, flag)    (!BITFLAG_ON(n, flag))

// https://github.com/tsoding/arena/blob/master/arena.h
typedef struct Region Region;
typedef struct Arena  Arena;
typedef void (*ErrorFn)(Arena *self, const char *msg, size sz, void *ctx);

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
    u8      flags;
};

typedef struct ArenaArgs {
    ErrorFn handler;
    void   *context;
    size    capacity;
    u8      flags;
} ArenaArgs;

Region *region_new(size cap);
void    region_free(Region *r);
size    region_active(const Region *r);
size    region_capacity(const Region *r);

ArenaArgs arena_defaultargs(void);
bool    arena_init(Arena *self, const ArenaArgs *args);
void    arena_set_flags(Arena *self, u8 flags);
void    arena_clear_flags(Arena *self, u8 flags);
void    arena_reset(Arena *self);
void    arena_free(Arena *self);
size    arena_active(const Arena *self);
size    arena_capacity(const Arena *self);
void    arena_print(const Arena *self);
void   *arena_rawalloc(Arena *self, size rawsz, size align);
void   *arena_rawrealloc(Arena *self, void *ptr, size oldsz, size newsz, size align);
void    arena_main(const StringView args[], size count, Arena *arena);

#define arena__xrawalloc(T, a, sz)  cast(T*, arena_rawalloc(a, sz, alignof(T)))
#define arena__xalloc4(T, a, n, ex) arena__xrawalloc(T, a, sizeof(T) * (n) + (ex))
#define arena__xalloc3(T, a, n)     arena__xalloc4(T, a, n, 0)
#define arena__xalloc2(T, a)        arena__xalloc3(T, a, 1)

// ... = self [, len [, extra] ]
#define arena_alloc(T, ...)                                                    \
    x__xselect4(                                                               \
        __VA_ARGS__,                                                           \
        arena__xalloc4,                                                        \
        arena__xalloc3,                                                        \
        arena__xalloc2                                                         \
    )(T, __VA_ARGS__)

#define arena__xrawrealloc(T, arena, ptr, oldsz, newsz) \
    cast(T*, arena_rawrealloc(arena, ptr, oldsz, newsz, alignof(T)))

#define arena_realloc_extra(T, arena, ptr, oldlen, newlen, extra)              \
    arena__xrawrealloc(                                                        \
        T,                                                                     \
        arena,                                                                 \
        ptr,                                                                   \
        sizeof(T) * (oldlen) + (extra),                                        \
        sizeof(T) * (newlen) + (extra)                                         \
    )

#define arena_realloc(T, arena, ptr, oldlen, newlen) \
    arena_realloc_extra(T, arena, ptr, oldlen, newlen, 0)

// Assumes `da` has members `data`, `length` and `capacity`.
// Inspired by: https://nullprogram.com/blog/2023/10/05/
// and: https://github.com/tsoding/arena/blob/master/arena.h#L89
#define dynarray_push(T, da, val, arena)                                       \
do {                                                                           \
    if ((da)->length >= (da)->capacity) {                                      \
        size oldn_     = (da)->capacity;                                       \
        size newn_     = (oldn_ < 8) ? 8 : oldn_ * 2;                          \
        (da)->data     = arena_realloc(T, arena, (da)->data, oldn_, newn_);    \
        (da)->capacity = newn_;                                                \
    }                                                                          \
    (da)->data[(da)->length++] = (val);                                        \
} while (false)
