local Class = require "class"
local Token = require "token"

---@alias NArray<T> {[integer]: T, n: integer}

---@class Parser : Class
---@field m_consumed  Token
---@field m_lookahead Token
---@field m_equation  NArray<string>
---@field m_results   NArray<number>
---
---@overload fun(): Parser
---@param inst Parser
local Parser = Class(function(inst)
    inst.m_consumed  = Token()
    inst.m_lookahead = Token()
    inst.m_equation  = {n = 0}
    inst.m_results   = {n = 0}
end)

---@alias Parser.Fn fun(self: Parser, lexer: Lexer)

---@class Parser.Rule : Class
---@field prefixfn? Parser.Fn
---@field infixfn?  Parser.Fn
---@field prec      Parser.Prec
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
        return
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
---@param prec  Parser.Prec
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

---@param what  string
---@param ...   string|number
function Parser:throw_error(what, ...)
    local who, where = self.m_lookahead, self.m_consumed
    error(string.format("%s at '%s' near '%s'", what:format(...), who.data, where.data))
end

---@param lexer Lexer
function Parser:parse_expression(lexer)
    self:parse_precedence(lexer, Parser.Prec.None + 1)
end

local function tointeger(value)
    local n = tonumber(value)
    if not n then
        return nil
    end
    local i = math.floor(n)
    -- Truncated fractional portion?
    if i ~= n then
        return nil
    end
    return i
end

---@param n   integer
---@param acc integer
local function factorial(n, acc)
    if not tointeger(n) or n < 0 then
        error(string.format("Attempt to get factorial of '%s'", tostring(n)))
    end
    -- print(n..'!')
    return n == 0 and acc or factorial(n - 1, acc * n)
end

local operations = {
    unary = {
        ['-'] = function(x) return -x end,
        ['!'] = function(x) return factorial(x, 1) end,
    },
    binary = {
        ['+'] = function(x, y) return x + y end,
        ['-'] = function(x, y) return x - y end,
        ['*'] = function(x, y) return x * y end,
        ['/'] = function(x, y) return x / y end,
        ['%'] = function(x, y) return x % y end,
        ['^'] = function(x, y) return x ^ y end,
    }
}

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

---@param result Token|number|string
function Parser:push_result(result)
    ---@type false|Token
    local token = Token.is_instance(result) and result
    if token then
        result = token.data
    end
    ---@cast result string|number
    if type(result) == "string" then
        local converted = tonumber(result)
        if not converted then
            self:throw_error("Non-number")
            return                
        end
        result = converted
    end
    push_narray(self.m_results, result)
end

function Parser:pop_result()
    return pop_narray(self.m_results)
end

---@param kind "unary"|"binary"
---@param op   Token.Type
function Parser:set_result(kind, op)
    -- Inefficient to pop then push but I don't really care at the moment
    if kind == "unary" then
        local x  = self:pop_result()
        local fn = operations.unary[op]
        self:push_result(fn(x))
    else
        local y, x = self:pop_result(), self:pop_result()
        local fn   = operations.binary[op]
        if not fn then
            self:throw_error("'%s' operator not yet implemented", op)
        end
        self:push_result(fn(x, y))
    end
end

---@param token Token
function Parser:push_equation(token)
    push_narray(self.m_equation, token.data)
end

function Parser:serialize_results()
    return serialize_narray(self.m_results)
end

function Parser:serialize_equation()
    return serialize_narray(self.m_equation)
end

---@param lexer Lexer
function Parser:number(lexer)
    self:push_equation(self.m_consumed)
    self:push_result(self.m_consumed)
end

-- Converts prefix `-1` to postfix `0 1 -`.
function Parser:unary(lexer)
    local token = Token(self.m_consumed)
    if token.type == Token.Type.Dash then
        self:push_equation(Token.ZERO) -- Rationale: -x == 0 - x
        self:parse_precedence(lexer, Parser.Prec.Unary)
        self:push_equation(token)
        self:set_result("unary", token.type)
    end
end

-- Factorials aren't infix expressions but rather postfix unary expressions.
-- However for our purposes the parser is in a better state to parse them as if
-- they were an infix rather than a prefix.
function Parser:postfix(lexer)
    local token = Token(self.m_consumed)
    self:push_equation(token)
    self:set_result("unary", token.type)
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
    
    self:set_result("binary", token.type)
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

return Parser
