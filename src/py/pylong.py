""" 
See the CPython definitions needed for Python's `int` type:
https://github.com/python/cpython/blob/main/Include/cpython/longintrepr.h

Also see:
https://rushter.com/blog/python-integer-implementation/
https://dev.to/artyomkaltovich/how-the-integer-type-is-implemented-in-cpython-8ei
"""
from ctypes import c_uint32 as Digit, c_uint64 as TwoDigits
import ctypes as c
import helper

PyLong_SHIFT = 30
PyLong_DECIMAL_SHIFT = 9
PyLong_DECIMAL_BASE = 10 ** PyLong_DECIMAL_SHIFT
PyLong_BASE = 1 << PyLong_SHIFT
PyLong_MASK = PyLong_BASE - 1

class PyLongObject(c.Structure):
    """ 
    Verify our implementation against CPython's.

    See my notes in `python-int.h`.
    """
    _fields_ = [("ob_refcnt", c.c_ssize_t),
                ("ob_type", c.c_void_p),
                ("ob_size", c.c_ssize_t),
                ("ob_digit", Digit)] # Actually a flexible array member
    
    def __repr__(self):
        return (f"PyLongObject("
                f"ob_refcnt = {self.ob_refcnt}, "
                f"ob_type = {self.ob_type}, "
                f"ob_size = {self.ob_size}, "
                f"ob_digit = {self.ob_digit})")
    

    def make_digit_list(self) -> list[int]:
        digits: list[int] = []
        ptr_digit = self.get_digit_pointer()
        for index in range(abs(self.ob_size)):
            digits.append(ptr_digit[index])
        return digits


    def get_digit_pointer(self):
        """ 
        Get the address of `self.ob_digit`.
        
        WARNING:
            Makes assumptions about the offset of `self.ob_digit`.
            See the possible output of the following Python code:

            ```py
            >>> PyLongObject
            <class '__main__.PyLongObject'>
            >>> PyLongObject.ob_refcnt
            <Field type=c_longlong, ofs=0, size=8>
            >>> PyLongObject.ob_type  
            <Field type=c_void_p, ofs=8, size=8>
            >>> PyLongObject.ob_size
            <Field type=c_longlong, ofs=16, size=8>
            >>> PyLongObject.ob_digit
            <Field type=c_ulong, ofs=24, size=4>
            ```
        """
        addr_digit = c.byref(self, 24)
        ptr_type = c.POINTER(Digit)
        ptr_digit = c.cast(addr_digit, ptr_type)
        return ptr_digit


def split_int(value: int) -> list[int]:
    value = abs(value)
    digits: list[int] = []
    while value != 0:
        # == `value % (DIGIT_MASK + 1)`
        digits.append(value & PyLong_MASK)

        # == `value // (DIGIT_MASK + 1)`
        value >>= PyLong_SHIFT
    return digits


def unsplit_int(digits: list[int]) -> int:
    final_value: int = 0
    for index, digit in enumerate(digits):
        place_value: int = 2 ** (PyLong_SHIFT * index)
        final_value += digit * place_value
    return final_value


def make_cpython_int(value: int|str) -> PyLongObject:
    return PyLongObject.from_address(id(int(value)))


def split_cpython_int(value: int|str) -> list[int]:
    return make_cpython_int(value).make_digit_list()


TEST_VALUE = 123_456_789_101_112_131_415
