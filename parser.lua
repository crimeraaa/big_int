local Class  = require "class"
local Token  = require "token"
local TkType = Token.Type
local format = string.format

---@type fun(tktype: Token.Type): Parser.Rule
local get_parserule

---@alias NArray<T> {[integer]: T, n: integer}

local function reset_tokens(self)
    self.m_consumed:copy_token(Token.EOF)
    self.m_lookahead:copy_token(Token.EOF)
    self.m_equation.n = 0
    self.m_results.n  = 0
end

---@alias Parser.Fn fun(self: Parser, lexer: Lexer)

---@enum Parser.Prec
local Prec = {
    None           = 1, "None",
    Equality       = 2, "Equality",     -- == ~=
    Relational     = 3, "Relational",   -- < <= >= >
    Additive       = 4, "Additive",     -- + -
    Multiplicative = 5, "Multiplicative", -- / * %
    Exponential    = 6,                 -- ^
    Unary          = 7, "Unary",        -- - !
}

---@class Parser.Rule : Class
---@field prefixfn? Parser.Fn
---@field infixfn?  Parser.Fn
---@field prec      Parser.Prec
---
---@overload fun(prefixfn?: Parser.Fn, infixfn?: Parser.Fn, prec?: Parser.Prec): Parser.Rule
---
---@param self      Parser.Rule
---@param prefixfn? Parser.Fn
---@param infixfn?  Parser.Fn
---@param prec?     Parser.Prec
local Rule = Class(function(self, prefixfn, infixfn, prec)
    self.prefixfn = prefixfn
    self.infixfn  = infixfn
    self.prec     = prec or Prec.None
end)

---@param self Parser
---@param ...  Token.Type
local function check_lookahead(self, ...)
    local argc = select('#', ...) ---@type integer
    for i = 1, argc do
        local tktype = select(i, ...) ---@type Token.Type
        if self.m_lookahead.type == tktype then
            return true
        end
    end
    return false
end

---@param self  Parser
---@param what  string
---@param ...   string|number
local function throw_error(self, what, ...)
    local who, where = self.m_lookahead, self.m_consumed
    error(format("%s at '%s' near '%s'", what:format(...), who.data, where.data))
end

---@param self  Parser
---@param lexer Lexer
local function update_lookahead(self, lexer)
    self.m_consumed:copy_token(self.m_lookahead)
    self.m_lookahead:copy_token(lexer:scan_token())
    if check_lookahead(self, TkType.Error) then
        throw_error(self, "Unexpected symbol")
    end
end

---@param self   Parser
---@param lexer  Lexer
---@param tktype Token.Type
local function expect_lookahead(self, lexer, tktype)
    if not check_lookahead(self, tktype) then
        throw_error(self, "Expected '%s'", tktype)
        return
    end
    update_lookahead(self, lexer)
end

---@param self  Parser
---@param lexer Lexer
---@param prec  Parser.Prec
local function parse_precedence(self, lexer, prec)
    local parserule = get_parserule(self.m_lookahead.type)
    if not (parserule and parserule.prefixfn) then
        throw_error(self, "Expected a prefix expression")
    end
    update_lookahead(self, lexer)
    parserule.prefixfn(self, lexer)

    while true do
        parserule = get_parserule(self.m_lookahead.type)
        if not (parserule and prec <= parserule.prec) then
            break
        end
        -- If this passes I messed up somewhere (e.g. bad mapped precedence)
        if not parserule.infixfn then
            throw_error(self, "Expected an infix expression")
        end
        update_lookahead(self, lexer)
        parserule.infixfn(self, lexer)
    end
end

---@param self  Parser
---@param lexer Lexer
local function parse_expression(self, lexer)
    parse_precedence(self, lexer, Prec.None + 1)
end

local floor = math.floor
local function to_integer(value)
    local num = tonumber(value)
    if not num then
        return nil
    end
    local int = floor(num)
    -- Truncated fractional portion?
    if int ~= num then
        return nil
    end
    return int
end

---@param n   integer
---@param acc integer
local function factorial_acc(n, acc)
    -- print(n..'!')
    if n == 0 then
        return acc
    end
    return factorial_acc(n - 1, acc * n)
end

--- PREFIX EXPRESSIONS ---------------------------------------------------- {{{2

---@generic T
---@param self NArray<T>
---@param value  T
local function push_narray(self, value)
    local i = self.n + 1
    self[i] = value
    self.n  = i
end

---@generic T
---@param self NArray<T>
---@return T
local function pop_narray(self)
    local i   = self.n
    local res = self[i]
    self.n    = i - 1
    return res
end

---@generic T
---@param self NArray<T>
local function serialize_narray(self)
    return table.concat(self, ' ', 1, self.n)
end

local PREFIX_BASES = {["0b"] = 2, ["0o"] = 8, ["0d"] = 10, ["0x"] = 16}

---@param input string
local function string_to_number(input)
    input = input:gsub("[%s,_]", "")
    local prefix = input:match("^0%a")
    local base   = 10
    if prefix then
        -- Get the rest of the string w/o the "0[bodx]" part.
        input = input:sub(3)
        base  = PREFIX_BASES[prefix]
        if not base then
            return nil, format("Unknown base '%s'", prefix)
        end
    end
    local converted = tonumber(input, base)
    if not converted then
        return nil, format("Non base-%i number", base)
    end
    return converted
end

local is_token = Token.is_instance

---@param self   Parser
---@param result Token|number|string
local function push_result(self, result)
    ---@type false|Token
    local token = is_token(result) and result
    if token then
        result = token.data
    end
    ---@cast result string|number
    if type(result) == "string" then
        local n, msg = string_to_number(result)
        if not n then
            throw_error(self, msg)
            return
        end
        result = n
    end
    push_narray(self.m_results, result)
end

---@param self Parser
local function pop_result(self)
    return pop_narray(self.m_results)
end

local unopr = {
    ['-'] = function(x) return -x end,
    ['!'] = function(x)
        local i = to_integer(x)
        if (not i) or (i < 0) then
            error(format("Attempt to get factorial of '%s'", tostring(x)))
        end
        return factorial_acc(i, 1)
    end,
}

local binopr = {
    ['+'] = function(x, y) return x + y end,
    ['-'] = function(x, y) return x - y end,
    ['*'] = function(x, y) return x * y end,
    ['/'] = function(x, y) return x / y end,
    ['%'] = function(x, y) return x % y end,
    ['^'] = function(x, y) return x ^ y end,
}

---@param self Parser
---@param kind "unary"|"binary"
---@param op   Token.Type
local function set_result(self, kind, op)
    -- Inefficient to pop then push but I don't really care at the moment
    if kind == "unary" then
        local x  = pop_result(self)
        local fn = unopr[op]
        push_result(self, fn(x))
    else
        local y, x = pop_result(self), pop_result(self)
        local fn   = binopr[op]
        if not fn then
            throw_error(self, "'%s' operator not yet implemented", op)
        end
        push_result(self, fn(x, y))
    end
end

---@param self  Parser
---@param token Token|string|number
local function push_equation(self, token)
    push_narray(self.m_equation, is_token(token) and token.data or token)
end

---@param self Parser
local function serialize_results(self)
    return serialize_narray(self.m_results)
end

---@param self Parser
local function serialize_equation(self)
    return serialize_narray(self.m_equation)
end

---@param self  Parser
---@param lexer Lexer
local function number(self, lexer)
    local n, msg = string_to_number(self.m_consumed.data)
    if not n then
        throw_error(self, msg)
        return
    end
    push_equation(self, n)
    push_result(self, n)
end

-- Converts prefix `-1` to postfix `0 1 -`.
local function unary(self, lexer)
    local token = Token(self.m_consumed)
    if token.type == TkType.Dash then
        push_equation(self, Token.ZERO) -- Rationale: -x == 0 - x
        parse_precedence(self, lexer, Prec.Unary)
        push_equation(self, token)
        set_result(self, "unary", token.type)
    end
end

-- Factorials aren't infix expressions but rather postfix unary expressions.
-- However for our purposes the parser is in a better state to parse them as if
-- they were an infix rather than a prefix.
---@param self  Parser
---@param lexer Lexer
local function postfix(self, lexer)
    local token = self.m_consumed
    push_equation(self, token)
    set_result(self, "unary", token.type)
end

---@param self  Parser
---@param lexer Lexer
local function group(self, lexer)
    parse_expression(self, lexer)
    expect_lookahead(self, lexer, TkType.RightParen)
end

--- 2}}} -----------------------------------------------------------------------

--- INFIX EXPRESSIONS ----------------------------------------------------- {{{2

local function binary(self, lexer)
    -- Do a deep copy as a mere reference to `self.m_consumed` may be updated.
    local token = Token(self.m_consumed)
    local assoc = (token.type == TkType.Caret) and 0 or 1 -- L/R associative?
    parse_precedence(self, lexer, get_parserule(token.type).prec + assoc)
    push_equation(self, token)
    set_result(self, "binary", token.type)
end

--- 2}}} -----------------------------------------------------------------------

---@see Token.Type values.
local PARSERULES = {
    ['(']  = Rule(group, nil,     Prec.None),

    ['+']  = Rule(nil,   binary,  Prec.Additive),
    ['-']  = Rule(unary, binary,  Prec.Additive),
    ['*']  = Rule(nil,   binary,  Prec.Multiplicative),
    ['/']  = Rule(nil,   binary,  Prec.Multiplicative),
    ['%']  = Rule(nil,   binary,  Prec.Multiplicative),
    ['^']  = Rule(nil,   binary,  Prec.Exponential),
    ['!']  = Rule(nil,   postfix, Prec.Unary),

    ["=="] = Rule(nil,   binary,  Prec.Equality),
    ["~="] = Rule(nil,   binary,  Prec.Equality),
    ['<']  = Rule(nil,   binary,  Prec.Relational),
    ["<="] = Rule(nil,   binary,  Prec.Relational),
    ['>']  = Rule(nil,   binary,  Prec.Relational),
    [">="] = Rule(nil,   binary,  Prec.Relational),

    ["<number>"] = Rule(number),
}

---@param tktype Token.Type
get_parserule = function(tktype)
    return PARSERULES[tktype]
end

---@class Parser : Class
---@field m_consumed  Token
---@field m_lookahead Token
---@field m_equation  NArray<string>
---@field m_results   NArray<number>
---
---@overload fun(): Parser
---@param self      Parser
local Parser = Class(function(self)
    self.m_consumed  = Token()
    self.m_lookahead = Token()
    self.m_equation  = {n = 0}
    self.m_results   = {n = 0}
end)

Parser.reset_tokens       = reset_tokens
Parser.update_lookahead   = update_lookahead
Parser.expect_lookahead   = expect_lookahead
Parser.serialize_equation = serialize_equation
Parser.serialize_results  = serialize_results
Parser.parse_expression   = parse_expression

return Parser
