local Class = require "class"
local Token

---@enum Token.Type
local Type = {
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

---@class Token : Class
---@field type Token.Type
---@field data string
---
---@overload fun(tktype: Token.Type, data?: string): Token
---@overload fun(token: Token): Token
---@overload fun(): Token
---
---@param self    Token
---@param tktype? Token.Type|Token
---@param data?   string
Token = Class(function(self, tktype, data)
    tktype = tktype or Type.Eof

    -- Do a deep copy if applicable.
    ---@type false|Token
    local token = Token.is_instance(tktype) and tktype
    if token then
        self.type = token.type
        self.data = token.data
        return
    end
    self.type = tktype
    self.data = (tktype == Type.Eof and "<eof>") or data or tktype
end)

Token.Type = Type

Token.ZERO = Token(Type.Number, '0')
Token.MUL  = Token(Type.Star)
Token.EOF  = Token(Type.Eof)

return Token
