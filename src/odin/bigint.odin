package bigint

import "base:intrinsics"
import "core:fmt"
import "core:log"
import "core:math"
import "core:mem"
import "core:strings"

BigInt :: struct {
    digits: [dynamic]DIGIT, // each digit is in the range 0..<10^DIGIT_WIDTH
    active: int, // Independent of `len(digits)`.
    sign:   Sign,
}

Sign :: enum i8 {
    Positive = 1,  // For simplicity, even 0 is considered positive.
    Negative = -1,
}

/* 
Must be a strict superset of `runtime.Allocation_Error`.

See: https://pkg.odin-lang.org/base/runtime/#Allocator_Error
 */
Error :: union #shared_nil {
    mem.Allocator_Error,
    Parse_Error,
}

Parse_Error :: enum u8 {
    None = 0,
    Invalid_Digit,
    Invalid_Radix,
}

// --- INITIALIZATION FUNCTIONS ------------------------------------------- {{{1


/* 
    Important to call this so that `self.digits` has an allocator it can remember.
 */
bigint_init :: proc(self: ^BigInt, capacity := 0, allocator := context.allocator) -> Error {
    // make($T, len, allocator) -> mem.Allocator_Error
    self.digits = make([dynamic]DIGIT, capacity, allocator) or_return
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

is_zero :: proc {
    bigint_is_zero,
}

/*
    Ideally, if we are 0, then `self.active` should also be 0.
 */
bigint_is_zero :: #force_inline proc(self: BigInt) -> bool {
    return self.active == 0
}

is_neg :: proc {
    bigint_is_neg,
}

bigint_is_neg :: #force_inline proc(self: BigInt) -> bool {
    return self.sign == .Negative
}

/* 
    Since `strings.Builder` just wraps `[dynamic]u8`, we can leave freeing it to
    be the caller's responsibility.
 */
bigint_to_string :: proc(self: BigInt, allocator := context.allocator) -> (out: string, err: Error) {
    context.allocator = allocator
    builder := strings.builder_make_len(1 + self.active * DIGIT_WIDTH) or_return
    if is_zero(self) {
        fmt.sbprint(&builder, '0')
    } else {
        // Most significant digit should have no padding.
        if first := self.digits[self.active - 1]; is_neg(self) {
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

set :: proc {
    bigint_set_from_integer,
    bigint_set_from_string,
}

set_zero :: bigint_clear

/* 
    WARNING(2024-08-22): If `T` is one of `i128, u128` this will break! This is
    because we hard-code the casts to be `UWORD` which is a `distinct u64`.
 */
bigint_set_from_integer :: proc(self: ^BigInt, value: $T) -> Error 
where intrinsics.type_is_integer(T) {
    value := value
    if value == 0 {
        bigint_clear(self)
        return nil
    }
    n_digits := math.count_digits_of_base(value, DIGIT_BASE)
    internal_bigint_grow(self, n_digits) or_return
    self.sign = .Positive if value >= 0 else .Negative
    
    /* 
    Will getting the absolute value cause `rest` to overflow?
    e.g. -(i8(-128)) will result in 0 because +128 does not fit in an i8.
    So we add 1 to get -127 of which +127 does fit in an i8.

    Then, later on, we do an unsigned addition to turn -127 to -128.
    */
    is_maximally_negative := value == min(T)
    when !intrinsics.type_is_unsigned(T) {
        if is_maximally_negative {
            value += 1
        }
    }
    iter  := UWORD(math.abs(value))
    radix := UWORD(DIGIT_BASE)
    for index in 0..<n_digits {
        rest, digit := math.divmod(iter, radix)
        self.digits[index] = DIGIT(digit)
        iter = rest
    }
    return internal_add(self, self^, 1) if is_maximally_negative else nil
}

/* 
    Assumes `input` represents a base-10 number, for simplicity.

    May error out, in which case handling it is the caller's responsibility.
 */
bigint_set_from_string :: proc(self: ^BigInt, input: string, radix := 10) -> Error {
    line := Number_String{
        data  = input,
        radix = radix,
        sign  = .Positive
    }
    number_string_trim_leading(&line)
    if line.radix == 0 {
        ok := number_string_detect_radix(&line)
        if !ok {
            log.errorf("Invalid base prefix in '%s'", input)
            return .Invalid_Radix
        }
        if line.radix != 10 {
            log.warnf("Base-%i strings may not be decoded properly!", line.radix)
        }
    }
    if len(line.data) == 0 {
        set_zero(self)
        return nil
    }
    
    /* 
        e.g. "123456789780" has 12 characters, which is 12 base-10 characters.
        However, for base 10^9 we need to divide the length by 9.
        
        NOTE:

            digits[0] := 456789780  (10^9)^0 place
            digits[1] := 123        (10^9)^1 place
     */
    ndigits := number_string_count_digits(line)
    nwidth  := internal_get_block_width(line.radix)
    nblocks := ndigits / nwidth
    nextra  := ndigits % nwidth
    if nextra != 0 {
        nblocks += 1
    }
    self.sign = line.sign
    internal_bigint_grow(self, nblocks) or_return
    return internal_bigint_set_from_string(self, line.data, Block_Info{
        radix = line.radix,
        width = nwidth,
        index = nextra if nextra > 0 else nwidth,
    })
}

// 1}}} ------------------------------------------------------------------------

/* 
    Trim leading zeroes so that `bigint_to_string()` slices the correct amount.

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
            return memerr
        }
    }
    return nil
}

// --- "COMPARE" FUNCTIONS ------------------------------------------------ {{{1

Comparison :: enum {
    Less    = -1,
    Equal   = 0,
    Greater = +1,
}

cmp :: proc {
    bigint_cmp,
    bigint_cmp_digit,
}

bigint_cmp :: proc(x, y: BigInt) -> Comparison {
    x_is_negative := is_neg(x)
    switch {
    /* 
    With different signs we can assume that a negative number is always
    less than a positive one. Accounts for:

        +x, -y, (x > y):   1  > (-2)
        -x, +y, (x < y): (-1) <   2
        +x, -y, (x > y):   2  > (-1)
        -x, +y, (x < y): (-2) <   1
     */
    case x.sign != y.sign:     return .Less if x_is_negative else .Greater
    /* 
    At this point we know that both numbers have the same sign. Meaning, if `x`
    is negative, we can also assume that `y` is negative and vice-versa.
    Accounts for:

        +x, +y, (x > y):   11  >    2
        -x, -y, (x < y): (-11) <  (-2)
        +x, +y, (x < y):   2   <   11
        -x, -y, (x > y): (-2)  > (-11)
     */
    case x.active < y.active:  return .Greater if x_is_negative else .Less
    case x.active == y.active: break
    case x.active > y.active:  return .Less if x_is_negative else .Greater
    }

    /* 
    At this point we know both `x` and `y` have the same sign and the same
    number of active digits. Accounts for:

          123  <   456
        (-123) > (-456)
          456  >   123
        (-456) < (-123)
     */
    for xdigit, xindex in x.digits[:x.active] {
        ydigit := y.digits[xindex]
        switch {
        case xdigit == ydigit: continue
        case xdigit < ydigit:  return .Greater if x_is_negative else .Less
        case xdigit > ydigit:  return .Less    if x_is_negative else .Greater
        }
    }
    return .Equal
}

bigint_cmp_digit :: proc(x: BigInt, y: DIGIT) -> Comparison {
    switch {
    /* 
        `x` may be conceptually negative, but `y`, being unsigned, can never be.
        Negative values are always lower than positive ones.
     */
    case is_neg(x):         return .Less
    
    /* 
        Not safe to assume `x.digits[0]` can be safely read, because there's a
        chance that the dynamic array is still nil (i.e. an empty slice).
     */
    case is_zero(x):        return .Equal if y == 0 else .Less

    /* 
        Both are positive. More than 1 digit means greater place value.
     */
    case x.active > 1:      return .Greater
    
    /* 
        We can now assume that `x.active` is probably 0 or 1.
     */
    case x.digits[0] <  y:  return .Less
    case x.digits[0] == y:  return .Equal
    case x.digits[0] >  y:  return .Greater
    }
    
    /* 
    Unreachable.
     */
    return .Equal
}

eq :: proc {
    bigint_eq,
    bigint_eq_digit,
}

bigint_eq :: #force_inline proc(x, y: BigInt) -> bool {
    return cmp(x, y) == .Equal
}

bigint_eq_digit :: #force_inline proc(x: BigInt, y: DIGIT) -> bool {
    return cmp(x, y) == .Equal
}

lt :: proc {
    bigint_lt,
    bigint_lt_digit,
}

bigint_lt :: #force_inline proc(x, y: BigInt) -> bool {
    return cmp(x, y) == .Less
}

bigint_lt_digit :: #force_inline proc(x: BigInt, y: DIGIT) -> bool {
    return cmp(x, y) == .Less
}

gt :: proc {
    bigint_gt,
    bigint_gt_digit,
}

bigint_gt :: #force_inline proc(x, y: BigInt) -> bool {
    return cmp(x, y) == .Greater
}

bigint_gt_digit :: #force_inline proc(x: BigInt, y: DIGIT) -> bool {
    return cmp(x, y) == .Greater
}

cmp_abs :: proc {
    bigint_cmp_abs,
    bigint_cmp_abs_digit,
}

/* 
Compare the digit arrays without considering sign.
 */
bigint_cmp_abs :: proc(x, y: BigInt) -> Comparison {
    switch {
    case x.active == y.active: break
    case x.active < y.active:  return .Less
    case x.active > y.active:  return .Greater
    }

    for xdigit, xindex in x.digits[:x.active] {
        ydigit := y.digits[xindex]
        switch {
        case xdigit == ydigit: continue
        case xdigit < ydigit:  return .Less
        case xdigit > ydigit:  return .Greater
        }
    }
    return .Equal
}

bigint_cmp_abs_digit :: proc(x: BigInt, y: DIGIT) -> Comparison {
    switch {
    case x.active > 1:      return .Greater
    /* 
        Not safe to assume `x.digits[0]` can be safely read.
     */
    case x.active == 0:     return .Equal if y == 0 else .Less
    case x.digits[0] < y:   return .Less
    case x.digits[0] == y:  return .Equal
    case x.digits[0] > y:   return .Greater
    }
    return .Equal
}

eq_abs :: proc {
    bigint_eq_abs,
    bigint_eq_abs_digit,
}

/* 
    |x| == |y|
 */
bigint_eq_abs :: #force_inline proc(x, y: BigInt) -> bool {
    return cmp_abs(x, y) == .Equal
}

bigint_eq_abs_digit :: #force_inline proc(x: BigInt, y: DIGIT) -> bool {
    return cmp_abs(x, y) == .Equal
}

lt_abs :: proc {
    bigint_lt_abs,
    bigint_lt_abs_digit,
}

/* 
    |x| < |y|
 */
bigint_lt_abs :: #force_inline proc(x, y: BigInt) -> bool {
    return cmp_abs(x, y) == .Less
}

bigint_lt_abs_digit :: #force_inline proc(x: BigInt, y: DIGIT) -> bool {
    return cmp_abs(x, y) == .Less
}

/*  
    |x| > |y|
 */
bigint_gt_abs :: #force_inline proc(x, y: BigInt) -> bool {
    return cmp_abs(x, y) == .Greater
}

// 1}}} ------------------------------------------------------------------------

// --- ARITHMETIC --------------------------------------------------------- {{{1

abs :: proc {
    bigint_abs,
}

bigint_abs :: #force_inline proc(self: ^BigInt) {
    self.sign = .Positive    
}

neg :: proc {
    bigint_neg,
}

bigint_neg :: proc(self: ^BigInt) {
    if is_zero(self^) {
        return
    }
    self.sign = .Positive if self.sign == .Negative else .Negative
}

add :: proc {
    bigint_add,
    bigint_add_digit,
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
    n_len := max(x.active, y.active) + 1
    internal_bigint_grow(dst, n_len) or_return
    dst.sign = x.sign

    switch {
    /* 
        Assumptions 1 and 2. Accounts for:
            +x, +y:   1  +   2  = 3
            -x, -y: (-1) + (-2) = -3
     */
    case x.sign == y.sign: return internal_add(dst, x, y)

    /* 
        Assumptions 3 and 4. Accounts for:
            +x, -y, |x| < |y|:   1  + (-2) = -(2 - 1) = -1
            -x, +y, |x| < |y|: (-1) +   2  =  (2 - 1) =  1
        
        Note:
            We defer the negation because if `dst` is currently zero, nothing
            will happen. But after the subtraction it may be nonzero hence we
            defer to negate only after that point.
     */
    case lt_abs(x, y):     defer neg(dst)
                           return internal_sub(dst, y, x)
    /* 
        Assumptions 3 and 4. Accounts for:
            -x, +y, |x| > |y|:   2  + (-1) =  (2 - 1) =  1
            +x, -y, |x| > |y|: (-2) +   1  = -(2 - 1) = -1
     */
    case:                  return internal_sub(dst, x, y)
    }
}

/* 
High-level integer addition so that `y` doesn't need to be stored in a `BigInt`.

NOTE: Because `y` is a `DIGIT`, it is unsigned and thus never negative.
 */
bigint_add_digit :: proc(dst: ^BigInt, x: BigInt, y: DIGIT) -> Error {
    n_len := max(x.active, math.count_digits_of_base(y, DIGIT_BASE)) + 1
    internal_bigint_grow(dst, n_len) or_return
    dst.sign = x.sign
    
    switch {
    case !is_neg(x):   return internal_add(dst, x, y)

    /* 
        Accounts for:
            -x, +y, |x| < y : -1 + 2 =   2 - 1  =  1
            -x, +y, |x| > y : -2 + 1 = -(2 - 1) = -1
            
        In the event of `|x| < y` and `x` is zero, -0 + y == y.
        This case will only pass if `x` has 0 or 1 active digits.
     */
    case lt_abs(x, y): return set(dst, y) if is_zero(x) else set(dst, y - x.digits[0])
    case:              return internal_sub(dst, x, y)
    }
}

sub :: proc {
    bigint_sub,
    bigint_sub_digit,
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
    n_len := max(x.active, y.active)
    internal_bigint_grow(dst, n_len) or_return
    
    /* 
        NOTE: May need to be negated depending on the assumption.
     */
    dst.sign = x.sign

    switch {
    /* 
        Assumptions 3 and 4. Accounts for:
            +x, -y, |x| < |y|:    1  - (-2) =  (1 + 2) =  3
            -x, +y, |x| < |y|:  (-1) -   2  = -(1 + 2) = -3
            +x, -y, |x| > |y|:    2  - (-1) =  (2 + 1) =  3
            -x, +y, |x| > |y|:  (-2) -   1  = -(2 + 1) = -3
     */
    case x.sign != y.sign: return internal_add(dst, x, y)

    /* 
        Assumptions 1 and 2. Accounts for:
            +x, +y, |x| < |y|:   1  -   2  = -(2 - 1) = -1
            -x, -y, |x| < |y|: (-1) - (-2) =  (2 - 1) =  1

        Note:
            We defer for the same reasons in `bigint_add()`.
     */
    case lt_abs(x, y):     defer neg(dst)
                           return internal_sub(dst, y, x)
    /* 
        Assumptions 1 and 2. Accounts for:
            +x, +y, |x| > |y|:   2  -   1  =  (2 - 1) =  1
            -x, -y, |x| > |y|: (-2) - (-1) = -(2 - 1) = -1
     */
    case:                  return internal_sub(dst, x, y)
    }
    
}

bigint_sub_digit :: proc(dst: ^BigInt, x: BigInt, y: DIGIT) -> Error {
    n_len := max(x.active, math.count_digits_of_base(y, DIGIT_BASE))
    internal_bigint_grow(dst, n_len) or_return
    dst.sign = x.sign
    
    switch {
    /* 
        Accounts for:
            -x, (|x| < y): -1 - 2 = -(1 + 2) = -3
            -x, (|x| > y): -2 - 1 = -(2 + 1) = -3
     */
    case is_neg(x):     return internal_add(dst, x, y)

    /* 
        Accounts for:
            +x, (|x| < y) = -z

        Note:
            We defer for the same reasons in `bigint_add()`.
     */
    case lt_abs(x, y): defer neg(dst)
                       return set(dst, SWORD(y - x.digits[0]))
    /* 
        Accounts for:
            +x, (|x| > y)
     */
    case:              return internal_sub(dst, x, y)
    }
}


// 1}}} ------------------------------------------------------------------------
