-- local BigInt = require "bigint"
local Class  = require "class"

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

local Token

---@class Token : Class
---@field type Token.Type
---@field data string
---
---@overload fun(tktype: Token.Type, data?: string): Token
---@overload fun(token: Token): Token
---@overload fun(): Token
---
---@param inst Token
---@param tktype? Token.Type|Token
---@param data? string
Token = Class(function(inst, tktype, data)
    tktype = tktype or Token.Type.Eof
    -- Do a deep copy if applicable.
    local token = Token.is_instance(tktype) and tktype
    if token then
        inst.type = token.type
        inst.data = token.data
        return
    end
    inst.type = tktype
    inst.data = (tktype == Token.Type.Eof and "<eof>") or data or tktype
end)

---@enum Token.Type
Token.Type = {
    LeftParen   = '(',  RightParen = ')',
    Plus        = '+',  Dash       = '-',
    Star        = '*',  Slash      = '/',
    Percent     = '%',  Caret      = '^',
    Exclamation = '!',
    
    EqualEqual  = "==", TildeEqual      = "~=",
    LeftAngle   = '<',  LeftAngleEqual  = "<=",
    RightAngle  = '>',  RightAngleEqual = ">=",

    Number      = "<number>",
    Error       = "<error>",
    Eof         = "<eof>",
}

Token.ZERO = Token(Token.Type.Number, '0')
Token.MUL  = Token(Token.Type.Star)

--- LEXER ----------------------------------------------------------------- {{{1

---@class Lexer : Class
---@field m_index integer Index into the full string of the current lexeme.
---@field m_data string The current lexeme substring.
---@field m_input string The full string of the user's input.
---@field m_rest string Slice of the full string starting at the current lexeme.
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
    return Token(tktype or Token.Type.Error, self.m_data)
end

---@private
---@nodiscard
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
    if not (len and data) then
        return false
    end
    local newindex = self.m_index + len
    self.m_index   = newindex
    self.m_data    = data
    self.m_rest    = self.m_input:sub(newindex)
    return true
end

function Lexer:skip_whitespace()
    return self:found_pattern("%s+")
end

local singlechars = {
    ['('] = Token.Type.LeftParen,   [')'] = Token.Type.RightParen,
    ['+'] = Token.Type.Plus,        ['-'] = Token.Type.Dash,
    ['*'] = Token.Type.Star,        ['/'] = Token.Type.Slash,
    ['%'] = Token.Type.Percent,     ['^'] = Token.Type.Caret,
    ['!'] = Token.Type.Exclamation,
}

local doublechars = {
    ["=="] = Token.Type.EqualEqual, ["~="] = Token.Type.TildeEqual,
    ['<']  = Token.Type.LeftAngle,  ["<="] = Token.Type.LeftAngleEqual,
    ['>']  = Token.Type.RightAngle, [">="] = Token.Type.RightAngleEqual,
}

---@nodiscard
function Lexer:scan_token()
    self:skip_whitespace()
    -- Digits followed by 0 or more digit-likes.
    if self:found_pattern("%d[%d,_.]*") then
        return self:make_token(Token.Type.Number)
    end
    -- Guaranteed single-character tokens.
    -- Note we put '-' first because otherwise it'll be considered a range.
    if self:found_pattern("[-()+*/%%^!]") then
        return self:make_token(singlechars[self.m_data])
    end
    -- Potentially double-character tokens.
    -- In the case of single '=', there is no mapping for it so it is nil.
    if self:found_pattern("[=~<>]=?") then
        return self:make_token(doublechars[self.m_data])
    end
    -- Still have non-whitespace stuff left in the rest string?
    local more = self:found_pattern("[%S]+")
    return self:make_token(more and Token.Type.Error or Token.Type.Eof)
end

--- 1}}} -----------------------------------------------------------------------

--- PARSER ---------------------------------------------------------------- {{{1

---@class Parser : Class
---@field m_consumed Token
---@field m_lookahead Token
---@field m_equation {[integer]: string, n: integer}
---
---@overload fun(): Parser
---@param inst Parser
local Parser = Class(function(inst)
    inst.m_consumed  = Token()
    inst.m_lookahead = Token()
    inst.m_equation  = {n = 0}
end)

---@alias Parser.Fn fun(self: Parser, lexer: Lexer)

---@class Parser.Rule : Class
---@field prefixfn? Parser.Fn
---@field infixfn? Parser.Fn
---@field prec Parser.Prec
---
---@overload fun(prefixfn?: Parser.Fn, infixfn?: Parser.Fn, prec?: Parser.Prec): Parser.Rule
---
---@param inst      Parser.Rule
---@param prefixfn? Parser.Fn
---@param infixfn?  Parser.Fn
---@param prec?     Parser.Prec
Parser.Rule = Class(function(inst, prefixfn, infixfn, prec)
    inst.prefixfn = prefixfn
    inst.infixfn  = infixfn
    inst.prec     = prec or Parser.Prec.None
end)

---@enum Parser.Prec
Parser.Prec = {
    None           = 1, "None",
    Equality       = 2, "Equality",     -- == ~=
    Relational     = 3, "Relational",   -- < <= >= >
    Additive       = 4, "Additive",     -- + -
    Multiplicative = 5, "Multiplicative", -- / * %
    Exponential    = 6,                 -- ^
    Unary          = 7, "Unary",        -- - !
}

---@param lexer Lexer
---@param tktype Token.Type
function Parser:expect_lookahead(lexer, tktype)
    if not self:check_lookahead(tktype) then
        self:throw_error("Expected '%s'", tktype)
    end
    self:update_lookahead(lexer)
end

---@param lexer Lexer
function Parser:update_lookahead(lexer)
    self.m_consumed, self.m_lookahead = self.m_lookahead, lexer:scan_token()
    if self:check_lookahead(Token.Type.Error) then
        self:throw_error("Unexpected symbol")
    end
end

---@param ... Token.Type
function Parser:check_lookahead(...)
    for _, tktype in ipairs({...}) do
        if self.m_lookahead.type == tktype then
            return true
        end
    end
    return false
end

---@param lexer Lexer
---@param prec Parser.Prec
function Parser:parse_precedence(lexer, prec)
    local parserule = self:get_parserule(self.m_lookahead.type)
    if not (parserule and parserule.prefixfn) then
        self:throw_error("Expected a prefix expression")
    end
    self:update_lookahead(lexer)
    parserule.prefixfn(self, lexer)
    
    while true do
        parserule = self:get_parserule(self.m_lookahead.type)
        if not (parserule and prec <= parserule.prec) then
            break
        end
        -- If this passes I messed up somewhere (e.g. bad mapped precedence)
        if not parserule.infixfn then
            self:throw_error("Expected an infix expression")
        end
        self:update_lookahead(lexer)
        parserule.infixfn(self, lexer)
    end
end

---@param info  string
---@param ...   string|number
function Parser:throw_error(info, ...)
    info = info:format(...)
    local cultprit, where = self.m_lookahead, self.m_consumed
    error(string.format("%s at '%s' near '%s'", info, cultprit.data, where.data))
end

---@param lexer Lexer
function Parser:parse_expression(lexer)
    self:parse_precedence(lexer, Parser.Prec.None + 1)
end

--- PREFIX EXPRESSIONS ---------------------------------------------------- {{{2

---@param token? Token
function Parser:push_equation(token)
    local i = self.m_equation.n + 1
    self.m_equation[i] = token and token.data or self.m_consumed.data
    self.m_equation.n  = i
end

function Parser:print_equation()
    print(table.concat(self.m_equation, ' '))
end

---@param lexer Lexer
function Parser:number(lexer)
    self:push_equation()
end

-- Converts prefix `-1` to postfix `0 1 -`.
function Parser:unary(lexer)
    local token = Token(self.m_consumed)
    if token.type == Token.Type.Dash then
        self:push_equation(Token.ZERO)
        self:parse_precedence(lexer, Parser.Prec.Unary)
        self:push_equation(token)
    end
end

-- Factorials aren't infix expressions but rather postfix unary expressions.
-- However for our purposes the parser is in a better state to parse them as if
-- they were an infix rather than a prefix.
function Parser:postfix(lexer)
    self:push_equation(self.m_consumed)
end

---@param lexer Lexer
function Parser:group(lexer)
    self:parse_expression(lexer)
    self:expect_lookahead(lexer, Token.Type.RightParen)
end

--- 2}}} -----------------------------------------------------------------------

--- INFIX EXPRESSIONS ----------------------------------------------------- {{{2

function Parser:binary(lexer)
    -- Do a deep copy as a mere reference to `self.m_consumed` may be updated.
    local token = Token(self.m_consumed)
    local assoc = (token.type == Token.Type.Caret) and 0 or 1 -- L/R associative?
    self:parse_precedence(lexer, self:get_parserule(token.type).prec + assoc)
    self:push_equation(token)
end

--- 2}}} -----------------------------------------------------------------------

---@see Token.Type values.
local parserules = {
    ['(']  = Parser.Rule(Parser.group, nil,            Parser.Prec.None),
    
    ['+']  = Parser.Rule(nil,          Parser.binary,  Parser.Prec.Additive),
    ['-']  = Parser.Rule(Parser.unary, Parser.binary,  Parser.Prec.Additive),
    ['*']  = Parser.Rule(nil,          Parser.binary,  Parser.Prec.Multiplicative),
    ['/']  = Parser.Rule(nil,          Parser.binary,  Parser.Prec.Multiplicative),
    ['%']  = Parser.Rule(nil,          Parser.binary,  Parser.Prec.Multiplicative),
    ['^']  = Parser.Rule(nil,          Parser.binary,  Parser.Prec.Exponential),
    ['!']  = Parser.Rule(nil,          Parser.postfix, Parser.Prec.Unary),

    ["=="] = Parser.Rule(nil,          Parser.binary,  Parser.Prec.Equality),
    ["~="] = Parser.Rule(nil,          Parser.binary,  Parser.Prec.Equality),
    ['<']  = Parser.Rule(nil,          Parser.binary,  Parser.Prec.Relational),
    ["<="] = Parser.Rule(nil,          Parser.binary,  Parser.Prec.Relational),
    ['>']  = Parser.Rule(nil,          Parser.binary,  Parser.Prec.Relational),
    [">="] = Parser.Rule(nil,          Parser.binary,  Parser.Prec.Relational),

    ["<number>"] = Parser.Rule(Parser.number),
}

---@param tktype Token.Type
function Parser:get_parserule(tktype)
    return parserules[tktype]
end


--- 1}}} -----------------------------------------------------------------------

---@param input string
local function compile(input)
    local lexer, parser = Lexer(input), Parser()
    
    -- Ensure lookahead token is something we can start with
    parser:update_lookahead(lexer)
    while not parser:check_lookahead(Token.Type.Error, Token.Type.Eof) do
        parser:parse_expression(lexer)
    end
    parser:print_equation()
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
    -- compile(input)
end
