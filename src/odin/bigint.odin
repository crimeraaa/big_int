package bigint

import "base:intrinsics"
import "core:fmt"
import "core:mem"

Error :: union {
    Helper_Error,
    mem.Allocator_Error,
}

Sign :: enum i8 {
    Positive = 1,  // For simplicity, even 0 is considered positive.
    Negative = -1,
}

BigInt :: struct {
    digits: [dynamic]u8, // each digit is in the range 0..=9
    active: int,
    sign:   Sign,
}

@(private="package")
DIGIT_BASE :: 10

bigint_init :: proc {
    bigint_init_none,
    bigint_init_cap,
}

bigint_init_none :: proc(self: ^BigInt, allocator := context.allocator) -> Error {
    return bigint_init_cap(self, 0, allocator)
}

bigint_init_cap :: proc(self: ^BigInt, capacity: int, allocator := context.allocator) -> Error {
    // make($T, len, allocator) -> mem.Allocator_Error
    self.digits = make([dynamic]u8, capacity, allocator) or_return
    bigint_set_zero(self)
    return nil
}

bigint_is_zero :: proc(self: BigInt) -> bool {
    return self.active == 0
}

bigint_set :: proc {
    bigint_set_zero,
    bigint_set_integer,
    bigint_set_string,
}

bigint_set_zero :: proc(self: ^BigInt) {
    self.active = 0
    self.sign   = .Positive
}

bigint_set_integer :: proc(self: ^BigInt, value: $T) -> Error {
    #assert(intrinsics.type_is_integer(T))
    if value == 0 {
        bigint_set_zero(self)
        return nil
    }
    ndigits := count_digits(value)
    
    resize(&self.digits, ndigits) or_return
    self.active = ndigits
    self.sign   = .Positive if value >= 0 else .Negative
    
    /* 
        Will getting the absolute value cause `rest` to overflow?
        e.g. -(i8(-128)) will result in 0 because +128 does not fit in an i8.
    */
    rest := value
    is_maxneg := rest == min(T)
    if is_maxneg {
        rest += 1        
    }
    rest = rest if value >= 0 else -rest
    for index in 0..<ndigits {
        defer rest /= DIGIT_BASE
        digit := rest % DIGIT_BASE
        self.digits[index] = u8(digit)
    }
    /* 
        TODO: Add 1 again when is_maxneg
     */
    return nil
}

/* 
    Assumes `input` represents a base-10 number, for simplicity.
 */
bigint_set_string :: proc(self: ^BigInt, input: string, minimize := false) -> Error {
    input   := input
    ndigits := len(input)

    sign: Sign = .Positive
    for 1 <= len(input) && input[0] == '-' {
        input = input[1:]
        sign  = .Negative if sign == .Positive else .Positive
        ndigits -= 1
    }

    if ndigits == 0 {
        bigint_set_zero(self)
        return nil
    }
    
    // `ndigits` may be overkill if there are delimeters.
    resize(&self.digits, ndigits) or_return
    self.active = 0
    self.sign   = sign
    
    // Right (least significant) to left (most significant)
    #reverse for char in input {
        switch char {
        case '0'..='9':
            self.digits[self.active] = u8(char - '0')
            self.active += 1
        case ' ', '_', ',':
            continue
        case:
            fmt.eprintfln("Unknown base-10 digit '%c'", char)
            return .Invalid_Digit
        }
    }
    trim_leading_zeroes(self, minimize) or_return
    return nil
}

@(private="file")
trim_leading_zeroes :: proc(self: ^BigInt, minimize := false) -> mem.Allocator_Error {
    count := 0
    #reverse for digit in self.digits[:self.active] {
        if digit == 0 {
            count += 1
        } else {
            break
        }
    }
    if self.active -= count; self.active == 0 {
        self.sign = .Positive
    }
    if minimize {
        resize(&self.digits, self.active) or_return
    }
    return .None
}

bigint_print :: proc(self: BigInt, newline := true) {
    if self.sign == .Negative {
        fmt.print('-')
    }
    // Print only the active digits as otherwise we'll print all allocated.
    #reverse for digit in self.digits[:self.active] {
        fmt.print(digit)
    }
    if bigint_is_zero(self) {
        fmt.print('0')
    }
    if newline {
        fmt.println()
    }
}

bigint_destroy :: proc(self: ^BigInt) {
    delete(self.digits)
    bigint_set_zero(self)
}
