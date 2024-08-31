#pragma once

#ifdef DEBUG_USE_ASSERT
    #include <cassert>
#else // DEBUG_USE_ASSERT
    #define assert(expr)
#endif // DEBUG_USE_ASSERT

#include <cstddef>
#include <cstdint>

// Uncomment this if you want array/slice indexing operations to not check.
// #define NO_BOUNDS_CHECK

using i8      = std::int8_t;
using i16     = std::int16_t;
using i32     = std::int32_t;
using i64     = std::int64_t;
using u8      = std::uint8_t;
using u16     = std::uint16_t;
using u32     = std::uint32_t;
using u64     = std::uint64_t;
using byte    = u8;
using isize   = std::ptrdiff_t;
using usize   = std::size_t;
using cstring = const char *;
using rawptr  =       void *;

#define unused(Expr)            static_cast<void>(Expr)
#define size_of(Expr)           static_cast<isize>(sizeof(Expr))
#define align_of(Type)          static_cast<isize>(alignof(Type))
#define offset_of(Struct, Memb) static_cast<isize>(offsetof(Struct, Memb))

// TODO(2020-08-31): Fix recursive include issue
// #include "mem.hpp"
// #include "array.hpp"
