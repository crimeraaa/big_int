package bigint

import "core:encoding/ansi"
import "core:fmt"
import "core:mem"
import "core:os"
import "core:io"
import "core:log"
import "core:testing"

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
        logger := log.create_console_logger(opt = log.Options{
            .Level, .Terminal_Color, .Procedure
        })
        context.logger = logger
        defer log.destroy_console_logger(logger)
    }
    
    x, y, result: BigInt
    bigint_init_multi(&x, &y, &result)
    defer bigint_destroy_multi(&x, &y, &result)

    input_loop: for {
        // When this scope is exited, `context.allocator` will be restored.
        context.allocator = context.temp_allocator
        defer free_all(context.temp_allocator)

        switch get_string_and_set_bigint(&x, "x") {
        case 2: break    input_loop
        case 1: continue input_loop
        }
        
        switch get_string_and_set_bigint(&y, "y") {
        case 2: break    input_loop
        case 1: continue input_loop
        }

        xstr  := bigint_to_string(x)        or_break
        ystr  := bigint_to_string(y)        or_break

        bigint_add(&result, x, y)           or_break
        bsum  := bigint_to_string(result)   or_break

        bigint_sub(&result, x, y)           or_break
        bdiff := bigint_to_string(result)   or_break

        bigint_mul(&result, x, y)           or_break
        bprod := bigint_to_string(result)   or_break

        bigint_add_digit(&result, x, 7)     or_break
        dsum := bigint_to_string(result)    or_break
        bigint_sub_digit(&result, x, 7)     or_break
        ddiff := bigint_to_string(result)   or_break
        bigint_mul_digit(&result, x, 999_999_999)     or_break
        dprod := bigint_to_string(result)   or_break

        print_equation(xstr, '+', ystr, bsum)
        print_equation(xstr, '-', ystr, bdiff)
        print_equation(xstr, '*', ystr, bprod)
        print_equation(xstr, '+', "7", dsum)
        print_equation(xstr, '-', "7", ddiff)
        print_equation(xstr, '*', "999_999_999", dprod)
    }
}

@(private="file")
BigInt3 :: distinct [3]BigInt

@(private="file")
String3 :: distinct [3]string

@(test)
tests_run :: proc(t: ^testing.T) {
    v: BigInt3
    if err := bigint_init_multi(&v.x, &v.y, &v.z); err != nil {
        testing.fail_now(t, "BigInt initalization/s failed!")
    }
    defer bigint_destroy_multi(&v.x, &v.y, &v.z)
    
    /* 
                 x  +   y  =  z
               (-x) + (-y) = -z
        x < y, (-x) +   y  =  z
        x > y,   x  + (-y) = -z
     */
     
    log.info(set_color("single digit addition"))
    try_expression(t, &v, { "8",  "13",  "21"}, '+')
    try_expression(t, &v, {"-8", "-13", "-21"}, '+')
    try_expression(t, &v, {"-8",  "13",   "5"}, '+')
    try_expression(t, &v, { "8", "-13",  "-5"}, '+')

    log.info(set_color("multi-digit addition"))
    try_expression(t, &v, { "1234567890",  "101112131415",  "102346699305"}, '+')
    try_expression(t, &v, {"-1234567890", "-101112131415", "-102346699305"}, '+')
    try_expression(t, &v, {"-1234567890",  "101112131415",   "99877563525"}, '+')
    try_expression(t, &v, { "1234567890", "-101112131415",  "-99877563525"}, '+')

    /* 
                 x  -   y  =  z if |x| >= |y| else -z
               (-x) - (-y) =  z if |x| <= |y| else -z
        x < y, (-x) -   y  = -z
        x > y    x  - (-y) =  z
     */
    log.info(set_color("single digit subtraction"))
    try_expression(t, &v, { "8",  "13",  "-5"}, '-')
    try_expression(t, &v, {"-8", "-13",   "5"}, '-')
    try_expression(t, &v, {"-8",  "13", "-21"}, '-')
    try_expression(t, &v, { "8", "-13",  "21"}, '-')

    log.info(set_color("multi-digit subtraction"))
    try_expression(t, &v, { "1234567890",  "101112131415",  "-99877563525"}, '-')
    try_expression(t, &v, {"-1234567890", "-101112131415",   "99877563525"}, '-')
    try_expression(t, &v, {"-1234567890",  "101112131415", "-102346699305"}, '-')
    try_expression(t, &v, { "1234567890", "-101112131415",  "102346699305"}, '-')

    /* 
          x  *   y  = z
        (-x) * (-y) = z
        (-x) *   y  = -z
          x  *   y  = -z
     */
    log.info(set_color("single digit multiplication"))
    try_expression(t, &v, { "8",  "13",  "104"}, '*')
    try_expression(t, &v, {"-8", "-13",  "104"}, '*')
    try_expression(t, &v, {"-8",  "13", "-104"}, '*')
    try_expression(t, &v, { "8", "-13", "-104"}, '*')
    
    log.info(set_color("multi-digit multiplication"))
    try_expression(t, &v, { "1234567890",  "101112131415",  "124829790734419264350"}, '*')
    try_expression(t, &v, {"-1234567890", "-101112131415",  "124829790734419264350"}, '*')
    try_expression(t, &v, {"-1234567890",  "101112131415", "-124829790734419264350"}, '*')
    try_expression(t, &v, { "1234567890", "-101112131415", "-124829790734419264350"}, '*')
}

@(private="file")
set_color :: proc($msg: string) -> string {
    return ansi.CSI + ansi.FG_MAGENTA + ansi.SGR + msg + ansi.CSI + ansi.RESET + ansi.SGR    
}


@(private="file")
try_expression :: proc(t: ^testing.T, vals: ^BigInt3, strs: String3, op: rune) {
    log.debugf("%s %c %s = %s", strs.x, op, strs.y, strs.z)
    try_bigint_set(t, &vals.x, strs.x)
    try_bigint_set(t, &vals.y, strs.y)
    try_operation(t, &vals.z, vals.x, vals.y, op, strs.z)
}

@(private="file")
try_bigint_set :: proc(t: ^testing.T, self: ^BigInt, input: string) {
    if err := bigint_set_from_string(self, input); err != nil {
        testing.fail_now(t, "BigInt set from string failed!")
    }
}

@(private="file")
try_operation :: proc(t: ^testing.T, dst: ^BigInt, x, y: BigInt, op: rune, expected: string) {
    err: Error
    switch op {
    case '+': err = bigint_add(dst, x, y)
    case '-': err = bigint_sub(dst, x, y)
    case '*': err = bigint_mul(dst, x, y)
    case:
        testing.fail_now(t, "Invalid/unsupported operation received")
    }

    if err != nil {
        testing.fail_now(t, "BigInt operation failed")
    }
    
    if s, err := bigint_to_string(dst^); err != nil {
        testing.fail_now(t, "BigInt to string failed!")
    } else {
        defer delete(s)
        testing.expect_value(t, s, expected)
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
    switch bigint_set_from_string(self, line, 0) {
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
