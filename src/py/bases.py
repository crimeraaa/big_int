BASE  = int(1e9)
MAX   = BASE - 1

# For non-decimal number strings, trim the `0{b,o,x}` prefix.
MAX_STRINGS = {
    2:  f"{MAX:0b}",
    8:  f"{MAX:0o}",
    16: f"{MAX:0x}",
}

RADIX_TO_MAX_CHAR = {
    2: '1',
    8: '7',
    10: '9',
    16: 'f',
}

print(f"Number of base-N digits that fit in a base-{BASE} digit:")
for radix, repr in MAX_STRINGS.items():
    n = len(repr)
    # {var: <2} means left-align  to 2 whitespaces ' '
    # {var: >2} means right-align to 2 whitespaces ' '
    # If you replace ' ' with '0', you'll fill the alignment gaps with '0'.
    print(f"{MAX} => base-{radix} : {repr} ({n} digits)")
    # print(f"\tbase-{radix: <2} ({len(repr): >2} digits): {repr}")
    
    digits: list[str] = []
    for i in range(n):
        digits.append(RADIX_TO_MAX_CHAR[radix])
    baseN = "".join(digits)
    base10 = str(int(baseN, radix))
    print(f"\tmax base-{radix} of len {n}: {baseN} ({len(baseN)} base-{radix} digits)")
    print(f"\tabove value in base-10:      {base10} ({len(base10)} base-10 digits)")
    

