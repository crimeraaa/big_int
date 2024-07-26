#pragma once

#include <stdalign.h>   //  alignof
#include <stdbool.h>    //  bool
#include <stdlib.h>     //  malloc family
#include <stdio.h>      //  printf family
#include <stdint.h>     //  [u]int family
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
#   define cast(T, expr)        (static_cast<T>(expr))
// https://news.ycombinator.com/item?id=18438288
#   define coerce(T, expr)      (reinterpret_cast<T>(expr))
#   define cmp_literal(T, ...)  {__VA_ARGS__}
#else   // __cplusplus not defined.
#   define nullptr              NULL
#   define cast(T, expr)        ((T)(expr))
#   define coerce(T, expr)      ((T)(expr))
#   define cmp_literal(T, ...)  (T){__VA_ARGS__}
#endif  // __cplusplus

#define fprintfln(stream, fmt, ...) fprintf(stream, fmt "\n", __VA_ARGS__)
#define fprintln(stream, s)         fprintfln(stream, "%s", s)
#define printfln(fmt, ...)          fprintfln(stdout, fmt, __VA_ARGS__)
#define println(s)                  printfln("%s", s)
#define eprintf(fmt, ...)           fprintf(stderr, fmt, __VA_ARGS__)
#define eprintfln(fmt, ...)         eprintf(fmt "\n", __VA_ARGS__)
#define eprintln(s)                 eprintfln("%s", s)

#ifdef DEBUG_USE_PRINT
#   define DEBUG_PRINTF(fmt, ...)   eprintf("[DEBUG] %-12s: " fmt, __func__, __VA_ARGS__)
#   define DEBUG_PRINTFLN(fmt, ...) DEBUG_PRINTF(fmt "\n", __VA_ARGS__)
#   define DEBUG_PRINTLN(msg)       DEBUG_PRINTFLN("%s", msg)
#else   // DEBUG_USE_PRINT not defined.
#   define DEBUG_PRINTF(fmt, ...)
#   define DEBUG_PRINTFLN(fmt, ...)
#   define DEBUG_PRINTLN(msg)
#endif  // DEBUG_USE_PRINT

#define array_literal(T, ...)       cmp_literal(T[], __VA_ARGS__)
#define array_sizeof(T, N)          (sizeof(T) * (N))
#define array_countof(T)            (sizeof(T) / sizeof((T)[0]))
#define fam_sizeof(T, memb, n)      (sizeof(T) + sizeof(memb) * (n))
#define fam_new(T, memb, n)         cast(T*, malloc(fam_sizeof(T, memb, n)))
#define fam_free(T, memb, ptr, n)   free(cast(void*, ptr))

#define unused(expr)                cast(void, expr)
#define unused2(x, y)               unused(x), unused(y)
#define unused3(x, y, z)            unused(x), unused2(y, z)

typedef unsigned char byte;
typedef     ptrdiff_t size;

typedef struct LString {
    const char *data;   // Read-only and non-owning view to some buffer.
    size        length; // Number of desired characters sans the nul terminator.
} LString;

#define lstr_make(s, n) cmp_literal(LString, s, n)
#define lstr_literal(s) lstr_make(s, array_countof(s) - 1)

typedef struct Buffer {
    size length;    // Number of desired characters, sans nul terminator.
    char data[];    // Read-write buffer.
} Buffer;
