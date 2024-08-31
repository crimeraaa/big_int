#pragma once

#include "odin.hpp"

enum class Allocator_Mode : u8 {
    Alloc,
    Resize,
    Free,
    Free_All,
};

/**
 * @brief
 *      Separate T because the sheer number of arguments can get unwieldy.
 */
struct Allocator_Proc_Args {
    isize size;     // number of bytes to [re]allocate.
    isize align;    // desired alignment of the T.
    void *old_ptr;  // previous pointer to reallocate or free.
    isize old_size; // number of bytes allocated for `old_ptr`.
};

/**
 * @link
 *      https://github.com/gingerBill/gb/blob/master/gb.h#L1099
 */
using Allocator_Proc = void *(*)(void *allocator_data, Allocator_Mode mode, Allocator_Proc_Args args);

struct Allocator {
    Allocator_Proc procedure; // user defined allocation function.
    void *         data;      // userdata/context pointer.
};

/**
 * @brief
 *      Standard C malloc-based allocator.
 * 
 * @warning
 *      On allocation failure, calls the standard C `abort()` function.
 */
extern const Allocator heap_allocator;

void *mem_alloc   (Allocator a, isize size, isize align);
void *mem_resize  (Allocator a, void *ptr, isize old_size, isize new_size, isize align);
void  mem_free    (Allocator a, void *ptr, isize size);
void  mem_free_all(Allocator a);

template<class T>
T *single_ptr_new(const Allocator &a)
{
    return static_cast<T *>(mem_alloc(a, size_of(T), align_of(T)));
}

template<class T>
T *multi_ptr_new(const Allocator &a, isize count)
{
    return static_cast<T *>(mem_alloc(a, size_of(T) * count, align_of(T)));
}
