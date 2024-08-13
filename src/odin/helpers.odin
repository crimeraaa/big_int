//+private
package bigint

import "core:unicode"
import "base:intrinsics"

Helper_Error :: enum {
    None = 0,
    Invalid_Radix,
    Invalid_Digit,
}

@(require_results)
count_digits :: proc(value: $T, radix := int(10)) -> int {
    #assert(intrinsics.type_is_integer(T))
    if value == 0 {
        return 1
    }
    temp, count := value, 0
    // We rely on the fact that integer division always truncates.
    for temp != 0 {
        count += 1
        temp  /= T(radix)
    }
    return count
}

@(require_results)
detect_radix :: proc(input: string) -> (rest: string, radix: int, err: Helper_Error) {
    radix, rest, err = 10, input, .None
    if len(input) < 2 {
        return
    }

    if input[0] == '0' && unicode.is_alpha(rune(input[1])) {
        rest, err = input[2:], .None
        switch input[1] {
        case 'b': radix = 2
        case 'o': radix = 8
        case 'd': radix = 10
        case 'x': radix = 16
        case:
            rest, err = input, .Invalid_Radix
        }
    }
    return
}

@(require_results)
get_digit :: proc(character: rune, radix: int) -> (digit: int, err: Helper_Error) {
    err = .None
    switch character {
    case '0'..='9': digit = int(character - '0')
    case 'a'..='f': digit = int(character - 'a' + 10)
    case 'A'..='F': digit = int(character - 'A' + 10)
    case: err = .Invalid_Digit
    }
    if !(0 <= digit && digit < radix) {
        err = .Invalid_Digit
    }
    return
}
