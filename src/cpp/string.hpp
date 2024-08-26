#pragma once

#ifdef  STRING_INCLUDE_IMPLEMENTATION
#define ALLOCATOR_INCLUDE_IMPLEMENTATION
#endif // STRING_INCLUDE_IMPLEMENTATION

#include "dynamic_array.hpp"
#include "slice.hpp"

/**
 * @brief
 *      An immutable sequence of `char`.
 *
 * @note
 *      The data being pointed at need not be nul terminated.
 */
using String = Slice<const char>;

/**
 * @brief
 *      A mutable and growable sequence of `char`.
 */
using String_Builder = Dynamic_Array<char>;

String_Builder string_builder_make(const Allocator& allocator);
String_Builder string_builder_make(isize n_len, const Allocator& allocator);
void           string_builder_destroy(String_Builder* self);

void    string_builder_write_char(String_Builder* self, char ch);
void    string_builder_write_string(String_Builder* self, String str);
void    string_builder_write_cstring(String_Builder* self, const char* c_str);
String  string_builder_to_string(const String_Builder& self);
void    string_builder_reset(String_Builder* self);

#ifdef STRING_INCLUDE_IMPLEMENTATION

#include <cstring>

String_Builder string_builder_make(const Allocator& allocator)
{
    return string_builder_make(0, allocator);
}

String_Builder string_builder_make(isize n_len, const Allocator& allocator)
{
    return dynamic_array_make<char>(0, n_len, allocator);
}

void string_builder_destroy(String_Builder* self)
{
    destroy(self);
}

String string_builder_to_string(const String_Builder& self)
{
    String ret{&self[0], len(self)};
    return ret;
}

void string_builder_reset(String_Builder* self)
{
    clear(self);
}

void string_builder_write_char(String_Builder* self, char ch)
{
    append(self, ch);
}

void string_builder_write_string(String_Builder* self, String str)
{
    for (char ch : str) {
        append(self, ch);
    }
}

void string_builder_write_cstring(String_Builder* self, const char* c_str)
{
    isize n_len = isize(std::strlen(c_str));
    string_builder_write_string(self, {c_str, n_len});
}
#endif // STRING_INCLUDE_IMPLEMENTATION

