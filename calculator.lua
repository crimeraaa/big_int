local getinfo = debug.getinfo
local arg     = arg
local dirsep  = package.config:sub(1, 1) ---@type '\\'|'/'
local function get_scriptdir()
    -- Trim the leading '@'
    local path = arg and arg[0] or getinfo(2, 'S').source:sub(2)
    return path:match("(.*[/\\])") or ('.' .. dirsep)
end

package.path = get_scriptdir().."?.lua;"..package.path

require "src/lua/strict".restrict(_G)

local Token  = require "src/lua/token"
local Lexer  = require "src/lua/lexer"
local Parser = require "src/lua/parser"

local stdin, stdout = io.stdin, io.stdout

---@param fmt string
---@param ... string|number
local function printf(fmt, ...)
    return stdout:write(fmt:format(...))
end

---@param fmt string
---@param ... string|number
local function printfln(fmt, ...)
    return stdout:write(fmt:format(...), '\n')
end

local lexer, parser = Lexer(), Parser()

---@param input string
local function compile(input)
    lexer:set_input(input)
    parser:reset_tokens()

    -- Ensure lookahead token is something we can start with
    parser:update_lookahead(lexer)
    parser:parse_expression(lexer)
    parser:expect_lookahead(lexer, Token.Type.Eof)
    printfln("RPN = %s", parser:serialize_equation())
    printfln("    = %s", parser:serialize_results())
end

printfln("To exit (Windows): Type <CTRL-Z> then hit <ENTER>.")
printfln("To exit (Linux):   Type <CTRL-D>.")

---@type string
local PROGNAME = arg[0]:match("%w+%.lua$")

while true do
    printf("%s> ", PROGNAME)
    local input = stdin:read("*l")
    if not input then break end
    -- Prevent the call to `Parser:throw_error()` from killing our entire program.
    local ok, msg = pcall(compile, input)
    if not ok then
        print(msg)
    end
end
