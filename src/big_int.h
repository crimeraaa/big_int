#pragma once

/// local
#include "common.h"

typedef struct BigInt BigInt;
typedef u8 digit;

#define BIGINT_BASE     10
#define BIGINT_BUFSIZE  16

struct BigInt {
    size  length;
    size  capacity;
    digit digits[BIGINT_BUFSIZE];
};

#define BIGINT_INCLUDE_IMPLEMENTATION
#ifdef BIGINT_INCLUDE_IMPLEMENTATION

/// standard
#include <ctype.h>

/// local
#include "log.h"

#define WARN_RESIZE(self)   log_warnf("Need resize (length=%td, capacity=%td)", \
                                      (self)->length, (self)->capacity)
#define WARN_DIGIT(d)       log_warnf("Invalid digit '%hhu'.", (d))
#define WARN_POP()          log_warnln("Nothing to pop.")

static bool check_digit(digit d)
{
    return 0 <= d && d < BIGINT_BASE;
}

/**
 * EXAMPLE:
 *      push_left(self, d = 5)
 *
 * CONCEPTUALLY:
 *      1234 to 51234
 *
 * INTERNALLY:
 *      {4,3,2,1} to {4,3,2,1,5}
 */
static bool push_left(BigInt *self, digit d)
{
    // TODO: Dynamically grow buffer?
    log_tracecall();
    if (self->length >= self->capacity) {
        WARN_RESIZE(self);
        return false;
    } else if (!check_digit(d)) {
        WARN_DIGIT(d);
        return false;
    }
    self->digits[self->length++] = d;
    return true;
}

static digit pop_left(BigInt *self)
{
    if (self->length <= 0) {
        WARN_POP();
        return 0;
    }
    digit d = self->digits[self->length - 1];
    self->length -= 1;
    return d;
}

static bool shift_left1(BigInt *self)
{
    log_tracecall();
    if (self->length >= self->capacity) {
        WARN_RESIZE(self);
        return false;
    }
    // {4,3,2,1} to {4,3,2,1,0}
    self->length++;

    // {4,3,2,1,0} to {4,4,3,2,1}
    for (size i = self->length - 1; 1 <= i && i < self->capacity; i--) {
        self->digits[i] = self->digits[i - 1];
    }
    // {4,4,3,2,1} to {0,4,3,2,1}
    self->digits[0] = 0;
    return true;
}

static bool push_right(BigInt *self, digit d)
{
    log_tracecall();
    if (check_digit(d) && shift_left1(self)) {
        log_debugf("digits[0] = %hhu", d);
        self->digits[0] = d;
    } else {
        WARN_DIGIT(d);
    }
}

static bool shift_right1(BigInt *self)
{
    // Note how we DON'T modify digits[length - 1] in the loop.
    // {4,3,2,1} to {3,2,1,1}
    for (size i = 0, end = self->length - 1; 0 <= i && i < end; i++) {
        self->digits[i] = self->digits[i + 1];
    }

    // Ensure buffer is clean after shifting.
    // {3,2,1,1} -> {3,2,1,0}
    self->digits[self->length - 1] = 0; 

    // {3,2,1,1} -> {3,2,1}
    self->length -= 1; 
    return true;
}

static digit pop_right(BigInt *self)
{
    if (self->length <= 0) {
        WARN_POP();
        return 0;
    }
    digit d = self->digits[0];
    shift_right1(self);
    return d;
}

BigInt bigint_from_string(const char *s, size len)
{
    BigInt self;
    self.length   = 0;
    self.capacity = array_countof(self.digits);
    memset(self.digits, 0, sizeof(self.digits));
    // Iterate string in reverse to go from smallest place value to largest.
    for (size i = len - 1; i >= 0; i--) {
        char ch = s[i];
        // TODO: Parse better than this
        if (isdigit(ch))
            push_left(&self, ch - '0'); // We assume ASCII
    }
    return self;
}

/**
 * {4,3,2,1} prints out 1234
 */
void bigint_print(const BigInt *self)
{
    size len = self->length;
    size cap = self->capacity;
    printf("digits=");
    for (size i = len - 1; 0 <= i && i < cap; i--) {
        printf("%hhu", self->digits[i]);
    }
    printf(", length=%td, capacity=%td\n", len, cap);
}

#endif  // BIGINT_INCLUDE_IMPLEMENTATION
