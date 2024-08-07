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

---@enum Precedence
local Precedence = {
    None       = 0,
    Equality   = 1,     -- == ~=
    Terminal   = 2,     -- + -
    Comparison = 3,   -- < <= >= >
    Factor     = 4,       -- / *
}

---@class Token
---@field type  Token.Type
---@field data  string

local Token = { 
    ---@enum Token.Type
    Type = {
        LeftParen    = 1,
        RightParen   = 2,

        Plus         = 3,
        Minus        = 4,
        Star         = 5,
        Slash        = 6,
        
        EqualEqual   = 7,
        TildeEqual   = 8,
        LessThan     = 9,
        LessEqual    = 10,
        GreaterThan  = 11,
        GreaterEqual = 12,

        Number       = 13,
        Error        = 14,
        Eof          = 15,
    },
}

Token.CharToType = {
    ['(']  = Token.Type.LeftParen,
    [')']  = Token.Type.RightParen,
    ['+']  = Token.Type.Plus,
    ['-']  = Token.Type.Minus,
    ['*']  = Token.Type.Star,
    ['/']  = Token.Type.Slash,
    
    ["=="] = Token.Type.EqualEqual,
    ["~="] = Token.Type.TildeEqual,
    ['<']  = Token.Type.LessThan,
    ["<="] = Token.Type.LessEqual,
    ['>']  = Token.Type.GreaterThan,
    [">="] = Token.Type.GreaterEqual,
}

local lexer = {
    index   = 1,
    data    = "",
    input   = "",
    rest    = "",
}

---@param type? Token.Type
function lexer:make_token(type)
    ---@type Token
    local token = {
        type = type or Token.Type.Error,
        data = self.data,
    }
    return token
end

function lexer:match_start_of_rest(pattern)
    ---@type integer|nil, integer|nil, string|nil
    return self.rest:find('^(' .. pattern .. ')')
end

function lexer:found_pattern(pattern)
    ---`lexer.rest` is equivalent to the slice `lexer.input[index:]`, and we
    ---only look for matches at the beginning of `lexer.rest`, we can assume
    ---`stop` is the same as the substring length.
    local _, len, data = self:match_start_of_rest(pattern)
    if len and data then
        local oldindex = self.index
        local newindex = oldindex + len
        self.index = newindex
        self.data  = data
        self.rest  = self.input:sub(newindex)
        -- printfln("input[%i:%i] '%s', #input %i", oldindex, newindex - 1, data, #self.input)
        return true
    end
    return false
end

function lexer:skip_whitespace()
    return self:found_pattern("%s+")
end

function lexer:scan_token()
    self:skip_whitespace()
    if self:found_pattern("[%d,_.]+") then
        return self:make_token(Token.Type.Number)
    elseif self:found_pattern("[-+*/]") then
        return self:make_token(Token.CharToType[self.data])
    elseif self:found_pattern("[=~<>]=?") then
        -- Lone '=' won't ever receive a valid token.
        return self:make_token(Token.CharToType[self.data])
    else
        -- Still have non-whitespace stuff left in the rest string?
        local more = self:found_pattern("[%S]+")
        if not more then
            self.data = "(eof)"
        end
        return self:make_token(more and Token.Type.Error or Token.Type.Eof)
    end
end

local parser = {
    consumed  = lexer:make_token(),
    lookahead = lexer:make_token(),
}

---@param ... Token.Type
function parser:match_lookahead(...)
    for _, ttype in ipairs{...} do
        if self.lookahead.type == ttype then
            return true
        end
    end
    return false
end

function parser:update_lookahead()
    self.consumed  = self.lookahead
    self.lookahead = lexer:scan_token()
    if self:match_lookahead(Token.Type.Error) then
        printfln("[ERROR] Unexpected symbol '%s'", lexer.data)
    end
end

---@param token Token
---@param prec  Precedence
function parser:precedence(token, prec)
end

function parser:print()
    local x, y = self.consumed, self.lookahead
    printfln("consumed '%s'\tlookahead '%s'", x.data, y.data)
end

function parser:expression()
    parser:precedence(parser.consumed, Precedence.None + 1)
end


---@param input string
local function compile(input)
    lexer.index = 1
    lexer.data  = ""
    lexer.input, lexer.rest = input, input
    
    -- Ensure lookahead token is something we can start with
    parser:update_lookahead()
    while not parser:match_lookahead(Token.Type.Error, Token.Type.Eof) do
        parser:update_lookahead()
        parser:expression()
        parser:print()
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
