local ltask = require "ltask"

local soluna = {}

function soluna.gamepad_init()
	local gamepad = require "soluna.gamepad"
	local state = {}
	soluna.gamepad = state
	local gs = ltask.uniqueservice "gamepad"
	local S = ltask.dispatch()
	
	function S._gamepad_update()
		gamepad.update(state)
	end

	ltask.call(gs, "register", ltask.self(), "_gamepad_update")
	
	return state
end

return soluna