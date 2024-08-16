package bigint

import "core:fmt"
import "core:mem"
import "core:os"
import "core:io"
import "core:testing"
import "core:log"

stdin: io.Stream

@(private="file", init)
init_globals :: proc() {
    stdin = os.stream_from_handle(os.stdin)
}

main :: proc() {
    // https://gist.github.com/karl-zylinski/4ccf438337123e7c8994df3b03604e33
    when ODIN_DEBUG {
        track: mem.Tracking_Allocator
        mem.tracking_allocator_init(&track, context.allocator)
        context.allocator = mem.tracking_allocator(&track)
        defer {
            print_tracking_allocator(track, "context.allocator")
            mem.tracking_allocator_destroy(&track)
        }
        
        // No file path and timestamp.
        logger := log.create_console_logger(opt = {
            .Level, .Terminal_Color, .Procedure
        })
        context.logger = logger
        defer log.destroy_console_logger(logger)
    }

    x, y, result: BigInt
    bigint_init_multi(&x, &y, &result)
    defer bigint_destroy_multi(&x, &y, &result)

    saved_allocator := context.allocator
    input_loop: for {
        context.allocator = context.temp_allocator
        defer {
            free_all(context.temp_allocator)
            context.allocator = saved_allocator
        }

        switch get_string_and_set_bigint(&x, "x") {
        case 2: break input_loop
        case 1: continue input_loop
        }

        switch get_string_and_set_bigint(&y, "y") {
        case 2: break input_loop
        case 1: continue input_loop
        }
        xstr := bigint_to_string(x) or_break
        ystr := bigint_to_string(y) or_break

        bigint_add(&result, x, y) or_break
        sum := bigint_to_string(result) or_break
        
        bigint_sub(&result, x, y)
        diff := bigint_to_string(result) or_break

        print_equation(xstr, '+', ystr, sum)
        print_equation(xstr, '-', ystr, diff)
    }
}

@(private="file")
print_equation :: proc(x: string, op: rune, y, z: string) {
    fmt.printf("(x %c y = z): ", op)
    fmt.printfln("%s %c %s = %s", x, op, y, z)    
}

@(private="file", require_results)
get_string_and_set_bigint :: proc(self: ^BigInt, name: string) -> int {
    context.allocator = context.temp_allocator

    fmt.printf("Enter value for '%s': ", name)
    line, oserr := read_line(stdin)
    if oserr != nil {
        return 2
    }
    #partial switch bigint_set_from_string(self, line, 0) {
    case .Out_Of_Memory:
        return 2
    case .Invalid_Digit, .Invalid_Radix:
        return 1
    }
    return 0
}

@(private="file", disabled=!ODIN_DEBUG)
print_tracking_allocator :: proc(track: mem.Tracking_Allocator, name: string) {
    if count := len(track.allocation_map); count > 0 {
        fmt.eprintfln("=== '%s': %v allocations not freed: ===", name, count)
        for _, entry in track.allocation_map {
            fmt.eprintfln("- %v bytes @ %v", entry.size, entry.location)
        }
    }
    if count := len(track.bad_free_array); count > 0 {
        fmt.eprintfln("=== '%s': %v incorrect frees: ===", name, count)
        for entry in track.bad_free_array {
            fmt.eprintfln("- %p @ %v", entry.memory, entry.location)
        }
    }
}
