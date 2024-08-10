local Token  = require "token"
local Lexer  = require "lexer"
local Parser = require "parser"

---@param fmt string
---@param ... string|number
local function printf(fmt, ...)
    return io.stdout:write(fmt:format(...))
end

---@param fmt string
---@param ... string|number
local function printfln(fmt, ...)
    return printf(fmt .. '\n', ...)
end

---@param input string
local function compile(input)
    local lexer, parser = Lexer(input), Parser()
    
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
while true do
    printf("%s> ", arg[0])
    local input = io.stdin:read("*l")
    if not input then break end
    -- Prevent the call to `Parser:throw_error()` from killing our entire program.
    local ok, msg = pcall(compile, input)
    if not ok then
        print(msg)
    end
end
