local Class = require "class"

---Think of this like `enum class` in C++.
local Enum

-- Hack to avoid infinite recursion with the `__tostring()` metamethod.
local function rawtostring(t)
    local mt = getmetatable(t) -- Save the current metatable
    setmetatable(t, nil)       -- Then remove it
    local ret = tostring(t)    -- Call tostring without invoking any metamethods
    setmetatable(t, mt)        -- Return the metatable
    return ret
end

---@param self Enum     Created inside `__call()` inside of `Class()`.
---@param keys string[] Array of string names for your enums.
local function enum_ctor(self, keys)
    self.m_enums = {}
    self.m_names = {}
    self.m_string = rawtostring(self):gsub("table", "Enum")
    for i, key in ipairs(keys) do
        local enum = Enum.Value(key, i, self)
        self.m_enums[key]  = enum
        self.m_enums[i]    = enum
        self.m_names[enum] = key
    end
end

---@class Enum: Class
---@field m_enums  {[string|integer]: Enum.Value}
---@field m_names  {[Enum.Value]: string}
---@field m_string string
---@field [string] Enum.Value Map enum names to enum values.
---@field [Enum.Value] string Map enum values back to enum names.
---
---@overload fun(keys: string[]): Enum
Enum = Class(enum_ctor)

function Enum:__index(key)
    local val ---@type Enum.Value|string|nil
    if type(key) == "string" then
        val = self.m_enums[key]
    elseif Enum.Value.is_instance(key) then
        val = self.m_names[key]
    end
    if not val then
        error(string.format("Unknown Enum key '%s'", tostring(key)))
    end
    return val
end

function Enum:__tostring()
    return self.m_string
end

---@param self   Enum.Value
---@param key    string
---@param value  integer
---@param parent Enum
local function value_ctor(self, key, value, parent)
    self.m_value  = value
    self.m_parent = parent
    self.m_string = string.format("Enum[%i]: %s", value, key)
end

---@class Enum.Value : Class
---@field m_value  integer
---@field m_parent Enum     Pointer to parent enum type to ensure correctness.
---@field m_string string
---
---@overload fun(key: string, value: integer, parent: Enum): Enum.Value
Enum.Value = Class(value_ctor)

local function check_types(x, y)
    local fn = Enum.Value.is_instance
    return fn(x), fn(y)
end

local function throw_error(action, x, y)
    error(string.format("Cannot %s '%s' and '%s'", action, tostring(x), tostring(y)))
end

---@param x Enum.Value|integer
---@param y Enum.Value|integer
local function check_arithmetic(x, y)
    local x_is, y_is = check_types(x, y)
    if x_is == y_is then
        throw_error("add/subtract", x, y)
    end
    local x_num, y_num = tonumber(x), tonumber(y)
    local sum, enum
    if x_num then
        sum  = x_num + y.m_value
        enum = y.m_parent.m_enums[sum]
    else
        sum  = x.m_value + y_num
        enum = x.m_parent.m_enums[sum]
    end
    return sum, enum
end

---@param x Enum.Value
---@param y Enum.Value
local function check_comparison(x, y)
    local x_is, y_is = check_types(x, y)
    if x_is and y_is and x.m_parent == y.m_parent then
        return true
    end
    throw_error("compare", x, y)
end

---@param x Enum.Value|integer
---@param y Enum.Value|integer
function Enum.Value.__add(x, y)
    local sum, enum = check_arithmetic(x, y)
    if not enum then
        error(string.format("Invalid result Enum '%i'", sum))
    end
    return enum
end

---@param x Enum.Value|integer
---@param y Enum.Value|integer
function Enum.Value.__sub(x, y)
    local x_num, y_num = tonumber(x), tonumber(y)
    if y_num then
        return x:__add(-y_num)
    else
        return y:__add(-x_num)
    end
end

---@param x Enum.Value
---@param y Enum.Value
local function quick_eq(x, y)
    return x.m_value == y.m_value
end

---@param x Enum.Value
---@param y Enum.Value
local function quick_lt(x, y)
    return x.m_value < y.m_value
end

function Enum.Value.__eq(x, y)
    return check_comparison(x, y) and quick_eq(x, y)
end

function Enum.Value.__lt(x, y)
    return check_comparison(x, y) and (not quick_eq(x, y) and quick_lt(x, y))
end

function Enum.Value.__le(x, y)
    return check_comparison(x, y) and (quick_eq(x, y) or quick_lt(x, y))
end

function Enum.Value:__tostring()
    return self.m_string
end

Day = Enum {
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
    "Sunday",
}

Month = Enum {
    "January",
    "Febraury",
    "March",
    "April",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December",
}

return Enum
