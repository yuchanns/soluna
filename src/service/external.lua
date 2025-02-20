local ltask = require "ltask"
local message = require "soluna.appmessage"
local render = require "soluna.render"

local STATE

local S = {}

ltask.send(1, "external_forward", ltask.self(), "external")

local command = {}

function command.frame()
	if STATE then
		render.commit(STATE)
	end
end

function command.cleanup()
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
	assert(STATE == nil)
	STATE = render.init(arg.width, arg.height)
end

return S
