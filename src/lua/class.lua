---@class Class
---@field is_instance fun(inst: Class): boolean
---@field __index     Class

local type = type
local getmetatable, setmetatable = getmetatable, setmetatable

---See: https://github.com/penguin0616/dst_gamescripts/blob/master/class.lua
---@param constructor? fun(inst: Class, ...)
local function new_class(constructor)
    ---@type Class
    ---@diagnostic disable-next-line: missing-fields
    local class_def = {}

    ---This will also be the metatable to allow objects to look up methods from
    ---the class definition.
    class_def.__index = class_def
    
    -- Expose a constructor callable by the form <class-name>(<args...>)
    ---@type metatable
    local class_mt = {
        __call = function(_, ...)
            ---@type Class
            local inst = setmetatable({}, class_def)
            if constructor then
                constructor(inst, ...)
            end
            return inst
        end
    }
    
    class_def.is_instance = function(obj)
        return type(obj) == "table" and getmetatable(obj) == class_def
    end
    
    setmetatable(class_def, class_mt)
    return class_def
end

return new_class
