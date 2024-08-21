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

--- Get only the integer part of `value`.
---
--- We truncate towards zero, because `floor` truncates towards negative
--- infinity if `number` is negative.
---@param number number
function M.truncate_number(number)
    return number >= 0 and floor(number) or ceil(number)
end

local truncate = M.truncate_number

---@param value  string|number
---@param radix? integer        Used only when `value` is a string.
---
---@return integer? integer    `value` with the fractional part truncated.
function M.to_integer(value, radix)
    local number = type(value) == "number" and value or tonumber(value, radix)
    if not number then
        return nil
    else
        return truncate(number)
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
    return truncate(quotient), truncate(remainder)
end

---@param value  integer
---@param radix? integer Default is `10`.
---
---@return integer count Number of `radix` digits in `value`.
function M.count_integer_digits(value, radix)
    -- Flooring negative quotient otherwise rounds towards negative infinity,
    -- and this algorithm is easier if `value` is assumed to be positive.
    if value < 0 then
        value = floor(-value)
    end
    radix = radix or 10
    assert(radix >= 2, "base must be 2 or greater")
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

M.PREFIX_TO_RADIX = {
    ['b'] = 2,  ['B'] = 2,
    ['o'] = 8,  ['O'] = 8,
    ['d'] = 10, ['D'] = 10,
    ['x'] = 16, ['X'] = 16,
}

M.RADIX_TO_PREFIX = {
    [2]  = 'b',
    [8]  = 'o',
    [16] = 'x',
}

---@note Base 10 is too difficult at the moment!
M.RADIX_TO_BIN_DIGIT = {
    [2] = {
        [0]  = '0',      [1]  = '1',
    },
    [8] = {
        [0]  = "000",   [1]  = "001",   [2]  = "010",   [3]  = "011",
        [4]  = "100",   [5]  = "101",   [6]  = "110",   [7]  = "111",
    },
    [16] = {
        [0]  = "0000",  [1]  = "0001",  [2]  = "0010",  [3]  = "0011",
        [4]  = "0100",  [5]  = "0101",  [6]  = "0110",  [7]  = "0111",
        [8]  = "1000",  [9]  = "1001",  [10] = "1010",  [11] = "1011",
        [12] = "1100",  [13] = "1101",  [14] = "1110",  [15] = "1111",
    },
    [32] = {
        [0]  = "00000", [1]  = "00001", [2]  = "00010", [3]  = "00011",
        [4]  = "00100", [5]  = "00101", [6]  = "00110", [7]  = "00111",
        [8]  = "01000", [9]  = "01001", [10] = "01010", [11] = "01011",
        [12] = "01100", [13] = "01101", [14] = "01110", [15] = "01111",
        [16] = "10000", [17] = "10001", [18] = "10010", [19] = "10011",
        [20] = "10100", [21] = "10101", [22] = "10110", [23] = "10111",
        [24] = "11000", [25] = "11001", [26] = "11010", [27] = "11011",
        [28] = "11100", [29] = "11101", [30] = "11110", [31] = "11111",
    }
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
            local radix = M.PREFIX_TO_RADIX[prefix]
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
