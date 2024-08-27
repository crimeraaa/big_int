#pragma once

#include "common.hpp"
#include "pointer.hpp"

struct Allocator {
    using AllocFn = void*(*)(void* hint, isize sz_old, isize sz_new, void* ctx);
    using ErrorFn = void (*)(const char* msg, void* ctx);

    AllocFn alloc_fn;
    ErrorFn error_fn;
    void*   context;  // Passed to both `alloc_fn` and `error_fn`.
};

/**
 * @brief
 *      You may define this macro (either right before including or using a
 *      compiler switch) to avoid creating `allocator_default`.
 * 
 * @note
 *      Doing so will also avoid including the C headers `stdlib` and `stdio`.
 *      Such behavior may be useful if you want to avoid those headers.
 */
#ifndef ALLOCATOR_NODEFAULT

extern const Allocator allocator_default;

#endif // ALLOCATOR_NODEFAULT

void* allocate(isize sz_new, const Allocator& allocator);
void* reallocate(void* hint, isize sz_old, isize sz_new, const Allocator& allocator);
void  deallocate(void* ptr, isize sz_old, const Allocator& allocator);

template<class T>
T* raw_pointer_new(isize n_new, const Allocator& allocator)
{
    void *next = allocate(size_of<T>(n_new), allocator);
    return static_cast<T*>(next);
}

template<class T>
T* raw_pointer_resize(T* rawptr, isize n_old, isize n_new, const Allocator& allocator)
{
    void *prev = static_cast<void*>(rawptr);
    void *next = reallocate(prev, size_of<T>(n_old), size_of<T>(n_new), allocator);
    return static_cast<T*>(next);
}

template<class T>
void raw_pointer_free(T* rawptr, isize n_old, const Allocator& allocator)
{
    deallocate(static_cast<void*>(rawptr), size_of<T>(n_old), allocator);
}

template<class T>
Pointer<T> pointer_new(isize n_new, const Allocator& allocator)
{
    return {raw_pointer_new<T>(n_new, allocator), n_new};
}

// Convenience overloaded template to default n_len to 1.
template<class T>
Pointer<T> pointer_new(const Allocator& allocator)
{
    return pointer_new<T>(1, allocator);
}

template<class T>
void pointer_resize(Pointer<T>* fatptr, isize n_new, const Allocator& allocator)
{
    T* next = raw_pointer_resize(fatptr->start, len(fatptr), n_new, allocator);
    *fatptr = {next, n_new};
}

template<class T>
void pointer_free(Pointer<T>* fatptr, const Allocator& allocator)
{
    raw_pointer_free(fatptr->start, len(fatptr), allocator);
    *fatptr = {nullptr, 0};
}

#ifdef ALLOCATOR_INCLUDE_IMPLEMENTATION

void* allocate(isize sz_new, const Allocator& allocator)
{
    return reallocate(nullptr, 0, sz_new, allocator);
}

void* reallocate(void* prev, isize sz_old, isize sz_new, const Allocator& allocator)
{
    void* next = allocator.alloc_fn(prev, sz_old, sz_new, allocator.context);
    // Nonzero allocation request failed, and we have an error handler?
    if (!next && sz_new != 0 && allocator.error_fn) {
        allocator.error_fn("[Re]allocation failure", allocator.context);
    }
    return next;
}

void deallocate(void* ptr, isize sz_old, const Allocator& allocator)
{
    reallocate(ptr, sz_old, 0, allocator);
}

// --- DEFAULT ALLOCATION ------------------------------------------------- {{{1

#ifndef ALLOCATOR_NODEFAULT

#include <cstdlib>
#include <cstdio>

const Allocator allocator_default = {
    // AllocFn
    [](void* hint, isize sz_old, isize sz_new, void* ctx) -> void*
    {
        unused(ctx);
        unused(sz_old);
        if (!hint && sz_new == 0) {
            std::free(hint);
            return nullptr;
        }
        return std::realloc(hint, sz_new);
    },
    // ErrorFn
    [](const char* msg, void* ctx)
    {
        unused(ctx);
        std::fprintf(stderr, "[FATAL] %s\n", msg);
        std::fflush(stderr);
        std::abort();
    },
    // context
    nullptr,
};

#endif // ALLOCATOR_NODEFAULT

// 1}}} ------------------------------------------------------------------------

#endif // ALLOCATOR_INCLUDE_IMPLEMENTATION
