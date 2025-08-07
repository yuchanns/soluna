local layout = require "soluna.layout"
local datalist = require "soluna.datalist"
local matquad = require "soluna.material.quad"

local args = ...

local hud = [[
id : screen
padding : 10
direction : row
gap : 10
left :
	width : 400
	background : 0x40000000
right :
	flex : 1
	gap : 10
	node :
		flex : 0.7
		background : 0x40ffffff
	node :
		flex : 0.3
		background : 0x40ffffff
]]

local dom = layout.load(datalist.parse_list(hud))
local screen = dom.screen

local function calc_hub()
	screen.width = args.width
	screen.height = args.height
	return layout.calc(dom)
end

local draw_list = calc_hub()

local function draw_hud()
	for _, obj in ipairs(draw_list) do
		args.batch:add(matquad.quad(obj.w, obj.h, obj.background), obj.x, obj.y)
	end
end

local callback = {}

function callback.frame(count)
	draw_hud()
end

function callback.window_resize(w,h)
	args.width = w
	args.height = h
	draw_list = calc_hub()
end

return callback

