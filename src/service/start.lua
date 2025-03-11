local ltask = require "ltask"
local spritemgr = require "soluna.spritemgr"
local file = require "soluna.file"
local soluna = require "soluna"

local arg, app = ...

local settings = ltask.uniqueservice "settings"
ltask.call(settings, "init", arg)

local external = ltask.spawn "external"
ltask.call(external, "init", app)

local entry = soluna.settings().entry

local source = entry and file.loadstring(entry)
if not source then
	error ("Can't load entry " .. tostring(entry))
end
local f = assert(load(source, "@"..entry, "t"))
local batch = spritemgr.newbatch()
local callback = f(batch) or { frame = function() end }

local render = ltask.uniqueservice "render"
local batch_id = ltask.call(render, "register_batch", ltask.self())
local quit = false

local function mainloop()
	soluna.gamepad_init()
	local count = 0
	while not quit do
		count = count + 1
		batch:reset()
		callback.frame(count)
		ltask.call(render, "submit_batch", batch_id, batch:ptr())
	end
	if quit ~= "finish" then
		ltask.wakeup(quit)
	end
	quit = "finish"
end

ltask.fork(mainloop)

local S = {}

function S.quit()
	if not quit then
		quit = true
	elseif quit ~= "finish" then
		quit = {}
		ltask.wait(quit)
	end
end

return S
