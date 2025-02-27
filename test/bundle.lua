local sb = require "soluna.spritebundle"

local filecache = setmetatable({ __missing = {} }, { __index = sb.loadimage })

print_r(sb.load(filecache, "asset/sprites.dl"))

local ltask = require "ltask"

local s = ltask.uniqueservice "loader"
ltask.call(s, "init", { max_sprite = 65536, texture_size = 1024 })

local b = ltask.call(s, "loadbundle", "asset/sprites.dl")
print_r(b)