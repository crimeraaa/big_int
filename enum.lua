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

---@class Enum : Class
---@field m_enums  {[string|integer]: Enum.Value} Map names, values to unique enums.
---@field m_names  {[Enum.Value]: string} May unique enums back to names.
---@field m_string string
---
---@field [string|integer] Enum.Value
---@field [Enum.Value]     string
---
---@overload fun(keys: string[]): Enum
---@param inst Enum Created and passed by `__call()` in `Class()`.
---@param keys string[] Array of string names for your enums.
Enum = Class(function(inst, keys)
    inst.m_enums = {}
    inst.m_names = {}
    inst.m_string = rawtostring(inst):gsub("table", "Enum")
    for i, key in ipairs(keys) do
        local enum = Enum.Value(key, i, inst)
        inst.m_enums[key]  = enum
        inst.m_enums[i]    = enum
        inst.m_names[enum] = key
    end
end)

function Enum:__index(key)
    local val ---@type Enum.Value|string|nil
    if type(key) == "string" or type(key) == "number" then
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

---@class Enum.Value : Class
---@field m_value  integer
---@field m_parent Enum     Pointer to parent enum type to ensure correctness.
---@field m_string string
---
---@overload fun(key: string, value: integer, parent: Enum): Enum.Value
---@param inst   Enum.Value
---@param key    string
---@param value  integer
---@param parent Enum
Enum.Value = Class(function(inst, key, value, parent)
    inst.m_value  = value
    inst.m_parent = parent
    inst.m_string = string.format("Enum[%i]: %s", value, key)
end)

function Enum.Value:value()
    return self.m_value
end

local function throw_error(action, x, y)
    error(string.format("Cannot %s '%s' and '%s'", action, tostring(x), tostring(y)))
end

---@private
---@param x Enum.Value|integer
---@param y Enum.Value|integer
function Enum.Value.check_arithmetic(x, y)
    if Enum.Value.is_instance(x) == Enum.Value.is_instance(y) then
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
    local is_inst = Enum.Value.is_instance
    if is_inst(x) and is_inst(y) and x.m_parent == y.m_parent then
        return true
    end
    throw_error("compare", x, y)
end

---@param y Enum.Value|integer
function Enum.Value:__add(y)
    local sum, enum = self:check_arithmetic(y)
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
