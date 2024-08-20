---builtin
local type   = type
local tostring, tonumber = tostring, tonumber

---standard
local concat = table.concat
local format = string.format

---local
local H      = require "src/lua/helper"
local divmod = H.divmod

---@generic T
---@param array   T[]
---@param length? integer
local function reverse(array, length)
    local left  = 1
    local right = length or #array
    while left <= right do
        array[left], array[right] = array[right], array[left]
        left  = left + 1
        right = right - 1
    end
    return array
end

---@param value integer
---@param radix integer
local function convert_integer(value, radix)
    local out = {}
    local n   = #out
    
    -- Fill in from least significant going to most significant
    local iter = value
    while iter ~= 0 do
        local rest, digit = divmod(iter, radix)
        n = n + 1
        out[n] = tonumber(digit, radix)
        iter = rest
    end
    return concat(reverse(out, n))
end

---@param value  integer|string
---@param radix? integer        May only be used only when `value` is a string.
---
---@overload fun(value: integer): string
---@overload fun(line: string, radix: integer): string
function bin(value, radix)
    if value == 0 then
        return "0b0"
    end
    local tp = type(value)
    if tp == "number" then
        return "0b"..convert_integer(value, 2)
    elseif tp == "string" then
        local out = {}
        local n = #out
        local line, sign
        line, radix, sign = H.prepare_string_number(value, radix)
        if sign == -1 then
            n = n + 1
            out[n] = '-'
        end
        error("bin() with a string argument is WIP")
    else
        error(format("Cannot convert '%s' to binary string",  tostring(value)))
    end
end
