local ltask = require "ltask"
local app = require "soluna.app"

local S = {}

local render = ltask.uniqueservice "render"
local gamepad = ltask.uniqueservice "gamepad"

ltask.send(1, "external_forward", ltask.self(), "external")

local command = {}

function command.frame(_, _, count)
	ltask.send(gamepad, "update")
	ltask.call(render, "frame", count)
	app.nextframe()
end

function command.cleanup()
	app.frameready(false)
	app.nextframe()
	ltask.call(render, "quit")
	ltask.send(1, "quit_ltask")
end

local listener = {}
local message_filter = {}

function message_filter.mouse_move(x, y)
	return x, y
end

function S.listen(addr, msg)
	local queue = listener[msg]
	if queue then
		queue[#queue+1] = addr
	else
		listener[msg] = { addr }
	end
end

local function dispatch(type, ...)
	local q = listener[type]
	if q then
		local f = message_filter[type]
		if f then
			for i = 1, #q do
				local addr = q[i]
				ltask.send(addr, "message", type, f(...))
			end
		else
			for i = 1, #q do
				local addr = q[i]
				ltask.send(addr, "message", type, ...)
			end
		end
	end
	local f = command[type]
	if f then
		f(...)
	else
--		todo:	
--		print(type, ...)
	end
end

function S.external(p)
	dispatch(app.unpackmessage(p))
end

function S.update()
	local change = device.update()
	if change then
		for addr, msg in pairs(listener) do
			ltask.send(addr, msg)
		end
	end
end

function S.init(arg)
	ltask.call(render, "init", arg)
	app.frameready(true)
end

return S
