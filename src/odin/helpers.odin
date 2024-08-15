//+private
package bigint

import "core:unicode"
import "base:intrinsics"

/*
Get the number of `radix` digits needed to represent `value`.
 */
count_digits :: proc(value: $T, #any_int radix := 10) -> int
where intrinsics.type_is_integer(T) {
    value := value
    // This is always the same no matter the base (except base 0).
    if value == 0 {
        return 1
    }
    count := 0
    // We rely on the fact that integer division always truncates.
    for value != 0 {
        count += 1
        value /= T(radix)
    }
    return count
}

/* 
`rest` is a slice of `input` which excludes the prefix, if applicable. Otherwise,
if there is no prefix, we assume we just need to return `input` as is.
 */
detect_radix :: proc(input: string) -> (rest: string, radix: int, ok: bool) {
    // May have prefix AND some number of digits? e.g. "0x" alone is invalid.
    if len(input) > 2 && input[0] == '0' && unicode.is_alpha(rune(input[1])) {
        switch input[1] {
        case 'b', 'B': radix = 2
        case 'o', 'O': radix = 8
        case 'd', 'D': radix = 10
        case 'x', 'X': radix = 16
        case:
            return input, 10, false
        }
        return input[2:], radix, true
    }
    return input, 10, true
}

get_digit :: proc(character: rune, radix: int) -> (digit: int, ok: bool) {
    switch character {
    case '0'..='9': digit = int(character - '0')
    case 'a'..='f': digit = int(character - 'a' + 10)
    case 'A'..='F': digit = int(character - 'A' + 10)
    case:
        return 0, false
    }
    if !(0 <= digit && digit < radix) {
        return 0, false
    }
    return digit, true
}
