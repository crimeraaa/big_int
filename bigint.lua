require "class"

--- UTILITY --------------------------------------------------------------- {{{1

---@param self BigInt
---@param index integer
local function check_index(self, index)
    return 1 <= index and index <= self.m_length
end

local function check_digit(digit)
    return 0 <= digit and digit < BigInt.BASE
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
    if 1 <= index and check_digit(digit) then
        if index > self.m_capacity then
            resize_buffer(self, find_next_power_of(2, index))
        end
        if index > self:len() then
            self.m_length = index
        end
        self.m_digits[index] = digit
        return true
    end
    return false
end

---@param self  BigInt
---@param digit integer
local function push_msd(self, digit)
    if not check_digit(digit) then
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
    local digit   = self.m_digits[len]
    self.m_length = len - 1
    return digit
end

---@param self BigInt
local function trim_zeroes_msd(self)
    -- NOTE: Dangerous to modify invariant during a loop!
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
        self.m_negative = true
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
            self.m_negative = not self:is_negative()
        end
        -- Think of this like doing `char` manipulation in C
        if '0' <= char and char <= '9' then
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
end

--- 1}}} -----------------------------------------------------------------------

---@param inst   BigInt
---@param value? string|integer|BigInt
local function ctor(inst, value)
    inst.m_digits   = {}
    inst.m_length   = 0
    inst.m_capacity = 0
    inst.m_negative = false
    if BigInt.is_instance(value) then
        set_bigint(inst, value) 
    elseif type(value) == "number" then
        set_integer(inst, value)
    elseif type(value) == "string" then
        set_string(inst, value)
    end
end

---@class BigInt : Class
---@operator add:BigInt
---@operator sub:BigInt
---@operator unm:BigInt
---@operator call:BigInt
---@field m_digits   integer[]  Stored in a little-endian fashion.
---@field m_length   integer    Number of active values in `digits`.
---@field m_capacity integer    Number of total values in `digits`.
---@field m_negative boolean    For simplicity even 0 is positive.
BigInt = Class(ctor)
BigInt.BASE = 10

---@param inst BigInt
---@param key any
function BigInt.__index(inst, key)
    if type(key) == "number" then
        return read_at(inst, key)
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
    self.m_negative = false
    return self
end

-- Note that this is a toggle!
function BigInt:unm()
    self.m_negative = not self:is_negative()
    return self
end

---@param inst BigInt
function BigInt.__unm(inst)
    local copy = BigInt()
    set_bigint(copy, inst)
    return copy:unm()
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

---@param x BigInt
---@param y BigInt
function BigInt:add(x, y)
    -- (-x) + y == y - x
    -- x + (-y) == x - y
    -- Create a copy in case 'x' or 'y' is the same as self, so we don't want to
    -- mutate both at the same time.
    if x:is_negative() ~= y:is_negative() then
        local tmp_x = not x:is_negative() and x or y
        local tmp_y = x:is_negative() and x or y
        return self:sub(tmp_x, BigInt(tmp_y):abs())
    end

    -- (-x) + (-y) == -(x + y)
    -- Since 0 is not negative here, we assume that addition of 2 negatives
    -- will always result in a negative.
    self.m_negative = x:is_negative() and y:is_negative()

    local len   = pick_greater_length(x, y) + 1
    local carry = 0
    resize_buffer(self, len)
    self.m_length = len
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
        local borrow = ((digit == 0) and self.BASE or digit) - 1
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
    self.m_length   = 1
    self.m_negative = false
    return self
end

---@param x BigInt
---@param y BigInt
function BigInt:sub(x, y)
    -- (-x) - (-y) == (-x) + y == y - x
    -- Note how we flip the operands' order as well.
    -- We create copies in case either 'x' or 'y' points to 'self'.
    if x:is_negative() and y:is_negative() then
        x, y = BigInt(y):abs(), BigInt(x):abs()
    end
    -- Adjust for smaller minuend, ensure clean slate
    if x:__lt(y) then
        if x:is_negative() then
            -- TODO: How to handle -77 + 11 ...
        end
        x, y = y, x
        self.m_negative = true
    else
        self.m_negative = false
    end

    local len = pick_lesser_length(x, y)
    -- Copy minuend's digits as we will need to modify them when borrowing.
    set_bigint(self, x)
    self.m_length = len
    for i in self:iter_lsd() do
        local minuend, subtrahend = read_at(x, i), read_at(y, i)
        if (minuend < subtrahend) then
            if borrowed(self, i) then
                minuend = minuend + self.BASE
            else
                minuend, subtrahend = subtrahend, minuend
                self.m_negative = true
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
res = BigInt()
a   = BigInt(11)
b   = BigInt(22)
c   = BigInt(44)
d   = BigInt(88)

function print_keys(t)
    for k in pairs(t) do
        print(k, type(k))
    end
end
