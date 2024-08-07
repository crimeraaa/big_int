---@class Class
---@field init        fun(inst: Class, ...)      Constructor function.
---@field is_instance fun(inst: Class): boolean
---
---@field __index     function|table
---
---@operator call: Class

---See: https://github.com/penguin0616/dst_gamescripts/blob/master/class.lua
---@param ctor?  fun(inst: Class, ...)
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
        if t.init then
            t.init(inst, ...)
        end
        return inst
    end
    
    c.init = ctor
    
    c.is_instance = function(obj)
        return type(obj) == "table" and getmetatable(obj) == c
    end
    
    setmetatable(c, mt)
    return c
end

-- ---@param inst Array
-- ---@param key  any
-- local function array__index(inst, key)
--     if type(key) == "number" then
--         return inst.m_data[key]
--     end
--     -- return nothing to signal that we should try a deeper metamethod
-- end

-- ---@param inst Array
-- local function array__ctor(inst)
--     setmetatable(inst, {__index = array__index})
--     inst.m_data   = {}
--     inst.m_length = 0
-- end

-- ---@class Array : Class
-- ---@field m_data   any[]
-- ---@field m_length integer
-- Array = Class(array__ctor)

-- function Array:length()
--     return self.m_length
-- end

return Class
