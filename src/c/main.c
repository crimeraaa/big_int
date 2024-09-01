#include "everything.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void *heap_allocator_proc(void *udata, const Allocator_Proc_Args *args)
{
    void *new_ptr = NULL;
    unused(udata);
    switch (args->mode) {
    case ALLOCATOR_MODE_ALLOC:
        new_ptr = malloc(args->new_size);
        goto check_ptr;
    case ALLOCATOR_MODE_RESIZE:
        new_ptr = realloc(args->old_ptr, args->new_size);
        check_ptr:
        if (!new_ptr) {
            fprintf(stderr, "[FATAL]: %s\n", "[Re]allocation failure!");
            fflush(stderr);
            abort();
        }
        Slice(byte) region = {
            cast(byte *)new_ptr + args->old_size,
            args->new_size - args->old_size
        };
        // Zero out the new region, only if we grew.
        if (region.len > 0) {
            memset(region.data, 0, region.len);
        }
        break;
    case ALLOCATOR_MODE_FREE:
        // printf("bye %p\n", args->old_ptr); //!DEBUG
        free(args->old_ptr);
        break;
    }
    return new_ptr;
}

static const Allocator heap_allocator = {&heap_allocator_proc, NULL};

static void string_builder_print(const String_Builder *self)
{
    const Dynamic_Header *hd = &self->header;
    printf("String_Builder{header{len=%ti, cap=%ti}, data=\"%s\"}\n",
            hd->len, hd->cap, self->data);
}

int main(void)
{
    String_Builder bd;

    // trust me
    String s = string_literal("Hi!");
    dynamic_array_init(&bd, &heap_allocator, s.len + 1);
    string_builder_print(&bd);
    dynamic_array__append(&bd.header, s.data, size_of(s.data[0]), s.len + 1, align_of(char));
    string_builder_print(&bd);

    dynamic_array_reserve(&bd, 10);
    string_builder_print(&bd);

    dynamic_array_free(&bd);
    return 0;
}

void *raw_alloc(const Allocator *a, isize new_size, isize align)
{
    Allocator_Proc_Args args;
    args.mode     = ALLOCATOR_MODE_ALLOC;
    args.new_size = new_size;
    args.align    = align;
    args.old_ptr  = NULL;
    args.old_size = 0;
    return a->procedure(a->userdata, &args);
}

void *raw_resize(const Allocator *a, void *old_ptr, isize old_size, isize new_size, isize align)
{
    Allocator_Proc_Args args;
    args.mode     = ALLOCATOR_MODE_RESIZE;
    args.new_size = new_size;
    args.align    = align;
    args.old_ptr  = old_ptr;
    args.old_size = old_size;
    return a->procedure(a->userdata, &args);
}

void raw_free(const Allocator *a, void *old_ptr, isize old_size)
{
    Allocator_Proc_Args args;
    args.mode     = ALLOCATOR_MODE_FREE;
    args.new_size = 0;
    args.align    = 0;
    args.old_ptr  = old_ptr;
    args.old_size = old_size;
    a->procedure(a->userdata, &args);
}

void dynamic_array__init(Dynamic_Header *self, const Allocator *a, isize tp_size, isize count, isize tp_align)
{
    /**
     * @warning
     *      We assume the address of the `<struct>.data` pointer is found here.
     *      Remember, we want the address of a pointer, not the pointer itself.
     */
    void **data = cast(void **)(self + 1);
    self->allocator = a;
    self->len       = 0;
    self->cap       = count;
    *data           = raw_alloc(a, tp_size * count, tp_align);
}

void dynamic_array__reserve(Dynamic_Header *self, isize tp_size, isize new_cap, isize tp_align)
{
    void **data     = cast(void **)(self + 1);
    void  *old_ptr  = *data;
    isize  old_cap  = self->cap;
    isize  old_size = tp_size * old_cap;
    isize  new_size = tp_size * new_cap;
    void  *new_ptr  = raw_resize(self->allocator, old_ptr, old_size, new_size, tp_align);
    
    self->cap = new_cap;
    *data     = new_ptr;
}

void dynamic_array__free(Dynamic_Header *self, isize size)
{
    void **data = cast(void **)(self + 1);
    raw_free(self->allocator, *data, size * self->cap);
    *data = NULL;
}

void dynamic_array__append(Dynamic_Header *self, const void *value, isize tp_size, isize count, isize tp_align)
{
    isize old_cap = self->cap;
    isize old_len = self->len;
    isize new_len = old_len + count;
    if (new_len > old_cap) {
        isize new_cap = (old_cap < 8) ? 8 : old_cap * 2;
        dynamic_array__reserve(self, tp_size, new_cap, tp_align);
    }
    // Load only here as `reserve()` may have reallocated the pointer.
    // This time we want the pointer itself, not the address thereof.
    byte *data = *(cast(byte **)(self + 1));
    memcpy(&data[tp_size * old_len], value, tp_size * count);
    self->len = new_len;
}
