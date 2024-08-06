local Class = require "class"

--- UTILITY --------------------------------------------------------------- {{{1

-- This is here only because it's easier to grep than the regex `/m_length =/`.
---@param self BigInt
---@param length integer
local function set_length(self, length)
    self.m_length = length
end

-- Same idea as `set_length`.
---@param self BigInt
---@param negative boolean
local function set_negative(self, negative)
    self.m_negative = negative
end

---@param self BigInt
---@param index integer
local function check_index(self, index)
    return 1 <= index and index <= self.m_length
end

---@param self BigInt
---@param digit integer
local function check_digit(self, digit)
    return 0 <= digit and digit < self.BASE
end

---@param self  BigInt
---@param index integer
local function read_at(self, index)
    -- Don't rely on `self.m_digits[index]` being `nil` if we 'shrank' the buffer.
    if check_index(self, index) then
        return self.m_digits[index]
    end
    return 0
end

---@param self      BigInt
---@param start?    integer
---@param stop?     integer
local function clear_buffer(self, start, stop)
    for i = start or 1, stop or self:len(), 1 do
        self.m_digits[i] = 0
    end
end

-- Equivalent to realloc'ing the array then memset'ing it to 0 in C.
---@param self BigInt
---@param newcap integer
local function resize_buffer(self, newcap)
    local oldcap = self.m_capacity
    if newcap <= oldcap then
        return
    end
    clear_buffer(self, self:len() + 1, newcap)
    self.m_capacity = newcap
end

---@param base  integer
---@param start integer
local function find_next_power_of(base, start)
    local pow = 1
    while pow < start do
        pow = pow * base
    end
    return pow
end

---@param self  BigInt
---@param index integer
---@param digit integer
local function write_at(self, index, digit)
    if 1 <= index and check_digit(self, digit) then
        if index > self.m_capacity then
            resize_buffer(self, find_next_power_of(2, index))
        end
        if index > self:len() then
            set_length(self, index)
        end
        self.m_digits[index] = digit
        return true
    end
    return false
end

---@param self  BigInt
---@param digit integer
local function push_msd(self, digit)
    if not check_digit(self, digit) then
        return false
    end
    local i = self:len() + 1
    self.m_digits[i] = digit
    self.m_length    = i
    return true
end

---@param self BigInt
local function pop_msd(self)
    local len = self:len()
    if len <= 0 then
        return 0
    end
    local digit = self.m_digits[len]
    set_length(self, len - 1)
    return digit
end

---@param self BigInt
local function trim_zeroes_msd(self)
    -- NOTE: Normally dangerous to modify invariant during a loop! In this case
    -- it's fine since we iterate from the highest index going to the lowest.
    -- So any invalidation of the upper indexes does not affect our traversal.
    for _, v in self:iter_msd() do
        -- No more leading 0's?
        if v ~= 0 then
            break
        end
        pop_msd(self)
    end
end

---@param self  BigInt
---@param value integer
local function set_integer(self, value)
    if value < 0 then
        value = math.abs(value)
        set_negative(self, true)
    end

    value = math.floor(value)
    while value ~= 0 do
        local next  = math.floor(value / self.BASE)
        local digit = math.floor(value % self.BASE)
        push_msd(self, digit)
        value = next
    end
end

---@param input string
local function set_string(self, input)
    for char in input:reverse():gmatch(".") do
        if char == '-' then
            set_negative(self, not self:is_negative())
        end
        -- Think of this like doing `char` manipulation in C
        if tonumber(char) and '0' <= char and char <= '9' then
            push_msd(self, char - '0')
        end
    end
end

---@param self  BigInt
---@param other BigInt
local function set_bigint(self, other)
    if self == other then
        return
    end
    resize_buffer(self, other:len())
    for i, v in other:iter_lsd() do
        write_at(self, i, v)
    end
    set_negative(self, other:is_negative())
end

--- 1}}} -----------------------------------------------------------------------

local BigInt

---@param self BigInt
---@param value? BigInt|integer|string
local function ctor(self, value)
    self.m_digits   = {}
    self.m_length   = 0
    self.m_capacity = 0
    self.m_negative = false
    -- WARNING: Relies on `BigInt` to be global! If the declaration above is
    -- local, this will fail!
    if BigInt.is_instance(value) then
        set_bigint(self, value)
    elseif type(value) == "number" then
        set_integer(self, value)
    elseif type(value) == "string" then
        set_string(self, value)
    elseif value == nil then
        push_msd(self, 0)
    end
end

---@class BigInt: Class
---@field m_digits   integer[]  Stored in a little-endian fashion.
---@field m_length   integer    Number of active values in `digits`.
---@field m_capacity integer    Number of total values in `digits`.
---@field m_negative boolean    For simplicity even 0 is positive.
---
---@operator add: BigInt
---@operator sub: BigInt
---@operator unm: BigInt
---@overload fun(value?: BigInt|integer|string): BigInt
BigInt = Class(ctor)
BigInt.BASE = 10

---@param key any
---@see   Class.__index
function BigInt:__index(key)
    if type(key) == "number" then
        return read_at(self, key)
    else
        return BigInt[key]
    end
end

function BigInt:__tostring()
    local t, n = {}, 0
    if self:is_negative() then
        n = n + 1
        t[n] = '-'
    end
    -- Iterate in reverse.
    for _, v in self:iter_msd() do
        n = n + 1
        t[n] = v
    end
    return "BigInt: " .. (self:is_empty() and "(empty)" or table.concat(t))
end

function BigInt:len()
    return self.m_length
end

function BigInt:is_negative()
    return self.m_negative
end

function BigInt:is_empty()
    return self:len() == 0
end

--- ITERATORS ------------------------------------------------------------- {{{1

---@param self  BigInt  Also known as the 'invariant' state.
---@param index integer Also known as the 'control' variable.
local function iterfn_lsd(self, index)
    index = index + 1
    if check_index(self, index) then
        return index, self.m_digits[index]
    end
end

---@param self  BigInt
---@param index integer
local function iterfn_msd(self, index)
    index = index - 1
    if check_index(self, index) then
        return index, self.m_digits[index]
    end
end

-- Iterate starting from the least significant digits going to most significant.
---@param start? integer
function BigInt:iter_lsd(start)
    start = start or 1
    -- iterator function, 'invariant' state, 'control' variable.
    return iterfn_lsd, self, start - 1
end

-- Iterate starting from the most significant digits going to least significant.
---@param start? integer
function BigInt:iter_msd(start)
    start = start or self:len()
    return iterfn_msd, self, start + 1
end

--- 1}}} -----------------------------------------------------------------------

--- OPERATORS ------------------------------------------------------------- {{{1

function BigInt:abs()
    set_negative(self, false)
    return self
end

-- Note that this is a toggle!
function BigInt:unm()
    set_negative(self, not self:is_negative())
    return self
end

---@param x BigInt
function BigInt.__unm(x)
    return BigInt(x):unm()
end

--- ARITHMETIC ------------------------------------------------------------ {{{2

---@param x BigInt
---@param y BigInt
local function pick_greater_length(x, y)
    return (x:len() > y:len()) and x:len() or y:len()
end

---@param x BigInt
---@param y BigInt
local function pick_lesser_length(x, y)
    return (x:len() < y:len()) and x:len() or y:len()
end

-- 1.)    x + y
-- 2.)    x + (-y) ==   x - y
-- 3.) (-x) + y    == -(x - y)
-- 4.) (-x) + (-y) == -(x + y)
---@param x BigInt
---@param y BigInt
---@see BigInt.sub
function BigInt:add(x, y)
    -- Create copies in case 'x' or 'y' is the same as 'self'.
    x, y = BigInt(x), BigInt(y)

    if x:is_negative() ~= y:is_negative() then
        if x:is_negative() then
            return self:sub(x:abs(), y):unm()
        else
            return self:sub(x, y:abs())
        end
    end

    -- Since 0 is not negative here, we assume that addition of 2 negatives
    -- will always result in a negative.
    set_negative(self, x:is_negative() and y:is_negative())

    -- Include 1 more for the very last carry if it extends our length.
    local len   = pick_greater_length(x, y) + 1
    local carry = 0
    resize_buffer(self, len)
    set_length(self, len)
    for i in self:iter_lsd() do
        local sum = read_at(x, i) + read_at(y, i) + carry
        if (sum >= self.BASE) then
            carry = math.floor(sum / self.BASE)
            sum   = math.floor(sum % self.BASE)
        else
            carry = 0
        end
        write_at(self, i, sum)
    end
    trim_zeroes_msd(self)
    return self
end

---@param self  BigInt
---@param index integer
local function borrowed(self, index)
    for i in self:iter_lsd(index + 1) do
        local digit  = read_at(self, i)
        local borrow = (digit == 0) and self.BASE - 1 or digit - 1
        write_at(self, i, borrow)
        -- Assume that somewhere down the line is a nonzero.
        if digit ~= 0 then
            -- Borrowing caused this place to become zero?
            if borrow == 0 then
                pop_msd(self)
            end
            return true
        end
    end
    return false
end

---@param self BigInt
local function check_zero(self)
    for _, v in self:iter_msd() do
        if v ~= 0 then
            return self
        end
    end
    set_length(self, 1)
    set_negative(self, false)
    return self
end

-- 1.)    x - y
-- 2.)    x - (-y) ==   x + y
-- 3.) (-x) - y    == -(x + y)
-- 4.) (-x) - (-y) ==   y - x
---@param x BigInt
---@param y BigInt
---@see BigInt.add
function BigInt:sub(x, y)
    -- Creates copies in case either or both point to 'self'.
    x, y = BigInt(x), BigInt(y)
    if x:is_negative() ~= y:is_negative() then
        if x:is_negative() then
            return self:add(x:abs(), y):unm()
        else
            return self:add(x, y:abs())
        end
    end
    -- Note how we flip the operands' order as well.
    if x:is_negative() and y:is_negative() then
        x, y = y:abs(), x:abs()
    end

    -- Small minuend minus large subtrahend is always negative.
    local adjusted = x:__lt(y)
    if adjusted then
        x, y = y, x
    end

    local len = pick_lesser_length(x, y)
    -- Copy minuend's digits as we will need to modify them when borrowing.
    set_bigint(self, x)
    set_length(self, len)
    set_negative(self, adjusted)

    for i in self:iter_lsd() do
        local minuend, subtrahend = read_at(x, i), read_at(y, i)
        if (minuend < subtrahend) then
            if borrowed(self, i) then
                minuend = minuend + self.BASE
            else
                minuend, subtrahend = subtrahend, minuend
                set_negative(self, true)
            end
        end
        write_at(self, i, minuend - subtrahend)
    end
    return check_zero(self)
end

---@param x BigInt
---@param y BigInt
function BigInt.__add(x, y)
    return BigInt():add(x, y)
end

---@param x BigInt
---@param y BigInt
function BigInt.__sub(x, y)
    return BigInt():sub(x, y)
end

--- 2}}} -----------------------------------------------------------------------

--- RELATIONAL ------------------------------------------------------------ {{{2

---@param x BigInt
---@param y BigInt
local function quick_eq(x, y)
    return x:len() == y:len() and x:is_negative() == y:is_negative()
end

---@param x BigInt
---@param y BigInt
local function quick_lt(x, y)
    if x:is_negative() ~= y:is_negative() then
        -- Since both are different, if x is negative then y is positive.
        -- Negatives are always less than positives.
        return x:is_negative()
    end
    -- If same signs and same lengths both paths will return false either way.
    -- E.g. -11 > -222, although -11 has only 2 digits it is higher up the
    -- number line than -222.
    if x:is_negative() then
        return x:len() > y:len()
    else
        return x:len() < y:len()
    end
end

---@param x BigInt
---@param y BigInt
function BigInt.__eq(x, y)
    if not quick_eq(x, y) then
        return false
    end
    for i, v in x:iter_msd() do
        if v ~= read_at(y, i) then
            return false
        end
    end
    return true
end

---@param x BigInt
---@param y BigInt
function BigInt.__lt(x, y)
    if x:__eq(y) then
        return false
    elseif not quick_eq(x,y) then
        return quick_lt(x, y)
    end

    -- Always check most significant digits first.
    for i, v in x:iter_msd() do
        if v > y[i] then
            return false
        end
    end
    return true
end

---@param x BigInt
---@param y BigInt
function BigInt.__le(x, y)
    return x:__eq(y) or x:__lt(y)
end

--- 2}}} -----------------------------------------------------------------------

--- 1}}} -----------------------------------------------------------------------

--- TEST
x, y, z = BigInt(4), BigInt(5), BigInt()

print("===BOTH POSITIVE===")
print("   4 - 5 =", 4 - 5, z:sub(x, y))
print("   5 - 4 =", 5 - 4, z:sub(y, x))

print("===BOTH NEGATIVE===")
x:unm(); y:unm()
print("(-4) - (-5)", (-4) - (-5), z:sub(x, y))
print("(-5) - (-4)", (-5) - (-4), z:sub(y, x))

print("===ONE POSITIVE, ONE NEGATIVE (1)===")
y:abs()
print("(-4) - 5    =", (-4) - 5, z:sub(x, y))
print("   5 - (-4) =", 5 - (-4), z:sub(y, x))

print("===ONE POSITIVE, ONE NEGATIVE (2)===")
x:abs()
y:unm()
print("   4 - (-5) =", 4 - (-5), z:sub(x, y))
print("(-5) - 4    =", (-5) - 4, z:sub(y, x))

x, y = BigInt(11), BigInt(77)

print("===11 AND 77===")
print("   11 - 77 =", 11 - 77,  z:sub(x, y))
print("(-66) - 11 =", (-66) - 11, z:sub(z, x))
print("(-77) + 11 =", (-77) + 11, z:add(z, x))
print("(-66) + 77 =", (-66) + 77, z:add(z, y))
print("   11 - 77 =", 11 - 77,    z:sub(z, y))
print("(-66) - 77 =", (-66) - 77, z:sub(z, y))

function print_keys(t)
    for k in pairs(t) do
        print(k, type(k))
    end
end

return BigInt
