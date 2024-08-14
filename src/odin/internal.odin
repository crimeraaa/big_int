//+private
package bigint

/*
Wrapper to convert `mem.Allocator_Error` to `Error`.
 */
internal_resize :: proc(self: ^BigInt, nlen: int) -> Error {
    memerr := resize(&self.digits, nlen)
    if memerr != .None || len(self.digits) != nlen {
        return .Out_Of_Memory
    }
    return nil
}

/* 
Set `dst` to be `|x + y|`, ignoring signedness.

Determining the correct order of operands and signedness of the result is the
responsibility of the caller.
 */
internal_add_unsigned :: proc(dst, x, y: ^BigInt, minimize := false) -> Error {
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
        if sum >= DIGIT_BASE {
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
Set `dst` to be `x + digit`.

TODO: Handle signedness of `x`?
 */
internal_add_digit :: proc(dst, x: ^BigInt, digit: DIGIT_TYPE, minimize := false) -> Error {
    internal_resize(dst, x.active + 1) or_return
    carry := digit
    for index in 0..<x.active {
        sum := x.digits[index] + carry
        /* 
        TODO: `carry` will likely break when DIGIT_BASE != 10
         */
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
Set `dst` to be `|x - y|`, ignoring signedness. Assumes `x > y`.

Determining the correct order of operands and signedness of the result is the
responsibility of the caller.
 */
internal_sub_unsigned :: proc(dst, x, y: ^BigInt, minimize := false) -> Error {
    carry := false
    for index in 0..<dst.active {
        /* 
        Instead of mutating `x` by subtracting 1 from `x.digits[index + 1]`, we
        can just keep track of when we carried/borrowed.
        
        This has the bonus effect of allowing `x` and/or `y` to alias `dst`!
         */
        result: int = -1 if carry else 0
        if index < x.active {
            result += int(x.digits[index])
        }
        if index < y.active {
            result -= int(y.digits[index])
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
