#include "strings.hpp"

#include <cstring>

String string_from_cstring(cstring c_str)
{
    return {c_str, len(c_str)};
}

isize len(cstring c_str)
{
    const char *start = c_str;
    const char *stop  = c_str;
    while (stop[0] != '\0') {
        stop++;
    }
    return stop - start;
}

isize cstring_find_first_index_char(cstring c_str, char ch)
{
    return string_find_first_index_char(string_from_cstring(c_str), ch);
}

isize cstring_find_first_index_any(cstring c_str, cstring set)
{
    return string_find_first_index_any(
        string_from_cstring(c_str),
        string_from_cstring(set));
}

isize cstring_find_first_index_any(cstring c_str, const String &set)
{
    return string_find_first_index_any(string_from_cstring(c_str), set);
}

isize string_find_first_index_char(const String &self, char ch)
{
    for (isize i = 0; i < len(self); i++) {
        if (self[i] == ch) {
            return i;
        }        
    }
    return -1;
}

isize string_find_first_index_any(const String &self, cstring set)
{
    return string_find_first_index_any(self, {set, len(set)});
}

isize string_find_first_index_any(const String &self, const String &set)
{
    for (isize i = 0; i < len(set); i++) {
        isize ii = string_find_first_index_char(self, set[i]);
        if (ii != -1) {
            return ii;
        }
    }
    return -1;
}

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

void string_builder_reset(String_Builder *self)
{
    array_clear(&self->buffer);
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
    string_builder_append_string(self, string_from_cstring(c_str));
}

String string_builder_to_string(const String_Builder &self)
{
    return {cbegin(self.buffer), len(self.buffer)};
}

cstring string_builder_to_cstring(String_Builder *self)
{
    array_append(&self->buffer, '\0');
    array_pop(&self->buffer);
    return cbegin(self->buffer);
}
