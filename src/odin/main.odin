package bigint

import "core:fmt"
import "core:mem"
import "core:os"
import "core:io"
import "core:testing"

main :: proc() {
    // https://gist.github.com/karl-zylinski/4ccf438337123e7c8994df3b03604e33
    when ODIN_DEBUG {
        track: mem.Tracking_Allocator
        mem.tracking_allocator_init(&track, context.allocator)
        context.allocator = mem.tracking_allocator(&track)
        defer {
            print_tracking_allocator(track)
            mem.tracking_allocator_destroy(&track)
        }
    }
    stdin := os.stream_from_handle(os.stdin)
    x, y, dst: BigInt
    /* 
    NOTE: Assumes all BigInt was allocated via `context.allocator`!
     */
    defer {
        bigint_destroy(&dst)
        bigint_destroy(&y)
        bigint_destroy(&x)
    }

    input_loop: for {
        defer free_all(context.temp_allocator)

        fmt.print("Enter a value for 'x': ")
        first := try_read_line(stdin, context.temp_allocator) or_break
        fmt.print("Enter a value for 'y': ")
        second := try_read_line(stdin, context.temp_allocator) or_break
        
        #partial switch bigint_set_from_string(&x, first, 0) {
        case .Out_Of_Memory:
            break input_loop
        case .Invalid_Digit, .Invalid_Radix:
            continue input_loop
        }
        #partial switch bigint_set_from_string(&y, second, 0) {
        case .Out_Of_Memory:
            break input_loop
        case .Invalid_Digit, .Invalid_Radix:
            continue input_loop
        }
        
        fmt.print("x, y := ")
        bigint_print(x, newline = false)
        fmt.print(", ")
        bigint_print(y)
        
        bigint_add(&dst, x, y) or_break
        fmt.print("x + y = ")
        bigint_print(dst)
        
        bigint_sub(&dst, x, y) or_break
        fmt.print("x - y = ")
        bigint_print(dst)
    }
}

@(test)
arith_tests :: proc(test: ^testing.T) {
    x, y, dst: BigInt
    defer {
        bigint_destroy(&x)
        bigint_destroy(&y)
        bigint_destroy(&dst)
    }
    bigint_init_multi(&x, &y, &dst)
    
    /* 
    Accounts for |x| < |y|:
        1    +   2  and   1  -   2
        1    + (-2) and   1  - (-2)
        (-1) +   2  and  (-1) -   2
        (-1) + (-2) and (-1) - (-2)
     */
    fmt.println("=== begin tests ===", flush = false)
    for mulx := 1; mulx >= -1; mulx -= 2 {
        for muly := 1; muly >= -1; muly -= 2 {
            bigint_set_from_integer(&x, 1 * mulx)
            bigint_set_from_integer(&y, 2 * muly)

            bigint_add(&dst, x, y)
            print_equation(x, y, dst, '+')
            
            bigint_sub(&dst, x, y)
            print_equation(x, y, dst, '-')
        }
    }
    fmt.println("=== end tests ===", flush = false)
    fmt.println(flush = true)
}

@(test)
set_tests :: proc(_: ^testing.T) {
    x: BigInt
    defer bigint_destroy(&x)
    
    fmt.println("=== begin test ===", flush = false)
    
    bigint_set_from_integer(&x, min(u8))

    buf: [64]byte
    fmt.printfln("x := %s", bigint_to_string(x, buf[:]), flush = false)
    
    fmt.println("=== end test ===", flush = true)
}

print_equation :: proc(x, y, dst: BigInt, op: rune) {
    xbuf, ybuf, zbuf: [64]byte
    xstr, ystr := bigint_to_string(x, xbuf[:]), bigint_to_string(y, ybuf[:])
    zstr := bigint_to_string(dst, zbuf[:])
    fmt.printf("%s %c %s = %s\n", xstr, op, ystr, zstr, flush = false)
}

try_read_line :: proc(stream: io.Stream, allocator := context.allocator) -> (string, bool) {
    input, err := read_line(stream, allocator)
    ok := err == nil
    if !ok {
        if err != .EOF {
            fmt.eprintfln("Error in read_line(): %v", err)
        }
    }
    return input, ok
}

print_comparison :: proc(x, y: BigInt) {
    result := bigint_cmp(x, y)
    bigint_print(x, newline = false)
    fmt.printf(" is %s ", Comparison_String[result])
    bigint_print(y, newline = true)
}

// This is just here so I can conceptualize how to convert between bases.
print_number :: proc(input: string, radix := int(0)) {
    input, radix := input, radix
    if radix == 0 {
        ok: bool
        prefix := string(input[:2]) if len(input) > 2 else input
        input, radix, ok = detect_radix(input)
        if !ok {
            fmt.eprintfln("Unknown base prefix '%s'", prefix)
            return
        }
    }
    
    // Dependent on `base`.
    final_value, place_value := 0, 1
    #reverse for char in input {
        digit, ok := get_digit(char, radix)
        if !ok {
            fmt.eprintfln("Unknown base-%i digit '%c'", radix, char)
            return
        }
        final_value += digit * place_value
        place_value *= radix
    }
    fmt.println(final_value)
}

@(private="file", disabled=!ODIN_DEBUG)
print_tracking_allocator :: proc(track: mem.Tracking_Allocator) {
    if count := len(track.allocation_map); count > 0 {
        fmt.eprintfln("=== %v allocations not freed: ===", count)
        for _, entry in track.allocation_map {
            fmt.eprintfln("- %v bytes @ @v", entry.size, entry.location)
        }
    }
    if count := len(track.bad_free_array); count > 0 {
        fmt.eprintfln("=== %v incorrect frees: ===", count)
        for entry in track.bad_free_array {
            fmt.eprintfln("- %p @ %v", entry.memory, entry.location)
        }
    }
}
