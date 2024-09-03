#pragma once

#ifdef DEBUG_USE_ASSERT
    #include <cassert>
#else // DEBUG_USE_ASSERT
    #define assert(expr)
#endif // DEBUG_USE_ASSERT

#include <cstddef>
#include <cstdint>

// Uncomment this if you want array/slice indexing operations to not check.
// #define NO_BOUNDS_CHECK

using i8      = std::int8_t;
using i16     = std::int16_t;
using i32     = std::int32_t;
using i64     = std::int64_t;
using u8      = std::uint8_t;
using u16     = std::uint16_t;
using u32     = std::uint32_t;
using u64     = std::uint64_t;
using byte    = u8;
using isize   = std::ptrdiff_t;
using usize   = std::size_t;
using cstring = const char *;
using rawptr  =       void *;

#define unused(Expr)            static_cast<void>(Expr)
#define size_of(Expr)           static_cast<isize>(sizeof(Expr))
#define align_of(Type)          static_cast<isize>(alignof(Type))
#define offset_of(Type, Memb)   static_cast<isize>(offsetof(Type, Memb))

enum class Allocator_Mode : u8 {
    Alloc,
    Resize,
    Free,
    Free_All,
};

/**
 * @brief
 *      Separate type because the sheer number of arguments can get unwieldy.
 */
struct Allocator_Proc_Args {
    isize size;     // number of bytes to [re]allocate.
    isize align;    // desired alignment of the type.
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

#ifndef ODIN_NOSTDLIB

/**
 * @brief
 *      Standard C malloc-based allocator.
 * 
 * @warning
 *      On allocation failure, calls the standard C `abort()` function.
 */
extern const Allocator heap_allocator;

#endif // ODIN_NOSTDLIB

void *allocator_alloc   (Allocator a, isize size, isize align);
void *allocator_resize  (Allocator a, void *ptr, isize old_size, isize new_size, isize align);
void  allocator_free    (Allocator a, void *ptr, isize size);
void  allocator_free_all(Allocator a);

template<class T>
T *rawptr_new(const Allocator &a)
{
    return static_cast<T *>(allocator_alloc(a, size_of(T), align_of(T)));
}

template<class T>
void rawptr_free(const Allocator &a, T *ptr)
{
    allocator_free(a, static_cast<void *>(ptr), size_of(T));
}

template<class T>
T *rawarray_new(const Allocator &a, isize count)
{
    return static_cast<T *>(allocator_alloc(a, size_of(T) * count, align_of(T)));
}

template<class T>
T *rawarray_resize(const Allocator &a, T *array, isize old_len, isize new_len)
{
    isize old_size = size_of(T) * old_len;
    isize new_size = size_of(T) * new_len;
    return static_cast<T *>(allocator_resize(a, array, old_size, new_size, align_of(T)));
}

template<class T>
void rawarray_free(const Allocator &a, T *array, isize count)
{
    allocator_free(a, static_cast<void *>(array), size_of(T) * count);
}

/**
 * @brief
 *      A slice is a fixed-size view into some memory. It may be mutable.
 *
 * @warning
 *      If this points to heap-allocated memory, it may not necessarily view
 *      the entire allocation. Do not attempt to free it in that case.
 */
template<class T>
struct Slice {
    T    *data;
    isize len;
    
    /**
     * @brief
     *      Read-write access. Returns an L-value.
     */
    T &operator[](isize index)
    {
        #if !defined(NO_BOUNDS_CHECK)
            assert(0 <= index && index < this->len);
        #endif
        return this->data[index];
    }
    
    /**
     * @brief
     *      Read-only access. Returns an R-value.
     */
    const T &operator[](isize index) const
    {
        #if !defined(NO_BOUNDS_CHECK)
            assert(0 <= index && index < this->len);
        #endif
        return this->data[index];
    }
};

template<class T>
Slice<T> slice(T *ptr, isize count, isize start, isize stop)
{
    assert(0 <= start && start <= stop && stop <= count);
    Slice<T> out{nullptr, 0};
    isize    len = stop - start;
    if (len > 0) {
        out.data = ptr + start;
        out.len  = stop;
    }
    return out;
}

template<class T>
Slice<T> slice(const Slice<T> &self, isize start, isize stop)
{
    return slice(begin(self), len(self), start, stop);
}

template<class T>
Slice<const T> slice_const(const Slice<T> &self, isize start, isize stop)
{
    return slice<const T>(cbegin(self), len(self), start, stop);
}

template<class T>
Slice<const T> slice_const_cast(const Slice<T> &self)
{
    return {cbegin(self), len(self)};
}

template<class T>
Slice<T> slice_make(const Allocator &a, isize len)
{
    return {rawarray_new<T>(a, len), len};
}

template<class T>
void slice_free(Slice<T> *self, const Allocator &a)
{
    allocator_free(a, self->data, size_of(T) * self->len);
    self->data = nullptr;
    self->len  = 0;
}

/**
 * @brief
 *      A dynamic array akin to C++ `std::vector`. It remembers its allocator.
 */
template<class T>
struct Array {
    Allocator allocator;
    T *       data;
    isize     len;
    isize     cap;

    /**
     * @brief
     *      Read-write access. Returns an L-value.
     */
    T &operator[](isize index)
    {
        #if !defined(NO_BOUNDS_CHECK)
            assert(0 <= index && index < this->len);
        #endif
        return this->data[index];
    }
    
    /**
     * @brief
     *      Read-only access. Returns an R-value.
     */
    const T &operator[](isize index) const
    {
        #if !defined(NO_BOUNDS_CHECK)
            assert(0 <= index && index < this->len);
        #endif
        return this->data[index];
    }
};

template<class T>
Slice<T> slice(const Array<T> &self, isize start, isize stop)
{
    return slice(begin(self), len(self), start, stop);
}

template<class T>
Slice<const T> slice_const(const Array<T> &self, isize start, isize stop)
{
    return slice<const T>(cbegin(self), len(self), start, stop);
}

template<class T>
void array_init(Array<T> *self, const Allocator &a)
{
    array_init(self, a, 0, 0);
}

template<class T>
void array_init(Array<T> *self, const Allocator &a, isize len)
{
    array_init(self, a, len, len);
}

template<class T>
void array_init(Array<T> *self, const Allocator &a, isize len, isize cap)
{
    self->allocator = a;
    if (cap > 0) {
        self->data = rawarray_new<T>(a, cap);
    } else {
        self->data = nullptr;
    }
    self->len = len;
    self->cap = cap;
}

template<class T>
Array<T> array_make(const Allocator &a)
{
    return array_make(a, 0, 0);
}

template<class T>
Array<T> array_make(const Allocator &a, isize len)
{
    return array_make(a, len, len);
}

template<class T>
Array<T> array_make(const Allocator &a, isize len, isize cap)
{
    Array<T> out;
    array_init(&out, a, len, cap);
    return out;
}

template<class T>
void array_resize(Array<T> *self, isize new_len)
{
    // Need to grow?
    if (cap(self) < new_len) {
        array_grow(self);
    }
    self->len = new_len;
}

template<class T>
void array_grow(Array<T> *self)
{
    array_reserve(self, (self->cap < 8) ? 8 : self->cap * 2);
}

template<class T>
void array_reserve(Array<T> *self, isize new_cap)
{
    isize old_cap = cap(self);
    // Nothing to do
    if (new_cap == old_cap) {
        return;
    }
    
    T *   old_data = self->data;
    isize old_size = size_of(T) * old_cap;
    isize new_size = size_of(T) * new_cap;
    
    self->data = static_cast<T *>(allocator_resize(self->allocator, old_data, old_size, new_size, align_of(T)));
    self->cap  = new_cap;
}

template<class T>
void array_free(Array<T> *self)
{
    allocator_free(self->allocator, self->data, size_of(T) * self->cap);
    self->data = nullptr;
    self->len  = self->cap = 0;
}

template<class T>
void array_append(Array<T> *self, const T &value)
{
    if (len(self) >= cap(self)) {
        array_grow(self);
    }
    self->data[self->len++] = value;
}

template<class T>
void array_append(Array<T> *self, const Slice<const T> &values)
{
    isize old_len = len(self);
    isize new_len = old_len + len(values);
    if (new_len > cap(self)) {
        // Get next power of 2
        isize n = 1;
        while (n < new_len) {
            n *= 2;
        }
        new_len = n;
        array_reserve(self, new_len);
    }

    for (isize i = 0; i < len(values); i++) {
        self->data[old_len + i] = values[i];
    }
    self->len = new_len;
}

// Promotes `Slice<T>` to a `Slice<const T>` to call the above signature.
template<class T>
void array_append(Array<T> *self, const Slice<T> &values)
{
    array_append(self, {cbegin(values), len(values)});
}

template<class T>
T array_pop(Array<T> *self)
{
    // Can't pop if nothing to pop!
    assert(self->len != 0);
    isize index = self->len - 1;
    self->len   = index;
    return self->data[index];
}

///--- GLOBAL UTILITY ----------------------------------------------------- {{{1

///--- REFERENCE ---------------------------------------------------------- {{{2

template<class T>
isize len(const Slice<T> &self)
{
    return self.len;
}

template<class T>
isize len(const Array<T> &self)
{
    return self.len;
}

template<class T>
isize cap(const Array<T> &self)
{
    return self.cap;
}

///--- ITERATORS ---------------------------------------------------------- {{{3

template<class T>
T *begin(const Array<T> &self)
{
    return self.data;
}

template<class T>
T *end(const Array<T> &self)
{
    return self.data + self.len;
}

template<class T>
const T *cbegin(const Array<T> &self)
{
    return self.data;
}

template<class T>
const T *cend(const Array<T> &self)
{
    return self.data + self.len;
}

template<class T>
T *begin(const Slice<T> &self)
{
    return self.data;
}

template<class T>
T *end(const Slice<T> &self)
{
    return self.data + self.len;
}

template<class T>
const T *cbegin(const Slice<T> &self)
{
    return self.data;
}

template<class T>
const T *cend(const Slice<T> &self)
{
    return self.data + self.len;
}

///--- 3}}} --------------------------------------------------------------------

///--- 2}}} --------------------------------------------------------------------

///--- POINTER ------------------------------------------------------------ {{{2

template<class T>
isize len(const Slice<T> *self)
{
    return self->len;
}

template<class T>
isize len(const Array<T> *self)
{
    return self->len;
}

template<class T>
isize cap(const Array<T> *self)
{
    return self->cap;
}

///--- ITERATORS ---------------------------------------------------------- {{{3

template<class T>
T *begin(const Array<T> *self)
{
    return self->data;
}

template<class T>
T *end(const Array<T> *self)
{
    return self->data + self->len;
}

template<class T>
const T *cbegin(const Array<T> *self)
{
    return self->data;
}

template<class T>
const T *cend(const Array<T> *self)
{
    return self->data + self->len;
}

template<class T>
T *begin(const Slice<T> *self)
{
    return self->data;
}

template<class T>
T *end(const Slice<T> *self)
{
    return self->data + self->len;
}

template<class T>
const T *cbegin(const Slice<T> *self)
{
    return self->data;
}

template<class T>
const T *cend(const Slice<T> *self)
{
    return self->data + self->len;
}

///--- 3}}} --------------------------------------------------------------------

///--- 2}}} --------------------------------------------------------------------

///--- 1}}} --------------------------------------------------------------------

#ifdef ODIN_IMPLEMENTATION

#ifndef ODIN_NOSTDLIB

#include <cstdio>
#include <cstdlib>
#include <cstring>

void *heap_allocator_proc(void *allocator_data, Allocator_Mode mode, Allocator_Proc_Args args)
{
    void *ptr{nullptr};
    isize new_region_len = args.size - args.old_size;
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
            if (new_region_len > 0) {
                byte *new_region_start = static_cast<byte *>(ptr) + args.old_size;
                std::memset(new_region_start, 0, new_region_len);
            }
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

#endif // ODIN_NOSTDLIB

void *allocator_alloc(Allocator a, isize size, isize align)
{
    Allocator_Proc_Args args;
    args.size     = size;
    args.align    = align;
    args.old_ptr  = nullptr;
    args.old_size = 0;
    return a.procedure(a.data, Allocator_Mode::Alloc, args);
}

void *allocator_resize(Allocator a, void *ptr, isize old_size, isize new_size, isize align)
{
    Allocator_Proc_Args args;
    args.size     = new_size;
    args.align    = align;
    args.old_ptr  = ptr;
    args.old_size = old_size;
    return a.procedure(a.data, Allocator_Mode::Resize, args);

}

void allocator_free(Allocator a, void *ptr, isize size)
{
    Allocator_Proc_Args args;
    args.size     = 0;
    args.align    = 0;
    args.old_ptr  = ptr;
    args.old_size = size;
    a.procedure(a.data, Allocator_Mode::Free, args);
}

void allocator_free_all(Allocator a)
{
    Allocator_Proc_Args args{0, 0, nullptr, 0};
    a.procedure(a.data, Allocator_Mode::Free_All, args);
}

#endif // ODIN_IMPLEMENTATION
