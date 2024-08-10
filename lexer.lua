local Class = require "class"
local Token = require "token"

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

function Lexer:found_pattern(pattern)
    ---`lexer.rest` is equivalent to the slice `lexer.input[index:]`, and we
    ---only look for matches at the beginning of `lexer.rest`, we can assume
    ---`stop` is the same as the substring length.
    local _, len, data = self.m_rest:find('^('..pattern..')')
    if not (len and data) then
        return false
    end
    ---@cast data string
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

return Lexer
