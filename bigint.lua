local Class = require "class"

local BigInt

---@class BigInt: Class
---@field m_digits   integer[]  Stored in a little-endian fashion.
---@field m_length   integer    Number of active values in `digits`.
---@field m_capacity integer    Number of total values in `digits`.
---@field m_negative boolean    For simplicity even 0 is positive.
---
---@operator add: BigInt
---@operator sub: BigInt
---@operator unm: BigInt
---
---@overload fun(value?: BigInt|integer|string): BigInt
---@param inst   BigInt
---@param value? integer|string|BigInt
BigInt = Class(function(inst, value)
    inst.m_digits   = {}
    inst.m_length   = 0
    inst.m_capacity = 0
    inst.m_negative = false
    if BigInt.is_instance(value) then
        inst:set_bigint(value)
    elseif type(value) == "number" then
        inst:set_integer(value)
    elseif type(value) == "string" then
        inst:set_string(value)
    elseif value == nil then
        inst:set_integer(0)
    end
end)

BigInt.BASE = 10

--- UTILITY --------------------------------------------------------------- {{{1

---@param self BigInt
---@param digit integer
local function check_digit(self, digit)
    return 0 <= digit and digit < self.BASE
end

---@param self  BigInt
---@param digit integer
local function push_msd(self, digit)
    if not check_digit(self, digit) then
        return false
    end
    local i = self:length() + 1
    self:write_at(i, digit)
    self:set_length(i)
    return true
end

---@param value integer
function BigInt:set_integer(value)
    if value < 0 then
        value = math.abs(value)
        self:set_negative(true)
    else
        self:set_negative(false)
    end

    value = math.floor(value)
    while value ~= 0 do
        local next  = math.floor(value / self.BASE)
        local digit = math.floor(value % self.BASE)
        push_msd(self, digit)
        value = next
    end
    return self
end

---@param input string
function BigInt:set_string(input)
    self:set_negative(false)
    for char in input:reverse():gmatch(".") do
        if char == '-' then
            self:set_negative(not self:is_negative())
        end
        -- Think of this like doing `char` manipulation in C
        if tonumber(char) and '0' <= char and char <= '9' then
            push_msd(self, char - '0')
        end
    end
    return self
end

---Do a deep copy of `other` into `self`.
---@param other BigInt
function BigInt:set_bigint(other)
    if self == other then
        return self
    end
    self:resize(other:length())
    for i, v in other:iter_lsd() do
        self:write_at(i, v)
    end
    self:set_negative(other:is_negative())
    return self
end

function BigInt:clone()
    return BigInt():set_bigint(self)
end

--- 1}}} -----------------------------------------------------------------------

--- MUTATION -------------------------------------------------------------- {{{1

-- This is here only because it's easier to grep than the regex `/m_length =/`.
---@param len integer
function BigInt:set_length(len)
    self.m_length = len
end

---@param cap integer
function BigInt:set_capacity(cap)
    self.m_capacity = cap
end

-- Same idea as `set_length`.
---@param neg boolean
function BigInt:set_negative(neg)
    self.m_negative = neg
end

---@param index integer
function BigInt:read_at(index)
    return self.m_digits[index] or 0
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

---@param index integer
---@param digit integer
function BigInt:write_at(index, digit)
    if 1 <= index and check_digit(self, digit) then
        if index > self:capacity() then
            self:resize(find_next_power_of(2, index))
        end
        if index > self:length() then
            self:set_length(index)
        end
        self.m_digits[index] = digit
        return true
    end
    return false
end

---@param start?    integer
---@param stop?     integer
function BigInt:clear(start, stop)
    for i = start or 1, stop or self:length(), 1 do
        self.m_digits[i] = 0
    end
end

-- Equivalent to realloc'ing the array then memset'ing it to 0 in C.
function BigInt:resize(newcap)
    local oldcap = self:capacity()
    if newcap <= oldcap then
        return
    end
    self:clear(self:length() + 1, newcap)
    self:set_capacity(newcap)
end

--- 1}}} -----------------------------------------------------------------------

---@param key any
---@see   Class.__index
function BigInt:__index(key)
    if type(key) == "number" then
        return self.m_digits[key] or 0
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
    return "BigInt: " .. table.concat(t)
end

function BigInt:length()
    return self.m_length
end

function BigInt:capacity()
    return self.m_capacity
end

function BigInt:is_negative()
    return self.m_negative
end

--- ITERATORS ------------------------------------------------------------- {{{1

---@param self BigInt
---@param index integer
local function check_index(self, index)
    return 1 <= index and index <= self:length()
end

---@param index integer
function BigInt:iterfn_lsd(index)
    index = index + 1
    if check_index(self, index) then
        return index, self.m_digits[index]
    end
end

---@param index integer
function BigInt:iterfn_msd(index)
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
    return self.iterfn_lsd, self, start - 1
end

-- Iterate starting from the most significant digits going to least significant.
---@param start? integer
function BigInt:iter_msd(start)
    start = start or self:length()
    return self.iterfn_msd, self, start + 1
end

--- 1}}} -----------------------------------------------------------------------

--- OPERATORS ------------------------------------------------------------- {{{1

function BigInt:abs()
    self:set_negative(false)
    return self
end

-- Note that this is a toggle!
function BigInt:unm()
    self:set_negative(not self:is_negative())
    return self
end

---@param x BigInt
function BigInt.__unm(x)
    return x:clone():unm()
end

--- ARITHMETIC ------------------------------------------------------------ {{{2

---@param x BigInt
---@param y BigInt
local function pick_greater_length(x, y)
    local xlen, ylen = x:length(), y:length()
    return (xlen > ylen) and xlen or ylen
end

---@param x BigInt
---@param y BigInt
local function pick_lesser_length(x, y)
    local xlen, ylen = x:length(), y:length()
    return (xlen < ylen) and xlen or ylen
end

---@param self BigInt
local function pop_msd(self)
    local len = self:length()
    if len <= 0 then
        return 0
    end
    local digit = self:read_at(len)
    -- Don't pop so we always have at least 0 as our sole digit.
    if len == 1 and digit == 0 then
        return 0
    end
    self:set_length(len - 1)
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
    local xneg, yneg = x:is_negative(), y:is_negative()
    if xneg ~= yneg then
        if xneg then
            return self:sub(x:abs(), y):unm()
        else
            return self:sub(x, y:abs())
        end
    end

    -- Since 0 is not negative here, we assume that addition of 2 negatives
    -- will always result in a negative.
    self:set_negative(xneg and yneg)

    -- Include 1 more for the very last carry if it extends our length.
    local len   = pick_greater_length(x, y) + 1
    local carry = 0
    self:resize(len)
    self:set_length(len)
    for i in self:iter_lsd() do
        local sum = x:read_at(i) + y:read_at(i) + carry
        if (sum >= self.BASE) then
            carry = math.floor(sum / self.BASE)
            sum   = math.floor(sum % self.BASE)
        else
            carry = 0
        end
        self:write_at(i, sum)
    end
    trim_zeroes_msd(self)
    return self
end

---@param self  BigInt
---@param index integer
local function borrowed(self, index)
    for i in self:iter_lsd(index + 1) do
        local digit  = self:read_at(i)
        local borrow = (digit == 0) and self.BASE - 1 or digit - 1
        self:write_at(i, borrow)
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
    self:set_length(1)
    self:set_negative(false)
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
    local xneg, yneg = x:is_negative(), y:is_negative()
    if xneg ~= yneg then
        if xneg then
            self:add(x:abs(), y)
            return self:unm()
        else
            self:add(x, y:abs())
            return self
        end
    end
    -- Note how we flip the operands' order as well.
    if xneg and yneg then
        x, y = y:abs(), x:abs()
    end

    -- Small minuend minus large subtrahend is always negative.
    local adjusted = x:__lt(y)
    if adjusted then
        x, y = y, x
    end

    local len = pick_lesser_length(x, y)
    -- Copy minuend's digits as we will need to modify them when borrowing.
    self:set_bigint(x)
    self:set_length(len)
    self:set_negative(adjusted)

    for i in self:iter_lsd() do
        local minuend, subtrahend = x:read_at(i), y:read_at(i)
        if (minuend < subtrahend) then
            if borrowed(self, i) then
                minuend = minuend + self.BASE
            else
                minuend, subtrahend = subtrahend, minuend
                self:set_negative(true)
            end
        end
        self:write_at(i, minuend - subtrahend)
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

---Same length and same sign?
---@param x BigInt
---@param y BigInt
local function quick_eq(x, y)
    return x:length() == y:length() and x:is_negative() == y:is_negative()
end

---@param x BigInt
---@param y BigInt
local function quick_lt(x, y)
    local xneg, yneg = x:is_negative(), y:is_negative()
    if xneg ~= yneg then
        -- Since both are different, if x is negative then y is positive.
        -- Negatives are always less than positives.
        return xneg
    end
    -- If same signs and same lengths both paths will return false either way.
    -- E.g. -11 > -222, although -11 has only 2 digits it is higher up the
    -- number line than -222.
    local xlen, ylen = x:length(), y:length()
    if xneg then
        return xlen > ylen
    else
        return xlen < ylen
    end
end

---@param x BigInt
---@param y BigInt
function BigInt.__eq(x, y)
    if not quick_eq(x, y) then
        return false
    end
    for i, v in x:iter_msd() do
        if v ~= y:read_at(i) then
            return false
        end
    end
    return true
end

---@param x BigInt
---@param y BigInt
function BigInt.__lt(x, y)
    -- Different length and/or different sign?
    if not quick_eq(x, y) then
        return quick_lt(x, y)
    end
    -- Always check most significant digits first. -E.g. in 102 < 201,
    -- if we did lsd first we would compare 3 > 1 first.
    for i, v in x:iter_msd() do
        if v > y:read_at(i) then
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
-- x, y, z = BigInt(4), BigInt(5), BigInt()

-- print("===BOTH POSITIVE===")
-- print("   4 - 5 =", 4 - 5, z:sub(x, y))
-- print("   5 - 4 =", 5 - 4, z:sub(y, x))

-- print("===BOTH NEGATIVE===")
-- x:unm(); y:unm()
-- print("(-4) - (-5)", (-4) - (-5), z:sub(x, y))
-- print("(-5) - (-4)", (-5) - (-4), z:sub(y, x))

-- print("===ONE POSITIVE, ONE NEGATIVE (1)===")
-- y:abs()
-- print("(-4) - 5    =", (-4) - 5, z:sub(x, y))
-- print("   5 - (-4) =", 5 - (-4), z:sub(y, x))

-- print("===ONE POSITIVE, ONE NEGATIVE (2)===")
-- x:abs()
-- y:unm()
-- print("   4 - (-5) =", 4 - (-5), z:sub(x, y))
-- print("(-5) - 4    =", (-5) - 4, z:sub(y, x))

-- x, y = BigInt(11), BigInt(77)

-- print("===11 AND 77===")
-- print("   11 - 77 =", 11 - 77,  z:sub(x, y))
-- print("(-66) - 11 =", (-66) - 11, z:sub(z, x))
-- print("(-77) + 11 =", (-77) + 11, z:add(z, x))
-- print("(-66) + 77 =", (-66) + 77, z:add(z, y))
-- print("   11 - 77 =", 11 - 77,    z:sub(z, y))
-- print("(-66) - 77 =", (-66) - 77, z:sub(z, y))

function print_keys(t)
    for k in pairs(t) do
        print(k, type(k))
    end
end

return BigInt
