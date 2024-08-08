-- local BigInt = require "bigint"
local Class  = require "class"

---@param fmt string
---@param ... string|integer
local function printf(fmt, ...)
    return io.stdout:write(fmt:format(...))
end

---@param fmt string
---@param ... string|integer
local function printfln(fmt, ...)
    return printf(fmt .. '\n', ...)
end

---@param fmt string
---@param ... string|integer
local function errorf(fmt, ...)
    error(("[ERROR] " .. fmt):format(...))
end

local Token

---@class Token: Class
---@field type  Token.Type
---@field data  string
---
---@overload fun(type?: Token.Type, data?: string): Token
---@overload fun(token: Token): Token
---
---@param inst    Token
---@param tktype? Token.Type|Token
---@param data?   string
Token = Class(function(inst, tktype, data)
    -- Do a deep copy if applicable.
    local token = Token.is_instance(tktype) and tktype
    if token then
        inst.type = token.type
        inst.data = token.data
    else
        inst.type = tktype or Token.Type.Error
        inst.data = tktype == Token.Type.Eof and "<eof>" or data or "<error>"
    end
end)

---@enum Token.Type
Token.Type = {
    ['('] = 1, '(',
    [')'] = 2, ')',
    ['+'] = 3, '+',
    ['-'] = 4, '-',
    ['*'] = 5, '*',
    ['/'] = 6, '/',
    
    ["=="] = 7,  "==",
    ["~="] = 8,  "~=",
    ['<']  = 9,  '<',
    ["<="] = 10, "<=",
    ['>']  = 11, '>',
    [">="] = 12, ">=",

    Number = 13, "Number",
    Error  = 14, "Error",
    Eof    = 15, "Eof",
}

--- LEXER ----------------------------------------------------------------- {{{1

---@class Lexer: Class
---@field m_index integer Index into the full string of the current lexeme.
---@field m_data  string  The current lexeme substring.
---@field m_input string  The full string of the user's input.
---@field m_rest  string  Slice of the full string starting at the current lexeme.
---
---@overload fun(data: string): Lexer
---@param inst Lexer  Created and passed by `__call()` in `Class()`.
---@param data string
local Lexer = Class(function(inst, data)
    inst.m_index = 1
    inst.m_data  = ""
    inst.m_input = data
    inst.m_rest  = data
end)

---@param tktype? Token.Type
function Lexer:make_token(tktype)
    return Token(tktype, self.m_data)
end

function Lexer:match_start_of_rest(pattern)
    ---@type integer|nil, integer|nil, string|nil
    local start, stop, data = self.m_rest:find('^(' .. pattern .. ')')
    return start, stop, data
end

function Lexer:found_pattern(pattern)
    ---`lexer.rest` is equivalent to the slice `lexer.input[index:]`, and we
    ---only look for matches at the beginning of `lexer.rest`, we can assume
    ---`stop` is the same as the substring length.
    local _, len, data = self:match_start_of_rest(pattern)
    if len and data then
        local oldindex = self.m_index
        local newindex = oldindex + len
        self.m_index = newindex
        self.m_data  = data
        self.m_rest  = self.m_input:sub(newindex)
        -- printfln("input[%i:%i] '%s', #input %i", oldindex, newindex - 1, data, #self.input)
        return true
    end
    return false
end

function Lexer:skip_whitespace()
    return self:found_pattern("%s+")
end

function Lexer:scan_token()
    self:skip_whitespace()
    -- Digits followed by 0 or more digit-likes.
    if self:found_pattern("%d[%d,_.]*") then
        return self:make_token(Token.Type.Number)
    end
    -- Guaranteed single-character tokens.
    if self:found_pattern("[-+*/()]") then
        return self:make_token(Token.Type[self.m_data])
    end
    -- Potentially double-character tokens.
    -- In the case of single '=', there is no mapping for it so it is nil.
    if self:found_pattern("[=~<>]=?") then
        return self:make_token(Token.Type[self.m_data])
    end
    -- Still have non-whitespace stuff left in the rest string?
    local more = self:found_pattern("[%S]+")
    return self:make_token(more and Token.Type.Error or Token.Type.Eof)
end

--- 1}}} -----------------------------------------------------------------------

--- PARSER ---------------------------------------------------------------- {{{1

---@class Parser: Class
---@field m_consumed  Token
---@field m_lookahead Token
---
---@overload fun(): Parser
---@param inst Parser
local Parser = Class(function(inst)
    inst.m_consumed  = Token()
    inst.m_lookahead = Token()
end)

---@alias Parser.Fn fun(self: Parser, lexer: Lexer)

---@class Parser.Rule: Class
---@field prefixfn? Parser.Fn
---@field infixfn?  Parser.Fn
---@field prec      Parser.Prec
---
---@overload fun(pfx?: Parser.Fn, ifx?: Parser.Fn, prec?: Parser.Prec): Parser.Rule
---@param inst  Parser.Rule
---@param pfx?  Parser.Fn
---@param ifx?  Parser.Fn
---@param prec? Parser.Prec
Parser.Rule = Class(function(inst, pfx, ifx, prec)
    inst.prefixfn = pfx
    inst.infixfn  = ifx
    inst.prec     = prec or Parser.Prec.None
end)

---@enum Parser.Prec
Parser.Prec = {
    None       = 1, "None",
    Equality   = 2, "Equality",     -- == ~=
    Terminal   = 3, "Terminal",     -- + -
    Comparison = 4, "Comparison",   -- < <= >= >
    Factor     = 5, "Factor",       -- / *
    Unary      = 6, "Unary",        -- unary -
}

---@param ... Token.Type
function Parser:match_lookahead(...)
    for _, tktype in ipairs{...} do
        if self.m_lookahead.type == tktype then
            return true
        end
    end
    return false
end

---@param lexer Lexer
function Parser:update_lookahead(lexer)
    self.m_consumed  = self.m_lookahead
    self.m_lookahead = lexer:scan_token()
    if self:match_lookahead(Token.Type.Error) then
        errorf("Unexpected symbol '%s'", lexer.m_data)
    end
end

---@param lexer Lexer
---@param tktype  Token.Type
function Parser:expect_lookahead(lexer, tktype)
    if not self:match_lookahead(tktype) then
        errorf("Expected '%s'", Token.Type[tktype])
    end
    self:update_lookahead(lexer)
end

---@param lexer Lexer
---@param prec  Parser.Prec
function Parser:precedence(lexer, prec)
    local parserule = self.rules[self.m_lookahead.type]
    if not (parserule and parserule.prefixfn) then
        errorf("Expected a prefix expression at '%s'", self.m_lookahead.data)
    end
    self:update_lookahead(lexer)
    parserule.prefixfn(self, lexer)
    
    while true do
        parserule = self.rules[self.m_lookahead.type]
        if not (parserule and prec <= parserule.prec) then
            break
        end
        self:update_lookahead(lexer)
        parserule.infixfn(self, lexer)
    end
end

---@param lexer Lexer
function Parser:expression(lexer)
    self:precedence(lexer, Parser.Prec.None + 1)
end

--- PREFIX EXPRESSIONS ---------------------------------------------------- {{{2

---@param token Token
local function print_token(token)
    printfln("%12s: %s", Token.Type[token.type], token.data)
end

---@param lexer Lexer
function Parser:number(lexer)
    -- local bigint = BigInt(self.m_consumed.data)
    print_token(self.m_consumed)
end

function Parser:unary(lexer)
    local token = Token(self.m_consumed)
    self:precedence(lexer, Parser.Prec.Unary)
    print_token(token)
end

---@param lexer Lexer
function Parser:grouping(lexer)
    self:expression(lexer)
    self:expect_lookahead(lexer, Token.Type[')'])
end
--- 2}}} -----------------------------------------------------------------------

--- INFIX EXPRESSIONS ----------------------------------------------------- {{{2

function Parser:binary(lexer)
    -- Do a deep copy because otherwise we'll just hold a reference.
    local token = Token(self.m_consumed)
    self:precedence(lexer, self.rules[self.m_consumed.type].prec + 1)
    print_token(token)
end

--- 2}}} -----------------------------------------------------------------------

Parser.rules = {
    [Token.Type['(']]  = Parser.Rule(Parser.grouping),
    
    [Token.Type['+']]  = Parser.Rule(nil, Parser.binary, Parser.Prec.Terminal),
    [Token.Type['-']]  = Parser.Rule(Parser.unary, Parser.binary, Parser.Prec.Terminal),
    [Token.Type['*']]  = Parser.Rule(nil, Parser.binary, Parser.Prec.Factor),
    [Token.Type['/']]  = Parser.Rule(nil, Parser.binary, Parser.Prec.Factor),

    [Token.Type["=="]] = Parser.Rule(nil, Parser.binary, Parser.Prec.Equality),
    [Token.Type["~="]] = Parser.Rule(nil, Parser.binary, Parser.Prec.Equality),
    [Token.Type['<']]  = Parser.Rule(nil, Parser.binary, Parser.Prec.Comparison),
    [Token.Type["<="]] = Parser.Rule(nil, Parser.binary, Parser.Prec.Comparison),
    [Token.Type['>']]  = Parser.Rule(nil, Parser.binary, Parser.Prec.Comparison),
    [Token.Type[">="]] = Parser.Rule(nil, Parser.binary, Parser.Prec.Comparison),

    [Token.Type.Number] = Parser.Rule(Parser.number),
}

--- 1}}} -----------------------------------------------------------------------

---@param input string
local function compile(input)
    local lexer, parser = Lexer(input), Parser()
    
    -- Ensure lookahead token is something we can start with
    parser:update_lookahead(lexer)
    while not parser:match_lookahead(Token.Type.Error, Token.Type.Eof) do
        parser:expression(lexer)
    end
end

while true do
    printf("%s> ", arg[0])
    local input = io.stdin:read("*l")
    if not input then
        break
    end
    -- Prevent the call to `errorf()` from killing our entire program.
    local ok, msg = pcall(compile, input)
    if not ok then
        print(msg)
    end
    -- compile(input)
end
