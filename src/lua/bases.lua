--- builtin
local type = type
local tostring = tostring

--- standard
local format = string.format

--- local
local H = require "src/lua/helper"
local divmod = H.divmod

--- Static buffer for use by everyone here.
local static_buf = require "string.buffer".new(64)

---@param value integer
---@param radix integer
local function convert_integer(value, radix)
    static_buf:reset()
    
    -- Fill in from least significant going to most significant
    local iter = value
    while iter ~= 0 do
        local rest, digit = divmod(iter, radix)
        static_buf:put(digit)
        iter = rest
    end
    local prefix = H.RADIX_TO_PREFIX[radix]
    if prefix then
        static_buf:put(prefix, '0')
    end
    if value < 0 then
        static_buf:put('-')
    end
    return static_buf:tostring():reverse()
end

--[[ 

NOTE: The number of binary digits increases every nonzero power of 2.

| dec |  bin |*| dec |   bin |*| dec |   bin |*| dec |    bin |
|   0 |    0 |*|  10 |  1010 |*|  20 | 10100 |*|  30 |  11110 |
|   1 |    1 |*|  11 |  1011 |*|  21 | 10101 |*|  31 |  11111 |
|   2 |   10 |*|  12 |  1100 |*|  22 | 10110 |*|  32 | 100000 |
|   3 |   11 |*|  13 |  1101 |*|  23 | 10111 |*|  33 | 100001 |
|   4 |  100 |*|  14 |  1110 |*|  24 | 11000 |*|  34 | 100010 |
|   5 |  101 |*|  15 |  1111 |*|  25 | 11001 |*|  35 | 100011 |
|   6 |  110 |*|  16 | 10000 |*|  26 | 11010 |*|  36 | 100100 |
|   7 |  111 |*|  17 | 10001 |*|  27 | 11011 |*|  37 | 100101 |
|   8 | 1000 |*|  18 | 10010 |*|  28 | 11100 |*|  38 | 100110 |
|   9 | 1001 |*|  19 | 10011 |*|  29 | 11101 |*|  39 | 100111 |

0d13:
   3 = 0b0011
  10 = 0b1010
  13 = 0b1101
  
0d123:
    3 = 0b0000_0011
   20 = 0b0001_0100
  100 = 0b0110_0100
  123 = 0b0111_1011

--]]

--[[ 
0x0000007f:
         f =  15 = 0b0000_1111
        70 = 112 = 0b0111_0000
        7f = 127 = 0b0111_1111

0xfeedbeef:
         f =            15 = 0b0000_0000_0000_0000_0000_0000_0000_1111
        e0 =           224 = 0b0000_0000_0000_0000_0000_0000_1110_0000
       e00 =         3_584 = 0b0000_0000_0000_0000_0000_1110_0000_0000
      b000 =        57_344 = 0b0000_0000_0000_0000_1011_0000_0000_0000
     d0000 =       851_968 = 0b0000_0000_0000_1101_0000_0000_0000_0000
    e00000 =    14_680_064 = 0b0000_0000_1110_0000_0000_0000_0000_0000
   e000000 =   234_810_024 = 0b0000_1110_0000_0000_0000_0000_0000_0000
  f0000000 = 4_026_531_840 = 0b1111_0000_0000_0000_0000_0000_0000_0000
  feedbeef = 4_276_993_775 = 0b1111_1110_1110_1101_1011_1110_1110_1111
--]]

local function to_bin_string(line, radix)
    static_buf:reset()
    local sign
    line, radix, sign = H.prepare_string_number(line, radix)
    if sign == -1 then
        static_buf:put('-')
    end
    static_buf:put("0b")
    local bin_digit = H.RADIX_TO_BIN_DIGIT[radix]
    if bin_digit then
        -- Iterate from most significant digit to least significant.
        for char in line:gmatch '.' do
            local base_digit = tonumber(char, radix)
            if base_digit then
                static_buf:put(bin_digit[base_digit])
            end
        end
    else
        error(format("base-%i to base-2 not yet implemented", radix))
    end
    return static_buf:tostring()
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
        return convert_integer(value, 2)
    elseif tp == "string" then
        return to_bin_string(value, radix)
    else
        error(format("Cannot convert '%s' to binary string",  tostring(value)))
    end
end
