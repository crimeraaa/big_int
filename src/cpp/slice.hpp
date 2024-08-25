#pragma once

#include "pointer.hpp"

/**
 * @brief
 *      A non-owning view into some memory. You may adjust the view as needed.
 * 
 * @note
 *      The data being viewed is not necessarily immutable.
 */
template<class T>
struct Slice : public Pointer<T> {

    bool operator==(const Slice<T>& rhs) const
    {
        if (this->length != rhs.length) {
            return false;
        }
        for (isize i = 0; i < this->length; i += 1) {
            if (this.start[i] != rhs.start[i]) {
                return false;
            }
        }
        return true;
    }
    
    bool operator!=(const Slice<T>& rhs) const
    {
        return !operator==(rhs);
    }
};

/**
 * @brief
 *      An immutable sequence of `char`.
 *
 * @note
 *      The data being pointed at need not be nul terminated.
 */
using String = Slice<const char>;

template<class T>
isize len(const Slice<T>& self)   { return self.length; }

template<class T>
isize len(const Slice<T>* self)   { return self->length; }
