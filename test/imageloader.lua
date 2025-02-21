local ltask = require "ltask"

local loader = ltask.uniqueservice "loader"

local id = ltask.call(loader, "load", "asset/avatar.png", -0.5, -1)
assert(id)
ltask.call(loader, "write", id, "test.png")