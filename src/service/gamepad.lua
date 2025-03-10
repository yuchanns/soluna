local ltask = require "ltask"
local device = require "soluna.gamepad.device"

local S = {}

local listener = {}

function S.register(addr, msg)
	listener[addr] = msg
	ltask.send(addr, msg)
end

function S.update()
	local change = device.update()
	if change then
		for addr, msg in pairs(listener) do
			ltask.send(addr, msg)
		end
	end
end

return S
