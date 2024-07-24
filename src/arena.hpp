#pragma once

#include <cstddef>
#include <cstdint>
#include "common.hpp"

#define ARENA_FMTSTR        "Arena(active=%zu, capacity=%zu, buffer=%p)"
#define ARENA_FMTSTRLN      ARENA_FMTSTR "\n"
#define ARENA_FMTARGS(a)    (a)->active, (a)->capacity, cast(const void*, (a)->buffer)
#define ARENA_DEFAULTSIZE   (1024 * 4)

// https://en.wikipedia.org/wiki/Data_structure_alignment#Computing_padding
#define ARENA_GETPADDING(align, offset)                                        \
(                                                                              \
    ( (align) - ( (offset) % (align) ) ) % (align)                             \
)

using Size = std::size_t;
using Byte = std::uint8_t;

struct Arena {
    Arena* next;      // Chain arenas together.
    Size   active;    // Numbers of bytes allocated so far, including padding.
    Size   capacity;  // Total length of `buffer`.
    Byte   buffer[1]; // Pseudo flexible array for compatibility with C89.
};

Arena*  arena_new(Size cap);
void    arena_reset(Arena* self);
void    arena_free(Arena*& self);
void    arena_print(const Arena* self);
void*   arena_alloc(Arena* self, Size sz, Size align);
void    arena_main(LString args[], size_t count, Arena* arena);
#define arena_alloc_n(T, self, len) arena_alloc_g(T, self, len, 0)
#define arena_alloc_1(T, self)      arena_alloc_n(T, self, 1)
// Semi-generic "interface" allowing you to pass in `extra`, say for flexarrays.
#define arena_alloc_g(T, self, len, extra) \
    cast(T*, arena_alloc(self, sizeof(T) * (len) + (extra), alignof(T)))

