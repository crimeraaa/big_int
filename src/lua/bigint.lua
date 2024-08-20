local math   = math
local string = string
local table  = table

---@class BigInt:  Class
---@field m_digits integer[]
---@field m_active integer
---@field m_sign   BigInt.Sign
---
---@overload fun(value?: string|integer): BigInt
local BigInt

---@alias BigInt.Radix 2|8|10|16
---@alias BigInt.Sign -1|1

local Class  = require "src/lua/class"
local H      = require "src/lua/helper"

---Reduce table lookups for frequent and/or expensive operations.
local tonumber = tonumber
local format = string.format
local concat = table.concat
local divmod = H.divmod

---Use a power of 10 for our sanity.
---The square of this must fit in the underlying type of Lua's number.
---
---There's a good chance your Lua is just using a C `double`, which can hold
---`-(2^53)..=(2^53)` integers without loss of precision.
local DIGIT_BASE  = 10e6
local DIGIT_MAX   = DIGIT_BASE - 1

-- How many base-N digits can fit in a single base-DIGIT_BASE digit?
---See the Python output of `bases.py`.
local DIGIT_WIDTH = {
    [2]  = 24,
    [8]  = 8,
    [10] = 7,
    [16] = 6
}

---@param self BigInt
local function set_zero(self)
    self.m_active = 0
    self.m_sign   = 1
    return self
end

---@param self  BigInt
---@param value integer
local function set_from_integer(self, value)
    if value == 0 then
        return set_zero(self)
    end
    
    if value >= 0 then
        self.m_sign = 1
    else
        value = -value
        self.m_sign = -1
    end

    local radix = DIGIT_BASE
    local count = H.count_integer_digits(value, radix)
    -- print("value", value, "count", count) --!DEBUG
    self.m_active = count
    for index = 1, count, 1 do
        local rest, digit = divmod(value, radix)
        -- print("index", index, "digit", digit, "rest", rest) --!DEBUG
        self.m_digits[index] = digit
        value = rest
    end
    return self
end

--[[ 
Assume BASE = 10_000_000.
    Given string "0xfeedbeefc0ffee":

    rest  = "feedbeefc0ffee"
    radix = 16

    str_ndigits = 14
    width       = DIGIT_WIDTH[radix]   = 6
    bi_ndigits  = str_ndigits // width = 14 // 6 = 2
    leftover    = str_ndigits  % width =  8  % 6 = 2

    if leftover ~= 0 then
        bi_ndigits = bi_ndigits + 1 = 2 + 1 = 3
    end
--]]

---@param line string
---
---@return string       rest
---@return BigInt.Radix radix
---@return BigInt.Sign  sign
local function prepare_string(line)
    local rest = H.trim_string_leading_non_numeric(line)
    local sign = 1
    local skipped = 0
    for char in rest:gmatch '.' do
        if char == '+' then
            sign = 1
            skipped = skipped + 1
        elseif char == '-' then
            sign = (sign == 1) and -1 or 1
            skipped = skipped + 1
        else
            break
        end
    end
    local radix
    rest, radix = H.detect_string_radix(rest:sub(skipped + 1))
    if not (rest and radix) then
        error(format("Invalid number string '%s'", line))
    end
    return H.trim_string_leading_zeroes(rest), radix, sign
end

---@param self  BigInt
---@param line  string
---@param radix BigInt.Radix
local function set_from_pow2_string(self, line, radix)
    error(format("BigInt from base-%i string not yet implemented", radix))
    return self
end

---@param self  BigInt
---@param line  string
---@param radix integer
local function set_from_pow10_string(self, line, radix)
    local value = 0
    local block = 1
    local place = 1
    
    -- print("line", line) --!DEBUG
    for char in line:reverse():gmatch '.' do
        if not (char == ' ' or char == ',' or char == '_') then
            local digit = tonumber(char, radix)
            if digit then
                value = value + digit*place
                place = place * radix 

                -- Overflow?
                if value > DIGIT_MAX then
                    -- Quotient:   Upper digits that caused us to overflow.
                    -- Remainder: `value` without the upper digits.
                    local carry, rest = divmod(value, DIGIT_BASE)
                    self.m_digits[block] = rest
                    value = carry
                    place = radix
                    block = block + 1
                end
                -- print("block", block, "value", value) --!DEBUG
            else
                error(format("Invalid base-%i digit '%s'", radix, char))
            end
        end
    end
    if value ~= 0 then
        self.m_digits[block] = value
    end
    return self
end

---@param self BigInt
---@param line string
local function set_from_string(self, line)
    local rest, radix, sign = prepare_string(line)
    local str_ndigits       = H.count_string_digits(rest, radix)
    local width             = DIGIT_WIDTH[radix]

    self.m_sign = sign

    ---Assume BASE = 10_000_000.
    ---Given the string "1_234_567_890":
    ---
    ---radix       = 10
    ---str_ndigits = 10
    ---
    ---We know that 7 base-10 digits can fit in 1 base-BASE digit.
    ---
    ---width = DIGIT_WIDTH[radix]
    ---      = 7
    ---
    ---We first calculate the initial amount of needed base-BASE digits.
    ---
    ---bi_ndigits = str_digit // width
    ---           = 10 // 7
    ---           = 1
    ---
    ---However, we know that 1 base-BASE digit isn't enough to fit the
    ---10 base-10 digits. So we get the remainder to check for how many base-10
    ---digits we have left over:
    ---
    ---leftover = str_ndigits % width
    ---         = 10 % 7
    ---         = 3
    ---
    ---The remainder is nonzero, meaning we need more base-BASE digits.
    ---We assume we only need 1 more since `width` guarantees that the leftovers
    ---will already fit in 1 base-BASE digit.
    local bi_ndigits, leftover = divmod(str_ndigits, width)
    if leftover ~= 0 then
        bi_ndigits = bi_ndigits + 1
    end

    self.m_active = bi_ndigits
    if radix == 10 then
        return set_from_pow10_string(self, rest, radix)
    else
        return set_from_pow2_string(self, rest, radix)
    end
end

---@param self   BigInt 
---@param value? string|integer
local function constructor(self, value)
    self.m_digits = {}
    self.m_active = 0
    self.m_sign   = 1
    local tp = type(value)
    if tp == "number" then
        set_from_integer(self, value)
    elseif tp == "string" then
        set_from_string(self, value)
    end
end

BigInt = Class(constructor)

---@param self BigInt
local function get_pretty_string(self)
    -- Push most significant digits first.
    local top = self.m_active

    -- For the very most significant digit, don't add leading zeroes.
    ---@type string[]
    local out = {format("%i", self.m_digits[top])}
    if self.m_sign == -1 then
        table.insert(out, 1, '-')
    end
    local n = #out
    for index = top - 1, 1, -1 do
        n = n + 1
        out[n] = format("%07i", self.m_digits[index])
    end
    
    return "BigInt: "..concat(out)
end

BigInt.set_from_integer = set_from_integer
BigInt.set_from_string  = set_from_string
BigInt.__tostring       = get_pretty_string

if arg and arg[1] == "set-global" then
    _G.BigInt = BigInt
end

return BigInt
