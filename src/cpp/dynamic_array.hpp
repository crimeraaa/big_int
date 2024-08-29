#pragma once

#include "cpp_odin.hpp"
#include "slice.hpp"

/**
 * @note
 *      Order of members, due to single inheritance, are:
 *
 *      Basic_Array<T>::data
 *      Basic_Array<T>::length
 *      Dynamic_Array<T>::allocator
 *      Dynamic_Array<T>::capacity
 */
template<class T>
struct Dynamic_Array : public Basic_Array<T> {
    
    Allocator allocator;
    isize     capacity;
    
};

template<class T>
Dynamic_Array<T> dynamic_array_make(Allocator a, isize len = 0, isize cap = 0)
{
    // Crappy check to somewhat mimic Odin's proc groups
    if (cap == 0) {
        cap = len;
    }
    Dynamic_Array<T> self;
    self.data      = pointer_new<T>(a, cap);
    self.length    = len;
    self.allocator = a;
    self.capacity  = cap;
    return self;
}

template<class T>
void dynamic_array_destroy(Dynamic_Array<T> *self)
{
    pointer_free(self->allocator, self->data, self->capacity);
    self->data     = nullptr;
    self->length   = 0;
    self->capacity = 0;
}

template<class T>
void dynamic_array_clear(Dynamic_Array<T> *self)
{
    self->length = 0;
}

template<class T>
void dynamic_array_reserve(Dynamic_Array<T> *self, isize new_cap)
{
    T    *old_ptr = self->data;
    isize old_cap = cap(self);
    if (new_cap <= old_cap) {
        return;
    }
    self->data     = pointer_resize(self->allocator, new_cap, old_ptr, old_cap);
    self->capacity = new_cap;
}

template<class T>
isize dynamic_array_append(Dynamic_Array<T> *self, const T &value)
{
    if (len(self) >= cap(self)) {
        reserve(self, len(self) + 1);
    }
    self->data[self->length++] = value;
    return 1;
}

template<class T>
isize dynamic_array_append(Dynamic_Array<T> *self, const Slice<T> &values)
{
    isize old_len = len(self);
    isize new_len = old_len + len(values);
    if (new_len > cap(self)) {
        reserve(self, new_len);
    }
    
    for (isize i = 0, limit = len(values); i < limit; i++) {
        self->data[old_len + i] = values[i];
    }
    self->length = new_len;
    return len(values);
}

template<class T>
T dynamic_array_pop(Dynamic_Array<T> *self)
{
    isize index = len(self) - 1;
#ifdef DEBUG_USE_ASSERT
    assert(0 <= index && index < len(self));
#endif
    self->length = index;
    return self->data[index];
}

template<class T>
isize dynamic_array_len(const Dynamic_Array<T> &self)
{
    return self.length;
}

template<class T>
isize dynamic_array_len(const Dynamic_Array<T> *self)
{
    return self->length;
}

template<class T>
isize dynamic_array_cap(const Dynamic_Array<T> &self)
{
    return self.capacity;
}

template<class T>
isize dynamic_array_cap(const Dynamic_Array<T> *self)
{
    return self->capacity;
}

///--- GLOBAL UTILITY ----------------------------------------------------- {{{1

template<class T>
void destroy(Dynamic_Array<T> *self)
{
    dynamic_array_destroy(self);
}

template<class T>
void clear(Dynamic_Array<T> *self)
{
    dynamic_array_clear(self);
}

template<class T>
void reserve(Dynamic_Array<T> *self, isize new_cap)
{
    dynamic_array_reserve(self, new_cap);
}

template<class T>
isize append(Dynamic_Array<T> *self, const T &value)
{
    return dynamic_array_append(self, value);
}

template<class T>
isize append(Dynamic_Array<T> *self, const Slice<T> &values)
{
    return dynamic_array_append(self, values);
}

// Hack to allow slices pointing to const
template<class T>
isize append(Dynamic_Array<T> *self, const Slice<const T> &values)
{
    return dynamic_array_append(self, slice_cast<T>(values));
}

template<class T>
T pop(Dynamic_Array<T> *self)
{
    return dynamic_array_pop(self);
}

template<class T>
isize len(const Dynamic_Array<T> &self)
{
    return dynamic_array_len(self);
}

template<class T>
isize len(const Dynamic_Array<T> *self)
{
    return dynamic_array_len(self);
}

template<class T>
isize cap(const Dynamic_Array<T> &self)
{
    return dynamic_array_cap(self);
}

template<class T>
isize cap(const Dynamic_Array<T> *self)
{
    return dynamic_array_cap(self);
}

///--- 1}}} --------------------------------------------------------------------
