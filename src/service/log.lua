local ltask = require "ltask"
local writelog = require "soluna.log"

local S = {}

local sokol_log = writelog.sokol
local ltask_log = writelog.ltask

local function writelog()
	local flush
	while true do
		local ti, id, msg, sz = ltask.poplog()
		if ti == nil then
			if flush then
				io.flush()
			end
			break
		end
		if id == 0 then
			sokol_log(ti, msg)
		else
			ltask_log(ti, ltask.unpack_remove(msg, sz))
		end
		flush = true
	end
end

ltask.fork(function()
	while true do
		writelog()
		ltask.sleep(20)
	end
end)

function S.quit()
	writelog()
end

return S
