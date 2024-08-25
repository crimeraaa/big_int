#pragma once

#ifdef DEBUG_USE_ASSERT
#include <cassert>
#else  // DEBUG_USE_ASSERT not defined.
#define assert(expr)
#endif // DEBUG_USE_ASSERT
#include <cstddef>
#include <cstdint>

#define unused(expr)    static_cast<void>(expr)

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

using ibyte = i8;
using isize = std::ptrdiff_t;

using ubyte = u8;
using usize = std::size_t;
