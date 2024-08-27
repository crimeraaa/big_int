#pragma once

#ifdef STRING_INCLUDE_IMPLEMENTATION
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
 *      
 * @note
 *      This simply wraps `Dynamic_Array<char>` to be more opaque.
 */
struct String_Builder {
    Dynamic_Array<char> buffer;
};

// Buffer creation
String_Builder string_builder_make(const Allocator& allocator);
String_Builder string_builder_make(isize n_len, const Allocator& allocator);

// Buffer deletion
void string_builder_destroy(String_Builder* self);
void string_builder_reset(String_Builder* self);

// Buffer information
isize string_builder_len(const String_Builder& self);
isize string_builder_cap(const String_Builder& self);

// Buffer manipulation
void string_builder_write_char(String_Builder* self, char ch);
void string_builder_write_string(String_Builder* self, String str);
void string_builder_write_cstring(String_Builder* self, const char* c_str);

/**
 * @brief
 *      Creates a `String` from the underlying buffer.
 * 
 * @note
 *      The resulting string is only valid as long as the `String_Builder` and
 *      its underlying buffer are also valid.
 */
String string_builder_to_string(const String_Builder& self);

/**
 * @brief
 *      Appends a nul character to the buffer and returns the buffer.
 *      
 * @note
 *      We do pop the nul character right after appending it. This means that
 *      the resulting cstring is valid until the next buffer write.
 * 
 *      The moment something else is written to the buffer, you're out of luck.
 */
const char* string_builder_to_cstring(String_Builder* self);

// C String utility
isize len(const char* msg);

#ifdef STRING_INCLUDE_IMPLEMENTATION

#include <cstring>

String_Builder string_builder_make(const Allocator& allocator)
{
    return string_builder_make(0, allocator);
}

String_Builder string_builder_make(isize n_len, const Allocator& allocator)
{
    return {dynamic_array_make<char>(0, n_len, allocator)};
}

void string_builder_destroy(String_Builder* self)
{
    destroy(&self->buffer);
}

void string_builder_reset(String_Builder* self)
{
    clear(&self->buffer);
}

isize string_builder_len(const String_Builder& self)
{
    return len(self.buffer);
}

isize string_builder_cap(const String_Builder& self)
{
    return cap(self.buffer);
}

void string_builder_write_char(String_Builder* self, char ch)
{
    append(&self->buffer, ch);
}

void string_builder_write_string(String_Builder* self, String str)
{
    // for (char ch : str) {
    //     append(&self->buffer, ch);
    // }
    for (isize i = 0; i < len(str); i += 1) {
        append(&self->buffer, str[i]);
    }
}

void string_builder_write_cstring(String_Builder* self, const char* c_str)
{
    string_builder_write_string(self, {c_str, len(c_str)});
}

String string_builder_to_string(const String_Builder& self)
{
    return {&self.buffer[0], string_builder_len(self)};
}

const char* string_builder_to_cstring(String_Builder* self)
{
    append(&self->buffer, '\0');
    pop(&self->buffer);
    return &self->buffer[0];
}

isize len(const char* c_str)
{
    return static_cast<isize>(std::strlen(c_str));
}

#endif // STRING_INCLUDE_IMPLEMENTATION

