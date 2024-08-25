#pragma once

#include "common.hpp"


// A fat pointer wraps a raw pointer with its valid number of elements.
template<class T>
struct Pointer {
    
    using pointer   = T*;
    using reference = T&;
    using iterator  = T*;
    
    using const_pointer   = const T*;
    using const_reference = const T&;
    using const_iterator  = const T*;

    pointer start;
    isize   length; // How many elements can be validly dereferenced.
    
    const_reference operator[](isize index) const
    {
        assert(0 <= index && index < length);
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
    
    const_iterator cbegin() const
    {
        return this->start;
    }
    
    const_iterator cend() const
    {
        return this->start + this->length;
    }
    
    iterator begin() const
    {
        return cbegin();
    }
    
    iterator end() const
    {
        return cend();
    }
};

template<class T>
isize len(const Pointer<T>& self) { return self.length; }

template<class T>
isize len(const Pointer<T>* self) { return self->length; }
