#pragma once

#include "common.h"

typedef void  *(*AllocFn)(void *hint, size oldsz, size newsz, void *ctx);
typedef void   (*HandlerFn)(const char *msg, size reqsz, void *ctx);
typedef struct  Allocator Allocator;

struct Allocator {
    AllocFn   allocate_function;
    HandlerFn handler_function;
    void     *allocate_context;
    void     *handler_context;
};

void *allocator_rawrealloc(Allocator *self, void *hint, size oldsz, size newsz);

#define allocator_realloc(T, ptr, oldlen, newlen, a) \
    cast(T*, allocator_rawrealloc(a, ptr, array_sizeof(T, oldlen), array_sizeof(T, newlen)))
#define allocator_new(T, len, a)        allocator_realloc(T, nullptr, 0, len, a)
#define allocator_free(T, ptr, len, a)  allocator_realloc(T, ptr, len, 0, a)

#ifdef ALLOCATOR_INCLUDE_IMPLEMENTATION

void *allocator_rawrealloc(Allocator *self, void *hint, size oldsz, size newsz)
{
    void *ptr = self->allocate_function(hint, oldsz, newsz, self->allocate_context);
    // No need to handle when freeing, but handle any time else.
    if (!ptr && newsz > 0)
        self->handler_function("Out of memory", newsz, self->handler_context);
    return ptr;
}

#endif  // ALLOCATOR_INCLUDE_IMPLEMENTATION
