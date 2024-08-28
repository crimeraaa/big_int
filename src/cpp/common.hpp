#pragma once

#ifdef DEBUG_USE_ASSERT
#include <cassert>
#else  // DEBUG_USE_ASSERT not defined.
#define assert(expr)
#endif // DEBUG_USE_ASSERT
#include <cstddef>
#include <cstdint>

// Easier to grep
#define cast(T)         (T)
#define unused(expr)    cast(void)expr

using  i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using  u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using f32 = float;
using f64 = double;

using isize = std::ptrdiff_t;
using usize = std::size_t;
using byte  = u8;

#define size_of(T)      cast(isize)sizeof(T)
#define align_of(T)     alignof(T)
#define offset_of(T, M) offsetof(T, M)
#define count_of(a)     (size_of((a)) / size_of((a)[0]))
