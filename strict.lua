local format  = string.format
local getinfo = debug.getinfo

---stack level 0: debug.getinfo() itself
---stack level 1: caller of debug.getinfo, such as 'what()'
---stack level 2: caller of what(), such as 'strict()'
local function what()
    local info = getinfo(3, 'S')
    return info and info.what or 'C'
end

---@param env table
local function strict(env)
    local declared  = {}
    local metatable = {} ---@type metatable
    
    function metatable.__index(_, key)
        -- This key existed beforehand and just wasn't marked as declared?
        local value = env[key]
        if value ~= nil then
            declared[key] = true
        elseif not declared[key] then
            -- C variables like _PROMPT are unavailable to us but valid.
            if what() ~= 'C' then
                error(format(
                    "Attempt to read undeclared variable '%s'",
                    tostring(key)
                ))
            end
        end
        return value
    end

    function metatable.__newindex(_, key, value)
        local x = env[key]
        if x == nil and not declared[key] then
            local w = what()
            if w ~= "main" and w ~= 'C' then
                error(format(
                    "Attempt to write to undeclared variable '%s'",
                    tostring(key)
                ))
                return
            end
        end
        declared[key] = true
        env[key] = value
    end
    -- We set the metatable of a new table so we can still access `env` normally.
    return setmetatable({}, metatable)    
end

return strict
