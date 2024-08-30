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

using cstring = const char *; // https://odin-lang.org/docs/overview/#cstring-type
using rawptr  =       void *; // https://odin-lang.org/docs/overview/#basic-types

#define unused(Expr)                static_cast<void>(Expr)
#define size_of(Expr)               static_cast<isize>(sizeof(Expr))
#define align_of(Expr)              static_cast<isize>(alignof(Expr))
#define offset_of(Struct, Member)   static_cast<isize>(offsetof(Struct, Member))

namespace odin {

template<class T>
struct Type_Traits {
    using value_type      = T;
    using pointer         =       value_type *;
    using reference       =       value_type &;
    using iterator        =       pointer;
    using const_pointer   = const value_type *;
    using const_reference = const value_type &;
    using const_iterator  = const_pointer;
};

template<class T>
struct No_Pointer_Arith_Traits {

    using Traits          = Type_Traits<T>;
    using pointer         = typename Traits::pointer;
    using reference       = typename Traits::reference;
    using const_pointer   = typename Traits::const_pointer;
    using const_reference = typename Traits::const_reference;
    
    reference       operator* ()                  = delete;
    const_reference operator* ()            const = delete;
    pointer         operator->()                  = delete;
    const_pointer   operator->()            const = delete;
    reference       operator[](isize index)       = delete;
    const_reference operator[](isize index) const = delete;
    reference       operator++()                  = delete; // prefix increment.
    reference       operator--()                  = delete; // prefix decrement.
    reference       operator++(int)               = delete; // postfix increment.
    reference       operator--(int)               = delete; // postfix decrement.
    
    template<class Q = T> reference operator+ (const Q &rhs) = delete;
    template<class Q = T> reference operator- (const Q &rhs) = delete;
    template<class Q = T> reference operator+=(const Q &rhs) = delete;
    template<class Q = T> reference operator-=(const Q &rhs) = delete;

};

/**
 * @brief
 *      Slices wrap typed C pointers with the number of elements that can be
 *      validly accessed. They are fixed in size.
 * 
 * @note
 *      Dereferencing is disallowed because conceptually this is not a pointer
 *      to a single item. Use `Pointer<T>` for that.
 */
template<class T>
struct Slice : public No_Pointer_Arith_Traits<T> {
    
    using Traits          = Type_Traits<T>;
    using pointer         = typename Traits::pointer;
    using reference       = typename Traits::reference;
    using iterator        = typename Traits::iterator;
    using const_pointer   = typename Traits::const_pointer;
    using const_reference = typename Traits::const_reference;
    using const_iterator  = typename Traits::const_iterator;
    
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
    
    // read-only access (rvalue)
    const_reference operator[](isize index) const
    {
#ifdef DEBUG_USE_ASSERT
        assert(0 <= index && index < this->len);
#endif
        return this->data[index];
    }
    
    iterator       begin()       noexcept { return this->data; }
    iterator       end()         noexcept { return this->data + this->len; }
    const_iterator begin() const noexcept { return this->data; }
    const_iterator end()   const noexcept { return this->data + this->len; }
    
    bool operator==(const Slice<T> &rhs) const noexcept
    {
        const Slice<T> &lhs = *this;
        if (len(lhs) != len(rhs)) {
            return false;
        }
        
        for (isize i = 0; i < len(lhs); i++) {
            if (lhs[i] != rhs[i]) {
                return false;
            }            
        }
        return true;
    }
};

template<class T>
isize len(const Slice<T> &self) { return self.len; }

template<class T>
isize len(const Slice<T> *self) { return self->len; }

/**
 * @brief
 *      Wrap a raw C pointer to a single item with methods to ensure correct
 *      usage.
 *
 * @note
 *      You are only allowed to dereference the pointer, not subscript it as if
 *      it were an array. Use `Slice<T>` for subscripting.
 */
template<class T>
struct Pointer : public No_Pointer_Arith_Traits<T> {
    
    using Traits          = Type_Traits<T>;
    using pointer         = typename Traits::pointer;
    using reference       = typename Traits::reference;
    using const_pointer   = typename Traits::const_pointer;
    using const_reference = typename Traits::const_reference;

    pointer data;
    
    reference       operator* ()       noexcept { return *this->data; }
    const_reference operator* () const noexcept { return *this->data; }
    pointer         operator->()       noexcept { return  this->data; }
    const_pointer   operator->() const noexcept { return  this->data; }
};

}; // namespace odin

#include "mem.hpp"
