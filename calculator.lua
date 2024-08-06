local Enum = require "enum"

---@enum Token.OpCode
local OpCode = Enum {
    "Add", "Sub", "Mul", "Div", "Pow", "Mod",
    "Eq", "Neq", "Lt", "Le", "Gt", "Ge",
    "Number", "None",
}

---@enum Token.Family
local Family = Enum {
    "Literal",
    "Arithmetic",
    "Relational",
    "Error",
    "Eof"
}

---@class Token
---@field family Token.Family
---@field opcode Token.OpCode

---@type {[string]: Token}
local Token = {
    ['+']  = {family = Family.Arithmetic, opcode = OpCode.Add},
    ['-']  = {family = Family.Arithmetic, opcode = OpCode.Sub},
    ['*']  = {family = Family.Arithmetic, opcode = OpCode.Mul},
    ['/']  = {family = Family.Arithmetic, opcode = OpCode.Div}, 
    ["=="] = {family = Family.Relational, opcode = OpCode.Eq},
    ["~="] = {family = Family.Relational, opcode = OpCode.Neq},
    ['<']  = {family = Family.Relational, opcode = OpCode.Lt},
    ["<="] = {family = Family.Relational, opcode = OpCode.Le},
    ['>']  = {family = Family.Relational, opcode = OpCode.Gt},
    [">="] = {family = Family.Relational, opcode = OpCode.Ge},

    Number = {family = Family.Literal,  opcode = OpCode.Number},
    Error  = {family = Family.Error,    opcode = OpCode.None},
    Eof    = {family = Family.Eof,      opcode = OpCode.None},
}

local lexer = {
    index   = 1,
    data    = "",
    input   = "",
    rest    = "",
    type    = nil, ---@type Token
}

local function match_rest(pattern)
    ---@type integer|nil, integer|nil, string|nil
    local start, stop, rest = lexer.rest:find('^(' .. pattern .. ')')
    return start, stop, rest
end

local function token_consumed(pattern)
    local _, stop, data = match_rest(pattern)
    if stop and data then
        lexer.index = lexer.index + stop
        lexer.data  = data
        lexer.rest  = lexer.input:sub(lexer.index)
        return true
    end
    return false
end

local function skip_whitespace()
    return token_consumed("%s+")
end

local function token_available()
    skip_whitespace()
    if token_consumed("[%d,_.]+") then
        lexer.type = Token.Number
        return true
    end
    if token_consumed("[-+*/]") then
        lexer.type = Token[lexer.data]
        return true
    end
    if token_consumed("[=~<>]=?") then
        lexer.type = Token[lexer.data]
        -- Lone '=' won't ever receive a valid token.
        if lexer.type then
            return true
        else
            lexer.type = Token.Error
            return false
        end
    end
    lexer.type = token_consumed("[^%s]+") and Token.Error or Token.Eof
    return false
end

---@param fmt string
---@param ... any
local function printf(fmt, ...)
    return io.stdout:write(fmt:format(...))

end
---@param fmt string
---@param ... any
local function printfln(fmt, ...)
    return printf(fmt .. '\n', ...)
end

---@param input string
local function tokenize(input)
    lexer.index = 1
    lexer.data  = ""
    lexer.type  = nil
    lexer.input, lexer.rest = input, input
    while token_available() do
        local token = lexer.type
        local family, opcode = Family[token.family], OpCode[token.opcode]
        printfln("%12s %-8s %s", family, opcode, lexer.data)
    end
    if lexer.type == Token.Error then
        printfln("[ERROR]: Invalid token '%s'", lexer.data)
    end
end

while true do
    printf("%s> ", arg[0])
    local input = io.stdin:read("*l")
    if not input then
        break
    end
    tokenize(input)
end
