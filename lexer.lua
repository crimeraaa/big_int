local Class = require "class"
local Token = require "token"

---@class Lexer : Class
---@field m_index  integer Index into the full string of the current lexeme.
---@field m_lexeme string  The current lexeme substring.
---@field m_input  string  The full string of the user's input.
---@field m_rest   string  Slice of the full string starting at the current lexeme.
---@field m_token  Token   To be used for copying by `Parser`.
---
---@overload fun(data?: string): Lexer
---@param self Lexer  Created and passed by `__call()` in `Class()`.
---@param input string
local Lexer = Class(function(self, input)
    self:set_input(input or "")
end)

---@param input string
function Lexer:set_input(input)
    self.m_index  = 1
    self.m_lexeme = ""
    self.m_input  = input
    self.m_rest   = input
    self.m_token  = Token()
end

---@param tktype? Token.Type
function Lexer:make_token(tktype)
    self.m_token:set_token(tktype or Token.Type.Error, self.m_lexeme)
    return self.m_token
    -- return Token(tktype or Token.Type.Error, self.m_lexeme)
end

function Lexer:found_pattern(pattern)
    -- `lexer.rest` is equivalent to the slice `lexer.input[index:]`, and we
    -- only look for matches at the beginning of `lexer.rest`, we can assume
    -- `stop` is the same as the substring length.
    local _, len, data = self.m_rest:find('^('..pattern..')')
    if not (len and data) then
        return false
    end
    ---@cast data string
    local newindex = self.m_index + len
    self.m_index   = newindex
    self.m_lexeme  = data
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
    if self:found_pattern("%d[%w,_.]*") then
        return self:make_token(Token.Type.Number)
    end
    -- Guaranteed single-character tokens.
    -- Note we put '-' first because otherwise it'll be considered a range.
    if self:found_pattern("[-()+*/%%^!]") then
        return self:make_token(singlechars[self.m_lexeme])
    end
    -- Potentially double-character tokens.
    -- In the case of single '=', there is no mapping for it so it is nil.
    if self:found_pattern("[=~<>]=?") then
        return self:make_token(doublechars[self.m_lexeme])
    end
    -- Still have non-whitespace stuff left in the rest string?
    local more = self:found_pattern("[%S]+")
    return self:make_token(more and Token.Type.Error or Token.Type.Eof)
end

return Lexer
