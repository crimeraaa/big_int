#pragma once

#include "common.hpp"

struct Allocator {
    using AllocFn = void*(*)(void *hint, isize sz_old, isize sz_new, void *ctx);
    using ErrorFn = void (*)(const char *msg, void *ctx);

    AllocFn alloc_fn;
    ErrorFn error_fn;
    void *  context;  // Passed to both `alloc_fn` and `error_fn`.
};

/**
 * @brief
 *      Sort of a variable length structure but the flexible array portion is
 *      implied.
 */
struct Dynamic_Header {
    Allocator allocator;
    isize     length;
    isize     capacity;
};

/**
 * @warning
 *      Very dangerous! Relies on allocations/structs to be properly aligned.
 *      If `sizeof(Dynamic_Header)` is not aligned then this will break
 *      horribly.
 *      
 * @note    
 *      Assumes `ptr` is the address of the first element in the allocated array.
 */
#define DYNAMIC_HEADER(ptr)     (cast(Dynamic_Header *)ptr - 1)
#define allocate_item(a, T)      cast(T *)allocate(a, size_of(T))
#define allocate_array(a, T, n)  cast(T *)allocate(a, size_of(T) * (n))

void *allocate  (Allocator a, isize sz_new);
void *reallocate(Allocator a, void *hint, isize sz_old, isize sz_new);
void  deallocate(Allocator a, void *ptr, isize sz_used);

/**
 * @brief
 *      The implied flexible array portion of `Dynamic_Header`. Since this is
 *      just a `char*` you are free to index it.
 * 
 *      However, allocating and freeing this must be done through the `string_*`
 *      family of functions as there are assumptions about where the allocation
 *      header is found.
 * 
 * @note
 *      For interoperability with C it is always nul terminated.
 */
using String = char *;

String string_make        (Allocator a, const char *c_str);
String string_make_len    (Allocator a, const void *p_bytes, isize n_bytes);
String string_make_reserve(Allocator a, isize n_cap);
void   string_delete      (String *self);
void   string_clear       (String *self);
String string_resize      (String *self, isize n_extra);

isize  string_len         (const String self);
isize  string_cap         (const String self);
isize  string_available_space(const String self);
bool   string_eq          (const String lhs, const String rhs);

String string_append      (String *self, const String other);
String string_append_c_str(String *self, const char *c_str);
String string_append_char (String *self, char ch);
String string_append_len  (String *self, const void *p_bytes, isize n_bytes);

#ifdef PSEUDO_ODIN_IMPLEMENTATION

#include "allocator.hpp"
#include "string.hpp"

#endif // PSEUDO_ODIN_IMPLEMENTATION
