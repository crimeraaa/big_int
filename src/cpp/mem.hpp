#pragma once

#include "cpp_odin.hpp"

namespace odin::mem {

/**
 * @brief
 *      Raw memory. Here, `len` refers to the number of bytes being pointed to.
 */
struct Raw_Slice {
    rawptr data;
    isize  len;
};

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

rawptr mem_alloc            (const Allocator &a, isize new_size, isize align, Allocator_Error *e);
rawptr mem_alloc_non_zeroed (const Allocator &a, isize new_size, isize align, Allocator_Error *e);
rawptr mem_resize           (const Allocator &a, const Raw_Slice &rs, isize new_size, isize align, Allocator_Error *e);
rawptr mem_resize_non_zeroed(const Allocator &a, const Raw_Slice &rs, isize new_size, isize align, Allocator_Error *e);
void   mem_free             (const Allocator &a, const Raw_Slice &rs, Allocator_Error *e);

template<class T>
Pointer<T> new_ptr(const Allocator &a, Allocator_Error *e = nullptr)
{
    rawptr ptr = mem_alloc(a, size_of(T), align_of(T), e);
    return {{}, static_cast<T *>(ptr)}; // Stupid C++ non-aggregate brace init
}

template<class T>
void free_ptr(const Allocator &a, Pointer<T> ptr, Allocator_Error *e = nullptr)
{
    rawptr old_rawptr = static_cast<rawptr>(ptr.data);
    isize  old_size   = size_of(T);
    mem_free(a, {old_rawptr, old_size}, e);
}

template<class T>
Slice<T> make_slice(const Allocator &a, isize len, Allocator_Error *e = nullptr)
{
    rawptr ptr = mem_alloc(a, size_of(T) * len, align_of(T), e);    
    return {{}, static_cast<T *>(ptr), len};
}

template<class T>
void delete_slice(const Allocator &a, Slice<T> array, Allocator_Error *e = nullptr)
{
    rawptr old_rawptr = static_cast<rawptr>(array.data);
    isize  old_size   = size_of(T) * len(array);
    mem_free(a, {old_rawptr, old_size}, e);
}

}; // namespace odin::mem

#ifdef CPP_ODIN_IMPLEMENTATION

namespace odin::mem {

rawptr mem_alloc(const Allocator &a, isize new_size, isize align, Allocator_Error *e)
{
    // Nothing to do
    if (new_size == 0 || !a.procedure) {
        return nullptr;
    }
    Allocator_Proc_Args args;
    args.allocator_data = a.data;
    args.mode           = Allocator_Mode::Alloc;
    args.new_size       = new_size;
    args.align          = align;
    args.memory         = {nullptr, 0};
    return a.procedure(args, e);
}

rawptr mem_resize(const Allocator &a, const Raw_Slice &rs, isize new_size, isize align, Allocator_Error *e)
{
    // Nothing to do
    if (!a.procedure) {
        return nullptr;
    }
    Allocator_Proc_Args args;
    args.allocator_data = a.data;
    args.mode           = Allocator_Mode::Resize;
    args.new_size       = new_size;
    args.align          = align;
    args.memory         = rs;
    return a.procedure(args, e);
}

void mem_free(const Allocator &a, const Raw_Slice &rs, Allocator_Error *e)
{
    // Nothing to do
    if (!rs.data || !a.procedure) {
        return;
    }
    Allocator_Proc_Args args;
    args.allocator_data = a.data;
    args.mode           = Allocator_Mode::Free;
    args.new_size       = 0;
    args.align          = 0;
    args.memory         = rs;
    a.procedure(args, e);
}

rawptr mem_alloc_non_zeroed(const Allocator &a, isize new_size, isize align, Allocator_Error *e)
{
    // Nothing to do
    if (!a.procedure) {
        return nullptr;
    }
    Allocator_Proc_Args args;
    args.allocator_data = a.data;
    args.mode           = Allocator_Mode::Alloc_Non_Zeroed;
    args.new_size       = new_size;
    args.align          = align;
    args.memory         = {nullptr, 0};
    return a.procedure(args, e);
}

rawptr mem_resize_non_zeroed(const Allocator &a, const Raw_Slice &rs, isize new_size, isize align, Allocator_Error *e)
{
    // Nothing to do
    if (!rs.data || !a.procedure) {
        return nullptr;
    }
    Allocator_Proc_Args args;
    args.allocator_data = a.data;
    args.mode           = Allocator_Mode::Resize_Non_Zeroed;
    args.new_size       = new_size;
    args.align          = align;
    args.memory         = rs;
    return a.procedure(args, e);
}

}; // namespace odin::mem

#endif // ODIN_CPP_IMPLEMENTATION

