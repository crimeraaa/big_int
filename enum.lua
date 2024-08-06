local Class = require "class"

---Think of this like `enum class` in C++.
local Enum

---@param self Enum     Created inside `__call()` inside of `Class()`.
---@param keys string[] Array of string names for your enums.
local function ctor(self, keys)
    self.m_enums = {}
    self.m_names = {}
    for _, key in ipairs(keys) do
        local enum = {}
        self.m_enums[key] = enum
        self.m_names[enum] = key
    end
end

---@class Enum: Class
---@field m_enums  {[string]: table}
---@field m_names  {[table]: string}
---@field [string] table Map enum names to unique tables.
---@field [table] string Map enums to their names at time of construction.
---
---@overload fun(keys: string[]): Enum
Enum = Class(ctor)

function Enum:__index(key)
    local tp = type(key)
    if tp == "string" then
        return self.m_enums[key]
    elseif tp == "table" then
        return self.m_names[key]
    end
end

return Enum
