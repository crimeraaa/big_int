package bigint

import "base:intrinsics"
import "core:fmt"
import "core:mem"

/* 
`DIGIT_TYPE`:
    Our internal representation of a single digit of specified `DIGIT_BASE`.
 */
DIGIT_TYPE  :: distinct u32
DIGIT_BASE  :: 1_000_000_000
DIGIT_MAX   :: 2 * (DIGIT_BASE - 1)
DIGIT_WIDTH :: 9

#assert(DIGIT_BASE < max(DIGIT_TYPE))

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
    digits: [dynamic]DIGIT_TYPE, // each digit is in the range 0..<10^DIGIT_WIDTH
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

bigint_set_from_integer :: proc(self: ^BigInt, #any_int value: int) -> Error {
    if value == 0 {
        bigint_clear(self)
        return nil
    }
    ndigits := count_digits(value, DIGIT_BASE)
    
    internal_resize(self, ndigits) or_return
    self.active = ndigits
    self.sign   = .Positive if value >= 0 else .Negative
    
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
    return internal_add_digit(self, self, 1) if is_maxneg else nil
}

@(private, require_results)
count_only_numeric_characters :: proc(input: string, radix := 10) -> int {
    count := 0
    for char in input {
        _, err := get_digit(char, radix)
        if err == nil {
            count += 1
        }
    }
    return count
}

/* 
Assumes `input` represents a base-10 number, for simplicity.
 */
bigint_set_from_string :: proc(self: ^BigInt, input: string, radix := 10) -> Error {
    input  := input
    radix  := radix
    nchars := len(input)
    
    sign := Sign.Positive
    prefix_loop: for nchars > 1 {
        switch input[0] {
        case '-': sign = .Negative if sign == .Positive else .Positive
        case '+': sign = .Positive
        case '0':
            if radix == 0 && nchars > 2 && input[1] != '0' {
                switch input[1] {
                case 'b', 'B': radix = 2
                case 'o', 'O': radix = 8
                case 'd', 'D': radix = 10
                case 'x', 'X': radix = 16
                case:
                    break prefix_loop
                }
                input = input[1:]
                nchars -= 1
            }
        case:
            break prefix_loop
        }
        input = input[1:]
        nchars -= 1
    }
    if radix == 0 {
        radix = 10
    }
    if nchars == 0 {
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
    ndigits := count_only_numeric_characters(input, radix)
    nblocks := ndigits / DIGIT_WIDTH
    nextra  := ndigits % DIGIT_WIDTH
    if nextra != 0 {
        nblocks += 1
    }
    internal_resize(self, nblocks) or_return
    self.active = nblocks
    self.sign   = sign
    
    // Counters for base 10^9
    current_digit := DIGIT_TYPE(0)
    current_block := nblocks - 1
    block_index   := int(nextra if nextra > 0 else DIGIT_WIDTH)
    
    /* 
    TODO: Properly "slice" binary, octal and hexadecimal strings.
    e.g: printing out the raw slice of `0xffff_ffff` vs `4,294,967,295` gives us
    different results.
     */
    for char in input {
        if char == ' ' || char == '_' || char == ',' {
            continue
        }
        digit, err := get_digit(char, radix)
        if err != nil {
            fmt.eprintfln("Invalid base-%i digit '%c'", radix, char)
            bigint_destroy(self)
            return err
        }
        current_digit *= DIGIT_TYPE(radix)
        current_digit += DIGIT_TYPE(digit)
        block_index -= 1
        
        // At end of block, so set in array and prepare to write new block
        if block_index == 0 {
            self.digits[current_block] = current_digit
            current_block -= 1
            current_digit = 0
            block_index = DIGIT_WIDTH
        }
    }
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
    if bigint_is_zero(self) {
        fmt.print('0')
        if newline {
            fmt.println()
        }
        return
    }
    // Most significant digit should have no padding.
    if first := self.digits[self.active - 1]; bigint_is_negative(self) {
        fmt.print('-', first, sep = "")
    } else {
        fmt.print(first)
    }
    digits := self.digits[:self.active - 1]
    #reverse for digit in digits {
        // "%*spec" doesn't work anymore, need to do "%*[vararg-index]spec"
        // https://github.com/odin-lang/Odin/issues/3605#issuecomment-2143625629
        fmt.printf("%*[0]i", DIGIT_WIDTH, digit)
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

bigint_eq :: #force_inline proc(x, y: ^BigInt) -> bool {
    return bigint_cmp(x, y) == .Equal
}

bigint_lt :: #force_inline proc(x, y: ^BigInt) -> bool {
    return bigint_cmp(x, y) == .Less
}

bigint_gt :: #force_inline proc(x, y: ^BigInt) -> bool {
    return bigint_cmp(x, y) == .Greater
}

/* 
    x != y == !(x == y)
 */
bigint_neq :: #force_inline proc(x, y: ^BigInt) -> bool {
    return !bigint_eq(x, y)
}

/* 
    x <= y == !(x > y)
 */
bigint_leq :: #force_inline proc(x, y: ^BigInt) -> bool {
    return !bigint_gt(x, y)
}

/* 
    x >= y == !(x < y)
 */
bigint_geq :: #force_inline proc(x, y: ^BigInt) -> bool {
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
bigint_eq_abs :: #force_inline proc(x, y: ^BigInt) -> bool {
    return bigint_cmp_abs(x, y) == .Equal
}

/* 
    |x| < |y|
 */
bigint_lt_abs :: #force_inline proc(x, y: ^BigInt) -> bool {
    return bigint_cmp_abs(x, y) == .Less
}

/*  
    |x| > |y|
 */
bigint_gt_abs :: #force_inline proc(x, y: ^BigInt) -> bool {
    return bigint_cmp_abs(x, y) == .Greater
}

/* 
    |x| != |y| == !(|x| == |y|)
 */
bigint_neq_abs :: #force_inline proc(x, y: ^BigInt) -> bool {
    return !bigint_eq_abs(x, y)
}

/* 
    |x| <= |y| == !(|x| > |y|)
 */
bigint_leq_abs :: #force_inline proc(x, y: ^BigInt) -> bool {
    return !bigint_gt_abs(x, y)
}

/* 
    |x| >= |y| == !(|x| < |y|)
 */
bigint_geq_abs :: #force_inline proc(x, y: ^BigInt) -> bool {
    return !bigint_lt_abs(x, y)
}

// 1}}} ------------------------------------------------------------------------

// --- ARITHMETIC --------------------------------------------------------- {{{1

bigint_abs :: #force_inline proc(self: ^BigInt) {
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
