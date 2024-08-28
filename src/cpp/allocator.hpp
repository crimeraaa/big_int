#pragma once

#include "pseudo_odin.hpp"

void *allocate(Allocator a, isize sz_new)
{
    return reallocate(a, nullptr, 0, sz_new);
}

void *reallocate(Allocator a, void *prev, isize sz_old, isize sz_new)
{
    void *next = a.alloc_fn(prev, sz_old, sz_new, a.context);
    // Nonzero allocation request failed, and we have an error handler?
    if (!next && sz_new != 0 && a.error_fn) {
        a.error_fn("[Re]allocation failure", a.context);
    }
    return next;
}

void deallocate(Allocator a, void *ptr, isize sz_used)
{
    reallocate(a, ptr, sz_used, 0);
}
