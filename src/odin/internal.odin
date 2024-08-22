//+private
package bigint

import "core:log"
import "core:math"

Block_Info :: struct {
    radix: int, // What are we writing in terms of?
    width: int, // How many `radix` digits can likely fit in a block?
    index: int, // Where in the block are we currently at, in terms of `radix`?
}

/*
    Note: The builtin procedure `resize` does *not* take an `Allocator`, since
    dynamic arrays already remember their allocators.
 */
internal_bigint_grow :: proc(self: ^BigInt, n_len: int) -> Error {
    // The purpose of this function is only to grow, shrinking is a separate
    // operation. However we do want to update the active count.
    self.active = n_len
    if n_len <= len(self.digits) {
        return nil
    }
    memerr := resize(&self.digits, n_len)
    if memerr != .None || len(self.digits) != n_len {
        log.fatal("Failed to [re]allocate memory!")
        return memerr
    }
    return nil
}

/* 
    Assumes that we verified `radix` is already valid.
    Returns the number of `radix` digits that can likely fit in `DIGIT_BASE`.
 */
internal_get_block_width :: proc(#any_int radix: int) -> int {
    switch radix {
    case 2:  return DIGIT_WIDTH_BIN
    case 8:  return DIGIT_WIDTH_OCT
    case 10: return DIGIT_WIDTH
    case 16: return DIGIT_WIDTH_HEX
    }
    return DIGIT_WIDTH
}

/* 
    WARNING: This will break for non base-10 large number strings.
    TODO(2024-08-18): Perhaps re-implement in terms of high-level multiplication?
 */
internal_bigint_set_from_string :: proc(self: ^BigInt, input: string, block: Block_Info) -> Error {
    block := block
    // Counters for base 10^9
    current_digit := SWORD(0)
    current_block := self.active - 1
    carry_digits  := SWORD(0)
    
    for char in input {
        if char == ' ' || char == '_' || char == ',' {
            continue
        }
        digit, ok := char_to_digit(char, block.radix)
        if !ok {
            log.errorf("Invalid base-%i digit '%c'", block.radix, char)
            return .Invalid_Digit
        }
        current_digit *= SWORD(block.radix)
        current_digit += SWORD(digit)
        block.index -= 1
        
        /* 
            If we overflowed, we need to save the digit/s that caused us to overflow
            and remove them from the current digit.
         */
        overflowed := current_digit > DIGIT_MAX
        if overflowed {
            carry_digits = current_digit / DIGIT_BASE
            current_digit -= carry_digits * DIGIT_BASE
        } else {
            carry_digits = 0
        }
        
        // "flush" the current block by writing it.
        if block.index == 0 || overflowed {
            self.digits[current_block] = DIGIT(current_digit)
            current_block -= 1
            current_digit = carry_digits
            block.index = block.width
        }
    }
    
    if current_digit != 0 {
        internal_bigint_grow(self, self.active + 1) or_return
        self.digits[self.active - 1] = DIGIT(current_digit)
    }
    return nil
}

internal_bigint_set_from_string_slow :: proc(self: ^BigInt, line: Number_String) -> Error {
    for char in line.data {
        if char == ' ' || char == '_' || char == ',' {
            continue
        }
        digit, ok := char_to_digit(char, line.radix)
        if !ok {
            log.errorf("Invalid base-%i digit '%c'", line.radix, char)
            return .Invalid_Digit
        }
        bigint_mul_digit(self, self^, DIGIT(line.radix)) or_return
        bigint_add_digit(self, self^, DIGIT(digit))      or_return
    }
    log.debug("self.digits", self.digits[:])
    return nil
}

///--- ARITHMETIC --------------------------------------------------------- {{{1

/* 
    The `internal_add_*` functions do not take signedness into account.
 */
internal_add :: proc {
    internal_bigint_add,
    internal_bigint_add_digit,
}

/* 
    Set `dst` to `|x| + |y|`, ignoring signedness of either addend.
    Determining signedness of the result is the responsibility of the caller.
 */
internal_bigint_add :: proc(dst: ^BigInt, x, y: BigInt) {
    carry := DIGIT(0)
    // Iterate from least significant to most significant.
    for &digit, index in dst.digits[:dst.active] {
        sum := carry
        
        // If index is out of range, it is implicitly 0 (i.e. add nothing).
        if index < x.active {
            sum += x.digits[index]
        }
        if index < y.active {
            sum += y.digits[index]
        }
        // Need to carry?
        if sum > DIGIT_MAX {
            carry = 1
            sum  -= DIGIT_BASE
        } else {
            carry = 0
        }
        digit = sum
    }
}

/* 
    Set `dst` to `|x| + |y|`, ignoring the signedness of `x`.
 */
internal_bigint_add_digit :: proc(dst: ^BigInt, x: BigInt, y: DIGIT) {
    carry := y
    for value, index in x.digits[:x.active] {
        sum := value + carry
        if sum >= DIGIT_BASE {
            sum  -= DIGIT_BASE
            carry = 1
        } else {
            carry = 0
        }
        dst.digits[index] = sum
        if carry == 0 {
            break
        }
    }
}

/* 
    Like their `internal_add_*` counterparts, these don't consider signedness.
 */
internal_sub :: proc {
    internal_bigint_sub,
    internal_bigint_sub_digit,
}

/* 
    Assumes `|x| > |y|` and sets `dst` to `|x| - |y|`, ignoring signedness.
    Determining the correct order of operands and signedness of the result is the
    responsibility of the caller.
 */
internal_bigint_sub :: proc(dst: ^BigInt, x, y: BigInt) {
    carry := SWORD(0)
    for index in 0..<dst.active {
        /* 
            Instead of mutating `x` by subtracting 1 from `x.digits[index + 1]`,
            we can just keep track of when we carried/borrowed.
            
            This has the bonus effect of allowing `x` and/or `y` to alias `dst`!
         */
        diff := -carry
        if index < x.active {
            diff += SWORD(x.digits[index])
        }
        if index < y.active {
            diff -= SWORD(y.digits[index])
        }
        // Need to carry?
        if diff < 0 {
            diff += DIGIT_BASE
            carry = 1
        } else {
            carry = 0
        }
        dst.digits[index] = DIGIT(diff)
    }
}

/*
    Set `dst` to `|x| - |y|`, ignoring signedness of `x`.
 */
internal_bigint_sub_digit :: proc(dst: ^BigInt, x: BigInt, y: DIGIT) {
    carry := SWORD(y)
    for index in 0..<dst.active {
        diff := -carry
        if index < x.active {
            diff += SWORD(x.digits[index])
        }
        if diff < 0 {
            diff += DIGIT_BASE
            carry = 1
        } else {
            carry = 0
        }
        dst.digits[index] = DIGIT(diff)
        /* 
            We don't defer because if we deferred at the start, `defer` would
            evaluate the value of `carry` immediately!
        */
        if carry == 0 {
            break
        }
    }
}

internal_mul :: proc {
    internal_bigint_mul,
    internal_bigint_mul_digit,
}

/* 
    Assumes we already checked if either `x` or `y` are 0, we allocated enough
    space for `dst` and determined the signedness of the result.

    This will do long multiplication. We will iterate each digit of `x` and then
    sub-iterate each digit of `y`. 
    
    Each time, we will split the digit-to-digit product into its the upper DIGIT
    portion and lower DIGIT portion.

    Example in base-10:
    
           9
        *  7
        = 63

        upper = 6
        lower = 3
    
    Example in base-1_000_000_000:

                      999_999_999
        *             999_999_999
        = 999_999_998_000_000_001

        upper = 999_999_998
        lower = 000_000_001
    
*/
internal_bigint_mul :: proc(dst: ^BigInt, x, y: BigInt) {
    carry := DIGIT(0)
    for x_digit, x_index in x.digits[:x.active] {
        for y_digit, y_index in y.digits[:y.active] {
            /* 
                Note:
                    We specifically need to cast BOTH operands, not just the,
                    as if we do `UWORD(x * y)` then `x* y` may do DIGIT math
                    and overflow before the conversion to `UWORD`.
             */
            upper, lower := internal_split_product(x_digit, y_digit)
            
            /* 
                Add lower into `dst` at this particular offset, accounting for
                overflow as in addition.
                
                Note:
                    We may revisit this index multiple times hence we assume it
                    started out as zero and got gradually updated.

                    Remember that we are summing up the factors.
             */
            index := x_index + y_index
            digit := dst.digits[index]
            if digit += carry + lower; digit > DIGIT_MAX {
                digit -= DIGIT_BASE
                carry = 1
            } else {
                carry = 0
            }
            dst.digits[index] = digit
            
            // No need to check, may be zero anyway
            carry += upper
        }
        
        /*
            Deal with leftover carry by assigning to the tail position for this
            particular run.

            Maximum index is (x.active + y.active) - 1 so we assume this is safe.
        */
        if carry != 0 {
            dst.digits[x_index + y.active] = carry
        }
    }
}

internal_bigint_mul_digit :: proc(dst: ^BigInt, x: BigInt, y: DIGIT) {
    carry := y
    for x_digit, x_index in x.digits[:x.active] {
        upper, lower := internal_split_product(x_digit, carry)
        digit        := lower
        log.debug(upper, ',', lower, ":=", x_digit, '*', carry)
        if digit > DIGIT_MAX {
            digit -= DIGIT_BASE
            carry = 1
        } else {
            carry = 0
        }
        dst.digits[x_index] = digit
        // Might need to keep going
        if carry += upper; carry == 0 {
            break
        }
    }
    // `x.active` cannot be 0 here, should have checked before calling this.
    if carry != 0 {
        dst.digits[dst.active - 1] = carry
    }
}

/* 
    `DIGIT` to `DIGIT` multiplication most likely needs `UWORD` to act as an
    intermediate value capable of holding the product.
 */
internal_split_product :: proc(x, y: DIGIT) -> (upper: DIGIT, lower: DIGIT) {
    quot, rem   := math.divmod(UWORD(x) * UWORD(y), UWORD(DIGIT_BASE))
    upper, lower = DIGIT(quot), DIGIT(rem)
    return upper, lower
}

internal_count_trailing_zeroes :: proc(self: BigInt) -> int {
    count := 0
    #reverse for digit in self.digits[:] {
        if digit == 0 {
            count += 1
        } else {
            break
        }
    }
    return count
}

///--- 1}}} --------------------------------------------------------------------
