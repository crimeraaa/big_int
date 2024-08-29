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
#define unused(expr)    cast(void)(expr)

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

/**
 * @brief
 *      Basic_Array provides a uniform interface for array-like data structures
 *      to wrap raw C pointers with a runtime length.
 *
 *      It provides the bare operations needed to index into the underlying
 *      data, with bounds checking.
 */
template<class T>
struct Basic_Array {

    using value_type      = T;
    using pointer         = value_type *;
    using reference       = value_type &;
    using iterator        = pointer;
    using const_pointer   = const value_type *;
    using const_reference = const value_type &;
    using const_iterator  = const_pointer;

    pointer data;
    isize   length;
    
    reference operator[](isize index)
    {
        #ifdef DEBUG_USE_ASSERT
            assert(0 <= index && index < this->length);
        #endif
        return this->data[index];
    }
    
    explicit operator pointer() noexcept
    {
        return this->data;
    }
    
    iterator begin() noexcept
    {
        return this->data;
    }
    
    iterator end() noexcept
    {
        return this->data + this->length;
    }

    const_reference operator[](isize index) const
    {
        #ifdef DEBUG_USE_ASSERT
            assert(0 <= index && index < this->length);
        #endif
        return this->data[index];
    }

    explicit operator const_pointer() const noexcept
    {
        return this->data;
    }
    
    const_iterator begin() const noexcept
    {
        return this->data;
    }
    
    const_iterator end() const noexcept
    {
        return this->data + this->length;
    }

};

#include "allocator.hpp"
#include "dynamic_array.hpp"
#include "slice.hpp"
#include "strings.hpp"

