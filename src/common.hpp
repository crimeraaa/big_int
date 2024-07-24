#pragma once

#include <cstdlib>
#include <stdexcept>

#define array_size(T, N)    (sizeof((T)[0]) * (N))

#ifdef BIGINT_DEBUG

// In `"bigint_*"`, extract `*` only.
#define DEBUG_PRINTF(fmt, ...) std::fprintf(stderr, "%-12s: " fmt, __func__, __VA_ARGS__)
#else // _DEBUG not defined.
#define DEBUG_PRINTF(fmt, ...)
#endif // _DEBUG

#define DEBUG_PRINTFLN(fmt, ...)    DEBUG_PRINTF(fmt "\n", __VA_ARGS__)
#define DEBUG_PRINTLN(msg)          DEBUG_PRINTFLN("%s", msg)
