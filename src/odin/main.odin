package bigint

import "core:fmt"
import "core:mem"
import "core:os"
import "core:io"

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
    zero, _ := bigint_make()
    defer bigint_destroy(&zero)

    bigint_set_zero(&zero)
    for {
        defer free_all(context.temp_allocator)

        fmt.print("x: ")
        first := try_read_line(stdin) or_break
        defer delete(first)

        fmt.print("y: ")
        second := try_read_line(stdin) or_break
        defer delete(second)
        
        x := try_make_bigint(first) or_break
        defer bigint_destroy(&x)

        y := try_make_bigint(second) or_break
        defer bigint_destroy(&y)
        
        dst := bigint_make(context.allocator) or_break
        defer bigint_destroy(&dst)
        
        fmt.print("x: ")
        bigint_print(x)

        fmt.print("y: ")
        bigint_print(y)
        
        bigint_add(&dst, &x, &y) or_break
        fmt.print("x + y = ")
        bigint_print(dst)
    }
}

try_make_bigint :: proc(input: string, allocator := context.allocator) -> (self: BigInt, err: Error) {
    self = bigint_make(allocator) or_return
    bigint_set_from_string(&self, input) or_return
    return self, nil
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
        err: Helper_Error
        prefix := string(input[:2]) if len(input) > 2 else input
        input, radix, err = detect_radix(input)
        if err == .Invalid_Radix {
            fmt.eprintfln("Unknown base prefix '%s'", prefix)
            return
        }
    }
    
    // Dependent on `base`.
    final_value, place_value := 0, 1
    #reverse for char in input {
        digit, err := get_digit(char, radix)
        if err == .Invalid_Digit {
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
