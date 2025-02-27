local ltask = require "ltask"
local message = require "soluna.appmessage"

local S = {}

local render = ltask.uniqueservice "render"

ltask.send(1, "external_forward", ltask.self(), "external")

local command = {}

function command.frame(_, _, count)
	ltask.send(render, "frame", count)
end

function command.cleanup()
	ltask.call(render, "quit")
	ltask.send(1, "quit_ltask")
end

local function dispatch(type, ...)
	local f =command[type]
	if f then
		f(...)
	else
--		todo:	
--		print(type, ...)
	end
end

function S.external(p)
	dispatch(message.unpack(p))
end

function S.init(arg)
	ltask.call(render, "init", arg)
end

return S
