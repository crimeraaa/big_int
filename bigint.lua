---@class BigInt
---@field m_digits   integer[]  Stored in a little-endian fashion.
---@field m_length   integer    Used to control our access to `digits`.
---@field m_negative boolean
BigInt = {
    base = 10
}

---@type metatable
local mt = {}

--- UTILITY --------------------------------------------------------------- {{{1

---@param self BigInt
---@param index integer
local function check_index(self, index)
    return 1 <= index and index <= self.m_length
end

local function check_digit(digit)
    return 0 <= digit and digit < BigInt.base
end

--- ITERATORS ------------------------------------------------------------- {{{2

---@param self  BigInt  Also known as the 'invariant' state.
---@param index integer Also known as the 'control' variable.
local function iterfn(self, index)
    index = index + 1
    if check_index(self, index) then
        return index, self.m_digits[index]
    end
end

---@param self  BigInt
---@param index integer
local function riterfn(self, index)
    index = index - 1
    if check_index(self, index) then
        return index, self.m_digits[index]
    end
end

-- Iterate from right to left; least significant to most significant digits.
function BigInt:iter()
    -- iterator function, 'invariant' state, 'control' variable.
    return iterfn, self, 0
end

-- Iterate from left to right; most significant to least significant digits.
function BigInt:riter()
    return riterfn, self, self:length() + 1
end

--- 2}}} -----------------------------------------------------------------------

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
    for i = start or 1, stop or self:length(), 1 do
        self.m_digits[i] = 0
    end
end

-- Equivalent to realloc'ing the array then memset'ing it to 0 in C.
---@param self      BigInt
---@param newlen    integer
local function grow_buffer(self, newlen)
    local oldlen = self:length()
    if newlen <= oldlen then
        return
    end
    clear_buffer(self, oldlen, newlen)
    self.m_length = newlen
end

---@param self  BigInt
---@param index integer
---@param digit integer
local function write_at(self, index, digit)
    if 1 <= index and check_digit(digit) then
        if index > self:length() then
            grow_buffer(self, index)
        end
        self.m_digits[index] = digit
        return true
    end
    return false
end

---@param self  BigInt
---@param digit integer
local function push_left(self, digit)
    if not check_digit(digit) then
        return false
    end
    local i = self:length() + 1
    self.m_digits[i] = digit
    self.m_length = i
    return true
end

---@param self BigInt
local function pop_left(self)
    local len = self:length()
    if len <= 0 then
        return 0
    end
    local digit   = self.m_digits[len]
    self.m_length = len - 1
    return digit
end

---@param self BigInt
function trim_left(self)
    -- Can't use `self:riter()` because we modify the invariant `self` inside of
    -- the loop body.
    for i = self:length(), 1, -1 do
        -- No more leading 0's?
        if (self.m_digits[i] ~= 0) then
            break
        end
        pop_left(self)
    end
end

--- 1}}} -----------------------------------------------------------------------

---@param value? string|number
function BigInt:new(value)
    ---@type BigInt
    local inst      = setmetatable({}, mt)
    inst.m_digits   = {}
    inst.m_length   = 0
    inst.m_negative = false
    if type(value) == "number" then
        inst:set_integer(value)
    elseif type(value) == "string" then
        inst:set_string(value)
    end
    return inst
end

---@param self  BigInt
---@param value integer
function BigInt:set_integer(value)
    if value < 0 then
        value = math.abs(value)
        self.m_negative = true
    end
    value = math.floor(value)
    while value ~= 0 do
        local next  = math.floor(value / self.base)
        local digit = math.floor(value % self.base)
        push_left(self, digit)
        value = next
    end
end

---@param input string
function BigInt:set_string(input)
    input = input:reverse()
    for i = 1, input:len(), 1 do
        local char = input:sub(i, i)
        if char == '-' then
            self.m_negative = not self:is_negative()
        end
        -- Think of this like doing `char` manipulation in C
        if '0' <= char and char <= '9' then
            push_left(self, char - '0')
        end
    end
end

---@param other BigInt
function BigInt:set_bigint(other)
    grow_buffer(self, other:length())
    for i, v in other:iter() do
        write_at(self, i, v)
    end
end

function BigInt:to_string()
    local t, n = {}, 0
    if self:is_negative() then
        n = n + 1
        t[n] = '-'
    end
    -- Iterate in reverse.
    for _, v in self:riter() do
        n = n + 1
        t[n] = v
    end
    return "BigInt: " .. (n > 0 and table.concat(t) or "(empty)")
end

function BigInt:length()
    return self.m_length
end

function BigInt:is_negative()
    return self.m_negative
end

--- OPERATORS ------------------------------------------------------------- {{{1

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

---@param x BigInt
---@param y BigInt
function BigInt:add(x, y)
    local len   = pick_greater_length(x, y) + 1
    local carry = 0
    grow_buffer(self, len)
    for i = 1, len, 1 do
        local sum = x[i] + y[i] + carry
        if (sum >= self.base) then
            carry = math.floor(sum / self.base)
            sum   = math.floor(sum % self.base)
        else
            carry = 0
        end
        write_at(self, i, sum)
    end
    trim_left(self)
    return self
end

---@param self  BigInt
---@param index integer
local function borrowed(self, index)
    for i = index + 1, self:length(), 1 do
        local digit  = read_at(self, i)
        local borrow = ((digit == 0) and self.base or digit) - 1
        
        -- Assume that somewhere down the line is a nonzero.
        if digit ~= 0 then
            -- Borrowing caused this place to become zero?
            if borrow == 0 then
                pop_left(self)
            end
            return true
        end
    end 
    return false
end

---@param x BigInt
---@param y BigInt
function BigInt:sub(x, y)
    -- Adjust for smaller minuend, ensure clean slate
    if x < y then
        x, y = y, x
        self.m_negative = true
    else
        self.m_negative = false
    end

    local len = pick_lesser_length(x, y)
    -- Copy minuend's digits as we will need to modify them when borrowing.
    self:set_bigint(x)
    for i = 1, len, 1 do
        local mind, subt = read_at(x, i), read_at(y, i)
        if (mind < subt) then
            if borrowed(self, i) then
                mind = mind + self.base
            else
                mind, subt = subt, mind
                self.m_negative = true
            end
        end
        write_at(self, i, mind - subt)
    end
    return self
end

--- 2}}} -----------------------------------------------------------------------

--- RELATIONAL ------------------------------------------------------------ {{{2

---@param x BigInt
---@param y BigInt
local function maybe_equal(x, y)
    return x:length() == y:length() and x:is_negative() == y:is_negative()
end

---@param x BigInt
---@param y BigInt
local function maybe_less(x, y)
    local xneg, yneg = x:is_negative(), y:is_negative()
    local xlen, ylen = x:length(), y:length()
    if xneg ~= yneg then
        return xneg
    end
    if xlen ~= ylen then
        return xlen < ylen
    end
    return false
end

---@param x BigInt
function BigInt:eq(x)
    if not maybe_equal(self, x) then
        return false
    end
    for i, v in self:riter() do
        if v ~= x[i] then
            return false
        end
    end
    return true
end

---@param x BigInt
function BigInt:lt(x)
    if self:eq(x) then
        return false
    end
    if not maybe_equal(self, x) then
        return maybe_less(self, x)
    end
    for i, v in self:riter() do
        if v > x[i] then
            return false
        end
    end
    return true
end

---@param x BigInt
function BigInt:le(x)
    return self:eq(x) or self:lt(x)
end

---@param x BigInt
function BigInt:neq(x)
    return not self:eq(x)
end

---@param x BigInt
function BigInt:gt(x)
    return not self:le(x)
end

---@param x BigInt
function BigInt:ge(x)
    return not self:lt(x)
end

--- 2}}} -----------------------------------------------------------------------

--- 1}}} -----------------------------------------------------------------------

mt = {
    ---@param self BigInt
    ---@param key  any
    __index = function(self, key)
        if type(key) == "number" then
            return read_at(self, key)
        elseif type(key) == "string" then
            return BigInt[key]
        else
            return nil
        end
    end,
    __tostring = BigInt.to_string,
    __eq       = BigInt.eq,
    __lt       = BigInt.lt,
    __le       = BigInt.le,
}

BigInt = setmetatable(BigInt, {__call = BigInt.new})

--- TEST
a = BigInt()
b = BigInt(1)
c = BigInt(2)
d = BigInt(4)
e = BigInt(8)
