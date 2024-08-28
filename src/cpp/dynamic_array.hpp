#pragma once

#include "cpp_odin.hpp"
#include "slice.hpp"

template<class T>
struct Dynamic_Array {
    
    using value_type      = T;
    using pointer         = value_type *;
    using reference       = value_type &;
    using const_pointer   = const value_type *;
    using const_reference = const value_type &;

    Allocator allocator;
    isize     length;
    isize     capacity;
    pointer   data;
    
    reference operator[](isize index)
    {
        #ifdef DEBUG_USE_ASSERT
            assert(0 <= index && index < this->length);
        #endif
        return this->data[index];
    }

    explicit operator pointer()
    {
        return this->data;
    }
    
    const_reference operator[](isize index) const
    {
        #ifdef DEBUG_USE_ASSERT
            assert(0 <= index && index < this->length);
        #endif
        return this->data[index];
    }

    explicit operator const_pointer() const
    {
        return this->data;
    }
};

template<class T>
Dynamic_Array<T> dynamic_array_make(Allocator a, isize len = 0, isize cap = 0)
{
    // Crappy check to somewhat mimic Odin's proc groups
    if (cap == 0) {
        cap = len;
    }
    return {a, len, cap, pointer_new<T>(a, cap)};
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
    if (new_len >= cap(self)) {
        reserve(self, new_len);
    }
    
    for (isize i = 0, limit = len(values); i < limit; i++) {
        append(self, values[i]);
    }
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
    using value_type = std::remove_const_t<T>;
    using pointer    = value_type *;
    return dynamic_array_append(self, {cast(pointer)values.data, len(values)});
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
