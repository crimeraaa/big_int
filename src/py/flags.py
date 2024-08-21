BIT = [0b0000, 0b0001, 0b0010, 0b0100, 0b1000]

def print_flags(f):
    print("Flags: ", end="")
    for i in range(len(BIT) - 1, 0, -1):
        bit = 1 if f & BIT[i] != 0 else 0
        print(bit, end="")
    print()


print("iflags:")
iflags  = BIT[0]; print_flags(iflags); # 0b0000
iflags |= BIT[1]; print_flags(iflags); # 0b0001
iflags |= BIT[4]; print_flags(iflags); # 0b1001
iflags ^= BIT[1]; print_flags(iflags); # 0b1000

print("oflags:")
oflags = 0; print_flags(oflags); # 0b0000
oflags |= iflags & BIT[1]; print_flags(oflags); # 0b0000
oflags |= iflags & BIT[4]; print_flags(oflags); # 0b1000
