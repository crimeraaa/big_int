#pragma once

#include "allocator.hpp"

#include <cstring>

String string_make(Allocator a, const char *c_str)
{
    isize n_len = c_str ? cast(isize)std::strlen(c_str) : 0;
    return string_make_len(a, c_str, n_len);
}

String string_make_len(Allocator a, const void *p_bytes, isize n_bytes)
{
    Dynamic_Header *hdr;
    String str;
    isize  sz_hdr = size_of(*hdr);
    isize  sz_all = sz_hdr + n_bytes + 1;
    void  *ptr    = allocate(a, sz_all);
    
    // Can only happen when `a` has no error handler
    if (!ptr) {
        return nullptr;
    }
    
    str = cast(char *)ptr + sz_hdr;
    hdr = DYNAMIC_HEADER(str);

    hdr->allocator = a;
    hdr->length    = n_bytes;
    hdr->capacity  = n_bytes;
    
    if (p_bytes) {
        std::memcpy(str, p_bytes, n_bytes);
    }
    
    str[n_bytes] = '\0';
    return str;
}

String string_make_reserve(Allocator a, isize n_cap)
{
    Dynamic_Header *hdr;
    String str;
    
    isize sz_hdr = size_of(*hdr);
    isize sz_all = sz_hdr + n_cap + 1;
    void *ptr    = allocate(a, sz_all);
    
    // Can only occur when `a` has no error handler.
    if (!ptr) {
        return nullptr;
    }
    std::memset(ptr, 0, sz_all);
    
    str = cast(char*)ptr + sz_hdr;
    hdr = DYNAMIC_HEADER(str);

    hdr->allocator = a;
    hdr->length    = 0;
    hdr->capacity  = n_cap;
    str[n_cap]     = '\0';

    return str;
}

void string_delete(String *self)
{
    Dynamic_Header *hdr;
    String str;
    isize  sz_used;

    str     = *self;
    hdr     = DYNAMIC_HEADER(str);
    sz_used = size_of(*hdr) + string_cap(str) + 1;

    deallocate(hdr->allocator, hdr, sz_used);
}

void string_clear(String *self)
{
    Dynamic_Header *hdr;
    String str;

    str = *self;
    hdr = DYNAMIC_HEADER(str);

    hdr->length = 0;
    str[0]      = '\0';
}

static isize next_power_of_2(isize target)
{
    isize n = 1;
    while (n < target) {
        n *= 2;
    }
    return n;
}

String string_resize(String *self, isize n_len)
{
    String str;
    Dynamic_Header *hdr;

    str = *self;
    hdr = DYNAMIC_HEADER(str);
    
    // Already enough space to accomodate `n_len`.    
    if (string_cap(str) > n_len) {
        return str;
    } else {
        Allocator a   = hdr->allocator;
        isize sz_hdr  = size_of(*hdr);

        isize len_old = string_len(str);
        isize sz_old  = sz_hdr + len_old + 1;

        isize len_new = next_power_of_2(len_old + n_len);
        isize sz_new  = sz_hdr + len_new + 1;
        
        void *ptr = reallocate(a, hdr, sz_old, sz_new);
        if (!ptr) {
            return nullptr;
        }
        str = cast(char*)ptr + sz_hdr;
        hdr = DYNAMIC_HEADER(str);
        hdr->allocator = a;
        hdr->capacity  = len_new;
        *self = str;
    }
    return str;
}

isize string_len(const String self)
{
    return DYNAMIC_HEADER(self)->length;
}

isize string_cap(const String self)
{
    return DYNAMIC_HEADER(self)->capacity;
}

isize string_available_space(const String self)
{
    Dynamic_Header *hdr = DYNAMIC_HEADER(self);
    if (hdr->capacity > hdr->length) {
        return hdr->capacity - hdr->length;
    }
    return 0;
}

bool string_eq(const String lhs, const String rhs)
{
    isize len_lhs = string_len(lhs);
    isize len_rhs = string_len(rhs);
    if (len_lhs != len_rhs) {
        return false;
    }
    
    for (isize i = 0; i < len_lhs; i++) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

String string_append_len(String *self, const void *p_bytes, isize n_bytes)
{
    String str = *self;
    if (str && n_bytes > 0) {
        isize len_old = string_len(str);
        isize len_new = len_old + n_bytes;
        
        str = string_resize(self, len_new);
        if (!str) {
            return nullptr;
        }
        DYNAMIC_HEADER(str)->length = len_new;
        std::memcpy(&str[len_old], p_bytes, n_bytes);
        str[len_new] = '\0';
    }
    return str;
}

String string_append_char(String *self, char ch)
{
    char buf[1] = {ch};
    return string_append_len(self, buf, size_of(buf));
}

String string_append(String *self, const String other)
{
    return string_append_len(self, other, string_len(other));
}

String string_append_c_str(String *self, const char *c_str)
{
    isize n_len = c_str ? cast(isize)std::strlen(c_str) : 0;
    return string_append_len(self, c_str, n_len);
}
