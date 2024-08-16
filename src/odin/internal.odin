//+private
package bigint

import "core:log"

/*
Wrapper to convert `mem.Allocator_Error` to `Error`.

Note: The builtin procedure `resize` does *not* take an `Allocator`, since
dynamic arrays already remember their allocators.
 */
internal_resize :: proc(self: ^BigInt, nlen: int) -> Error {
    memerr := resize(&self.digits, nlen)
    if memerr != .None || len(self.digits) != nlen {
        log.fatal("Failed to [re]allocate memory!")
        return Error(memerr)
    }
    self.active = nlen
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

Block_Info :: struct {
    radix: int, // What are we writing in terms of?
    width: int, // How many `radix` digits can likely fit in a block?
    index: int, // Where in the block are we currently at, in terms of `radix`?
}

/* 
WARNING: This will break for non base-10 large number strings.
 */
internal_set_from_string :: proc(self: ^BigInt, input: string, block: Block_Info) -> Error {
    block := block
    // Counters for base 10^9
    current_digit := WORD_TYPE(0)
    current_block := self.active - 1
    carry_digits  := WORD_TYPE(0)
    
    for char in input {
        if char == ' ' || char == '_' || char == ',' {
            continue
        }
        digit, ok := convert_char_to_digit(char, block.radix)
        if !ok {
            log.errorf("Invalid base-%i digit '%c'", block.radix, char)
            return .Invalid_Digit
        }
        current_digit *= WORD_TYPE(block.radix)
        current_digit += WORD_TYPE(digit)
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
            log.debugf("[%i] := %i", current_block, current_digit)
            self.digits[current_block] = DIGIT_TYPE(current_digit)
            current_block -= 1
            current_digit = carry_digits
            block.index = block.width
        }
    }
    
    if current_digit != 0 {
        internal_resize(self, self.active + 1) or_return
        self.digits[self.active - 1] = DIGIT_TYPE(current_digit)
        log.debugf("[%i] := %i", self.active - 1, current_digit)
    }
    log.debug("result:", self.digits[:])
    return nil
}

/* 
Set `dst` to be `|x| + |y|`, ignoring signedness of either addend.

Determining signedness of the result is the responsibility of the caller.
 */
internal_add_unsigned :: proc(dst: ^BigInt, x, y: BigInt, minimize := false) -> Error {
    carry := DIGIT_TYPE(0)
    // Iterate from least significant to most significant.
    for index in 0..<dst.active {
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
        dst.digits[index] = sum
    }
    return bigint_trim(dst, minimize)
}

/* 
Set `dst` to be `|x| + digit`.

TODO: Handle signedness of `x`?
 */
internal_add_digit :: proc(dst: ^BigInt, x: BigInt, digit: DIGIT_TYPE, minimize := false) -> Error {
    internal_resize(dst, x.active + 1) or_return
    carry := digit
    for index in 0..<x.active {
        sum := x.digits[index] + carry
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
    return bigint_trim(dst, minimize)
}

/* 
Set `dst` to be `|x| - |y|`, ignoring signedness. Assumes `x > y`.

Determining the correct order of operands and signedness of the result is the
responsibility of the caller.
 */
internal_sub_unsigned :: proc(dst: ^BigInt, x, y: BigInt, minimize := false) -> Error {
    carry := false
    for index in 0..<dst.active {
        /* 
        Instead of mutating `x` by subtracting 1 from `x.digits[index + 1]`, we
        can just keep track of when we carried/borrowed.
        
        This has the bonus effect of allowing `x` and/or `y` to alias `dst`!
         */
        result := WORD_TYPE(-1 if carry else 0)
        if index < x.active {
            result += WORD_TYPE(x.digits[index])
        }
        if index < y.active {
            result -= WORD_TYPE(y.digits[index])
        }
        // Need to carry?
        if result < 0 {
            carry = true
            result += DIGIT_BASE
        } else {
            carry = false
        }
        dst.digits[index] = DIGIT_TYPE(result)
    }
    return bigint_trim(dst, minimize)
}
