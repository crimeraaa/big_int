#include "strings.hpp"

#include <cstring>

void string_builder_init(String_Builder *self, const Allocator &a)
{
    string_builder_init(self, a, 0, 0);
}

void string_builder_init(String_Builder *self, const Allocator &a, isize len)
{
    string_builder_init(self, a, len, len);
}

void string_builder_init(String_Builder *self, const Allocator &a, isize len, isize cap)
{
    array_init(&self->buffer, a, len, cap);
}

void string_builder_free(String_Builder *self)
{
    array_free(&self->buffer);
}

isize string_builder_len(const String_Builder &self)
{
    return len(self.buffer);
}

isize string_builder_cap(const String_Builder &self)
{
    return cap(self.buffer);
}

void string_builder_append_char(String_Builder *self, char ch)
{
    array_append(&self->buffer, ch);
}

void string_builder_append_string(String_Builder *self, const String &str)
{
    array_append(&self->buffer, str);
}

void string_builder_append_cstring(String_Builder *self, cstring c_str)
{
    string_builder_append_string(self, {c_str, static_cast<isize>(std::strlen(c_str))});
}

String string_builder_to_string(const String_Builder &self)
{
    return {cbegin(self.buffer), len(self.buffer)};
    // lol
    // return slice_const_cast(slice(self.buffer, 0, len(self.buffer)));
}

cstring string_builder_to_cstring(String_Builder *self)
{
    array_append(&self->buffer, '\0');
    array_pop(&self->buffer);
    return cbegin(self->buffer);
}
