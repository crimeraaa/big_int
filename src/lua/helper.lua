---builtin
local type        = type
local tonumber    = tonumber

---standard
local format      = string.format
local floor, ceil = math.floor, math.ceil

---@class Helper
local M = {}

--- TYPE CHECKING/CONVERSION ---------------------------------------------- {{{1

---@param char string
function M.is_ascii_alpha(char)
    return ('a' <= char and char <= 'z') or ('A' <= char and char <= 'Z')
end

---@param char string
function M.is_ascii_digit(char)
    return '0' <= char and char <= '9'
end

---@param char string
function M.is_ascii_alnum(char)
    return M.is_ascii_alpha(char) or M.is_ascii_digit(char)
end

---@param value  string|number
---@param radix? integer        Used only when `value` is a string.
---
---@return integer? integer    `value` with the fractional part truncated.
function M.to_integer(value, radix)
    local number = type(value) == "number" and value or tonumber(value, radix)
    if not number then
        return nil
    else
        -- Truncate towards zero, because `floor` truncates towards negative
        -- infinity if `number` is negative.
        return number >= 0 and floor(number) or ceil(number)
    end
end

---@param value string|number
function M.is_integer(value)
    if type(value) ~= "number" then
        return false
    else
        -- No truncation occured?
        return M.to_integer(value) == value
    end
end

--- 1}}} -----------------------------------------------------------------------

---@param dividend integer
---@param divisor  integer
---
---@return integer quotient
---@return integer remainder
function M.divmod(dividend, divisor)
    local quotient  = dividend / divisor
    local remainder = dividend % divisor
    return quotient >= 0 and floor(quotient) or ceil(quotient), remainder
end

---@param value integer
---@param radix integer
---
---@return integer count Number of `radix` digits in `value`.
function M.count_integer_digits(value, radix)
    -- Flooring negative quotient will otherwise round towards negative infinity
    if value < 0 then
        value = floor(-value)
    end
    local count = 0
    while value ~= 0 do
        count = count + 1
        value = floor(value / radix)
    end
    return count
end

---@param line  string  Assumes prefix and leading 0's have been removed.
---@param radix integer
---
---@return integer count Number of `radix` digits in `line`.
function M.count_string_digits(line, radix)
    local count = 0
    for char in line:gmatch '.' do
        if tonumber(char, radix) then
            count = count + 1
        end
    end
    return count
end

local RADIX_PREFIX = {
    ['b'] = 2,  ['B'] = 2,
    ['o'] = 8,  ['O'] = 8,
    ['d'] = 10, ['D'] = 10,
    ['x'] = 16, ['X'] = 16,
}

---@param line string
---
---@return string rest
---@return 1|-1   sign
function M.trim_string_leading_non_numeric(line)
    local count = 0
    local sign  = 1
    for char in line:gmatch '.' do
        if M.is_ascii_alnum(char) then
            break
        end
        if char == '+' then
            sign = 1
        elseif char == '-' then
            sign = (sign == 1) and -1 or 1
        end
        count = count + 1
    end
    return line:sub(count + 1), sign
end

---@param line string
function M.trim_string_trailing_non_numeric(line)
    local count = 0
    for char in line:reverse():gmatch '.' do
        if M.is_ascii_alnum(char) then
            break
        end
        count = count + 1
    end
    return line:sub(1, -(count + 1))
end

---@param line string
function M.trim_string_leading_zeroes(line)
    local count = 0
    for char in line:gmatch '.' do
        if char == '0' then
            count = count + 1
        else
            break
        end
    end
    return line:sub(count + 1)
end

---Skip leading whitespaces, parse base prefix, and skip leading zeroes.
---@param line string
---
---@return string?    rest  Slice of `line` without the prefix, if any.
---@return 2|8|10|16? radix Detected prefix, if any and valid.
function M.detect_string_radix(line)
    local rest  = M.trim_string_leading_non_numeric(line)
    if #rest >= 2 and rest:sub(1, 1) == '0' then
        local prefix = rest:sub(2, 2)
        if M.is_ascii_alpha(prefix) then
            local radix = RADIX_PREFIX[prefix]
            if radix then
                return rest:sub(3), radix
            else
                return nil, nil
            end
        end
    end
    return rest, 10
end

---@param line   string
---@param radix? integer
function M.prepare_string_number(line, radix)
    local rest, sign = M.trim_string_leading_non_numeric(line)
    radix = radix or 0
    if radix == 0 then
        rest, radix = M.detect_string_radix(rest)
        if not (rest and radix) then
            error(format("Invalid number string '%s'", line))
        end
    end
    rest = M.trim_string_leading_zeroes(rest)
    rest = M.trim_string_trailing_non_numeric(rest)
    return rest, radix, sign
end

return M
