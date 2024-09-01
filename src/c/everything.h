#ifndef EVERYTHING_H
#define EVERYTHING_H

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

typedef   uint8_t byte;
typedef    size_t usize;
typedef ptrdiff_t isize;

#define cast(Type)      (Type)
#define unused(Expr)    cast(void)(Expr)
#define size_of(Type)   cast(isize)(sizeof(Type))
#define align_of(Type)  cast(isize)(alignof(Type))

// Hope you're happy with many anonymous structs!
#define Slice(Type)                                                            \
struct {                                                                       \
    Type *data;                                                                \
    isize len;                                                                 \
}

typedef enum {
    ALLOCATOR_MODE_ALLOC,
    ALLOCATOR_MODE_RESIZE,
    ALLOCATOR_MODE_FREE,
} Allocator_Mode;

typedef struct {
    Allocator_Mode mode;
    isize          new_size;
    isize          align;
    void *         old_ptr;
    isize          old_size;
} Allocator_Proc_Args;

typedef void *(*Allocator_Proc)(void *udata, const Allocator_Proc_Args *args);

typedef struct {
    Allocator_Proc procedure;
    void *         userdata;
} Allocator;

void *raw_alloc(const Allocator *a, isize new_size, isize align);
void *raw_resize(const Allocator *a, void *old_ptr, isize old_size, isize new_size, isize align);
void  raw_free(const Allocator *a, void *old_ptr, isize old_size);

#define raw_alloc_item(Type, allocator)                                        \
    cast(Type *)(raw_alloc(allocator,                                          \
                           size_of(Type),                                      \
                           align_of(Type)))

#define raw_alloc_array(Type, allocator, new_cap)                              \
    cast(Type *)(raw_alloc(allocator,                                          \
                           size_of(Type) * (new_cap),                          \
                           align_of(Type)))

#define raw_resize_array(Type, allocator, old_ptr, old_cap, new_cap)           \
    cast(Type *)(raw_resize(allocator,                                         \
                            old_ptr,                                           \
                            size_of(Type) * (old_cap),                         \
                            size_of(Type) * (new_cap),                         \
                            align_of(Type)))

#define raw_free_array(Type, allocator, old_ptr, old_cap)                      \
    raw_free(allocator,                                                        \
             old_ptr,                                                          \
             size_of(Type) * (old_cap))

typedef struct {
    const Allocator *allocator;
    isize            len;
    isize            cap;
} Dynamic_Header;

// Same issue as `Slice`.
#define Dynamic_Array(Type)                                                    \
struct {                                                                       \
    Dynamic_Header header;                                                     \
    Type *         data;                                                       \
}

typedef Slice(const char)   String;
typedef Dynamic_Array(char) String_Builder;

#define string_literal(c_str)   {(c_str), size_of(c_str) - 1}

void dynamic_array__init(Dynamic_Header *self, const Allocator *a, isize type_size, isize count, isize type_align);
void dynamic_array__reserve(Dynamic_Header *self, isize type_size, isize new_cap, isize type_align);
void dynamic_array__free(Dynamic_Header *self, isize size);
void dynamic_array__append(Dynamic_Header *self, const void *value, isize tp_size, isize count, isize tp_align);

/**
 * @warning
 *      Makes use of the GNU extension that allows `_Alignof` to be applied to
 *      expressions.
 * 
 *      This will compile with GCC and Clang if the warning is disabled via the
 *      flag `-Wno-error=gnu-alignof-expression` but will not compile with MSVC.
 */
#define dynamic_array_init(self, allocator, count)                             \
    dynamic_array__init(&(self)->header,                                       \
                        allocator,                                             \
                        size_of((self)->data[0]),                              \
                        count,                                                 \
                        align_of((self)->data[0]))

#define dynamic_array_reserve(self, new_cap)                                   \
    dynamic_array__reserve(&(self)->header,                                    \
                           size_of((self)->data[0]),                           \
                           new_cap,                                            \
                           align_of((self)->data[0]))

#define dynamic_array_free(self)                                               \
    dynamic_array__free(&(self)->header, size_of((self)->data[0]))

#endif // EVERYTHING_H
