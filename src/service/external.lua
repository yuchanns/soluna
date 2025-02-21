local ltask = require "ltask"
local message = require "soluna.appmessage"
local render = require "soluna.render"
local image = require "soluna.image"
local spritepack = require "soluna.spritepack"

local loader = ltask.uniqueservice "loader"

local STATE, RECT

local S = {}

ltask.send(1, "external_forward", ltask.self(), "external")

local command = {}

function command.frame()
	if STATE then
		render.commit(STATE, RECT.dx, RECT.dy, RECT.x, RECT.y, RECT.w, RECT.h)
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
	local S = render.init(arg.width, arg.height)
	
	-- todo: don't load texture here
	local id = ltask.call(loader, "load", "asset/avatar.png", -0.5, -1)
	local rect = ltask.call(loader, "pack", 1024)

	local img = image.new(1024, 1024)
	local canvas = img:canvas()
	local r
	for id, v in pairs(rect) do
		local src = image.canvas(v.data, v.w, v.h, v.stride)
		image.blit(canvas, src, v.x, v.y)
		r = v
	end
	RECT = {
		dx = r.dx,
		dy = r.dy,
		x = r.x,
		y = r.y,
		w = r.w,
		h = r.h,
	}
	render.make_image(S, img, 1024, 1024)
	
	STATE = S
end

return S
