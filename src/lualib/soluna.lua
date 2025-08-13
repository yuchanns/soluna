local ltask = require "ltask"
local app = require "soluna.app"

local soluna = {
	platform = app.platform
}

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

local settings
function soluna.settings()
	if settings == nil then
		local s = ltask.queryservice "settings"
		settings = ltask.call(s, "get")
	end
	return settings
end

function soluna.set_window_title(text)
	local window = ltask.uniqueservice "window"
	ltask.send(window, "set_title", text)
end

function soluna.gamedir(name)
	if name == nil then
		settings = settings and soluna.settings()
		name = settings.project or error "missing project name in settings"
	end
	if soluna.platform == "windows" then
		local lfs = require "soluna.lfs"
		local dir = lfs.personaldir() .. "\\My Games"
		lfs.mkdir(dir)
		dir = dir .. "\\" .. name
		lfs.mkdir(dir)
		return dir .. "\\"
	else
		error "TODO: support none windows"
	end
end

return soluna