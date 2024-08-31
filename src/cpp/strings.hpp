#pragma once

#include "odin.hpp"
#include "mem.hpp"
#include "array.hpp"

using String = Slice<const char>;

#define string_literal(c_str)   {(c_str), size_of(c_str) - 1}

/**
 * @brief
 *      A string builder simply wraps a dynamic array of `char` to be more
 *      opaque.
 *
 * @note
 *      When initializing, `len` refers to the current number of valid indexable
 *      characters. If you initialize with a nonzero `len` this will affect how
 *      things are appended.
 */
struct String_Builder {
    Array<char> buffer;
};

void    string_builder_init          (String_Builder *self, const Allocator &a);
void    string_builder_init          (String_Builder *self, const Allocator &a, isize len);
void    string_builder_init          (String_Builder *self, const Allocator &a, isize len, isize cap);
void    string_builder_free          (String_Builder *self);
isize   string_builder_len           (const String_Builder &self);
isize   string_builder_cap           (const String_Builder &self);
void    string_builder_append_char   (String_Builder *self, char ch);
void    string_builder_append_string (String_Builder *self, const String &str);
void    string_builder_append_cstring(String_Builder *self, cstring c_str);
String  string_builder_to_string     (const String_Builder &self);
cstring string_builder_to_cstring    (String_Builder *self);
