local ltask = require "ltask"
local file = require "soluna.file"
local spritemgr = require "soluna.spritemgr"
local soluna = require "soluna"
local soluna_app = require "soluna.app"
local event = require "soluna.event"

local message_unpack = soluna_app.unpackmessage

local args, ev = ...

local S = {}

local app = {}

function app.cleanup()
	ltask.send(1, "quit_ltask")
end

-- dummy frame
function app.frame(count)
	event.trigger(ev.frame)
end

-- external message from soluna host
function S.external(p)
	local what, arg1, arg2 = message_unpack(p)
	local f = app[what]
	if f then
		f(arg1, arg2)
	end
end

local function init(arg)
	if arg == nil then
		error "No command line args"
	end
	soluna.gamepad_init()
	local settings = ltask.uniqueservice "settings"
	ltask.call(settings, "init", arg)
	
	local render = ltask.uniqueservice "render"
	ltask.call(render, "init", arg.app)

	local entry = soluna.settings().entry
	local source = entry and file.load(entry)
	if not source then
		error ("Can't load entry " .. tostring(entry))
	end
	local f = assert(load(source, "@"..entry, "t"))
	-- todo: run entry (f)
	
	local batch = spritemgr.newbatch()
	local callback = f(batch)
	local frame_cb = callback.frame
	
	local messages = { "mouse_move", "mouse_button", "mouse_scroll", "mouse", "window_resize" }
	local avail = {}
	for _, v in ipairs(messages) do
		avail[v] = true
	end
	for k,v in pairs(callback) do
		if avail[k] then
			app[k] = v
		end
	end
	local batch_id = ltask.call(render, "register_batch", ltask.self())
	local async = ltask.async()

	local function frame(count)
		batch:reset()
		frame_cb(count)
		async:request(render, "submit_batch", batch_id, batch:ptr())
		async:request(render, "frame")
		async:wait()
	end
	
	function app.frame(count)
		local ok, err = pcall(frame, count)
		event.trigger(ev.frame)
		assert(ok, err)
	end
end

function S.quit()
	-- todo: cleanup
end

ltask.fork(function()
	ltask.call(1, "external_forward", ltask.self(), "external")
	event.trigger(ev.init)
	
	local ok , err = pcall(init, args)
	if not ok then
		ltask.log.error(err)
		soluna_app.quit()
	end
end)


return S
