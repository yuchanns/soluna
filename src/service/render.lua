local ltask = require "ltask"
local render = require "soluna.render"
local image = require "soluna.image"
local spritemgr = require "soluna.spritemgr"

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

local function mainloop(STATE)
	while true do
		local rad = barrier.count * 3.1415927 / 180
		local scale = math.sin(rad) + 1.2
		STATE.batch:reset()
		
		STATE.pass:begin()
			STATE.pipeline:apply()
			STATE.uniform:apply()

			STATE.batch:add(STATE.sprite_id, 256, 256, scale, rad)
			local n, tex = render.submit_batch(STATE.inst, 128, STATE.sprite, 128, STATE.srbuffer_mem, STATE.bank_ptr, STATE.batch:ptr())
			assert(n == 1 and tex == 0)
			STATE.srbuffer:update(STATE.srbuffer_mem:ptr())

			STATE.bindings:apply()
			render.draw(0, 4, 1)
		STATE.pass:finish()
		render.submit()
		barrier.wait()
	end
end

local S = {}

S.frame = barrier.trigger

function S.init(arg)
	local loader = ltask.uniqueservice "loader"

	local texture_size = 1024
	
	local img = render.image {
		width = texture_size,
		height = texture_size,
	}
	
	local inst_buffer = render.buffer {
		type = "vertex",
		usage = "stream",
		label = "texquad-instance",
		size = 128 * 3 * 4,	-- 128 sizeof(float) * 3
	}
	local sr_buffer = render.buffer {
		type = "storage",
		usage = "dynamic",
		label = "texquad-scalerot",
		size = 4096 * 4 * 4,	-- 4096 float * 4
	}
	local sprite_buffer = render.buffer {
		type = "storage",
		usage = "stream",
		label =  "texquad-scalerot",
		size = 128 * 3 * 4, -- 128 int32 * 3
	}

	-- todo: don't load texture here
	
	local bank_ptr = ltask.call(loader, "init", {
		max_sprite = 65536,
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
		bank_ptr = bank_ptr,
		batch = spritemgr.newbatch(),
		sprite_id = spr.avatar,
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

	STATE.srbuffer_mem = render.srbuffer()
	STATE.bindings = bindings
	
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
