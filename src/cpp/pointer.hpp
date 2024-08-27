#pragma once

#include "common.hpp"

/**
 * @brief
 *      A fat pointer. Wraps a raw pointer with the number of elements that are
 *      being pointed to.
 *      
 * @tparam T
 *      The datatype to be pointed at.
 */
template<class T>
struct Pointer {
    using value_type      = T;
    using pointer         = value_type*;
    using reference       = value_type&;
    using iterator        = value_type*;
    using const_pointer   = const value_type*;
    using const_reference = const value_type&;
    using const_iterator  = const value_type*;

    pointer start;  // Address of the first element.
    isize   length; // How many elements can be validly dereferenced.

    reference operator[](isize index)
    {
        assert(0 <= index && index < length);
        return this->start[index];
    }
    
    reference operator*()
    {
        return operator[](0);
    }

    pointer operator->()
    {
        return &operator[](0);
    }
    
    iterator begin() noexcept
    {
        return this->start;
    }
    
    iterator end() noexcept
    {
        return this->start + this->length;
    }

    const_reference operator[](isize index) const
    {
        assert(0 <= index && index < this->length);
        return this->start[index];
    }
    
    const_reference operator*() const
    {
        return operator[](0);
    }
    
    const_pointer operator->() const
    {
        return &operator[](0);
    }
    
    const_iterator begin() const noexcept
    {
        return this->start;
    }
    
    const_iterator end() const noexcept
    {
        return this->start + this->length;
    }
    
};

template<class T>
isize len(const Pointer<T>& self)
{
    return self.length;
}

template<class T>
isize len(const Pointer<T>* self)
{
    return self->length;
}
