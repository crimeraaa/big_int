package bigint

import "core:strings"
import "core:os"
import "core:io"

/*
Will allocate a `strings.Builder` for your input. WIll return the result of
`strings.to_string(builder)`.

Since `strings.Builder` simply wraps a `[dynamic]u8`, freeing the resulting
string is the caller's responsibilty.

See: https://github.com/odin-lang/examples/blob/master/by_example/read_console_input/read_console_input.odin
*/
@(require_results)
read_line :: proc(stream: io.Stream, allocator := context.allocator) -> (out: string, err: os.Error) {
    context.allocator = allocator
    buffer: [512]byte
    builder := strings.builder_make() or_return
    for {
        nread := io.read(stream, buffer[:]) or_return
        write_buffer_until_done(&builder, buffer[:nread]) or_break
    }
    return strings.to_string(builder), nil
}


// NOTE: Don't use `strings.trim_right_space` as it's too broad.
// We return `false` when we're certain there's no more input needed.
@(private="file", require_results)
write_buffer_until_done :: proc(builder: ^strings.Builder, buffer: []byte) -> bool {
    out := strings.trim_right(string(buffer[:]), "\r\n")
    strings.write_string(builder, out)
    return len(out) == len(buffer)
}
