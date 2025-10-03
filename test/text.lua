local soluna = require "soluna"
local ltask = require "ltask"
local mattext = require "soluna.material.text"
local matquad = require "soluna.material.quad"
local font = require "soluna.font"

local batch = ...

local function font_init()
	local sysfont = require "soluna.font.system"
	font.import(assert(sysfont.ttfdata "微软雅黑"))
	return font.name ""
end

soluna.set_window_title "soluna text sample"

local args = ...
local batch = args.batch
local fontid = font_init()
local fontcobj = font.cobj()

local callback = {}
local WIDTH <const> = 200
local HEIGHT <const> = 200
local screen_w = args.width
local screen_h = args.height

function callback.window_resize(w, h)
	screen_w = w
	screen_h = h
end

local TEXT <const> = "Hello, 这是一条很长的句子。它会在文本区居中。"
-- size 32; color 0; alignment center
local block, cursor = mattext.block(fontcobj, fontid, 32, 0, "CV")
local label = block(TEXT, WIDTH, HEIGHT)

local CURSOR_N = 0

function callback.frame(count)
	local x = (screen_w - WIDTH) / 2
	local y = (screen_h - HEIGHT) / 2
	batch:add(matquad.quad(WIDTH, HEIGHT, 0x400000ff), x, y)
	batch:add(label, x, y)
	-- cursor
	local cx, cy, cw, ch, n = cursor(TEXT, CURSOR_N, WIDTH, HEIGHT)
	CURSOR_N = n
	batch:add(matquad.quad(cw, ch, 0xffffff), cx + x, cy + y)
end

function callback.key(keycode, state)
	if state == 1 then	-- press
		if keycode == 262 then	-- right
			CURSOR_N = CURSOR_N + 1
		elseif keycode == 263 then	-- left
			CURSOR_N = CURSOR_N - 1
		else
			print(keycode)
		end
	end
end


return callback

