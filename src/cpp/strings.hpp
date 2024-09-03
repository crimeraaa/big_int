#pragma once

#include "odin.hpp"

/**
 * @brief
 *      A read-only view.
 * 
 * @note
 *      The underlying `data` pointer is not guaranteed to be nul terminated.
 */
using String = Slice<const char>;

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

// Global C string utility.
isize len(cstring c_str);

#define string_literal(c_str) {static_cast<cstring>(c_str), size_of(c_str) - 1}

String string_from_cstring(cstring c_str);

template<class T>
String string_from_slice(const Slice<T> &self, isize start, isize stop)
{
    return slice(static_cast<const char *>(self.data), len(self), start, stop);
}

isize cstring_find_first_index_char(cstring c_str, char ch);
isize cstring_find_first_index_any(cstring c_str, cstring set);
isize cstring_find_first_index_any(cstring c_str, const String &set);

/**
 * @brief
 *      Return the index of the first occurence of `ch` in `self`, else -1.
 */
isize string_find_first_index_char(const String &self, char ch);

/**
 * @brief
 *      Return the index of the first occurence of any character in `set` within
 *      `self`, else -1.
 */
isize string_find_first_index_any(const String &self, const String &set);
isize string_find_first_index_any(const String &self, cstring set);

// Initialization functions
void string_builder_init(String_Builder *self, const Allocator &a);
void string_builder_init(String_Builder *self, const Allocator &a, isize len);
void string_builder_init(String_Builder *self, const Allocator &a, isize len, isize cap);
void string_builder_free(String_Builder *self);
void string_builder_reset(String_Builder *self);

// Info functions
isize string_builder_len(const String_Builder &self);
isize string_builder_cap(const String_Builder &self);

// Buffer manipulation
void string_builder_append_char(String_Builder *self, char ch);
void string_builder_append_string(String_Builder *self, const String &str);
void string_builder_append_cstring(String_Builder *self, cstring c_str);

// Conversion
String string_builder_to_string(const String_Builder &self);

/**
 * @brief
 *      Nul terminates the underlying buffer and returns a read-only pointer.
 *
 * @note
 *      The nul termination is only guaranteed as long as there are no further
 *      writes to the builder.
 */
cstring string_builder_to_cstring(String_Builder *self);
