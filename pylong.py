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


def split_cpython_int(value: int|str) -> list[int]:
    return PyLongObject(int(value)).make_digit_list()


def PyLongObject(value: int):
    """ 
    HACK: We create a new class definition for each and every call, because we
    need to determine the flexible array member length each time.
    
    FIXME: Saving the result to a variable may not work properly, e.g:

    ```py
    >>> n = PyLongObject(1_234_567_890)
    >>> n.make_digit_list()
    [77_525_024, 32_765]
    >>> PyLongObject(1_234_567_890).make_digit_list()
    [160_826_066, 1]
    >>> split_int(1_234_567_890)
    [160_826_066, 1]
    ```
    
    However, saving the integer to a variable beforehand seems to work:

    ```py
    >>> n_ = 1_234_567_890
    >>> n = PyLongObject(n_)
    >>> n.make_digit_list()
    [160_826_066, 1]
    ```
    
    This likely has to do with the `id()` call.
    """
    address = id(value)
    n_digits = helper.count_digits(value, PyLong_BASE)
    class _PyLongObject(c.Structure):
        """ 
        Verify our implementation against CPython's.

        See my notes in `python-int.h`.
        """
        _fields_ = [("ob_refcnt", c.c_ssize_t),
                    ("ob_type", c.c_void_p),
                    ("ob_size", c.c_ssize_t),
                    ("ob_digit", Digit * n_digits)]
        
        def __repr__(self):
            return (f"PyLongObject("
                    f"ob_refcnt = {self.ob_refcnt}, "
                    f"ob_type = {self.ob_type}, "
                    f"ob_size = {self.ob_size}, "
                    f"ob_digit = {self.ob_digit})")
        

        def make_digit_list(self):
            digits: list[int] = []
            for digit in self.ob_digit:
                digits.append(digit)
            return digits

    return _PyLongObject.from_address(address)

TEST_VALUE = 123_456_789_101_112_131_415
