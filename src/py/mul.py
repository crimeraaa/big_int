BASE = 10
MAX = BASE - 1

def main():
    while True:
        try:
            x_string = input("x: ")
            y_string = input("y: ")
        except EOFError:
            break
        
        x_digits = int(x_string, 0)
        y_digits = int(y_string, 0)
        bigint_to_bigint_mul(x_digits, y_digits)
        # x_value  = int(x_string, 0)
        # y_value  = int(y_string, 0)
        # x_digits = isplit(x_value)
        
        # try:
        #     bigint_to_digit_mul(x_digits, y_value)
        # except ValueError as err:
        #     print(err)
        #     continue
        # print(f"{x_value} * {y_value} = {x_digits}")


def bigint_to_digit_mul(x_digits: list[int], y: int):
    carry = 0
    if not (0 <= y and y < BASE):
        raise ValueError(f"multiplier '{y}' is not a single base-{BASE} digit")
    for index, value in enumerate(x_digits):
        q, r = divmod(value * y, BASE)
        print(f"{value} * {y} = (upper: {q}, lower: {r}), ", end="")
        digit = r + carry
        if digit >= BASE:
            digit -= BASE
            carry = 1
        else:
            carry = 0
        x_digits[index] = digit
        carry += q
        print(f"x_digits[{index}] = (carry: {carry}, digit: {digit})")
        
    if carry != 0:
        x_digits.append(carry)

    

def bigint_to_bigint_mul(a: int, b: int):
    a_digits = isplit(a, BASE)
    b_digits = isplit(b, BASE)
    print("a:", a_digits, "b:", b_digits)
    for b_index, b_digit in enumerate(b_digits):
        print(f"b[{b_index}] = {b_digit}")
        for a_index, a_digit in enumerate(a_digits):
            product = a_digit * b_digit
            print(f"\ta[{a_index}] = {a_digit}; {a_digit} * {b_digit} = {product}")
            # print(f"\ta[{a_index}]*b[{b_index}] = {product}")
        print()


def isplit(value: int, radix: int = 10) -> list[int]:
    """ 
    Note: digits are stored with least significant in the lowest index first.
    """
    digits: list[int] = []
    iter = abs(value)
    while iter != 0:
        rest, digit = divmod(iter, radix)
        digits.append(digit)
        iter = rest
    return digits


if __name__ == "__main__":
    main()


""" 
Let B = 10.

Example:
       12
    *  34
    =  48
    + 360
    = 408
    
Can be generalized as:
        a1  a0
    *   b1  b0
    =   (a0b0 + (a1b0)B^1)) lshift 0
    +   (a0b1 + (a1b1)B^1)) lshift 1

Or:
                a1      a0
    *           b1      b0
    =           a1b0    a0b0
        a1b1    a0b0    -
    =
    
Another example:
        1234
    *    567
    =   8638 = ( (a0b0)B^0 + (a1b0)B^1 + (a2b0)B^2 + (a3b0)B^3 ) lshift 0
       74040 = ( (a0b1)B^0 + (a1b1)B^1 + (a2b1)B^2 + (a3b1)B^3 ) lshift 1
             = ( (a0b2)B^0 + (a1b2)B^1 + (a2b2)B^2 + (a3b2)B^3 ) lshift 2

Can be generalized as:
                        a3      a2      a1      a0
    *                           b2      b1      b0
    =                   a3b0    a2b0    a1b0    a0b0
                a3b1    a2b1    a1b1    a0b1    -
        a3b2    a2b2    a1b2    a0b2    -       -

Or:
    =   lshift 0 (a0b0)
    +   lshift 1 (a1b0 + a0b1)
    +   lshift 2 (a2b0 + a1b1 + a0b2)
    +   lshift 3 (a3b0 + a2b1 + a1b1)
    +   lshift 4 (a3b1 + a2b2)
    +   lshift 5 (a3b2)
"""
