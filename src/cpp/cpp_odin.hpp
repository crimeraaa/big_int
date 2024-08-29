#pragma once

#ifdef DEBUG_USE_ASSERT
#include <cassert>
#else  // !DEBUG_USE_ASSERT
#define assert(Expr)
#endif // DEBUG_USE_ASSERT

#include <cstddef>
#include <cstdint>

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

using byte  = u8;
using usize = std::size_t;
using isize = std::ptrdiff_t;

// Easier to grep
#define cast(Type)                  (Type)
#define unused(Expr)                cast(void)(Expr)
#define size_of(Expr)               cast(isize)(sizeof(Expr))
#define align_of(Expr)              cast(isize)(alignof(Expr))
#define offset_of(Struct, Member)   cast(isize)(offsetof(Struct, Member))

template<class T>
struct Slice {
    using value_type      = T;
    using pointer         = value_type *;
    using reference       = value_type &;
    using const_pointer   = const value_type *;
    using const_reference = const value_type &;

    pointer data;
    isize   len;
    
    // read-write access (lvalue)
    reference operator[](isize index)
    {
#ifdef DEBUG_USE_ASSERT
        assert(0 <= index && index < this->len);
#endif
        return this->data[index];
    }
    
    // Explicit conversion: `(T *)(<inst>)`
    explicit operator pointer() noexcept
    {
        return this->data;
    }
    
    // read-only access (rvalue)
    const_reference operator[](isize index) const
    {
#ifdef DEBUG_USE_ASSERT
        assert(0 <= index && index < this->len);
#endif
        return this->data[index];
    }
    
    // Explicit conversion: `(const T *)(<inst>)`
    explicit operator const_pointer() const noexcept
    {
        return this->data;
    }
};

template<class T>
struct Pointer : public Slice<T> {
    
    using Base = Slice<T>;

    // read-write access index 0 (lvalue)
    Base::reference operator*()
    {
        return Base::operator[](0);
    }

    Base::pointer operator->()
    {
        return &Base::operator[](0);
    }
    
    // read-only access index 0 (lvalue)
    Base::const_reference operator*() const
    {
        return Base::operator[](0);
    }

    Base::const_pointer operator->() const
    {
        return &Base::operator[](0);
    }
};

#include "allocator.hpp"
