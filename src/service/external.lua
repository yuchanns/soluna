local ltask = require "ltask"
local message = require "soluna.appmessage"

local S = {}

ltask.send(1, "external_forward", ltask.self(), "external")

local command = {}

function command.frame()
end

function command.cleanup()
	ltask.send(1, "quit_ltask")
end

local function dispatch(type, ...)
	local f =command[type]
	if f then
		f(...)
	else
		print(type, ...)
	end
end

function S.external(p)
	dispatch(message.unpack(p))
end

return S
