local Enum   = require "enum"
local BigInt = require "bigint"

---@enum OpCode
local OpCode = Enum {
    "Add",  "Sub",  "Mul",  "Div",
    "Eq",   "Neq",  "Lt",   "Leq",  "Gt",   "Geq",
    "Number", "Error", "Eof",
}

---@enum Token.Family
local Family = Enum {
    "Grouping",
    "Arithmetic",
    "Relational",
    "Literal",
    "Error",
    "Eof",
}

---@enum Precedence
local Precedence = Enum {
    "None",
    "Equality",     -- == ~=
    "Terminal",     -- + -
    "Comparison",   -- < <= >= >
    "Factor",       -- / *
}

---@class Token
---@field family Enum.Value|Token.Family
---@field opcode Enum.Value|OpCode
---@field prec   Enum.Value|Precedence
---@field data?  string

---@param family? Enum.Value|Token.Family
---@param opcode? Enum.Value|OpCode
---@param prec?   Enum.Value|Precedence
---@return Token token
local function new_token(family, opcode, prec)
    return {family = family or Family.Eof,
            opcode = opcode or OpCode.Eof,
            prec   = prec   or Precedence.None}
end

---@type {[string]: Token}
local Token = { 
    ['(']  = new_token(Family.Grouping),
    [')']  = new_token(Family.Grouping),
    
    ['+']  = new_token(Family.Arithmetic, OpCode.Add, Precedence.Terminal),
    ['-']  = new_token(Family.Arithmetic, OpCode.Sub, Precedence.Terminal),
    ['*']  = new_token(Family.Arithmetic, OpCode.Mul, Precedence.Factor),
    ['/']  = new_token(Family.Arithmetic, OpCode.Div, Precedence.Factor), 

    ["=="] = new_token(Family.Relational, OpCode.Eq,  Precedence.Equality),
    ["~="] = new_token(Family.Relational, OpCode.Neq, Precedence.Equality),
    ['<']  = new_token(Family.Relational, OpCode.Lt,  Precedence.Comparison),
    ["<="] = new_token(Family.Relational, OpCode.Leq, Precedence.Comparison),
    ['>']  = new_token(Family.Relational, OpCode.Gt,  Precedence.Comparison),
    [">="] = new_token(Family.Relational, OpCode.Geq, Precedence.Comparison),

    Number = new_token(Family.Literal,    OpCode.Number),
    Error  = new_token(Family.Error,      OpCode.Error),
    Eof    = new_token(),
}

local lexer = {
    index   = 1,
    data    = "",
    input   = "",
    rest    = "",
    ---@type Token
    token   = new_token(),
}

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

---@param token Token
local function set_token(token)
    lexer.token.data = lexer.data
    if token then
        lexer.token.family = token.family
        lexer.token.opcode = token.opcode
        return true
    else
        -- Copy just the pointer so we can compare directly to it.
        lexer.token = Token.Error
        return false
    end
end

---@param token Token
local function token_tostring(token)
    local family = Family[token.family]
    local opcode = OpCode[token.opcode]
    return string.format("%12s %-8s %8s", family, opcode, token.data or "(empty)")
end

local function token_available()
    skip_whitespace()
    if token_consumed("[%d,_.]+") then
        return set_token(Token.Number)
    elseif token_consumed("[-+*/]") then
        return set_token(Token[lexer.data])
    elseif token_consumed("[=~<>]=?") then
        -- Lone '=' won't ever receive a valid token.
        return set_token(Token[lexer.data])
    end
    lexer.token = token_consumed("[^%s]+") and Token.Error or Token.Eof
    return false
end

---@param token Token
---@param prec  Precedence
local function parse_precedence(token, prec)
end

---@param token Token
local function parse_expression(token)
    parse_precedence(token, token.prec)
    print(token_tostring(token))
end


---@param input string
local function compile(input)
    lexer.index = 1
    lexer.data  = ""
    lexer.input, lexer.rest = input, input
    
    while token_available() do
        parse_expression(lexer.token)
    end

    if lexer.token == Token.Error then
        printfln("[ERROR]: Syntax error at '%s'", lexer.data)
        return
    end
end

while true do
    printf("%s> ", arg[0])
    local input = io.stdin:read("*l")
    if not input then
        break
    end
    compile(input)
end
