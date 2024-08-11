local Class = require "src/lua/class"
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

---@param self   Token
---@param tktype Token.Type
---@param tkdata string
local function set_token(self, tktype, tkdata)
    self.type = tktype
    if tktype == Type.Eof then
        self.data = "<eof>"
    else
        self.data = tkdata or tktype
    end
end

---@param self  Token
---@param token Token
local function copy_token(self, token)
    self.type = token.type
    self.data = token.data
end

---@type fun(inst: Class): boolean
local is_token

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
---@param tkdata? string
Token = Class(function(self, tktype, tkdata)
    if is_token(tktype) then
        copy_token(self, tktype)
    else
        set_token(self, tktype or Type.Eof, tkdata)
    end
end)

is_token         = Token.is_instance
Token.set_token  = set_token
Token.copy_token = copy_token

Token.Type = Type

Token.ZERO = Token(Type.Number, '0')
Token.MUL  = Token(Type.Star)
Token.EOF  = Token(Type.Eof)

return Token
