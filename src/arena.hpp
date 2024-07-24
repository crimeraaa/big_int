#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include "common.hpp"

#define ARENA_FMTSTR        "Arena(active = %zu, capacity = %zu, buffer = %p)"
#define ARENA_FMTSTRLN      ARENA_FMTSTR "\n"
#define ARENA_FMTARGS(a)    (a)->active, (a)->capacity, static_cast<const void*>((a)->buffer)
#define ARENA_ALIGNMENT     alignof(Arena::Alignment)

using size_t = std::size_t;
using byte = std::uint8_t;

struct Arena {
    // This struct is simply used to calculate your machine's maximum alignment.
    // See: https://www.lua.org/source/5.1/luaconf.h.html#LUAI_USER_ALIGNMENT_T
    union Alignment {double d; long l; void* p;};

    Arena* next;      // Chain pages of allocations together.
    size_t active;
    size_t capacity;
    byte   buffer[1]; // Pseudo flexible array for compatibility with C89.
};

