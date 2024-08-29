#pragma once

#include "cpp_odin.hpp"

/**
 * @brief
 *      Simply a pointer to arbitrary data with how many bytes it points to.
 *
 * @note
 *      We create a separate struct because `Slice<void>` will result in compile
 *      time errors.
 *      
 *      Taken from: https://pkg.odin-lang.org/base/runtime/#Raw_Slice
 */
struct Raw_Slice {
    void *data;
    isize len;
};

enum class Allocator_Error : u8 {
    None                 = 0,
    Out_Of_Memory        = 1,
    Invalid_Pointer      = 2,
    Invalid_Argument     = 3,
    Mode_Not_Implemented = 4,
};

/**
 * @note
 *      See: https://pkg.odin-lang.org/base/runtime/#Allocator_Mode
 */
enum class Allocator_Mode : u8 {
    Alloc,
    Resize,
    Free,
    Alloc_Non_Zeroed,
    Resize_Non_Zeroed,
};

template<class T>
struct Allocation {
    Pointer<T>      pointer;
    Allocator_Error error;
};

struct Raw_Allocation {
    void *          pointer;
    Allocator_Error error;
};

/**
 * @brief
 *      A separate type because all the different parameters can get quite
 *      unwieldy.
 */
struct Allocator_Proc_Args {
    Raw_Slice old;
    isize     new_size;
    isize     align;
};

using Allocator_Proc = Raw_Allocation (*)(void *ctx, Allocator_Mode mode, Allocator_Proc_Args args);

/**
 * @note
 *      See: https://pkg.odin-lang.org/base/runtime/#Allocator
 */
struct Allocator {
    Allocator_Proc procedure;
    void *         context;
};


/**
 * @note
 *      See: https://pkg.odin-lang.org/base/runtime/#mem_alloc
 *      See: https://pkg.odin-lang.org/base/runtime/#mem_resize
 *      See: https://pkg.odin-lang.org/base/runtime/#mem_free
 *      See: https://pkg.odin-lang.org/base/runtime/#mem_alloc_non_zeroed
 */
Raw_Allocation  mem_alloc (const Allocator &a, isize size, isize align);
Raw_Allocation  mem_resize(const Allocator &a, const Raw_Slice &s, isize new_size, isize align);
Allocator_Error mem_free  (const Allocator &a, const Raw_Slice &s);

Raw_Allocation  mem_alloc_non_zeroed (const Allocator &a, isize size, isize align);
Raw_Allocation  mem_resize_non_zeroed(const Allocator &a, const Raw_Slice &s, isize new_size, isize align);

template<typename T>
Allocation<T> ptr_new_safe(const Allocator &a, isize len = 1)
{
    auto [ptr, err] = mem_alloc(a, size_of(T) * len, align_of(T));
    return {{cast(T *)ptr, len}, err};
}

template<class T>
Allocation<T> ptr_resize_safe(const Allocator &a, Pointer<T> *s, isize new_len)
{
    void *  old_ptr  = cast(void *)s->data;
    isize   old_size = s->len;
    isize   new_size = size_of(T) * new_len;
    auto  [ptr, err] = mem_resize(a, {old_ptr, old_size}, new_size, align_of(T));    

    *s = {cast(T *)ptr, new_len};
    return {*s, err};
}

template<class T>
Allocator_Error ptr_free_safe(const Allocator &a, Pointer<T> *s)
{
    void *old_ptr = cast(void *)s->data;
    isize old_len = size_of(T) * s->len;    
    *s = {nullptr, 0};
    return mem_free(a, {old_ptr, old_len});
}

// Wrapper to `ptr_new_safe` that immediately returns the pointer.
template<class T>
Pointer<T> ptr_new(const Allocator &a, isize len = 1)
{
    return ptr_new_safe<T>(a, len).pointer;
}

// Wrapper to `ptr_resize_safe` that immediately returns the pointer.
template<class T>
Pointer<T> ptr_resize(const Allocator &a, Pointer<T> *s, isize new_len)
{
    return ptr_resize_safe(a, s, new_len).pointer;
}

// Wrapper to `ptr_free_safe` that ignores the error.
template<class T>
void ptr_free(const Allocator &a, Pointer<T> *s)
{
    ptr_free_safe(a, s);
}

#ifdef CPP_ODIN_IMPLEMENTATION

Raw_Allocation mem_alloc(const Allocator &a, isize size, isize align)
{
    // Nothing to do
    if (size == 0 || !a.procedure) {
        return {nullptr, Allocator_Error::None};
    }

    Allocator_Proc_Args args;
    args.old      = {nullptr, 0};
    args.new_size = size;
    args.align    = align;
    return a.procedure(a.context, Allocator_Mode::Alloc, args);
}

Raw_Allocation mem_resize(const Allocator &a, const Raw_Slice &s, isize new_size, isize align)
{
    // Nothing to do
    if (!a.procedure) {
        return {nullptr, Allocator_Error::None};
    }
    
    Allocator_Proc_Args args;
    args.old      = s;
    args.new_size = new_size;
    args.align    = align;
    return a.procedure(a.context, Allocator_Mode::Resize, args);
}

Allocator_Error mem_free(const Allocator &a, const Raw_Slice &s)
{
    // Nothing to do
    if (!s.data || !a.procedure) {
        return Allocator_Error::None;
    }

    Allocator_Proc_Args args;
    args.old      = s;
    args.new_size = 0;
    args.align    = 0;
    return a.procedure(a.context, Allocator_Mode::Free, args).error;
}

Raw_Allocation mem_alloc_non_zeroed(const Allocator &a, isize size, isize align)
{
    // Nothing to do
    if (!a.procedure) {
        return {nullptr, Allocator_Error::None};
    }

    Allocator_Proc_Args args;
    args.old      = {nullptr, 0};
    args.new_size = size;
    args.align    = align;
    return a.procedure(a.context, Allocator_Mode::Alloc_Non_Zeroed, args);
}

Raw_Allocation mem_resize_non_zeroed(const Allocator &a, const Raw_Slice &s, isize new_size, isize align)
{
    // Nothing to do
    if (!s.data || !a.procedure) {
        return {nullptr, Allocator_Error::None};
    }

    Allocator_Proc_Args args;
    args.old      = s;
    args.new_size = new_size;
    args.align    = align;
    return a.procedure(a.context, Allocator_Mode::Resize_Non_Zeroed, args);
}

#endif // ODIN_CPP_IMPLEMENTATION

