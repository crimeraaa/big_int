package bigint

import "core:fmt"
import "core:mem"
import "core:os"
import "core:strconv"

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
        
        bi: BigInt
        bigint_init(&bi) or_break
        defer bigint_destroy(&bi)

        value, _ := strconv.parse_int(input)
        bigint_set_integer(&bi, value) or_break
        fmt.println("'parse_int()':")
        bigint_print(bi)
        fmt.println(bi)
        // Unrecoverable error?
        if err := bigint_set_string(&bi, input, minimize = true); err != nil {
            if err != .Invalid_Digit {
                break
            }
        }
        fmt.println("'bigint_set_string()':")
        bigint_print(bi)
        fmt.println(bi)
    }
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
