package bigint

import "base:intrinsics"
import "core:fmt"
import "core:log"
import "core:mem"
import "core:strings"
import "core:unicode"

/* 
Our internal representation of a single digit of specified `DIGIT_BASE`.
It must be capable of holding all values in the range `0..=DIGIT_MAX`.
 */
DIGIT_TYPE :: distinct u32

/* 
Our internal representation of the result of any arithmetic between any 2
`DIGIT_TYPE`. 

E.g. if `DIGIT_BASE == 1_000_000_000`, this must be capable of representing the
following:

            0 - DIGIT_MAX = -999_999_999
    DIGIT_MAX + DIGIT_MAX = 1_999_999_998
    DIGIT_MAX * DIGIT_MAX = 999_999_998_000_000_001
 */
WORD_TYPE :: distinct i64

/* 
Some multiple of 10 that fits in `DIGIT_TYPE`. We use a multiple of 10 since
it makes things somewhat easier when conceptualizing.

Since `DIGIT_TYPE` is a `u32`, the maximum actual value is 4_294_967_296.
However we need the get the nearest lower power of 10.
 */
DIGIT_BASE :: 1_000_000_000
DIGIT_MAX :: DIGIT_BASE - 1

/* 
The number of base-10 digits in `DIGIT_MAX`.
 */
DIGIT_WIDTH :: 9

/* 
`DIGIT_WIDTH_{BIN,OCT,HEX}`:
    Number of base-N digits needed to represent `DIGIT_MAX`.

Rationale, see Python output of the following:

    DIGIT_WIDTH_BIN = len(bin(DIGIT_MAX)) - 2
    DIGIT_WIDTH_OCT = len(oct(DIGIT_MAX)) - 2
    DIGIT_WIDTH_HEX = len(hex(DIGIT_MAX)) - 2
 */
DIGIT_WIDTH_BIN :: 30
DIGIT_WIDTH_OCT :: 10
DIGIT_WIDTH_HEX :: 8

#assert(DIGIT_BASE < max(DIGIT_TYPE))

/* 
Must be a strict superset of `runtime.Allocation_Error`.

See: https://pkg.odin-lang.org/base/runtime/#Allocator_Error
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
    digits: [dynamic]DIGIT_TYPE, // each digit is in the range 0..<10^DIGIT_WIDTH
    active: int, // Independent of `len(digits)`.
    sign:   Sign,
}

// --- INITIALIZATION FUNCTIONS ------------------------------------------- {{{1

bigint_init :: proc(self: ^BigInt, capacity := 0, allocator := context.allocator) -> Error {
    // make($T, len, allocator) -> mem.Allocator_Error
    if capacity != 0 {
        memerr: mem.Allocator_Error
        self.digits, memerr = make([dynamic]DIGIT_TYPE, capacity, allocator)
        if memerr != .None {
            return Error(memerr)
        }
    }
    bigint_clear(self)
    return nil
}

/* 
Allocate several `BigInt` with `capacity` digits, all at once.
 */
bigint_init_multi :: proc(args: ..^BigInt, capacity := 0, allocator := context.allocator) -> Error {
    for arg in args {
        bigint_init(arg, capacity, allocator) or_return
    }
    return nil
}

bigint_make :: proc(capacity := 0, allocator := context.allocator) -> (BigInt, Error) {
    self: BigInt
    err := bigint_init(&self, capacity, allocator)
    return self, err
}

/* 
Zeroes out all elements in `self.digits` but does not deallocate it.
This ensures that all indexes in the range `0..<len(self.digits)` are still
writable.
 */
bigint_clear :: proc(self: ^BigInt) {
    mem.zero_slice(self.digits[:])
    self.active = 0
    self.sign   = .Positive
}

bigint_destroy :: proc(self: ^BigInt) {
    delete(self.digits)
    self.digits = nil
    bigint_clear(self)
}

bigint_destroy_multi :: proc(args: ..^BigInt) {
    for arg in args {
        bigint_destroy(arg)
    }
}

// 1}}} ------------------------------------------------------------------------

// --- PUBLIC HELPERS ----------------------------------------------------- {{{1

/*
Ideally, if we are 0, then `self.active` should also be 0.
 */
bigint_is_zero :: #force_inline proc(self: BigInt) -> bool {
    return self.active == 0
}

bigint_is_negative :: #force_inline proc(self: BigInt) -> bool {
    return self.sign == .Negative
}

/* 
Since `strings.Builder` just wraps `[dynamic]u8`, we can leave freeing it to
be the caller's responsibility.
 */
bigint_to_string :: proc(self: BigInt, allocator := context.allocator) -> (out: string, err: Error) {
    context.allocator = allocator
    builder, memerr := strings.builder_make_len(1 + self.active * DIGIT_WIDTH)
    if memerr != nil {
        return "", Error(memerr)
    }
    
    if bigint_is_zero(self) {
        fmt.sbprint(&builder, '0')
    } else {
        // Most significant digit should have no padding.
        if first := self.digits[self.active - 1]; bigint_is_negative(self) {
            fmt.sbprintf(&builder, "-%i", first)
        } else {
            fmt.sbprint(&builder, first)
        }        
        digits := self.digits[:self.active - 1]
        #reverse for digit in digits {
            // "%*spec" doesn't work anymore, need to do "%*[vararg-index]spec"
            // https://github.com/odin-lang/Odin/issues/3605#issuecomment-2143625629
            fmt.sbprintf(&builder, "%*[0]i", DIGIT_WIDTH, digit)
        }
    }
    return strings.to_string(builder), nil
}


// 1}}} ------------------------------------------------------------------------

// --- "SET" FUNCTIONS ---------------------------------------------------- {{{1

bigint_set :: proc {
    bigint_set_from_integer,
    bigint_set_from_string,
}

bigint_set_zero :: bigint_clear

bigint_set_from_integer :: proc(self: ^BigInt, value: int) -> Error {
    if value == 0 {
        bigint_clear(self)
        return nil
    }
    ndigits := count_digits(value, DIGIT_BASE)
    internal_resize(self, ndigits) or_return
    self.sign = .Positive if value >= 0 else .Negative
    
    /* 
    Will getting the absolute value cause `rest` to overflow?
    e.g. -(i8(-128)) will result in 0 because +128 does not fit in an i8.
    So we add 1 to get -127 of which +127 does fit in an i8.
    */
    rest := value
    is_maxneg := rest == min(int)
    if is_maxneg {
        rest += 1
    }
    rest = rest if value >= 0 else -rest
    for index in 0..<ndigits {
        self.digits[index] = DIGIT_TYPE(rest % DIGIT_BASE)
        rest /= DIGIT_BASE
    }
    /*
    TODO: Implement in terms of sub, not add.
    */
    return internal_add_digit(self, self^, 1) if is_maxneg else nil
}

/* 
Assumes `input` represents a base-10 number, for simplicity.

May error out, in which case handling it is the caller's responsibility.
 */
bigint_set_from_string :: proc(self: ^BigInt, input: string, radix := 10) -> Error {
    input, radix := input, radix
    sign := Sign.Positive
    prefix_loop: for len(input) > 1 {
        switch input[0] {
        case '-': sign = .Negative if sign == .Positive else .Positive
        case '+': sign = .Positive
        case '0':
            if radix == 0 {
                ok: bool
                input, radix, ok = detect_number_string_radix(input)
                if !ok {
                    log.errorf("Invalid base prefix in '%s'", input)
                    return .Invalid_Radix
                }
                if radix != 10 {
                    log.warnf("Base-%i strings may not be decoded properly!", radix)
                }
                continue prefix_loop
            }
        case:
            break prefix_loop
        }
        input = input[1:]
    }
    if radix == 0 {
        radix = 10
    }
    if len(input) == 0 {
        bigint_set_zero(self)
        return nil
    }
    
    /* 
    e.g. "123456789780" has 12 characters, which is 12 base-10 characters.
    However, for base 10^9 we need to divide the length by 9.
    
    Note:
        digits[0] := 456789780  (10^9)^0 place
        digits[1] := 123        (10^9)^1 place
     */
    ndigits := count_numeric_chars_only(input, radix)
    nwidth  := internal_get_block_width(radix)
    nblocks := ndigits / nwidth
    nextra  := ndigits % nwidth
    if nextra != 0 {
        nblocks += 1
    }
    self.sign = sign
    internal_resize(self, nblocks) or_return
    return internal_set_from_string(self, input, {
        radix = radix,
        width = nwidth,
        index = nextra if nextra > 0 else nwidth,
    })
}

// 1}}} ------------------------------------------------------------------------

/* 
Trim leading zeroes so that `bigint_print` slices the correct amount.

Note: Just like the builtin procedure `resize`, `shrink` does *not* take an
`Allocator` because dynamic arrays already remember their allocators.
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
    /* 
    See: https://pkg.odin-lang.org/base/runtime/#shrink_dynamic_array
     */
    if minimize {
        if _, memerr := shrink(&self.digits, self.active); memerr != nil {
            log.fatal("Failed to shrink memory usage!")
            return Error(memerr)
        }
    }
    return nil
}

bigint_print :: proc(self: BigInt, newline := true) {
    defer if newline {
        fmt.println()
    }
    output, _ := bigint_to_string(self)
    defer delete(output)
    fmt.print(output)
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
bigint_cmp :: proc(x, y: BigInt) -> Comparison {
    x_is_negative := bigint_is_negative(x)

    /* 
    With different signs we can assume that a negative number is always
    less than a postiive one.
    
    Accounts for:
          1  > (-2)
        (-1) <   2
          2  > (-1)
        (-2) <   1
     */
    if x.sign != y.sign {
        return .Less if x_is_negative else .Greater
    }

    /* 
    At this point we know that both numbers have the same sign.

    If both are positive, having more digits means greater in value.
    If both are negative, having more digits means lesser in value.
    
    Accounts for:
          11  >    2
        (-11) <  (-2)
           2  <   11
         (-2) > (-11)
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
    number of active digits.
    
    Accounts for:
          123  <   456
        (-123) > (-456)
          456  >   123
        (-456) < (-123)
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

bigint_eq :: #force_inline proc(x, y: BigInt) -> bool {
    return bigint_cmp(x, y) == .Equal
}

bigint_lt :: #force_inline proc(x, y: BigInt) -> bool {
    return bigint_cmp(x, y) == .Less
}

bigint_gt :: #force_inline proc(x, y: BigInt) -> bool {
    return bigint_cmp(x, y) == .Greater
}

/* 
    x != y == !(x == y)
 */
bigint_neq :: #force_inline proc(x, y: BigInt) -> bool {
    return !bigint_eq(x, y)
}

/* 
    x <= y == !(x > y)
 */
bigint_leq :: #force_inline proc(x, y: BigInt) -> bool {
    return !bigint_gt(x, y)
}

/* 
    x >= y == !(x < y)
 */
bigint_geq :: #force_inline proc(x, y: BigInt) -> bool {
    return !bigint_lt(x, y)
}

/* 
Compare the digit arrays without considering sign.
 */
bigint_cmp_abs :: proc(x, y: BigInt) -> Comparison {
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
bigint_eq_abs :: #force_inline proc(x, y: BigInt) -> bool {
    return bigint_cmp_abs(x, y) == .Equal
}

/* 
    |x| < |y|
 */
bigint_lt_abs :: #force_inline proc(x, y: BigInt) -> bool {
    return bigint_cmp_abs(x, y) == .Less
}

/*  
    |x| > |y|
 */
bigint_gt_abs :: #force_inline proc(x, y: BigInt) -> bool {
    return bigint_cmp_abs(x, y) == .Greater
}

/* 
    |x| != |y| == !(|x| == |y|)
 */
bigint_neq_abs :: #force_inline proc(x, y: BigInt) -> bool {
    return !bigint_eq_abs(x, y)
}

/* 
    |x| <= |y| == !(|x| > |y|)
 */
bigint_leq_abs :: #force_inline proc(x, y: BigInt) -> bool {
    return !bigint_gt_abs(x, y)
}

/* 
    |x| >= |y| == !(|x| < |y|)
 */
bigint_geq_abs :: #force_inline proc(x, y: BigInt) -> bool {
    return !bigint_lt_abs(x, y)
}

// 1}}} ------------------------------------------------------------------------

// --- ARITHMETIC --------------------------------------------------------- {{{1

bigint_abs :: #force_inline proc(self: ^BigInt) {
    self.sign = .Positive    
}

bigint_neg :: proc(self: ^BigInt) {
    if bigint_is_zero(self^) {
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
bigint_add :: proc(dst: ^BigInt, x, y: BigInt) -> Error {
    // Addition between 2 digits of the same base results in, at most, +1 digit.
    nlen := max(x.active, y.active) + 1
    internal_resize(dst, nlen) or_return
    mem.zero_slice(dst.digits[:])
    dst.sign = x.sign

    /* 
    Assumptions 3 and 4.
    
    Since `dst.sign` is the same as `x.sign`, we may re-negate it.

    Account for:
        |x| < |y|:   1  + (-2) = -(2 - 1) = -1
        |x| < |y|: (-1) +   2  =  (2 - 1) =  1
        |x| > |y|:   2  + (-1) =  (2 - 1) =  1
        |x| > |y|: (-2) +   1  = -(2 - 1) = -1
     */
    if x.sign != y.sign {
        if bigint_lt_abs(x, y) {
            bigint_neg(dst)
            return internal_sub_unsigned(dst, y, x)
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
bigint_sub :: proc(dst: ^BigInt, x, y: BigInt) -> Error {
    // No need for +1 since subtraction should never result in an extra digit.
    nlen := max(x.active, y.active)
    internal_resize(dst, nlen) or_return
    dst.sign = x.sign

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
        return internal_sub_unsigned(dst, y, x)
    }
    return internal_sub_unsigned(dst, x, y)
}

// 1}}} ------------------------------------------------------------------------
