#pragma once

#ifdef DEBUG_USE_ASSERT
#include <cassert>
#else  // !DEBUG_USE_ASSERT
#define assert(Expr)
#endif // DEBUG_USE_ASSERT

#include <cstddef>
#include <cstdint>
#include <type_traits>

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

using rawptr = void *;

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
    using iterator        = pointer;
    
    using const_pointer   = const value_type *;
    using const_reference = const value_type &;
    using const_iterator  = const_pointer;

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

/**
 * @brief
 *      A slice of void is just raw memory. Here, `len` refers to the number of
 *      bytes being pointed to.
 *
 * @note
 *      We explicitly specialize for void since references to void will result
 *      in a compile error.
 */
template<>
struct Slice<void> {
    
    using value_type    = void;
    using pointer       = value_type *;
    using const_pointer = const value_type *;

    pointer data;
    isize   len;
};

/**
 * @note
 *      Valid for `Pointer<T>` as well.
 */
template<class T>
isize len(const Slice<T> &self)
{
    return self.len;
}

template<class T>
isize len(const Slice<T> *self)
{
    return self->len;
}

/**
 * @brief
 *      Our Pointer is just, conceptually, a fat pointer.
 *      It wraps the `Slice` type by adding dereference operations.
 */
template<class T>
struct Pointer : public Slice<T> {
    using Base = Slice<T>;
    using typename Base::pointer;
    using typename Base::reference;
    using typename Base::const_pointer;
    using typename Base::const_reference;

    // read-write access index 0 (lvalue)
    reference operator*()
    {
        return Base::operator[](0);
    }

    pointer operator->()
    {
        return &Base::operator[](0);
    }
    
    // read-only access index 0 (lvalue)
    const_reference operator*() const
    {
        return Base::operator[](0);
    }

    const_pointer operator->() const
    {
        return &Base::operator[](0);
    }
};

/**
 * @note
 *      We explicitly specialize for void since one cannot have references to
 *      void, which will cause a compile error.
 */
template<>
struct Pointer<void> : public Slice<void>
{};

#include "allocator.hpp"
