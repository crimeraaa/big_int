local Class = require "class"

---Think of this like `enum class` in C++.
local Enum

local format = string.format

-- Hack to avoid infinite recursion with the `__tostring()` metamethod.
local function rawtostring(t)
    local mt = getmetatable(t) -- Save the current metatable
    setmetatable(t, nil)       -- Then remove it
    local ret = tostring(t)    -- Call tostring without invoking any metamethods
    setmetatable(t, mt)        -- Return the metatable
    return ret
end

---@alias Enum.Key string|integer

---@class Enum : Class
---@field m_enums  {[Enum.Key]: Enum.Value} Map names, values to unique enums.
---@field m_names  {[Enum.Value]: string} May unique enums back to names.
---@field m_string string
---
---@field [Enum.Key]   Enum.Value
---@field [Enum.Value] string
---
---@overload fun(keys: string[]): Enum
---@param self Enum     Created and passed by `__call()` in `Class()`.
---@param keys string[] Array of string names for your enums.
Enum = Class(function(self, keys)
    self.m_enums = {}
    self.m_names = {}
    self.m_string = rawtostring(self):gsub("table", "Enum")
    for i, key in ipairs(keys) do
        local enum = Enum.Value(key, i, self)
        self.m_enums[key]  = enum
        self.m_enums[i]    = enum
        self.m_names[enum] = key
    end
end)

local is_enum_value = Enum.Value.is_instance

function Enum:__index(key)
    local val ---@type Enum.Value|string|nil
    if type(key) == "string" or type(key) == "number" then
        val = self.m_enums[key]
    elseif is_enum_value(key) then
        val = self.m_names[key]
    end
    if not val then
        error(format("Unknown Enum key '%s'", tostring(key)))
    end
    return val
end

function Enum:__tostring()
    return self.m_string
end

---@class Enum.Value : Class
---@field m_value  integer
---@field m_parent Enum     Pointer to parent enum type to ensure correctness.
---@field m_string string
---
---@overload fun(key: string, value: integer, parent: Enum): Enum.Value
---@param self   Enum.Value
---@param key    string
---@param value  integer
---@param parent Enum
Enum.Value = Class(function(self, key, value, parent)
    self.m_value  = value
    self.m_parent = parent
    self.m_string = format("Enum[%i]: %s", value, key)
end)

function Enum.Value:value()
    return self.m_value
end

local function throw_error(action, x, y)
    error(format("Cannot %s '%s' and '%s'", action, tostring(x), tostring(y)))
end

---@private
---@param x Enum.Value|integer
---@param y Enum.Value|integer
function Enum.Value.check_arithmetic(x, y)
    if is_enum_value(x) == is_enum_value(y) then
        throw_error("add/subtract", x, y)
    end
    local x_num, y_num = tonumber(x), tonumber(y)
    local sum, enum
    if x_num then
        sum  = x_num + y:value()
        enum = y.m_parent[sum]
    else
        sum  = x:value() + y_num
        enum = x.m_parent[sum]
    end
    return sum, enum
end

---@private
---@param x Enum.Value
---@param y Enum.Value
function Enum.Value.check_comparison(x, y)
    if is_enum_value(x) and is_enum_value(y) and x.m_parent == y.m_parent then
        return true
    end
    throw_error("compare", x, y)
end

---@param y Enum.Value|integer
function Enum.Value:__add(y)
    local sum, enum = self:check_arithmetic(y)
    if not enum then
        error(format("Invalid result Enum '%i'", sum))
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
    return x:value() == y:value()
end

---@param x Enum.Value
---@param y Enum.Value
local function quick_lt(x, y)
    return x:value() < y:value()
end

function Enum.Value:__eq(y)
    if not self:check_comparison(y) then
        return false
    end
    return quick_eq(self, y)
end

function Enum.Value:__lt(y)
    if not self:check_comparison(y) then
        return false
    end
    return not quick_eq(self, y) and quick_lt(self, y)
end

function Enum.Value:__le(y)
    if not self:check_comparison(y) then
        return false
    end
    return quick_eq(self, y) or quick_lt(self, y)
end

function Enum.Value:__tostring()
    return self.m_string
end

return Enum
