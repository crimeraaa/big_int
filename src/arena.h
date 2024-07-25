#pragma once

#include "common.h"

#define ARENA_FMTSTR        "Arena(active=%zu, capacity=%zu, buffer=%p)"
#define ARENA_FMTSTRLN      ARENA_FMTSTR "\n"
#define ARENA_FMTARGS(a)    (a)->active, (a)->capacity, cast(const void*, (a)->buffer)
#define ARENA_DEFAULTSIZE   (1024 * 4)

// https://en.wikipedia.org/wiki/Data_structure_alignment#Computing_padding
#define ARENA_GETPADDING(align, offset)                                        \
(                                                                              \
    ( (align) - ( (offset) % (align) ) ) % (align)                             \
)

typedef struct Arena Arena;
typedef void (*ErrorFn)(const char *msg, Size sz, void *ctx);

#define ARENA_NOZERO    0x1
#define ARENA_NOTHROW   0x2
#define ARENA_NODEFAULT (ARENA_NOZERO | ARENA_NOTHROW)
#define ARENA_DEFAULT   0x0

struct Arena {
    Arena  *next;
    uint8_t flags;
    ErrorFn handler;
    void   *context;  // The sole argument to `handler`.
    Size    active;   // Number of `Byte` currently being used.
    Size    capacity; // Number of `Byte` we allocated in total.
    Byte    buffer[]; // Flexible `Byte` array.
};

Arena  *arena_new(Size cap, uint8_t flags, ErrorFn fn, void *ctx);
void    arena_set_flags(Arena *self, uint8_t flags);
void    arena_clear_flags(Arena *self, uint8_t flags);
void    arena_reset(Arena *self);
void    arena_free(Arena **self);
void    arena_print(const Arena *self);
void   *arena_alloc(Arena *self, Size sz, Size align);
void    arena_main(const LString args[], Size count, Arena *arena);
#define arena_new_default(cap)  arena_new(cap, ARENA_DEFAULT, nullptr, nullptr)

/* 
NOTE: ... = self [, len [, align [, extra] ] ]
arena_xalloc(T, ...)
    - arena_xalloc(char, self)
        - arena__xselect(a   = self,
                        b   = xmalloc3,
                        c   = xmalloc2,
                        d   = xmalloc1,
                        e   = xmalloc0,
                        ... = nullptr)
        - xmalloc0(T, self)
    - arena_xalloc(char, self, 1)
        - arena__xselect(a   = self,
                        b   = 1,
                        c   = xmalloc3,
                        d   = xmalloc2,
                        e   = xmalloc1,
                        ... = xmalloc0, nullptr)
        - xmalloc1(T, self, 1)
    - arena_xalloc(char, self, 1, 2)
        - arena__xselect(a   = self,
                        b   = 1,
                        c   = 2,
                        d   = xmalloc3,
                        e   = xmalloc2,
                        ... = xmalloc1, xmalloc0, nullptr)
        - xmalloc2(T, char, self, 1, 2)
    - arena_xalloc(char, arena, 1, 2, 3)
        - arena__xselect(a   = self,
                        b   = 1,
                        c   = 2,
                        d   = 3,
                        e   = xmalloc3,
                        ... = xmalloc2, xmalloc1, xmalloc0, 0)
        - xmalloc3(char, arena, len=1, align=2, extra=3)
    - arena_xalloc(char, arena, 1, 2, 3, 4)
        - arena__xselect(a   = self,
                        b   = 1,
                        c   = 2,
                        d   = 3,
                        e   = 4,
                        ... = xmalloc3, xmalloc2, xmalloc1, xmalloc0, 0)
        - 4(char, arena, len=1, align=2, extra=3)
        - should error!
    - arena_xalloc(char, arena, 1, 2, 3, 4, 5)
        - arena__xselect(a   = self,
                        b   = 1,
                        c   = 2,
                        d   = 3,
                        e   = 4,
                        ... = 5, xmalloc3, xmalloc2, xmalloc1, xmalloc0, 0)
        - calls `4`, same as directly above.
 */
#define arena__xselect(a, b, c, d, e, ...)   e
#define arena__xalloc3(T, self, len, align, extra) \
    cast(T*, arena_alloc(self, sizeof(T) * (len) + (extra), (align)))
#define arena__xalloc2(T, self, len, align) arena__xalloc3(T, self, len, align, 0)
#define arena__xalloc1(T, self, len)        arena__xalloc2(T, self, len, alignof(T))
#define arena__xalloc0(T, self)             arena__xalloc1(T, self, 1)

/*
User-facing "convenience" (lol) macro.

`...` = `self [, len [, align [, extra ] ] ]`

Form 1: all default arguments
    `arena_alloc(T, self)`
    Allocate a single `T`, 0 extra space, `alignof(T)` alignment.

Form 2: 1 custom argument (`len`)
    `arena_alloc(T, self, len)`
    Allocate `len` amount of `T`, 0 extra space, using `alignof(T)` alignment.

Form 3: 2 custom arguments (`len`, `align`)
    `arena_alloc(T, self, len, align)`
    Allocate `len` amount of `T`, no extra space, using `align` alignment.

Form 4: 3 custom arguments (`len`, `align`, `extra`)
    `arena_alloc(T, self, len, align, extra)`
    Allocate `len` amount of `T` with `extra` extra space and `align` alignment.
    
Any other form should cause a compilation error.
*/
#define arena_xalloc(T, ...)                                                   \
    arena__xselect(                                                            \
        __VA_ARGS__,                                                           \
        arena__xalloc3,                                                        \
        arena__xalloc2,                                                        \
        arena__xalloc1,                                                        \
        arena__xalloc0,                                                        \
        0                                                                      \
    )(T, __VA_ARGS__)
