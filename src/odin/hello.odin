package hello

import "core:fmt"
import "core:io"
import "core:log"
import "core:mem"
import "core:os"
import str "core:strings"

BUFFER_SIZE :: 16

interned: str.Intern

@(private="file", init)
init :: proc() {
    if merr := str.intern_init(&interned); merr != nil {
        panic("Unable to initialize interned_strings")
    }
}

@(private="file")
deinit :: proc() {
    defer str.intern_destroy(&interned)
    fmt.printfln("=== %i interned ===", len(interned.entries))
    for key in interned.entries {
        fmt.printfln("\"%s\"", key)
    }
}

@(deferred_none=deinit)
main :: proc() {
    logger := log.create_console_logger()
    defer log.destroy_console_logger(logger)
    context.logger = logger

    // https://gist.github.com/karl-zylinski/4ccf438337123e7c8994df3b03604e33
    when ODIN_DEBUG {
        track: mem.Tracking_Allocator
        mem.tracking_allocator_init(&track, context.allocator)
        context.allocator = mem.tracking_allocator(&track)
        defer {
            if len(track.allocation_map) > 0 {
                fmt.eprintfln("=== %v allocations not freed: ===", len(track.allocation_map))
                for _, entry in track.allocation_map {
                    fmt.eprintfln("- %v bytes @ @v", entry.size, entry.location)
                }
            }
            if len(track.bad_free_array) > 0 {
                fmt.eprintfln("=== %v incorrect frees: ===", len(track.bad_free_array))
                for entry in track.bad_free_array {
                    fmt.eprintfln("- %p @ %v", entry.memory, entry.location)
                }
            }
            mem.tracking_allocator_destroy(&track)
        }
    }
    for {
        input, err := get_string()
        if err != nil {
            if err != .EOF {
                log.errorf("Error in get_string(): %v", err)                
            }
            break
        }
        fmt.printfln("input: \"%s\"", input)
    }
}

// https://github.com/odin-lang/examples/blob/master/by_example/read_console_input/read_console_input.odin
get_string :: proc(prompt: string = "> ") -> (out: string, err: os.Error) {
    fmt.print(prompt)

    buffer: [BUFFER_SIZE]byte
    builder := str.builder_make() or_return
    stdin   := os.stream_from_handle(os.stdin)

    defer str.builder_destroy(&builder)
    for {
        nread := io.read(stdin, buffer[:]) or_return
        append_until_done(&builder, buffer[:nread]) or_break
    }
    out = str.intern_get(&interned, str.to_string(builder)) or_return
    return
}

// NOTE: Don't use `str.trim_right_space` as it's too broad.
// We return `false` when we're certain there's no more input needed.
append_until_done :: proc(builder: ^str.Builder, buffer: []byte) -> bool {
    out := str.trim_right(string(buffer[:]), "\r\n")
    str.write_string(builder, out)
    return len(out) == len(buffer)
}
