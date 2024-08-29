#pragma once

#include "cpp_odin.hpp"

/**
 * @brief
 *      A slice is simply a typed pointer with an associated length.
 *      You can index it and compare it against other slices like you would a
 *      normal raw C pointer.
 * 
 * @note
 *      The data being pointed to is not necessarily immutable.
 */
template<class T>
struct Slice : public Basic_Array<T>
{};

template<class T>
isize slice_len(const Slice<T> &self)
{
    return self.length;
}

template<class T>
isize slice_len(const Slice<T> *self)
{
    return self->length;
}

/**
 * @brief
 *      Reinterpet the data being pointed to by `self`.
 * 
 * @warning
 *      You must have a VERY good reason to do this!
 */
template<class To, class From>
Slice<To> slice_cast(const Slice<From> &self)
{
    return {cast(To *)self.data, len(self)};
}

///--- GLOBAL UTILITY ----------------------------------------------------- {{{1

template<class T>
isize len(const Slice<T> &self)
{
    return slice_len(self);
}

template<class T>
isize len(const Slice<T> *self)
{
    return slice_len(self);
}

template<typename T>
bool operator==(const Slice<T> &lhs, const Slice<T> &rhs)
{
    isize len_lhs = lhs.length;
    isize len_rhs = rhs.length;
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

template<typename T>
bool operator!=(const Slice<T> &lhs, const Slice<T> &rhs)
{
    return !operator==(lhs, rhs);
}

///--- 1}}} --------------------------------------------------------------------
