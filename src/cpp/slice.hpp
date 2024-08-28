#pragma once

#include "cpp_odin.hpp"

/**
 * @brief
 *      A slice is simply a typed pointer with an associated length.
 *      You can index it and compare it against other slices like you would a
 *      normal raw C pointer.
 * 
 * @note
 *      The data being pointed to is not necessarily immutable.
 */
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

template<class T>
isize slice_len(const Slice<T> &self)
{
    return self.length;
}

template<class T>
isize slice_len(const Slice<T> *self)
{
    return self->length;
}

///--- GLOBAL UTILITY ----------------------------------------------------- {{{1

template<class T>
isize len(const Slice<T> &self)
{
    return slice_len(self);
}

template<class T>
isize len(const Slice<T> *self)
{
    return slice_len(self);
}

template<typename T>
bool operator==(const Slice<T> &lhs, const Slice<T> &rhs)
{
    isize len_lhs = lhs.length;
    isize len_rhs = rhs.length;
    if (len_lhs != len_rhs) {
        return false;
    }
    for (isize i = 0; i < len_lhs; i++) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

template<typename T>
bool operator!=(const Slice<T> &lhs, const Slice<T> &rhs)
{
    return !operator==(lhs, rhs);
}

///--- 1}}} --------------------------------------------------------------------
