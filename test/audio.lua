local soluna = require "soluna"
local quad = require "soluna.material.quad"

soluna.load_sounds "asset/sounds.dl"
soluna.set_window_title "Soluna sound sample"

local args = ...
local batch = args.batch

local BTN_W <const> = 200
local BTN_H <const> = 60

local COLOR_NORMAL <const> = 0xFF4488FF
local COLOR_HOVER  <const> = 0xFF66AAFF
local COLOR_PRESS  <const> = 0xFF2266DD

local callback = {}
local screen_w = args.width
local screen_h = args.height
local mx = 0
local my = 0
local pressing = false

local function btn_rect()
	local x = (screen_w - BTN_W) / 2
	local y = (screen_h - BTN_H) / 2
	return x, y, BTN_W, BTN_H
end

local function in_button(x, y)
	local bx, by, bw, bh = btn_rect()
	return x >= bx and x < bx + bw and y >= by and y < by + bh
end

function callback.frame(count)
	local bx, by, bw, bh = btn_rect()
	local color
	if pressing then
		color = COLOR_PRESS
	elseif in_button(mx, my) then
		color = COLOR_HOVER
	else
		color = COLOR_NORMAL
	end
	batch:add(quad.quad(bw, bh, color), bx, by)
end

function callback.window_resize(w, h)
	screen_w = w
	screen_h = h
end

function callback.mouse_move(x, y)
	mx, my = x, y
end

function callback.mouse_button(button, state)
	if button ~= 0 then return end
	if state == 1 then
		pressing = in_button(mx, my)
	else
		if pressing and in_button(mx, my) then
			soluna.play_sound "bloop"
		end
		pressing = false
	end
end

function callback.touch_begin(x, y)
	pressing = in_button(x, y)
end

function callback.touch_end(x, y)
	if pressing and in_button(x, y) then
		soluna.play_sound "bloop"
	end
	pressing = false
end

return callback