#pragma once

/// local
#include "common.h"

#define REGION_FMTSTR       "Region(active=%td, capacity=%td)"
#define REGION_FMTARGS(r)   region_active(r), region_capacity(r)
#define REGION_DEFAULTSIZE  (1024 * 8)

/**
 * OVERVIEW:
 *     Bit flags allow us to combine multiple boolean flags into a single unsigned
 *     integer.
 *
 * ENABLING:
 *     To enable particular flags, simply bitwise OR all the flags you'd like, e.g:
 * 
 *     u8 myflags = ARENA_FZEROINIT | ARENA_FTHROW;
 *     myflags |= ARENA_FGROW;
 *
 * DISABLING: 
 *     Conversely, to disable particular flags, use `ARENA_FDEFAULT' as a bitmask,
 *     then bitwise XOR the masks you don't want, e.g:
 *     
 *     u8 myflags = ARENA_FDEFAULT ^ ARENA_FALIGN ^ ARENA_FSMARTREALLOC;
 *     myflags ^= ARENA_FGROW;
 *     
 * CHECKING:
 *      To check if a given bitflag is enabled use the bitwise AND.
 *  
 *      u8 test = ARENA_FDEFAULT;
 *      if (test & ARENA_FALIGN)
 *          printf("Alignment is enabled!\n");
 *      else
 *          printf("Alignment is not enabled.\n");
 *      
 *      To check if a given bitflag is NOT enabled, you have 2 options.
 *      1.) Simply logical NOT the result of the bitwise AND.
 *      
 *      u8 test = ARENA_FNODEFAULT;
 *      if (!(test & ARENA_FALIGN))
 *          printf("Alignment is not enabled!\n");
 *      else
 *          printf("Alignment is enabled!\n");
 *          
 *      2.) Bitwise XOR the result of the bitwise AND with the flag.
 * 
 *      u8 test = ARENA_FNODEFAULT;
 *      if ((test & ARENA_FALIGN) ^ ARENA_FALIGN)
 *          printf("Alignment is not enabled.\n");
 *      else
 *          printf("Alignment is enabled!\n");
 * 
 */
enum {
    ARENA_FNODEFAULT    = 0x0, // All succeeding bit flags unset.
    ARENA_FZEROINIT     = 0x1, // Memset Region buffer to 0 on init?
    ARENA_FGROW         = 0x2, // Allow Region to grow if allocation too large?
    ARENA_FTHROW        = 0x4, // Use error handler on malloc error?
    ARENA_FALIGN        = 0x8, // Align the returned pointers?
    ARENA_FSMARTREALLOC = 0x10, // Try to find last allocation when realloc'ing?
    ARENA_FDEFAULT      = (ARENA_FZEROINIT | ARENA_FGROW | ARENA_FTHROW 
                          | ARENA_FALIGN   | ARENA_FSMARTREALLOC),
};

#define BITFLAG_ON(n, flag)     ((n) & (flag))
#define BITFLAG_OFF(n, flag)    ((n) & (flag) ^ (flag))

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
