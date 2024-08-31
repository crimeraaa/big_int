#include "mem.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>

static void *heap_allocator_proc(void *allocator_data, Allocator_Mode mode, Allocator_Proc_Args args)
{
    void *ptr{nullptr};
    unused(allocator_data);
    switch (mode) {
        case Allocator_Mode::Alloc:
            ptr = std::malloc(args.size);
            goto check_allocation;
        case Allocator_Mode::Resize:
            ptr = std::realloc(args.old_ptr, args.size);

            check_allocation:
            if (!ptr) {
                std::fprintf(stderr, "[FATAL]: %s\n", "[Re]allocation failure!");
                std::fflush(stderr);
                std::abort();
            }
            // TODO(2024-08-31): Handle better, may invalidate realloc
            // std::memset(ptr, 0, args.size);
            break;
        case Allocator_Mode::Free:
            std::free(args.old_ptr);
            break;
        case Allocator_Mode::Free_All:
            // Nothing to do
            break;
    }
    return ptr;
}

const Allocator heap_allocator = {
    &heap_allocator_proc,
    nullptr,
};

void *mem_alloc(Allocator a, isize size, isize align)
{
    Allocator_Proc_Args args;
    args.size     = size;
    args.align    = align;
    args.old_ptr  = nullptr;
    args.old_size = 0;
    return a.procedure(a.data, Allocator_Mode::Alloc, args);
}

void *mem_resize(Allocator a, void *ptr, isize old_size, isize new_size, isize align)
{
    Allocator_Proc_Args args;
    args.size     = new_size;
    args.align    = align;
    args.old_ptr  = ptr;
    args.old_size = old_size;
    return a.procedure(a.data, Allocator_Mode::Resize, args);

}

void mem_free(Allocator a, void *ptr, isize size)
{
    Allocator_Proc_Args args;
    args.size     = 0;
    args.align    = 0;
    args.old_ptr  = ptr;
    args.old_size = size;
    a.procedure(a.data, Allocator_Mode::Free, args);
}

void mem_free_all(Allocator a)
{
    Allocator_Proc_Args args{0, 0, nullptr, 0};
    a.procedure(a.data, Allocator_Mode::Free_All, args);
}
