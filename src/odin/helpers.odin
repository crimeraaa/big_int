//+private
package bigint

import "base:intrinsics"
import "core:unicode"

Number_String :: struct {
    data:  string,
    radix: int,
    sign:  Sign,
}

/* 
    Skips leading whitespaces and signs. If possible, tries to set signedness.
 */
number_string_trim_leading :: proc(self: ^Number_String) {
    skipped := 0
    data := self.data
    sign := Sign.Positive
    for char in data {
        if unicode.is_alpha(char) || unicode.is_digit(char) {
            break
        }
        switch data[0] {
        case '+': sign = .Positive
        case '-': sign = .Negative if sign == .Positive else .Positive
        }
        skipped += 1
    }
    self.data = data[skipped:]
    self.sign = sign
}

/* 
    May mutate `self` by moving slicing `data` further.
 */
number_string_detect_radix :: proc(self: ^Number_String) -> (ok: bool) {
    // May have prefix AND some number of digits? e.g. "0x" alone is invalid.
    input := self.data
    if len(input) > 2 && input[0] == '0' && unicode.is_alpha(rune(input[1])) {
        radix := 0
        switch input[1] {
        case 'b', 'B': radix = 2
        case 'o', 'O': radix = 8
        case 'd', 'D': radix = 10
        case 'x', 'X': radix = 16
        case:
            return false
        }
        self.data  = input[2:]
        self.radix = radix
        return true
    }
    self.data  = input
    self.radix = 10
    return true
}

char_to_digit :: proc(char: rune, radix: int) -> (digit: int, ok: bool) {
    switch char {
    case '0'..='9': digit = int(char - '0')
    case 'a'..='z': digit = int(char - 'a' + 10)
    case 'A'..='Z': digit = int(char - 'A' + 10)
    case:
        return 0, false
    }
    if !(0 <= digit && digit < radix) {
        return 0, false
    }
    return digit, true
}

/* 
Assumptions:
    1. Base prefix, if any, was skipped.
    2. Leading 0's, if any, were skipped.
    3. Radix has been properly determined already.
 */
number_string_count_digits :: proc(self: Number_String) -> int {
    count := 0
    for char in self.data {
        _, ok := char_to_digit(char, self.radix)
        if ok {
            count += 1
        }
    }
    return count
}
