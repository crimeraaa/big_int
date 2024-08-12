package bigint

import "core:fmt"
import "core:mem"
import "core:os"
import "core:unicode"
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
    for {
        fmt.print("> ")
        input, err := read_line(stdin)
        if err != nil {
            if err != .EOF {
                fmt.eprintfln("Error in read_line(): %v", err)                
            }
            break
        }
        defer delete(input)
        fmt.printfln("input: \"%s\"", input)
        print_number(input)
    }
}

@(test)
run_tests :: proc(_: ^testing.T) {
    print_number("1234")
    print_number("1,234,567,890")
    print_number("1_234_567_890")
    
    print_number("123457890A") // error, assumed base=10
    print_number("0b1102")     // eror, detected base=2
    print_number("0b1101")     // 13

    print_number("0xfeedbeef")
    print_number("0xc0ffee")
    print_number("0xfeedbeefc0ffee")
}

print_number :: proc(input: string, base: int = 0) {
    input, base  := input, base
    if base == 0 {
        prefix := string(input[:2] if len(input) > 2 else input[:])
        switch _base, status := detect_base(prefix); status {
        case .Prefix_None:
            base = 10
        case .Prefix_Valid:
            input, base = input[2:], _base
        case .Prefix_Invalid:
            fmt.eprintfln("Unknown base prefix '%s'", prefix)
            return
        }
    }
    value, exponent, place := 0, 0, 1
    #reverse for character in input {
        digit: int
        defer {
            value += digit * place
            exponent += 1
            place *= base
        }
        switch unicode.to_lower(character) {
        case '0'..='9': digit = int(character - '0')
        case 'a'..='f': digit = int(character - 'a') + 10
        case:
            continue
        }
        if !(0 <= digit && digit < base) {
            fmt.eprintfln("Invalid base-%i digit '%c'", base, character)
            return
        }
        fmt.printfln("% 2i * (%i ^ % -2i = %i)", digit, base, exponent, place)
    }
    fmt.printfln("base-%i to base-10: %v", base, value)
}

Detect_Base_Status :: enum {
    Prefix_None = 0,
    Prefix_Valid,
    Prefix_Invalid,
}

detect_base :: proc(prefix: string) -> (base: int, status: Detect_Base_Status) {
    base, status = 10, .Prefix_None
    if len(prefix) < 2 {
        return
    }
    if prefix[0] == '0' {
        status = .Prefix_Valid
        switch prefix[1] {
        case 'b': base = 2
        case 'o': base = 8
        case 'd': base = 10
        case 'x': base = 16
        case: status = .Prefix_Invalid
        }
    }
    return
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
