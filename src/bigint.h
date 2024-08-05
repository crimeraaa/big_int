#pragma once

/// local
#include "allocator.h"
#include "common.h"

typedef struct BigInt BigInt;

#define BIGINT_BASE   10

void    bigint_library_free(void);
void    bigint_library_set_allocator(AllocFn fn, void *ctx);
void    bigint_library_set_handler(HandlerFn fn, void *ctx);

BigInt *bigint_new(size cap);
void    bigint_free(BigInt **self);

void    bigint_print(const BigInt *self);
BigInt *bigint_set_int(int n);
BigInt *bigint_set_string(const char *s, size len);
BigInt *bigint_set_cstring(const char *cstr);

/**
 * @brief   Compares the lengths and digits of `x` and `y`.
 *
 * @return  If x == y, 0. If x > y, +1. If x < y, -1.
 */
int  bigint_compare(const BigInt *x, const BigInt *y);
void bigint_clear(BigInt *self);
void bigint_add(BigInt *dst, const BigInt *x, const BigInt *y);
void bigint_sub(BigInt *dst, const BigInt *x, const BigInt *y);

#ifdef BIGINT_INCLUDE_IMPLEMENTATION

/// standard
#include <ctype.h>

/// local
#define LOG_INCLUDE_IMPLEMENTATION
#include "log.h"

#define WARN_DIGIT(d)   log_warnf("Invalid digit '%i'.", (d))
#define WARN_POP()      log_warnln("Nothing to pop.")

typedef unsigned char digit;

struct BigInt {
    BigInt *prev;
    BigInt *next;
    size    length;
    size    capacity;
    bool    negative;
    digit  *digits;
};

static void *stdc_allocate(void *hint, size oldsz, size newsz, void *ctx)
{
    unused(oldsz, ctx);
    if (newsz == 0) {
        free(hint);
        return nullptr;
    }
    return realloc(hint, newsz);
}

static void stdc_handler(const char *msg, size reqsz, void *ctx)
{
    unused(ctx);
    log_fatalf("%s (requested %td bytes)", msg, reqsz);
    log_flush();
    abort();
}

static struct {
    BigInt   *active; // Top of a doubly linked list of active BigInt's.
    BigInt   *free;   // Top of a doubly linked list of free BigInt's.
    Allocator allocator;
} bi_state = {
    /* active    */ nullptr,
    /* free      */ nullptr,
    /* allocator */ {&stdc_allocate, &stdc_handler, nullptr, nullptr}
};

static void bigint_resize(BigInt *self, size newcap)
{
    // No resize needed or would need to shrink?
    log_traceargs("newcap=%td", newcap);
    if (newcap <= self->capacity)
        return;
    size   oldcap  = self->capacity;
    digit *oldbuf  = self->digits;
    digit *newbuf  = allocator_realloc(digit, oldbuf, oldcap, newcap, &bi_state.allocator);
    self->capacity = newcap;
    self->digits   = newbuf;
}

static void bigint_grow(BigInt *self)
{
    bigint_resize(self, self->capacity * 2);
}

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
    if (idx >= self->capacity) {
        bigint_grow(self);
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
    if (self->length >= self->capacity) {
        bigint_grow(self);
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
    log_tracevoid();
    if (self->length >= self->capacity) {
        bigint_grow(self);
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
    log_traceargs("dgt=%i", dgt);
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

static void trim_left(BigInt *self)
{
    // Clean up leading 0's
    for (size i = self->length - 1; 0 <= i && i < self->capacity; i--) {
        // No more leading 0's?
        if (read_at(self, i) != 0)
            break;
        pop_left(self);
    }
}

static size next_pow2(size n)
{
    size p = 1;
    while (p < n)
        p *= 2;
    return p;
}

static void link_list(BigInt **list, BigInt *ptr)
{
    ptr->prev = *list;
    ptr->next = nullptr;
    // If we're not at the base of the list, we are the previous node's next.
    if (*list)
        (*list)->next = ptr;
    // New top of list.
    *list = ptr;
}

static void unlink_list(BigInt **list, BigInt *ptr)
{
    // `ptr` is the top of the list?
    if (ptr == *list) {
        *list = ptr->prev;
        return;
    }
    // `ptr` is in the middle, so remove links to it from the child nodes.
    if (ptr->prev)
        ptr->prev->next = ptr->next;
    if (ptr->next)
        ptr->next->prev = ptr->prev;
}

static void free_list(BigInt **list)
{
    BigInt *next;
    for (BigInt *it = *list; it != nullptr; it = next) {
        next = it->next;
        allocator_free(digit, it->digits, it->capacity, &bi_state.allocator);
        allocator_free(BigInt, it, 1, &bi_state.allocator);
    }
    *list = nullptr;
}

static BigInt *find_free_or_allocate(void)
{
    BigInt *self = bi_state.free;
    if (self)
        unlink_list(&bi_state.free, self);
    else
        self = allocator_alloc(BigInt, 1, &bi_state.allocator);
    link_list(&bi_state.active, self);
    return self;
}

BigInt *bigint_new(size cap)
{
    BigInt *self;
    log_traceargs("cap=%td", cap);
    cap  = next_pow2(cap);
    self = find_free_or_allocate();
    self->length   = 0;
    self->capacity = 0;
    self->negative = false;
    self->digits   = nullptr;
    bigint_resize(self, cap);
    bigint_clear(self);
    return self;
}

void bigint_free(BigInt **self)
{
    // log_traceargs("self=%p", *self);
    unlink_list(&bi_state.active, *self);
    link_list(&bi_state.free, *self);
    *self = nullptr;
}

void bigint_library_set_allocator(AllocFn fn, void *ctx)
{
    bi_state.allocator.allocate_function = fn;
    bi_state.allocator.allocate_context  = ctx;
}

void bigint_library_set_handler(HandlerFn fn, void *ctx)
{
    bi_state.allocator.handler_function = fn;
    bi_state.allocator.handler_context  = ctx;
}

void bigint_library_free(void)
{
    log_tracevoid();
    free_list(&bi_state.active);
    free_list(&bi_state.free);
}

void bigint_clear(BigInt *self)
{
    size sz = array_sizeof(self->digits[0], self->capacity);
    // memset on NULL is a bad idea
    if (self->digits && sz > 0)
        memset(self->digits, 0, sz);
    self->length = 0;
}

static size count_digits(int value)
{
    int count = 0;
    while (value != 0) {
        count++;
        value /= BIGINT_BASE;
    }
    return count;
}

BigInt *bigint_set_int(int n)
{
    BigInt *self;
    log_traceargs("n=%i", n);
    self = bigint_new(count_digits(n));
    
    if (n < 0)
        self->negative = true;

    // 13 / 10 = 1.3 = 1
    for (int it = n; it != 0; it /= BIGINT_BASE) {
        digit dgt = it % BIGINT_BASE; // 13 % 10 = 3
        push_left(self, dgt);
    }
    
    return self;
}

BigInt *bigint_set_cstring(const char *cstr)
{
    return bigint_set_string(cstr, strlen(cstr));
}

BigInt *bigint_set_string(const char *s, size len)
{
    BigInt *self;
    log_traceargs("len=%td", len);
    self = bigint_new(len);

    // Iterate string in reverse to go from smallest place value to largest.
    for (size i = len - 1; 0 <= i && i < len; i--) {
        char ch = s[i];
        // TODO: Parse better than this
        if (ch == '-')
            self->negative = !self->negative;
        else if (isdigit(ch))
            push_left(self, ch - '0'); // We assume ASCII
    }
    trim_left(self);
    return self;
}

static size greater_length(const BigInt *x, const BigInt *y)
{
    return (x->length > y->length) ? x->length : y->length;
}

static size lesser_length(const BigInt *x, const BigInt *y)
{
    return !greater_length(x, y) ? x->length : y->length;
}

void bigint_add(BigInt *dst, const BigInt *x, const BigInt *y)
{
    log_tracevoid();
    size len = greater_length(x, y) + 1;
    bigint_resize(dst, len);
    bigint_clear(dst);
    digit carry = 0;
    for (size i = 0; 0 <= i && i < len; i++) {
        digit sum = read_at(x, i) + read_at(y, i) + carry;
        if (sum >= BIGINT_BASE) {
            carry = sum / BIGINT_BASE; // Value of the higher place value.
            sum   = sum % BIGINT_BASE; // 'Pop' the higher place value.
        } else {
            carry = 0;
        }
        write_at(dst, i, sum);
    }
    trim_left(dst);
}

int bigint_compare(const BigInt *x, const BigInt *y)
{
    if (x->length != y->length) {
        bool gt = x->length > y->length;
        // One is negative but not the other?
        if (x->negative != y->negative) {
            // If `y` is negative we assume +x > -y, else assume -x < +y.
            gt = y->negative;
            return (gt) ? -1 : +1;
        }
        // Both negative? E.g. -10 and -100
        if (x->negative && y->negative)
            return (gt) ? -1 : +1;
        else
            return (gt) ? +1 : -1;
    }
    for (size i = 0; 0 <= i && i < x->length; i++) {
        digit dx = read_at(x, i);
        digit dy = read_at(y, i);
        if (dx != dy) {
            return (dx > dy) ? +1 : -1;
        }
    }
    return 0;
}

static void copy_digits(BigInt *dst, const BigInt *other)
{
    size len = other->length;
    size cap = other->capacity;
    log_tracevoid();

    dst->length = len;
    bigint_resize(dst, cap);
    for (size i = 0; 0 <= i && i < len; i++)
        dst->digits[i] = other->digits[i];
}

static bool borrow_needed(BigInt *self, size idx)
{
    size start = idx + 1;
    log_traceargs("idx=%td", idx);
    for (size i = start; start <= i && i < self->length; i++) {
        digit dgt  = read_at(self, i);
        digit diff = ((dgt == 0) ? BIGINT_BASE : dgt) - 1;
        log_debugf("digits[%td]=%i, diff=%i", i, dgt, diff);
        write_at(self, i, diff);
        // We assume somewhere down the line is a nonzero.
        if (dgt == 0)
            continue;
        // Borrowing caused this digit to be zero, so "erase" or "pop" it.
        if (diff == 0)
            pop_left(self);
        return true;
    }
    return false;
}

void bigint_sub(BigInt *dst, const BigInt *x, const BigInt *y)
{
    log_tracevoid();
    // The trick to negate a larger number from a smaller one is to swap them,
    // then negate the difference.
    if (bigint_compare(x, y) < 0) {
        const BigInt *tmp = x;
        x = y;
        y = tmp;
        dst->negative = true;
    }
    size len = lesser_length(x, y);
    // Copy the digits of the minuend in case we need to modify when borrowing.
    copy_digits(dst, x);
    for (size i = 0; 0 <= i && i < len; i++) {
        digit mind = read_at(dst, i);
        digit subt = read_at(y, i);
        if (mind < subt) {
            if (borrow_needed(dst, i)) {
                mind += BIGINT_BASE;
            } else {
                digit tmp = mind;
                mind = subt;
                subt = tmp;
                dst->negative = true;
            }
        }
        write_at(dst, i, mind - subt);
    }
}

void bigint_print(const BigInt *self)
{
    if (self->negative)
        printf("-");
    for (size i = self->length - 1; 0 <= i && i < self->capacity; i--)
        printf("%i", self->digits[i]);
    printf("\n");
}

#endif  // BIGINT_INCLUDE_IMPLEMENTATION
