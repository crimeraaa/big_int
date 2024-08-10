local Class = require "class"
local Token

---@class Token : Class
---@field type Token.Type
---@field data string
---
---@overload fun(tktype: Token.Type, data?: string): Token
---@overload fun(token: Token): Token
---@overload fun(): Token
---
---@param inst    Token
---@param tktype? Token.Type|Token
---@param data?   string
Token = Class(function(inst, tktype, data)
    tktype = tktype or Token.Type.Eof
    -- Do a deep copy if applicable.
    local token = Token.is_instance(tktype) and tktype
    
    ---@cast token Token
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

return Token
