def count_digits(value: int, radix: int = 10):
    """ 
    Count the number of `radix` digits needed to represent `value`.
    """
    if value == 0:
        return 1
    count = 0
    # Since Python already uses big integers we lose no information this way.
    if value < 0:
        value = abs(value)
    # We rely on the fact that integer division truncates.
    while value != 0:
        value //= radix
        count += 1
    return count


def decode_number_string(number_string: str, radix: int = 0):
    """
    Description:
        Decode the number string `value` as a Python `int` of radix `radix`.

        If `radix` is 0, we will attempt to detect the base from the prefix if
        applicable.
    """
    if radix == 0:
        number_string, radix = detect_number_string_radix(number_string)
    # Toss away leading zeroes
    while len(number_string) > 1 and number_string[0] == '0':
        number_string = number_string[1:]
    final_value = 0
    for char in number_string:
        # Delimeters
        if char in " _,":
            continue
        digit, valid = decode_char_to_digit(char, radix)
        if not valid:
            raise ValueError(f"Invalid base-{radix} digit '{char}'")
        # This part is important to keep in mind for BigInt!
        final_value *= radix
        final_value += digit
    return final_value



def detect_number_string_radix(value: str) -> tuple[str, int]:
    """ 
    Description:
        Interpret the base prefix of the number string `value`.
        Base prefixes are things like `0b` or `0o` or `0x`.
        `0d` is also supported for completeness.
        
    Raises:
        `KeyError` if we detected a base prefix but it wasn't present in the
        constant `NUMBER_BASES`.
        
    Returns:
        If a prefix was found, the first return value will be the remainder of
        the string without the prefix.
        Otherwise, it will be `value`.

        If a prefix was found, the 2nd return value is the detected radix.
        Otherwise, it will be `10`.
    """
    if len(value) > 2 and value[0] == '0' and value[1].isalpha():
        key = value[1]
        if key in RADIX_PREFIXES:
            return value[2:], RADIX_PREFIXES[key]
        else:
            raise ValueError(f"Unknown base prefix '{key}'")
    else:
        return value, 10


RADIX_PREFIXES = {
    'b': 2,  'B': 2,
    'o': 8,  'O': 8,
    'd': 10, 'D': 10,
    'x': 16, 'X': 16,
}


def decode_char_to_digit(char: str, radix: int) -> tuple[int, bool]:
    digit, valid = 0, False
    # May be a hex character, may not be
    if char.isalpha():
        digit = ord(char.upper()) - ord('A') + 10
    elif char.isdigit():
        digit = int(char)
    if 0 <= digit < radix:
        valid = True
    return digit, valid


# For quick and dirty testing in the interactive interpreter
# See: https://stackoverflow.com/questions/2356399/tell-if-python-is-in-interactive-mode
#
# NOTE: Pass '-i' flag, e.g `python -i helper.py`
import sys
if sys.flags.interactive:
    global cd, dns
    cd, dns = count_digits, decode_number_string
