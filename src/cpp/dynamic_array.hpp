#pragma once

#include "common.hpp"
#include "allocator.hpp"

/**
 * @brief
 *      Wraps a fat pointer with an allocator and the number of currently used
 *      elements.
 *
 * @tparam T
 *      Datatype for the underlying `Pointer<T>`.
 */
template<class T>
struct Dynamic_Array {

    using value_type      = T;
    using reference       = value_type&;
    using iterator        = value_type*;
    using const_reference = const value_type&;
    using const_iterator  = const value_type*;

    Allocator  allocator;
    Pointer<T> data;
    isize      active;
    
    reference operator[](isize index)
    {
        assert(0 <= index && index < this->active);
        return this->data[index];
    }

    iterator begin() noexcept
    {
        return this->data.begin();
    }

    iterator end() noexcept
    {
        return begin() + this->active;
    }
    
    const_reference operator[](isize index) const
    {
        assert(0 <= index && index < this->active);
        return this->data[index];
    }

    const_iterator begin() const noexcept
    {
        return this->data.begin();
    }

    const_iterator end() const noexcept
    {
        return begin() + this->active;
    }
};

/**
 * @brief
 *      Create a dynamic array with no allocated elements. It is simply
 *      initialized with the given allocator for later on.
 */
template<class T>
Dynamic_Array<T> dynamic_array_make(const Allocator& allocator)
{
    return dynamic_array_make<T>(0, allocator);
}

/**
 * @brief
 *      Create a dynamic array with `n_len` elements allocated and ready to
 *      index.
 * 
 * @note
 *      Calling `append()` will write at the `n_len` index, not the 0th index.
 */
template<class T>
Dynamic_Array<T> dynamic_array_make(isize n_len, const Allocator& allocator)
{
    return dynamic_array_make<T>(n_len, n_len, allocator);
}

/**
 * @brief
 *      Create a dynamic array with `n_cap` elements allocated and `n_len`
 *      elements ready to be indexed.
 * 
 * @note
 *      `n_cap` is intended purely for the backing memory, but `n_len` acts on
 *      a more conceptual level.
 * 
 *      That is, even if you have 8 elements allocated, if len is currently 4,
 *      attempting to access indexing 7 will cause an assertion failure.
 */
template<class T>
Dynamic_Array<T> dynamic_array_make(isize n_len, isize n_cap, const Allocator& allocator)
{
    Dynamic_Array<T> self;
    self.allocator = allocator;
    self.data      = pointer_new<T>(n_cap, allocator);
    self.active    = n_len;
    return self;
}

template<class T>
isize len(const Dynamic_Array<T>& self)
{
    return self.active;
}

template<class T>
isize len(const Dynamic_Array<T>* self)
{
    return self->active;
}

template<class T>
isize cap(const Dynamic_Array<T>& self)
{
    return len(self.data);
}

template<class T>
isize cap(const Dynamic_Array<T>* self)
{
    return len(self->data);
}

template<class T>
void append(Dynamic_Array<T>* self, const T& value)
{
    // Backing buffer needs resizing?
    isize n_cap = cap(self);
    if (self->active >= n_cap) {
        resize(self, n_cap == 0 ? 8 : n_cap * 2);
    }
    self->data[self->active] = value;
    self->active            += 1;
}

template<class T>
T pop(Dynamic_Array<T>* self)
{
    isize index = self->active - 1;
    assert(0 <= index && index < self->active);
    T value      = self->data[index];
    self->active = index;
    return value;
}

// Only grows, does not shrink.
template<class T>
void resize(Dynamic_Array<T>* self, isize n_len)
{
    // Do nothing if we're not trying to grow
    if (n_len <= cap(self)) {
        return;
    }
    pointer_resize(&self->data, n_len, self->allocator);
}

// Can't use `delete` as an identifier so `destroy` will have to do.
template<class T>
void destroy(Dynamic_Array<T>* self)
{
    pointer_free(&self->data, self->allocator);
    self->active = 0;
}

// Sets `active` to 0 but does not actually free the backing memory.
template<class T>
void clear(Dynamic_Array<T>* self)
{
    self->active = 0;
}
