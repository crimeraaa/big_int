#pragma once

#include "common.hpp"
#include "pointer.hpp"

struct Allocator {
    using AllocFn = void*(*)(void* hint, isize sz_old, isize sz_new, void* ctx);
    using ErrorFn = void (*)(const char* msg, void* ctx);

    AllocFn alloc_fn;
    ErrorFn error_fn;
    void*   context;  // Passed to both `alloc_fn` and `error_fn`.
    
    void* operator()(void* hint, isize sz_old, isize sz_new) const;
};

extern const Allocator allocator_default;

template<class T>
static constexpr isize size_of(isize count = 1, isize extra = 0)
{
    return sizeof(T) * count + extra;
}

template<class T>
T* raw_new(isize n_new, const Allocator& allocator)
{
    void *ptr = allocator(nullptr, 0, size_of<T>(n_new));
    return static_cast<T*>(ptr);
}

template<class T>
T* raw_resize(T* prev, isize n_old, isize n_new, const Allocator& allocator)
{
    void* hint = static_cast<void*>(prev);
    void* ptr  = allocator(hint, size_of<T>(n_old), size_of<T>(n_new));
    return static_cast<T*>(ptr);
}

template<class T>
void raw_free(T* ptr, isize n_old, const Allocator& allocator)
{
    void *hint = static_cast<void*>(ptr);
    allocator(hint, size_of<T>(n_old), 0);
}

template<class T>
Pointer<T> pointer_new(isize n_new, const Allocator& allocator)
{
    Pointer<T> ptr;
    ptr.start  = raw_new<T>(n_new, allocator);
    ptr.length = n_new;
    return ptr;
}

// Convenience overloaded template to default n_len to 1.
template<class T>
Pointer<T> pointer_new(const Allocator& allocator)
{
    return pointer_new<T>(1, allocator);
}

template<class T>
void pointer_resize(Pointer<T>* ptr, isize n_new, const Allocator& allocator)
{
    ptr->start  = raw_resize(ptr->start, len(ptr), n_new, allocator);
    ptr->length = n_new;
}

template<class T>
void pointer_free(Pointer<T>* ptr, const Allocator& allocator)
{
    raw_free(ptr->start, len(ptr), allocator);
    ptr->start  = nullptr;
    ptr->length = 0;
}

#ifdef ALLOCATOR_INCLUDE_IMPLEMENTATION

#include <cstdlib>
#include <cstdio>

void* Allocator::operator()(void* hint, isize sz_old, isize sz_new) const
{
    void* ptr = this->alloc_fn(hint, sz_old, sz_new, this->context);
    // Nonzero allocation request failed and we have an error handler?
    if (!ptr && sz_new != 0 && this->error_fn) {
        this->error_fn("[Re]allocation failed!", this->context);
    }
    return ptr;
}

// --- DEFAULT ALLOCATION ------------------------------------------------- {{{1

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

// 1}}} ------------------------------------------------------------------------

#endif // ALLOCATOR_INCLUDE_IMPLEMENTATION
