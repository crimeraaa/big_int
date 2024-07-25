#pragma once

#include <stdalign.h>   //  alignof
#include <stdlib.h>     //  malloc family
#include <stdio.h>      //  printf family
#include <stdint.h>     //  [u]int family
#include <string.h>     //  str* and mem* families

/*
C4200
    nonstandard extension used: zero-sized array in struct/union
    Even though flexible array members are part of the C11 standard...
 */
#ifdef _MSC_VER
#   pragma warning( disable : 4200 )
#endif

#ifdef __cplusplus
#   define cast(T, expr)        (static_cast<T>(expr))
#   define cmp_literal(T, ...)  {__VA_ARGS__}
#else   // __cplusplus not defined.
#   define nullptr              NULL
#   define cast(T, expr)        ((T)(expr))
#   define cmp_literal(T, ...)  (T){__VA_ARGS__}
#endif  // __cplusplus

#ifdef DEBUG_USE_PRINT
#   define DEBUG_PRINTF(fmt, ...)   fprintf(stderr, "%-12s: " fmt, __func__, __VA_ARGS__)
#else   // DEBUG_USE_PRINT not defined.
#   define DEBUG_PRINTF(fmt, ...)
#endif  // DEBUG_USE_PRINT

#define DEBUG_PRINTFLN(fmt, ...)    DEBUG_PRINTF(fmt "\n", __VA_ARGS__)
#define DEBUG_PRINTLN(msg)          DEBUG_PRINTFLN("%s", msg)

#define array_literal(T, ...)       cmp_literal(T[], __VA_ARGS__)
#define array_sizeof(T, N)          (sizeof((T)[0]) * (N))
#define array_countof(T)            (sizeof(T) / sizeof((T)[0]))
#define fam_sizeof(T, memb, n)      (sizeof(T) + array_sizeof(memb, n))
#define fam_new(T, memb, n)         cast(T*, malloc(fam_sizeof(T, memb, n)))

typedef   uint8_t byte;
typedef ptrdiff_t size;

typedef struct LString {
    const char *data;   // Read-only and non-owning view to some buffer.
    size        length; // Number of desired characters sans the nul terminator.
} LString;

#define lstr_make(s, n) cmp_literal(LString, s, n)
#define lstr_literal(s) lstr_make(s, array_countof(s) - 1)
