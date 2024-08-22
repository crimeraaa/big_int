BASE = 10

def imul(a: int, b: int):
    a_digits = isplit(a, BASE)
    b_digits = isplit(b, BASE)
    print("a:", a_digits)
    print("b:", b_digits)
    for b_index, b_digit in enumerate(b_digits):
        form: list[str] = []
        eq:   list[str] = []
        for a_index, a_digit in enumerate(a_digits):
            form.append(f"(a{a_index}b{b_index})*(B**{b_index})")
            eq.append(f"({a_digit}*{b_digit})*({BASE}**{b_index})")
            if a_index != len(a_digits) - 1:
                form.append(" + ")
                eq.append(" + ")
            # print(f"a{a_index}b{b_index} = {a_digit}*{b_digit}", end=", ")
        print(f"a*b{b_index}\t=", "".join(form))
        print( "\t=", "".join(eq))
        print(f"\t= {a}*{b_digit}")
        print(f"\t= {a*b_digit} lshift {b_index}")


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
