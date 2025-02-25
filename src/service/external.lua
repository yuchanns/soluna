local ltask = require "ltask"
local message = require "soluna.appmessage"
local render = require "soluna.render"
local image = require "soluna.image"
local spritemgr = require "soluna.spritemgr"

local loader = ltask.uniqueservice "loader"
local STATE

local S = {}

ltask.send(1, "external_forward", ltask.self(), "external")

local command = {}

function command.frame(_, _, count)
	if STATE then
		local rad = count * 3.1415927 / 180
		local scale = math.sin(rad) + 1.2
		STATE.batch:reset()
		
		STATE.pass:begin()
			STATE.pipeline:apply()
			STATE.uniform:apply()

			STATE.batch:add(STATE.sprite_id, 256, 256, scale, rad)
			local n, tex = render.submit_batch(STATE.inst, 128, STATE.sprite, 128, STATE.srbuffer_mem, STATE.bank_ptr, STATE.bank_sz, STATE.batch:ptr())
			assert(n == 1 and tex == 0)
			STATE.srbuffer:update(STATE.srbuffer_mem:ptr())

			STATE.bindings:apply()
			render.draw(0, 4, 1)
		STATE.pass:finish()
		render.submit()
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
	
	local img_width = 1024
	local img_height = 1024
	
	local img = render.image {
		width = img_width,
		height = img_height,
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
	local id = ltask.call(loader, "load", "asset/avatar.png", -0.5, -1)
	local rect = ltask.call(loader, "pack", img_width)

	local imgmem = image.new(img_width, img_height)
	local canvas = imgmem:canvas()
	for id, v in pairs(rect) do
		local src = image.canvas(v.data, v.w, v.h, v.stride)
		image.blit(canvas, src, v.x, v.y)
	end
	
	local bank_ptr, bank_sz = ltask.call(loader, "bank_ptr")
	
	img:update(imgmem)
	
	STATE = {
		pass = render.pass {
			color0 = 0x4080c0,
		},
		pipeline = render.pipeline "default",
		bank_ptr = bank_ptr,
		bank_sz = bank_sz,
		batch = spritemgr.newbatch(),
		sprite_id = id,
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
	STATE.uniform.tex_width = 1/img_width
	STATE.uniform.tex_height = 1/img_height
end

return S
