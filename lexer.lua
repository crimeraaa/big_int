local Class  = require "class"
local Token  = require "token"
local TkType = Token.Type

---@param self    Lexer
---@param tktype? Token.Type
local function make_token(self, tktype)
    self.m_token:set_token(tktype or TkType.Error, self.m_lexeme)
    return self.m_token
end


---@param self  Lexer
---@param input string
local function set_input(self, input)
    self.m_index  = 1
    self.m_lexeme = ""
    self.m_input  = input
    self.m_rest   = input
    self.m_token  = make_token(self, TkType.Eof)
end

-- `lexer.rest` is equivalent to the slice `lexer.input[index:]`, and we
-- only look for matches at the beginning of `lexer.rest`, we can assume
-- `stop` is the same as the substring length.
---@param self    Lexer
---@param pattern string
local function find_pattern(self, pattern)
    local _, len, data = self.m_rest:find('^('..pattern..')')
    ---@cast data string|nil
    return _, len, data
end

---@param self Lexer
---@param len  integer
---@param data string
local function update_lexeme(self, len, data)
    local newidx  = self.m_index + len
    self.m_index  = newidx
    self.m_lexeme = data
    self.m_rest   = self.m_input:sub(newidx)
end

---@param self    Lexer
---@param pattern string
local function append_pattern(self, pattern)
    local _, len, data = find_pattern(self, pattern)
    if not (len and data) then
        return false
    end
    update_lexeme(self, len, self.m_lexeme .. data)
    return true
end

---@param self    Lexer
---@param pattern string
local function found_pattern(self, pattern)
    local _, len, data = find_pattern(self, pattern)
    if not (len and data) then
        return false
    end
    update_lexeme(self, len, data)
    return true
end

local function skip_whitespace(self)
    return found_pattern(self, "%s+")
end

local single_chars = {
    ['('] = TkType.LeftParen,   [')'] = TkType.RightParen,
    ['+'] = TkType.Plus,        ['-'] = TkType.Dash,
    ['*'] = TkType.Star,        ['/'] = TkType.Slash,
    ['%'] = TkType.Percent,     ['^'] = TkType.Caret,
    ['!'] = TkType.Exclamation,
}

local double_chars = {
    ["=="] = TkType.EqualEqual, ["~="] = TkType.TildeEqual,
    ['<']  = TkType.LeftAngle,  ["<="] = TkType.LeftAngleEqual,
    ['>']  = TkType.RightAngle, [">="] = TkType.RightAngleEqual,
}

---@param self Lexer
---@nodiscard
local function scan_token(self)
    skip_whitespace(self)
    -- Digits followed by 0 or more digit-likes.
    if found_pattern(self, "[%d,_.]+") then
        append_pattern(self, "[eE][+-]?")
        append_pattern(self, "[%w,_.]+")
        return make_token(self, TkType.Number)
    end
    -- Guaranteed single-character tokens.
    -- Note we put '-' first because otherwise it'll be considered a range.
    if found_pattern(self, "[-()+*/%%^!]") then
        return make_token(self, single_chars[self.m_lexeme])
    end
    -- Potentially double-character tokens.
    -- In the case of single '=', there is no mapping for it so it is nil.
    if found_pattern(self, "[=~<>]=?") then
        return make_token(self, double_chars[self.m_lexeme])
    end
    -- Still have non-whitespace stuff left in the rest string?
    local more = found_pattern(self, "[%S]+")
    return make_token(self, more and TkType.Error or TkType.Eof)
end

---@class Lexer : Class
---@field m_index  integer Index into the full string of the current lexeme.
---@field m_lexeme string  The current lexeme substring.
---@field m_input  string  The full string of the user's input.
---@field m_rest   string  Slice of the full string starting at the current lexeme.
---@field m_token  Token   To be used for copying by `Parser`.
---
---@overload fun(input?: string): Lexer
---@param self Lexer  Created and passed by `__call()` in `Class()`.
---@param input string
local Lexer = Class(function(self, input)
    input = input or ""
    self.m_index  = 1
    self.m_lexeme = ""
    self.m_input  = input
    self.m_rest   = input
    self.m_token  = Token()
end)

Lexer.set_input  = set_input
Lexer.scan_token = scan_token

return Lexer
