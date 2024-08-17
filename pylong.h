#include <stdalign.h>
#include <stdint.h>

/// --- cpython/Include/pytypedefs.h -------------------------------------- {{{1

typedef struct _object PyObject;
typedef struct _longobject PyLongObject;
typedef struct _typeobject PyTypeObject;
typedef intptr_t Py_ssize_t;

/// 1}}} -----------------------------------------------------------------------

/// --- cpython/Include/object.h ------------------------------------------ {{{1

struct _object {
    union {
        Py_ssize_t ob_refcnt;
        /* ...implementation-specific details... */
    };
    PyTypeObject *ob_type;
};

#define PyObject_HEAD   PyObject ob_base;

/// 1}}} -----------------------------------------------------------------------

/// --- cpython/Include/cpython/longintrepr.h ----------------------------- {{{1

typedef uint32_t digit;

struct _longobject {
    /* macro PyObject_HEAD -> PyObject ob_base; -> fields thereof */
    Py_ssize_t ob_refcnt;
    PyTypeObject *ob_type;

    /* _PyLongObject long_value; -> fields thereof */
    uintptr_t lv_tag;
    digit ob_digit[1];
};

/// 1}}} -----------------------------------------------------------------------
