---@class Class
---@field is_instance fun(inst: Class): boolean
---
---@field __index     function|table

---See: https://github.com/penguin0616/dst_gamescripts/blob/master/class.lua
---@param ctor?  fun(inst: Class, ...)  Constructor function.
local function Class(ctor)
    ---@type Class
    ---@diagnostic disable-next-line: missing-fields
    local c = {}
    
    -- Allow objects to look up methods from the class definition.
    c.__index = c
    
    -- Expose a constructor callable by the form <class-name>(<args...>)
    ---@type metatable
    local mt = {}
    
    ---@param t   Class
    ---@param ... any
    mt.__call = function(t, ...)
        ---@type Class
        local inst = setmetatable({}, t)
        if ctor then
            ctor(inst, ...)
        end
        return inst
    end
    
    c.is_instance = function(obj)
        return type(obj) == "table" and getmetatable(obj) == c
    end
    
    setmetatable(c, mt)
    return c
end

return Class
