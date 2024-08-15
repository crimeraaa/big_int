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
