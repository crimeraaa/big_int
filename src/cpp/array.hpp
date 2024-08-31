#pragma once

#include "odin.hpp"
#include "mem.hpp"

/**
 * @brief
 *      A slice is a fixed-size view into some memory. It may be mutable.
 *
 * @warning
 *      If this points to heap-allocated memory, it may not necessarily view
 *      the entire allocation. Do not attempt to free it in that case.
 */
template<class T>
struct Slice {
    T    *data;
    isize len;
    
    /**
     * @brief
     *      Read-write access. Returns an L-value.
     */
    T &operator[](isize index)
    {
        #if !defined(NO_BOUNDS_CHECK)
            assert(0 <= index && index < this->len);
        #endif
        return this->data[index];
    }
    
    /**
     * @brief
     *      Read-only access. Returns an R-value.
     */
    const T &operator[](isize index) const
    {
        #if !defined(NO_BOUNDS_CHECK)
            assert(0 <= index && index < this->len);
        #endif
        return this->data[index];
    }
};

template<class T>
Slice<T> slice(T *ptr, isize count, isize start, isize stop)
{
    assert(0 <= start && start <= stop && stop <= count);
    Slice<T> out{nullptr, 0};
    isize    len = stop - start;
    if (len > 0) {
        out.data = ptr + start;
        out.len  = stop;
    }
    return out;
}

template<class T>
Slice<T> slice(const Slice<T> &self, isize start, isize stop)
{
    return slice(begin(self), len(self), start, stop);
}

template<class T>
Slice<const T> slice_const(const Slice<T> &self, isize start, isize stop)
{
    return slice<const T>(cbegin(self), len(self), start, stop);
}

template<class T>
Slice<const T> slice_const_cast(const Slice<T> &self)
{
    return {cbegin(self), len(self)};
}

template<class T>
Slice<T> slice_make(const Allocator &a, isize len)
{
    return {multi_ptr_new<T>(a, len), len};
}

template<class T>
void slice_free(Slice<T> *self, const Allocator &a)
{
    mem_free(a, self->data, size_of(T) * self->len);
    self->data = nullptr;
    self->len  = 0;
}

/**
 * @brief
 *      A dynamic array akin to C++ `std::vector`. It remembers its allocator.
 */
template<class T>
struct Array {
    Allocator allocator;
    T *       data;
    isize     len;
    isize     cap;

    /**
     * @brief
     *      Read-write access. Returns an L-value.
     */
    T &operator[](isize index)
    {
        #if !defined(NO_BOUNDS_CHECK)
            assert(0 <= index && index < this->len);
        #endif
        return this->data[index];
    }
    
    /**
     * @brief
     *      Read-only access. Returns an R-value.
     */
    const T &operator[](isize index) const
    {
        #if !defined(NO_BOUNDS_CHECK)
            assert(0 <= index && index < this->len);
        #endif
        return this->data[index];
    }
};

template<class T>
Slice<T> slice(const Array<T> &self, isize start, isize stop)
{
    return slice(begin(self), len(self), start, stop);
}

template<class T>
Slice<const T> slice_const(const Array<T> &self, isize start, isize stop)
{
    return slice<const T>(cbegin(self), len(self), start, stop);
}

template<class T>
void array_init(Array<T> *self, const Allocator &a)
{
    array_init(self, a, 0, 0);
}

template<class T>
void array_init(Array<T> *self, const Allocator &a, isize len)
{
    array_init(self, a, len, len);
}

template<class T>
void array_init(Array<T> *self, const Allocator &a, isize len, isize cap)
{
    self->allocator = a;
    if (cap > 0) {
        self->data = multi_ptr_new<T>(a, cap);
    } else {
        self->data = nullptr;
    }
    self->len = len;
    self->cap = cap;
}

template<class T>
Array<T> array_make(const Allocator &a)
{
    return array_make(a, 0, 0);
}

template<class T>
Array<T> array_make(const Allocator &a, isize len)
{
    return array_make(a, len, len);
}

template<class T>
Array<T> array_make(const Allocator &a, isize len, isize cap)
{
    Array<T> out;
    array_init(&out, a, len, cap);
    return out;
}

template<class T>
void array_resize(Array<T> *self, isize new_len)
{
    // Need to grow?
    if (cap(self) < new_len) {
        array_grow(self);
    }
    self->len = new_len;
}

template<class T>
void array_grow(Array<T> *self)
{
    array_reserve(self, (self->cap < 8) ? 8 : self->cap * 2);
}

template<class T>
void array_reserve(Array<T> *self, isize new_cap)
{
    isize old_cap = cap(self);
    // Nothing to do
    if (new_cap == old_cap) {
        return;
    }
    
    T *   old_data = self->data;
    isize old_size = size_of(T) * old_cap;
    isize new_size = size_of(T) * new_cap;
    
    self->data = static_cast<T *>(mem_resize(self->allocator, old_data, old_size, new_size, align_of(T)));
    self->cap  = new_cap;
}

template<class T>
void array_free(Array<T> *self)
{
    mem_free(self->allocator, self->data, size_of(T) * self->cap);
    self->data = nullptr;
    self->len  = self->cap = 0;
}

template<class T>
void array_append(Array<T> *self, const T &value)
{
    if (len(self) >= cap(self)) {
        array_grow(self);
    }
    self->data[self->len++] = value;
}

template<class T>
void array_append(Array<T> *self, const Slice<const T> &values)
{
    isize old_len = len(self);
    isize new_len = old_len + len(values);
    if (new_len > cap(self)) {
        // Get next power of 2
        isize n = 1;
        while (n < new_len) {
            n *= 2;
        }
        new_len = n;
        array_reserve(self, new_len);
    }

    for (isize i = 0; i < len(values); i++) {
        self->data[old_len + i] = values[i];
    }
    self->len = new_len;
}

// Promotes `Slice<T>` to a `Slice<const T>` to call the above signature.
template<class T>
void array_append(Array<T> *self, const Slice<T> &values)
{
    array_append(self, {cbegin(values), len(values)});
}

template<class T>
T array_pop(Array<T> *self)
{
    // Can't pop if nothing to pop!
    assert(self->len != 0);
    isize index = self->len - 1;
    self->len   = index;
    return self->data[index];
}

///--- GLOBAL UTILITY ----------------------------------------------------- {{{1

///--- REFERENCE ---------------------------------------------------------- {{{2

template<class T>
isize len(const Slice<T> &self)
{
    return self.len;
}

template<class T>
isize len(const Array<T> &self)
{
    return self.len;
}

template<class T>
isize cap(const Array<T> &self)
{
    return self.cap;
}

///--- ITERATORS ---------------------------------------------------------- {{{3

template<class T>
T *begin(const Array<T> &self)
{
    return self.data;
}

template<class T>
T *end(const Array<T> &self)
{
    return self.data + self.len;
}

template<class T>
const T *cbegin(const Array<T> &self)
{
    return self.data;
}

template<class T>
const T *cend(const Array<T> &self)
{
    return self.data + self.len;
}

template<class T>
T *begin(const Slice<T> &self)
{
    return self.data;
}

template<class T>
T *end(const Slice<T> &self)
{
    return self.data + self.len;
}

template<class T>
const T *cbegin(const Slice<T> &self)
{
    return self.data;
}

template<class T>
const T *cend(const Slice<T> &self)
{
    return self.data + self.len;
}

///--- 3}}} --------------------------------------------------------------------

///--- 2}}} --------------------------------------------------------------------

///--- POINTER ------------------------------------------------------------ {{{2

template<class T>
isize len(const Slice<T> *self)
{
    return self->len;
}

template<class T>
isize len(const Array<T> *self)
{
    return self->len;
}

template<class T>
isize cap(const Array<T> *self)
{
    return self->cap;
}

///--- ITERATORS ---------------------------------------------------------- {{{3

template<class T>
T *begin(const Array<T> *self)
{
    return self->data;
}

template<class T>
T *end(const Array<T> *self)
{
    return self->data + self->len;
}

template<class T>
const T *cbegin(const Array<T> *self)
{
    return self->data;
}

template<class T>
const T *cend(const Array<T> *self)
{
    return self->data + self->len;
}

template<class T>
T *begin(const Slice<T> *self)
{
    return self->data;
}

template<class T>
T *end(const Slice<T> *self)
{
    return self->data + self->len;
}

template<class T>
const T *cbegin(const Slice<T> *self)
{
    return self->data;
}

template<class T>
const T *cend(const Slice<T> *self)
{
    return self->data + self->len;
}

///--- 3}}} --------------------------------------------------------------------

///--- 2}}} --------------------------------------------------------------------

///--- 1}}} --------------------------------------------------------------------
