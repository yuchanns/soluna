local ltask = require "ltask"
local render = require "soluna.render"
local image = require "soluna.image"
local setting = require "soluna.setting"

local barrier = {} ; do
	local thread
	local status
	function barrier.init(func, ...)
		status = "running"
		barrier.count = 0
		thread = ltask.fork(func, ...)
	end
	
	function barrier.trigger(count)
		barrier.count = count
		if status == "sleeping" then
			ltask.wakeup(thread)
		end
		status = "wakeup"
	end
	
	function barrier.wait()
		if status == "running" then
			status = "sleeping"
			ltask.wait()
		end
		status = "running"
	end
end

local batch = {} ; do
	local thread
	local submit_n = 0
	function batch.register(addr)
		local n = #batch + 1
		batch[n] = {
			source = addr
		}
		return n
	end
	function batch.wait()
		if submit_n ~= #batch then
			thread = ltask.current_token()
			ltask.wait()
		end
		submit_n = 0
	end
	function batch.submit(id, ptr, size)
		local q = batch[id]
		local token = ltask.current_token()
		local function func ()
			return token, ptr, size
		end
		if q[1] == nil then
			submit_n = submit_n + 1
			if thread and submit_n == #batch then
				ltask.wakeup(thread)
				thread = nil
			end
			q[1] = func
		else
			q[#q+1] = func
		end
		ltask.wait()
	end
	function batch.consume(id)
		local q = batch[id]
		local r = assert(q[1])
		local n = #q
		for i = 1, n - 1 do
			q[i] = q[i+1]
		end
		q[n] = nil
		return r()
	end
end

local function mainloop(STATE)
	local batch_size = setting.batch_size
	while true do
		-- todo: do not wait all batch commits
		local batch_n = #batch
		if batch_n > 0 then
			batch.wait()
			STATE.pass:begin()
				STATE.pipeline:apply()
				STATE.uniform:apply()
				
				for i = 1, batch_n do
					local co, ptr, size = batch.consume(i)
					local tex = STATE.drawbuffer:append(ptr, size)
					assert(tex == 0)
					ltask.wakeup(co)
				end
				local n= STATE.drawbuffer:submit(STATE.inst, batch_size, STATE.sprite, batch_size)
				STATE.srbuffer:update(STATE.srbuffer_mem:ptr())
				STATE.bindings:apply()
				render.draw(0, 4, n)
			STATE.pass:finish()
			render.submit()
		end
		barrier.wait()
	end
end

local S = {}

S.frame = assert(barrier.trigger)
S.register_batch = assert(batch.register)
S.submit_batch = assert(batch.submit)

function S.quit()
	local workers = {}
	for _, v in ipairs(batch) do
		workers[v.source] = true
	end
	for addr in pairs(workers) do
		ltask.call(addr, "quit")
	end
	for _, v in ipairs(batch) do
		for _, resp in ipairs(v) do
			ltask.wakeup((resp()))
		end
	end
	-- double check
	for addr in pairs(workers) do
		ltask.call(addr, "quit")
	end
end

function S.init(arg)
	local loader = ltask.uniqueservice "loader"

	local texture_size = setting.texture_size
	
	local img = render.image {
		width = texture_size,
		height = texture_size,
	}
	
	local inst_buffer = render.buffer {
		type = "vertex",
		usage = "stream",
		label = "texquad-instance",
		size = render.buffer_size("inst", setting.batch_size),
	}
	local sr_buffer = render.buffer {
		type = "storage",
		usage = "dynamic",
		label = "texquad-scalerot",
		size = render.buffer_size("srbuffer", setting.srbuffer_size),
	}
	local sprite_buffer = render.buffer {
		type = "storage",
		usage = "stream",
		label =  "texquad-scalerot",
		size = render.buffer_size("sprite", setting.batch_size),
	}

	-- todo: don't load texture here
	
	local bank_ptr = ltask.call(loader, "init", {
		max_sprite = setting.batch_size,
		texture_size = texture_size,
	})
	
	local spr = ltask.call(loader, "loadbundle", "asset/sprites.dl")
	local rect = ltask.call(loader, "pack")

	local imgmem = image.new(texture_size, texture_size)
	local canvas = imgmem:canvas()
	for id, v in pairs(rect) do
		local src = image.canvas(v.data, v.w, v.h, v.stride)
		image.blit(canvas, src, v.x, v.y)
	end
	
	img:update(imgmem)
	
	STATE = {
		pass = render.pass {
			color0 = 0x4080c0,
		},
		pipeline = render.pipeline "default",
	}
	local bindings = STATE.pipeline:bindings()
	bindings.vbuffer0 = inst_buffer
	bindings.sbuffer_sr_lut = sr_buffer
	bindings.sbuffer_sprite_buffer = sprite_buffer
	bindings.image_tex = img
	bindings.sampler_smp = render.sampler { label = "texquad-sampler" }
	
	STATE.inst = assert(inst_buffer)
	STATE.srbuffer = assert(sr_buffer)
	STATE.sprite = assert(sprite_buffer)

	STATE.srbuffer_mem = render.srbuffer(setting.srbuffer_size)
	STATE.bindings = bindings
	
	STATE.drawbuffer = render.drawbuffer(setting.draw_instance, bank_ptr, STATE.srbuffer_mem)
	
	STATE.uniform = STATE.pipeline:uniform_slot(0):init {
		tex_width = {
			offset = 0,
			type = "float",
		},
		tex_height = {
			offset = 4,
			type = "float",
		},
		framesize = {
			offset = 8,
			type = "float",
			n = 2,
		},
	}
	STATE.uniform.framesize = { 2/arg.width, -2/arg.height }
	STATE.uniform.tex_width = 1/texture_size
	STATE.uniform.tex_height = 1/texture_size
	
	barrier.init(mainloop, STATE)
end

return S
