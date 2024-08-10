local Token  = require "token"
local Lexer  = require "lexer"
local Parser = require "parser"

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
    parser:reset()

    -- Ensure lookahead token is something we can start with
    parser:update_lookahead(lexer)
    while not parser:check_lookahead(Token.Type.Error, Token.Type.Eof) do
        parser:parse_expression(lexer)
    end
    printfln("RPN = %s", parser:serialize_equation())
    printfln("    = %s", parser:serialize_results())
end

printfln("To exit (Windows): Type <CTRL-Z> then hit <ENTER>.")
printfln("To exit (Linux):   Type <CTRL-D>.")

local PROGNAME = arg[0]
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
