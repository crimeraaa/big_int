package bigint

import "base:intrinsics"
import "core:fmt"
import "core:mem"

/* 
`DIGIT_TYPE`:
    Our internal representation of a single digit of specified `DIGIT_BASE`.
 */
DIGIT_TYPE  :: distinct u8
DIGIT_BASE  :: 10
DIGIT_WIDTH :: 1

#assert(DIGIT_BASE < max(DIGIT_TYPE), "DIGIT_BASE cannot fit in a DIGIT_TYPE")

/* 
Must be a strict superset of `runtime.Allocation_Error`.
 */
Error :: enum u8 {
    Okay                 = 0,
    Out_Of_Memory        = 1,    
    Invalid_Pointer      = 2,
    Invalid_Argument     = 3,
    Mode_Not_Implemented = 4,
    Invalid_Digit        = 5,
    Invalid_Radix        = 6,
}

Sign :: enum i8 {
    Positive = 1,  // For simplicity, even 0 is considered positive.
    Negative = -1,
}

BigInt :: struct {
    digits: [dynamic]DIGIT_TYPE, // each digit is in the range 0..=9
    active: int, // Independent of `len(digits)`.
    sign:   Sign,
}

// --- INITIALIZATION FUNCTIONS ------------------------------------------- {{{1

bigint_init :: proc {
    bigint_init_none,
    bigint_init_cap,
}

bigint_init_none :: proc(self: ^BigInt, allocator := context.allocator) -> Error {
    return bigint_init_cap(self, 0, allocator)
}

bigint_init_cap :: proc(self: ^BigInt, capacity: int, allocator := context.allocator) -> Error {
    // make($T, len, allocator) -> mem.Allocator_Error
    memerr: mem.Allocator_Error
    self.digits, memerr = make([dynamic]DIGIT_TYPE, capacity, allocator)
    if memerr != .None {
        return .Out_Of_Memory
    }
    bigint_clear(self)
    return nil
}

bigint_make :: proc {
    bigint_make_none,
    bigint_make_cap,
}

bigint_make_none :: proc(allocator := context.allocator) -> (BigInt, Error) {
    return bigint_make_cap(0, allocator)
}

bigint_make_cap :: proc(capacity: int, allocator := context.allocator) -> (BigInt, Error) {
    self: BigInt
    err := bigint_init_cap(&self, capacity, allocator)
    return self, err
}

bigint_clear :: proc(self: ^BigInt) {
    mem.zero_slice(self.digits[:])
    self.active = 0
    self.sign   = .Positive
}

bigint_destroy :: proc(self: ^BigInt) {
    delete(self.digits)
    bigint_clear(self)
}

// 1}}} ------------------------------------------------------------------------

// --- PUBLIC HELPERS ----------------------------------------------------- {{{1

/*
Ideally, if we are 0, then `self.active` should also be 0.
 */
bigint_is_zero :: #force_inline proc(self: ^BigInt) -> bool {
    return self.active == 0
}

bigint_is_negative :: #force_inline proc(self: ^BigInt) -> bool {
    return self.sign == .Negative
}

bigint_to_string :: proc {
    bigint_to_string_fixed,
}

/* 
TODO: Probably do something better than this...
 */
bigint_to_string_fixed :: proc(self: ^BigInt, buf: []byte) -> string {
    if bigint_is_zero(self) {
        return fmt.bprint(buf, '0')
    }
    // Most significant digit should have no padding.
    offset := 0
    if first := self.digits[self.active - 1]; bigint_is_negative(self) {
        fmt.bprintf(buf, "-%i", first)
        offset = 2
    } else {
        fmt.bprint(buf, first)
        offset = 1
    }
    #reverse for digit in self.digits[:self.active - 1] {
        if offset >= len(buf) {
            panic("buffer overflow")
        }
        tmp := fmt.bprintf(buf[offset:], "%*[0]i", DIGIT_WIDTH, digit)
        offset += len(tmp)
    }
    return string(buf[:])
}

// 1}}} ------------------------------------------------------------------------

// --- "SET" FUNCTIONS ---------------------------------------------------- {{{1

bigint_set :: proc {
    bigint_set_from_integer,
    bigint_set_from_string,
}

bigint_set_zero :: bigint_clear

bigint_set_from_integer :: proc(self: ^BigInt, value: $T) -> Error {
    #assert(intrinsics.type_is_integer(T))
    if value == 0 {
        bigint_clear(self)
        return nil
    }
    /* 
    TODO: When DIGIT_BASE != 10, need a different algorithm to calculate size.
     */
    ndigits := count_digits(value)
    
    internal_resize(self, ndigits) or_return
    self.active = ndigits
    self.sign   = .Positive if value >= 0 else .Negative
    
    /* 
    Will getting the absolute value cause `rest` to overflow?
    e.g. -(i8(-128)) will result in 0 because +128 does not fit in an i8.
    */
    rest := value
    is_maxneg := !intrinsics.type_is_unsigned(T) && rest == min(T)
    if is_maxneg {
        rest += 1
    }
    rest = rest if value >= 0 else -rest
    for index in 0..<ndigits {
        self.digits[index] = DIGIT_TYPE(rest % DIGIT_BASE)
        rest /= DIGIT_BASE
    }
    /* 
        TODO: Add 1 again when is_maxneg
     */
    return nil
}

/* 
Assumes `input` represents a base-10 number, for simplicity.
 */
bigint_set_from_string :: proc(self: ^BigInt, input: string, minimize := false) -> Error {
    input   := input
    ndigits := len(input)

    sign := Sign.Positive
    for 1 <= len(input) && input[0] == '-' {
        input = input[1:]
        sign  = .Negative if sign == .Positive else .Positive
        ndigits -= 1
    }
    
    // Account for base prefix
    if 2 < len(input) && input[0] == '0' {
        switch input[1] {
        case 'd':
            input = input[2:]
            ndigits -= 2
        case 'b', 'o', 'x':
            fmt.eprintln("Non-base-10 prefixes not yet supported")
            return .Invalid_Radix
        case:
        }
    }
    
    // Skip leading zeroes in the string
    for 1 <= len(input) && input[0] == '0' {
        input = input[1:]
        ndigits -= 1
    }

    /* 
        TODO: Fix returning 0 when strings like "-----" are entered?
     */
    if ndigits == 0 {
        bigint_clear(self)
        return nil
    }
    
    // `ndigits` may be overkill if there are delimeters.
    internal_resize(self, ndigits) or_return
    self.active = 0
    self.sign   = sign
    
    // Right (least significant) to left (most significant)
    #reverse for char in input {
        switch char {
        case '0'..='9':
            // TODO: Adjust for when `DIGIT_BASE`>10
            self.digits[self.active] = DIGIT_TYPE(char - '0')
            self.active += 1
        case ' ', '_', ',':
            continue
        case:
            fmt.eprintfln("Unknown base-10 digit '%c'", char)
            return .Invalid_Digit
        }
    }
    bigint_trim(self, minimize)
    return nil
}

// 1}}} ------------------------------------------------------------------------

/* 
Trim leading zeroes so that `bigint_print` slices the correct amount.
 */
bigint_trim :: proc(self: ^BigInt, minimize := false) -> Error {
    count := 0
    #reverse for digit in self.digits[:self.active] {
        if digit == 0 {
            count += 1
        } else {
            break
        }
    }
    // We turned into zero?
    if self.active -= count; self.active == 0 {
        self.sign = .Positive
    }
    return internal_resize(self, self.active) if minimize else nil
}

bigint_print :: proc(self: ^BigInt, newline := true) {
    if bigint_is_negative(self) {
        fmt.print('-')
    }

    // Print only the active digits as otherwise we'll print all allocated.
    #reverse for digit in self.digits[:self.active] {
        // "%*spec" doesn't work anymore, need to do "%*[vararg-index]spec"
        // https://github.com/odin-lang/Odin/issues/3605#issuecomment-2143625629
        fmt.printf("%*[0]i", DIGIT_WIDTH, digit)
    }
    if bigint_is_zero(self) {
        fmt.print('0')
    }
    if newline {
        fmt.println()
    }
}

// --- "COMPARE" FUNCTIONS ------------------------------------------------ {{{1

Comparison :: enum {
    Less    = -1,
    Equal   = 0,
    Greater = +1,
}

Comparison_String := [Comparison]string {
    .Less    = "less than",
    .Equal   = "equal to",
    .Greater = "greater than",
}

@(require_results)
bigint_cmp :: proc(x, y: ^BigInt) -> Comparison {
    x_is_negative := bigint_is_negative(x)

    /* 
    With different signs we can assume that a negative number is always
    less than a postiive one.
     */
    if x.sign != y.sign {
        return .Less if x_is_negative else .Greater
    }
    /* 
    At this point we know that both numbers have the same sign.
    If positive, more digits means greater in value.
    If negative, more digits means lesser in value.
     */
    if x.active != y.active {
        if x_is_negative {
            return .Less if x.active > y.active else .Greater
        } else {
            return .Less if x.active < y.active else .Greater
        }
    }
    /* 
    At this point we know both `x` and `y` have the same sign and the same
    number of active digits. Note: 213 < 214 but -213 < -214.
     */
    for xdigit, xindex in x.digits[:x.active] {
        ydigit := y.digits[xindex]
        if xdigit != ydigit {
            if xdigit < ydigit {
                return .Greater if x_is_negative else .Less
            } else {
                return .Less if x_is_negative else .Greater
            }
        }
    }
    return .Equal
}

bigint_eq :: proc(x, y: ^BigInt) -> bool {
    return bigint_cmp(x, y) == .Equal
}

bigint_lt :: proc(x, y: ^BigInt) -> bool {
    return bigint_cmp(x, y) == .Less
}

bigint_gt :: proc(x, y: ^BigInt) -> bool {
    return bigint_cmp(x, y) == .Greater
}

bigint_neq :: proc(x, y: ^BigInt) -> bool {
    return !bigint_eq(x, y)
}

/* 
    x <= y == !(x > y)
 */
bigint_leq :: proc(x, y: ^BigInt) -> bool {
    return !bigint_gt(x, y)
}

/* 
    x >= y == !(x < y)
 */
bigint_geq :: proc(x, y: ^BigInt) -> bool {
    return !bigint_lt(x, y)
}

/* 
Compare the digit arrays without considering sign.
 */
bigint_cmp_abs :: proc(x, y: ^BigInt) -> Comparison {
    if x.active != y.active {
        return .Less if x.active < y.active else .Greater
    }
    for xdigit, xindex in x.digits[:x.active] {
        ydigit := y.digits[xindex]
        if xdigit != ydigit {
            return .Less if xdigit < ydigit else .Greater
        }
    }
    return .Equal
}

/* 
    |x| == |y|
 */
bigint_eq_abs :: proc(x, y: ^BigInt) -> bool {
    return bigint_cmp_abs(x, y) == .Equal
}

/* 
    |x| < |y|
 */
bigint_lt_abs :: proc(x, y: ^BigInt) -> bool {
    return bigint_cmp_abs(x, y) == .Less
}

/*  
    |x| > |y|
 */
bigint_gt_abs :: proc(x, y: ^BigInt) -> bool {
    return bigint_cmp_abs(x, y) == .Greater
}

/* 
    |x| != |y| == !(|x| == |y|)
 */
bigint_neq_abs :: proc(x, y: ^BigInt) -> bool {
    return !bigint_eq_abs(x, y)
}

/* 
    |x| <= |y| == !(|x| > |y|)
 */
bigint_leq_abs :: proc(x, y: ^BigInt) -> bool {
    return !bigint_gt_abs(x, y)
}

/* 
    |x| >= |y| == !(|x| < |y|)
 */
bigint_geq_abs :: proc(x, y: ^BigInt) -> bool {
    return !bigint_lt_abs(x, y)
}

// 1}}} ------------------------------------------------------------------------

// --- ARITHMETIC --------------------------------------------------------- {{{1

bigint_abs :: proc(self: ^BigInt) {
    self.sign = .Positive    
}

bigint_neg :: proc(self: ^BigInt) {
    if bigint_is_zero(self) {
        return
    }
    self.sign = .Positive if self.sign == .Negative else .Negative
}

/* 
High-level integer addition.

Some key assumptions:

    1. x    +   y  ==   x + y
    2. (-x) + (-y) == -(x + y)
    3. (-x) +   y  ==   y - x  or  -(x - y)
    4. x    + (-y) ==   x - y
 */
bigint_add :: proc(dst, x, y: ^BigInt) -> Error {
    x, y := x, y

    // Addition between 2 digits of the same base results in, at most, +1 digit.
    nlen := max(x.active, y.active) + 1
    internal_resize(dst, nlen) or_return
    mem.zero_slice(dst.digits[:])
    dst.active = nlen
    dst.sign   = x.sign

    /* 
    Assumptions 3 and 4.
    
    Since `dst.sign` is the same as `x.sign`, we may need to negate it.

    Account for:
        |x| < |y|:   1  + (-2) = -(2 - 1) = -1
        |x| < |y|: (-1) +   2  =  (2 - 1) =  1
        |x| > |y|:   2  + (-1) =  (2 - 1) =  1
        |x| > |y|: (-2) +   1  = -(2 - 1) = -1
     */
    if x.sign != y.sign {
        if bigint_lt_abs(x, y) {
            bigint_neg(dst)
            x, y = y, x
        }
        return internal_sub_unsigned(dst, x, y)
    }
    return internal_add_unsigned(dst, x, y)
}

/* 
High-level integer subtraction.

Some key assumptions:

    1.   x  -   y  ==   x - y
    2. (-x) - (-y) ==   y - x  or -(x - y)
    3. (-x) -   y  == -(x + y)
    4.   x  - (-y) ==   x + y
 */
bigint_sub :: proc(dst, x, y: ^BigInt) -> Error {
    x, y := x, y

    // No need for +1 since subtraction should never result in an extra digit.
    nlen := max(x.active, y.active)
    internal_resize(dst, nlen) or_return
    dst.active = nlen
    dst.sign   = x.sign

    /* 
    Assumptions 3 and 4.
    
    Since `dst.sign` is the same as `x.sign`, we're in the right state already.

    Accounts for:
        |x| < |y|:    1  - (-2) =  (1 + 2) =  3
        |x| < |y|:  (-1) -   2  = -(1 + 2) = -3
        |x| > |y|:    2  - (-1) =  (2 + 1) =  3
        |x| > |y|:  (-2) -   1  = -(2 + 1) = -3
     */
    if x.sign != y.sign {
        return internal_add_unsigned(dst, x, y)
    }
    
    /* 
    Assumptions 1 and 2.
    
    Since `dst.sign` is the same as `x.sign`, we may need to negate it.

    Accounts for:
        |x| < |y|:   1  -   2  = -(2 - 1) = -1
        |x| < |y|: (-1) - (-2) =  (2 - 1) =  1
        |x| > |y|:   2  -   1  =  (2 - 1) =  1
        |x| > |y|: (-2) - (-1) = -(2 - 1) = -1
     */
    if bigint_lt_abs(x, y) {
        bigint_neg(dst)
        x, y = y, x
    }

    return internal_sub_unsigned(dst, x, y)
}

// 1}}} ------------------------------------------------------------------------
