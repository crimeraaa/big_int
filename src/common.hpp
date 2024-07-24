#pragma once

#include <cstdlib>
#include <cstdio>
#include <cstring>

#define array_size(T, N)    (sizeof((T)[0]) * (N))

#ifdef __cplusplus
#   define cast(T, expr)    (static_cast<T>(expr))
#else
#   define cast(T, expr)    ((T)(expr))
#endif

#ifdef BIGINT_DEBUG
#   define DEBUG_PRINTF(fmt, ...) std::fprintf(stderr, "%-12s: " fmt, __func__, __VA_ARGS__)
#else   // _DEBUG not defined.
#   define DEBUG_PRINTF(fmt, ...)
#endif  // _DEBUG

#define DEBUG_PRINTFLN(fmt, ...)    DEBUG_PRINTF(fmt "\n", __VA_ARGS__)
#define DEBUG_PRINTLN(msg)          DEBUG_PRINTFLN("%s", msg)

struct LString {
    const char* data;
    size_t      length;
};
