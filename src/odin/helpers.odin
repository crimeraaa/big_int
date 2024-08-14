//+private
package bigint

import "core:unicode"
import "base:intrinsics"

/*
Get the number of `radix` digits needed to represent `value`.
 */
count_digits :: proc(value: $T, #any_int radix := int(10)) -> int
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

detect_radix :: proc(input: string) -> (rest: string, radix: int, err: Error) {
    if len(input) < 2 {
        return input, 10, nil
    }

    if input[0] == '0' && unicode.is_alpha(rune(input[1])) {
        switch input[1] {
        case 'b', 'B': radix = 2
        case 'o', 'O': radix = 8
        case 'd', 'D': radix = 10
        case 'x', 'X': radix = 16
        case:
            return input, 10, .Invalid_Radix
        }
    }
    return input[2:], radix, nil
}

get_digit :: proc(character: rune, radix: int) -> (digit: int, err: Error) {
    switch character {
    case '0'..='9': digit = int(character - '0')
    case 'a'..='f': digit = int(character - 'a' + 10)
    case 'A'..='F': digit = int(character - 'A' + 10)
    case:
        return 0, .Invalid_Digit
    }
    if !(0 <= digit && digit < radix) {
        return 0, .Invalid_Digit
    }
    return digit, nil
}
