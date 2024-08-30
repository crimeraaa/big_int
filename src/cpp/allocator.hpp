#pragma once

#include "cpp_odin.hpp"

using Raw_Slice = Slice<void>;

enum class Allocator_Error : u8 {
    None                 = 0,
    Out_Of_Memory        = 1,
    Invalid_Pointer      = 2,
    Invalid_Argument     = 3,
    Mode_Not_Implemented = 4,
};

/**
 * @link
 *      https://pkg.odin-lang.org/base/runtime/#Allocator_Mode
 */
enum class Allocator_Mode : u8 {
    Alloc,
    Resize,
    Free,
    Alloc_Non_Zeroed,
    Resize_Non_Zeroed,
};

/**
 * @note
 *      The sheer number of arguments get rather unwieldy.
 */
struct Allocator_Proc_Args {
    rawptr         allocator_data;
    Allocator_Mode mode;
    isize          new_size;
    isize          align;
    Raw_Slice      memory;
};

using Allocator_Proc = rawptr (*)(const Allocator_Proc_Args &args, Allocator_Error *e);

/**
 * @link
 *      https://pkg.odin-lang.org/base/runtime/#Allocator
 *      https://pkg.odin-lang.org/base/runtime/#mem_alloc
 *      https://pkg.odin-lang.org/base/runtime/#mem_resize
 *      https://pkg.odin-lang.org/base/runtime/#mem_free
 *      https://pkg.odin-lang.org/base/runtime/#mem_alloc_non_zeroed
 */
struct Allocator {
    Allocator_Proc procedure;
    rawptr         data;
};

rawptr mem_alloc            (Allocator a, isize new_size, isize align, Allocator_Error *e);
rawptr mem_alloc_non_zeroed (Allocator a, isize new_size, isize align, Allocator_Error *e);
rawptr mem_resize           (Allocator a, Raw_Slice s, isize new_size, isize align, Allocator_Error *e);
rawptr mem_resize_non_zeroed(Allocator a, Raw_Slice s, isize new_size, isize align, Allocator_Error *e);
void   mem_free             (Allocator a, Raw_Slice s, Allocator_Error *e);

template<class T>
Pointer<T> ptr_new(Allocator a, isize len = 1, Allocator_Error *e = nullptr)
{
    isize  new_size   = size_of(T) * len;
    rawptr new_rawptr = mem_alloc(a, new_size, align_of(T), e);
    return {cast(T *)new_rawptr, len};
}

template<class T>
Pointer<T> ptr_resize(Allocator a, Pointer<T> *ptr, isize new_len, Allocator_Error *e = nullptr)
{
    rawptr old_rawptr = cast(void *)ptr->data;
    isize  old_size   = size_of(T) * ptr->len;
    isize  new_size   = size_of(T) * new_len;
    rawptr new_rawptr = mem_resize(a, {old_rawptr, old_size}, new_size, align_of(T), e);
    return *ptr = {cast(T *)new_rawptr, new_len};
}

template<class T>
void ptr_free(Allocator a, Pointer<T> *ptr, Allocator_Error *e = nullptr)
{
    rawptr old_rawptr = cast(void *)ptr->data;
    isize  old_size   = size_of(T) * ptr->len;
    mem_free(a, {old_rawptr, old_size}, e);
}

#ifdef CPP_ODIN_IMPLEMENTATION

static rawptr mem__alloc(Allocator a, Allocator_Mode mode, isize new_size, isize align, Allocator_Error *e)
{
    Allocator_Proc_Args args;
    args.allocator_data = a.data;
    args.mode           = mode;
    args.new_size       = new_size;
    args.align          = align;
    args.memory         = {nullptr, 0};
    return a.procedure(args, e);
}

static rawptr mem__resize(Allocator a, Allocator_Mode mode, Raw_Slice s, isize new_size, isize align, Allocator_Error *e)
{
    Allocator_Proc_Args args;
    args.allocator_data = a.data;
    args.mode           = mode;
    args.new_size       = new_size;
    args.align          = align;
    args.memory         = s;
    return a.procedure(args, e);
}

rawptr mem_alloc(Allocator a, isize new_size, isize align, Allocator_Error *e)
{
    // Nothing to do
    if (new_size == 0 || !a.procedure) {
        return nullptr;
    }
    return mem__alloc(a, Allocator_Mode::Alloc, new_size, align, e);
}

rawptr mem_resize(Allocator a, Raw_Slice s, isize new_size, isize align, Allocator_Error *e)
{
    // Nothing to do
    if (!a.procedure) {
        return nullptr;
    }
    return mem__resize(a, Allocator_Mode::Resize, s, new_size, align, e);
}

void mem_free(Allocator a, Raw_Slice s, Allocator_Error *e)
{
    // Nothing to do
    if (!s.data || !a.procedure) {
        return;
    }
    Allocator_Proc_Args args;
    args.allocator_data = a.data;
    args.mode           = Allocator_Mode::Free;
    args.new_size       = 0;
    args.align          = 0;
    args.memory         = s;
    a.procedure(args, e);
}

rawptr mem_alloc_non_zeroed(Allocator a, isize new_size, isize align, Allocator_Error *e)
{
    // Nothing to do
    if (!a.procedure) {
        return nullptr;
    }
    return mem__alloc(a, Allocator_Mode::Alloc_Non_Zeroed, new_size, align, e);
}

rawptr mem_resize_non_zeroed(Allocator a, Raw_Slice s, isize new_size, isize align, Allocator_Error *e)
{
    // Nothing to do
    if (!s.data || !a.procedure) {
        return nullptr;
    }
    return mem__resize(a, Allocator_Mode::Resize_Non_Zeroed, s, new_size, align, e);
}

#endif // ODIN_CPP_IMPLEMENTATION

