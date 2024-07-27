#pragma once

#include <stdalign.h>   //  alignof
#include <stdbool.h>    //  bool
#include <stdint.h>     //  [u]int family
#include <stdio.h>      //  printf family
#include <stdlib.h>     //  malloc family
#include <string.h>     //  str* and mem* families

/*
C4200
    nonstandard extension used: zero-sized array in struct/union
    Even though flexible array members are part of the C11 standard...
C4996
    '<func>': This function or variable may be unsafe. Consider using '<func>_s'
    instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS.
    Yeah, thanks but no thanks!
 */
#ifdef _MSC_VER
#   pragma warning( disable : 4200 4996 )
#endif

#ifdef __cplusplus
#   define cast(T, expr)            (static_cast<T>(expr))
// https://news.ycombinator.com/item?id=18438288
#   define coerce(T, expr)          (reinterpret_cast<T>(expr))
#   define cast_ptr(T, expr)        (static_cast<T*>(expr))
#   define cmp_literal(T, ...)      {__VA_ARGS__}
#else   // __cplusplus not defined.
#   define nullptr                  NULL
#   define cast(T, expr)            ((T)(expr))
#   define coerce(T, expr)          ((T)(expr))
#   define cast_ptr(T, expr)        ((T*)(expr))
#   define cmp_literal(T, ...)      (T){__VA_ARGS__}
#endif  // __cplusplus

#define fprintfln(stream, fmt, ...) fprintf(stream, fmt "\n", __VA_ARGS__)
#define fprintln(stream, s)         fprintfln(stream, "%s", s)
#define printfln(fmt, ...)          fprintfln(stdout, fmt, __VA_ARGS__)
#define println(s)                  printfln("%s", s)
#define eprintf(fmt, ...)           fprintf(stderr, fmt, __VA_ARGS__)
#define eprintfln(fmt, ...)         eprintf(fmt "\n", __VA_ARGS__)
#define eprintln(s)                 eprintfln("%s", s)

#ifdef DEBUG_USE_PRINT
#   define dprintf(fmt, ...)        eprintf("[DEBUG] %-12s: " fmt, __func__, __VA_ARGS__)
#   define dprintfln(fmt, ...)      dprintf(fmt "\n", __VA_ARGS__)
#   define dprintln(msg)            dprintfln("%s", msg)
#else   // DEBUG_USE_PRINT not defined.
#   define dprintf(fmt, ...)
#   define dprintfln(fmt, ...)
#   define dprintln(msg)
#endif  // DEBUG_USE_PRINT

#define array_literal(T, ...)       cmp_literal(T[], __VA_ARGS__)
#define array_sizeof(T, N)          (sizeof(T) * (N))
#define array_countof(T)            (sizeof(T) / sizeof((T)[0]))
#define fam_sizeof(T, memb, n)      (sizeof(T) + sizeof(memb) * (n))
#define fam_new(T, memb, n)         cast(T*, malloc(fam_sizeof(T, memb, n)))

// Horrible horribly preprocessor trickery!
#define x__xselect3(_1,_2,_3,...)           _3
#define x__xselect4(_1,_2,_3,_4,...)        _4
#define x__xselect5(_1,_2,_3,_4,_5,...)     _5
#define x__xselect6(_1,_2,_3,_4,_5,_6,...)  _6

#define x__xunused1(x)      cast(void, x)
#define x__xunused2(x, y)   x__xunused1(x), x__xunused1(y)
#define x__xunused3(x,...)  x__xunused1(x), x__xunused2(__VA_ARGS__)
#define unused(...) \
    x__xselect4(__VA_ARGS__, x__xunused3, x__xunused2, x__xunused1)(__VA_ARGS__)

typedef       uint8_t u8;
typedef      uint16_t u16;
typedef      uint32_t u32;
typedef      uint64_t u64;
typedef        int8_t i8;
typedef       int16_t i16;
typedef       int32_t i32;
typedef       int64_t i64;
typedef         float f32;
typedef        double f64;
typedef unsigned char byte;
typedef     ptrdiff_t size;

typedef struct StringView {
    const char *data;   // Read-only and non-owning view to some buffer.
    size        length; // Number of desired characters sans the nul terminator.
} StringView;

#define lstr_make(s, n) cmp_literal(StringView, s, n)
#define lstr_literal(s) lstr_make(s, array_countof(s) - 1)

typedef struct FamString {
    size length;    // Number of desired characters, sans nul terminator.
    char data[];    // Read-write buffer.
} FamString;

typedef struct I32Array {
    i32 *data;
    size length;
    size capacity;
} I32Array;

// Used mainly for `arena_rawgrow_array()`.
typedef struct RawBuffer {
    void *data;     // Must be convertible to any other data pointer.
    size  length;
    size  capacity;
} RawBuffer;
