DIGIT_BASE  = 10_000_000
DIGIT_MAX   = DIGIT_BASE - 1

# For non-decimal number strings, trim the `0{b,o,x}` prefix.
DIGIT_WIDTH = {
    2:  bin(DIGIT_MAX)[2:],
    8:  oct(DIGIT_MAX)[2:],
    10: str(DIGIT_MAX),
    16: hex(DIGIT_MAX)[2:],
}

print(f"Number of base-N digits that fit in a base-{DIGIT_BASE} digit:")
for radix, repr in DIGIT_WIDTH.items():
    # {var: <2} means left-align  to 2 whitespaces ' '
    # {var: >2} means right-align to 2 whitespaces ' '
    # If you replace ' ' with '0', you'll fill the alignment gaps with '0'.
    print(f"\tbase-{radix: <2} ({len(repr): >2} digits): {repr}")
