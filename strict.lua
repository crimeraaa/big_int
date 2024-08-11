local format  = string.format
local getinfo = debug.getinfo

-- For C functions, we assume they know what they're doing so don't bother them.
--
-- level 0: debug.getinfo()
-- level 1: is_cfunction()  (caller of level 0)
-- level 2: mt.__index()    (caller of level 1)
-- level 3: =stdin,*.lua    (caller of level 2)
--
-- See: https://www.lua.org/source/5.1/lua.c.html#get_prompt
local function is_cfunction()
    local info = getinfo(3, 'S')
    return info and info.what == 'C'
end

---@param env      table
---@param ignored? {[string]: true}
local function restrict(env, ignored)
    ignored = ignored or {}

    local function indexfn(_, key)
        if ignored[key] or is_cfunction() then
            return nil
        end
        error(format("Attempt to read undeclared variable '%s'", tostring(key)))
    end

    return setmetatable(env, {__index = indexfn})
end

---@param env table
local function unrestrict(env)
    return setmetatable(env, nil)
end

return setmetatable({
    restrict   = restrict,
    unrestrict = unrestrict,
}, {
    ---@param env       table
    ---@param ignored? {[string]: true}
    __call = function(_, env, ignored)
        return restrict(env, ignored)
    end,
})
