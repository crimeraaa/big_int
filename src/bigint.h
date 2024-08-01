#pragma once

/// local
#include "common.h"

typedef struct BigInt BigInt;
typedef i8 digit;

#define BIGINT_BASE     10
#define BIGINT_BUFSIZE  16

struct BigInt {
    size  length;
    size  capacity;
    digit digits[BIGINT_BUFSIZE];
};

void   bigint_print(const BigInt *self);
BigInt bigint_from_int(int n);
BigInt bigint_from_string(const char *s, size len);
BigInt bigint_from_cstring(const char *cstr);

void   bigint_iadd(BigInt *self, int addend);
void   bigint_isub(BigInt *self, int subtrahend);

#ifdef BIGINT_INCLUDE_IMPLEMENTATION

/// standard
#include <ctype.h>

/// local
#include "log.h"

#define WARN_RESIZE(self)   log_warnf("Need resize (length=%td, capacity=%td)", \
                                      (self)->length, (self)->capacity)
#define WARN_INDEX(i)       log_warnf("Invalid index '%td'.", (i))
#define WARN_DIGIT(d)       log_warnf("Invalid digit '%i'.", (d))
#define WARN_POP()          log_warnln("Nothing to pop.")

static const digit DIGIT_SUMS[BIGINT_BASE][BIGINT_BASE] = {
    {0,  1,  2,  3,  4,  5,  6,  7,  8,  9},
    {1,  2,  3,  4,  5,  6,  7,  8,  9, 10},
    {2,  3,  4,  5,  6,  7,  8,  9, 10, 11},
    {3,  4,  5,  6,  7,  8,  9, 10, 11, 12},
    {4,  5,  6,  7,  8,  9, 10, 11, 12, 13},
    {5,  6,  7,  8,  9, 10, 11, 12, 13, 14},
    {6,  7,  8,  9, 10, 11, 12, 13, 14, 15},
    {7,  8,  9, 10, 11, 12, 13, 14, 15, 16},
    {8,  9, 10, 11, 12, 13, 14, 15, 16, 17},
    {9, 10, 11, 12, 13, 14, 15, 16, 17, 18},
};

static const digit DIGIT_DIFFS[BIGINT_BASE][BIGINT_BASE] = {
    {0, -1, -2, -3, -4, -5, -6, -7, -8, -9},
    {1,  0, -1, -2, -3, -4, -5, -6, -7, -8},
    {2,  1,  0, -1, -2, -3, -4, -5, -6, -7},
    {3,  2,  1,  0, -1, -2, -3, -4, -5, -6},
    {4,  3,  2,  1,  0, -1, -2, -3, -4, -5},
    {5,  4,  3,  2,  1,  0, -1, -2, -3, -4},
    {6,  5,  4,  3,  2,  1,  0, -1, -2, -3},
    {7,  6,  5,  4,  3,  2,  1,  0, -1, -2},
    {8,  7,  6,  5,  4,  3,  2,  1,  0, -1},
    {9,  8,  7,  6,  5,  4,  3,  2,  1,  0},
};

static bool check_digit(digit dgt)
{
    return -(BIGINT_BASE-1) <= dgt && dgt <= BIGINT_BASE-1;
}

static bool check_index(const BigInt *self, size idx)
{
    return 0 <= idx && idx < self->capacity;
}

// Conceptually, reading out of bounds is not an error since it's all just 0's.
static digit read_at(const BigInt *self, size idx)
{
    return check_index(self, idx) ? self->digits[idx] : 0;
}

static bool write_at(BigInt *self, size idx, digit dgt)
{
    if (!check_index(self, idx)) {
        WARN_INDEX(idx);
        return false;
    } else if (!check_digit(dgt)) {
        WARN_DIGIT(dgt);
        return false;
    }
    self->digits[idx] = dgt;        
    if (idx >= self->length)
        self->length = idx + 1;
    return true;
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
static bool push_left(BigInt *self, digit dgt)
{
    // TODO: Dynamically grow buffer?
    if (self->length >= self->capacity) {
        WARN_RESIZE(self);
        return false;
    } else if (!check_digit(dgt)) {
        WARN_DIGIT(dgt);
        return false;
    }
    self->digits[self->length++] = dgt;
    return true;
}

/**
 * CONCEPTUALLY:
 *      1234 to 234
 *      
 * INTERNALLY:
 *      {4,3,2,1} to {4,3,2}
 */
static digit pop_left(BigInt *self)
{
    if (self->length <= 0) {
        WARN_POP();
        return 0;
    }
    digit dgt = self->digits[self->length - 1];
    self->length -= 1;
    return dgt;
}

/**
 * CONCEPTUALLY:
 *      1234 to 12340
 *      
 * INTERNALLY:
 *      {4,3,2,1} to {0,4,3,2,1}
 */
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

static bool push_right(BigInt *self, digit dgt)
{
    log_tracecall();
    if (check_digit(dgt) && shift_left1(self)) {
        log_debugf("digits[0] = %i", dgt);
        self->digits[0] = dgt;
    } else {
        WARN_DIGIT(dgt);
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
    digit dgt = self->digits[0];
    shift_right1(self);
    return dgt;
}

static void bigint_init(BigInt *self)
{
    log_tracecall();
    self->length   = 0;
    self->capacity = array_countof(self->digits);
    memset(self->digits, 0, sizeof(self->digits));
}

BigInt bigint_from_int(int n)
{
    BigInt self;
    log_tracecall();
    bigint_init(&self);

    // 13 / 10 = 1.3 = 1
    for (int it = n; it > 0; it /= BIGINT_BASE) {
        digit dgt = it % BIGINT_BASE; // 13 % 10 = 3
        push_left(&self, dgt);
    }
    
    return self;
}

BigInt bigint_from_cstring(const char *cstr)
{
    return bigint_from_string(cstr, strlen(cstr));
}

BigInt bigint_from_string(const char *s, size len)
{
    BigInt self;
    log_tracecall();
    bigint_init(&self);
    // Iterate string in reverse to go from smallest place value to largest.
    for (size i = len - 1; i >= 0; i--) {
        char ch = s[i];
        // TODO: Parse better than this
        if (isdigit(ch))
            push_left(&self, ch - '0'); // We assume ASCII
    }
    // Clean up leading 0's
    for (size i = self.length - 1; 0 <= i && i < self.capacity; i--) {
        // No more leading 0's?
        if (read_at(&self, i) != 0)
            break;
        pop_left(&self);
    }
    return self;
}

typedef struct Pair {
    size  index;
    digit digit;
} Pair;

// Addition of any 2 valid digits must be in the range 0..=(BASE-1) * 2
static bool add_at(BigInt *self, size index, digit dgt)
{
    Pair  add = {index, dgt};
    digit carry;
    log_tracecall();
    for (carry = 1;
         carry != 0;
         add.index++, add.digit = carry)
    {
        digit sum = read_at(self, add.index) + add.digit;
        if (sum >= BIGINT_BASE) {
            carry = sum / BIGINT_BASE; // Value of higher place value
            sum   = sum % BIGINT_BASE; // Pop higher place value
        } else {
            carry = 0;
        }
        if (!write_at(self, add.index, sum))
            return false;
    }
    return true;
}

void bigint_iadd(BigInt *self, int addend)
{
    // Add 1 digit at a time, going from lowest place value to highest.
    // If addend is 0 to begin with then we don't need to do anything.
    Pair dst = {0, 0};
    log_tracecall();
    for (int next = addend; next > 0; ++dst.index, next /= BIGINT_BASE) {
        dst.digit = next % BIGINT_BASE;
        add_at(self, dst.index, dst.digit);
    }
}

void bigint_isub(BigInt *self, int subtrahend)
{
    Pair dst = {0, 0};
    log_tracecall();
    for (int next = subtrahend; next > 0; ++dst.index, next /= BIGINT_BASE) {
        Pair sub  = {dst.index, next % BIGINT_BASE};
        dst.digit = read_at(self, dst.index);
        log_debugf("[%td]: minued=%i, subtrahend=%i",
                   dst.index, dst.digit, sub.digit);
    }
}

void bigint_print(const BigInt *self)
{
    size len = self->length;
    size cap = self->capacity;
    printf("digits=");
    for (size i = len - 1; 0 <= i && i < cap; i--) {
        printf("%i", self->digits[i]);
    }
    printf(", length=%td, capacity=%td\n", len, cap);
}

#endif  // BIGINT_INCLUDE_IMPLEMENTATION
