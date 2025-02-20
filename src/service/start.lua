local ltask = require "ltask"

local arg, app = ...

local external = ltask.spawn "external"

ltask.send(external, "init", app)

local filename = arg[1]

if filename then
	dofile(filename)
end

