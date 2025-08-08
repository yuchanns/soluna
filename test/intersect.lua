local soluna = require "soluna"
local quad = require "soluna.material.quad"

local args = ...
local batch = args.batch

local callback = {}

local mx = 0
local my = 0

function callback.mouse_move(x, y)
	mx, my = x, y
end

function callback.frame(count)
	local rad = count * math.pi / 180
	-- scale x2, move to the center of screen
	batch:layer(2, args.width / 2, args.height / 2)
		-- rotate canvas
		batch:layer(rad)
			-- use (-50, -50) , center of quad (100,100) as original point
			batch:layer(-50, -50)
				local x, y = batch:point(mx, my)
				local color = 0xffffff
				if x >=0 and x < 100 and y >=0 and y < 100 then
					color = 0xff0000
				end
				batch:add(quad.quad(100,100,color))
			batch:layer()
		batch:layer()
	batch:layer()
end

return callback

