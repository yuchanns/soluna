local ltask = require "ltask"

local arg = ...

ltask.spawn "external"

local filename = arg[1]

if filename then
	dofile(filename)
end

