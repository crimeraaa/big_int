#pragma once

#include "cpp_odin.hpp"

#include <cstdlib>
#include <cstdio>

struct Allocator {
    /**
     * @note
     *      Error handling is up to you.
     */
    using AllocFn = void *(*)(
        isize new_sz, isize align, void *old_ptr, isize old_sz, void *ctx
    );
    
    AllocFn alloc_fn;
    void *  context;
};

void *raw_allocate  (Allocator a, isize new_sz, isize align);
void *raw_reallocate(Allocator a, isize new_sz, isize align, void *old_ptr, isize old_sz);
void  raw_deallocate(Allocator a, void *ptr, isize sz);

template<class T>
T *pointer_new(Allocator a, isize len)
{
    return cast(T *)raw_allocate(a, size_of(T) * len, align_of(T));
}

template<class T>
T *pointer_resize(Allocator a, isize new_len, T *old_ptr, isize old_len)
{
    isize new_sz = size_of(T) * new_len;
    isize old_sz = size_of(T) * old_len;
    return cast(T *)raw_reallocate(a, new_sz, align_of(T), old_ptr, old_sz);
}

template<class T>
void pointer_free(Allocator a, T *ptr, isize len)
{
    raw_deallocate(a, ptr, len);
}

#ifdef PSEUDO_ODIN_IMPLEMENTATION

void *raw_allocate(Allocator a, isize new_sz, isize align)
{
    return a.alloc_fn(new_sz, align, nullptr, 0, a.context);
}

void *raw_reallocate(Allocator a, isize new_sz, isize align, void *prev, isize old_sz)
{
    return a.alloc_fn(new_sz, align, prev, old_sz, a.context);
}

void raw_deallocate(Allocator a, void *ptr, isize sz)
{
    a.alloc_fn(0, 0, ptr, sz, a.context);
}

#endif // PSEUDO_ODIN_IMPLEMENTATION
