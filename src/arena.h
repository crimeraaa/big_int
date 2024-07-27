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
    ARENA_FNODEFAULT    = 0x0,
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
    u8      flags;
};

Region *region_new(size cap);
void    region_free(Region *r);
size    region_active(const Region *r);
size    region_capacity(const Region *r);

bool    arena_rawinit(Arena *self, size cap, u8 flags, ErrorFn fn, void *ctx);
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

/**
 * @param   slice   Pointer to a struct which is similar to `RawBuffer`. It must
 *                  contain a data pointer, length, and capacity, in that order.
 *
 * @note    Relies on the above assumption as well as the idea that all data
 *          pointers are the same size. If they are not, this will fail.
 */
void   *arena_rawgrow_array(Arena *self, void *slice, size tsize, size align);

#define arena__xinit5(a, n, flg, fn, ctx) arena_rawinit(a, n, flg, fn, ctx)
#define arena__xinit4(a, n, fn, ctx) arena__xinit5(a, n, ARENA_FDEFAULT, fn, ctx)
#define arena__xinit3(a, n, flg)    arena__xinit5(a, n, flg, nullptr, nullptr)
#define arena__xinit2(a, n)         arena__xinit3(a, n, ARENA_FDEFAULT)
#define arena__xinit1(a)            arena__xinit2(a, REGION_DEFAULTSIZE)

// ... = self [, len [, flags [, fn, ctx ] ] ] OR
// ... = self [, len [, fn, ctx ] ]
// TODO: Differentiate (self, len, fn, ctx) and (self, flags, fn, ctx)?
#define arena_init(...)                                                        \
    x__xselect6(                                                               \
        __VA_ARGS__,                                                           \
        arena__xinit5,                                                         \
        arena__xinit4,                                                         \
        arena__xinit3,                                                         \
        arena__xinit2,                                                         \
        arena__xinit1,                                                         \
    )(__VA_ARGS__)


#define arena__xrawalloc(T, a, sz)  cast_ptr(T, arena_rawalloc(a, sz, alignof(T)))
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

#define arena__xrawrealloc(T, a, p, oldsz, newsz) \
    cast_ptr(T, arena_rawrealloc(a, p, oldsz, newsz, alignof(T)))
#define arena_realloc(T, a, ptr, oldn, newn) \
    arena__xrawrealloc(T, a, ptr, sizeof(T) * (oldn), sizeof(T) * (newn))

// Unfortunately we cannot do `alignof((s)->data[0])`, so align is fixed.
#define arena_grow_array(a, s)                                                 \
(                                                                              \
    arena_rawgrow_array(a, s, sizeof((s)->data[0]), alignof(RawBuffer))        \
)

// Checks if we need to grow and returns a pointer to the next element.
// https://nullprogram.com/blog/2023/10/05/
#define dynarray_push(da, arena)                                               \
(                                                                              \
    ((da)->length >= (da)->capacity)                                           \
        ? arena_grow_array(arena, da), (da)->data + (da)->length++             \
        : (da)->data + (da)->length++                                          \
)
