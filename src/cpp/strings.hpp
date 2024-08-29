#pragma once

#include "cpp_odin.hpp"

/**
 * @brief
 *      A C-string is a pointer to an immutable sequence of char.
 * 
 * @warning
 *      Converting to a C string, by itself, does not guarantee nul termination.
 *
 * @note
 *      You can "call" this like `cstring(value)` for types that are convertable
 *      to `const char *`.
 *
 *      This is useful for classes that implement `operator const char *()`.
 */
using cstring = const char *;

/**
 * @brief
 *      A string is a pointer of known length to an immutable sequence of char.
 *
 * @warning
 *      Converting to a C string does not guarantee nul termination.
 *
 * @note
 *      You can use `cstring(str)` on a `String` instance to call the
 *      `operator const char *()` conversion method.
 */
using String = Slice<const char>;

#define string_literal(c_str)   {(c_str), size_of(c_str) - 1}

/**
 * @brief
 *      A string builder simply wraps a dynamic char array to be more opaque.
 * 
 * @note
 *      You are not intended to access the underlying buffer by yourself.
 *      Please use the `string_builder_*` functions.
 */
struct String_Builder {
    Dynamic_Array<char> buffer;
};

isize len(cstring cstr);

String_Builder string_builder_make         (Allocator a, isize len = 0, isize cap = 0);
void           string_builder_destroy      (String_Builder *self);
void           string_builder_reset        (String_Builder *self);
void           string_builder_grow         (String_Builder *self, isize new_cap);

isize          string_builder_cap          (const String_Builder &self);
String         string_builder_to_string    (const String_Builder &self);
cstring        string_builder_to_cstring   (const String_Builder &self);

isize          string_builder_write_char   (String_Builder *self, char ch);
isize          string_builder_write_bytes  (String_Builder *self, Slice<byte> bytes);
isize          string_builder_write_string (String_Builder *self, String str);
isize          string_builder_write_cstring(String_Builder *self, cstring cstr);

#ifdef PSEUDO_ODIN_IMPLEMENTATION

#include <cstring>

isize len(cstring cstr)
{
    return cast(isize)std::strlen(cstr);    
}

String_Builder string_builder_make(Allocator a, isize len, isize cap)
{
    return {dynamic_array_make<char>(a, len, cap)};
}

void string_builder_destroy(String_Builder *self)
{
    destroy(&self->buffer);
}

void string_builder_reset(String_Builder *self)
{
    clear(&self->buffer);
}

void string_builder_grow(String_Builder *self, isize new_cap)
{
    if (new_cap > cap(self->buffer)) {
        reserve(&self->buffer, new_cap);
    }
}

isize string_builder_len(const String_Builder &self)
{
    return len(self.buffer);
}

isize string_builder_cap(const String_Builder &self)
{
    return cap(self.buffer);
}

String string_builder_to_string(const String_Builder &self)
{
    return {cstring(self.buffer), len(self.buffer)};
}

cstring string_builder_to_cstring(const String_Builder &self)
{
    return &self.buffer[0];
}

isize string_builder_write_char(String_Builder *self, char ch)
{
    append(&self->buffer, ch);
    append(&self->buffer, '\0');
    pop(&self->buffer);
    return 1;
}

isize string_builder_write_bytes(String_Builder *self, Slice<byte> bytes)
{
    append(&self->buffer, slice_cast<char>(bytes));
    append(&self->buffer, '\0');
    pop(&self->buffer);
    return len(bytes);
}

isize string_builder_write_string(String_Builder *self, String str)
{
    append(&self->buffer, str);
    append(&self->buffer, '\0');
    pop(&self->buffer);
    return len(str);
}

isize string_builder_write_cstring(String_Builder *self, cstring cstr)
{
    return string_builder_write_string(self, {cstr, len(cstr)});
}

#endif // PSEUDO_ODIN_IMPLEMENTATION
