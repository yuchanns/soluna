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

-- Try to create a "play" text label for the button.
-- Font loading may fail on some platforms (e.g. WASM without bundled fonts),
-- in which case text_label remains nil and the button renders without text.
local text_label = nil
local function try_load_font()
	local font    = require "soluna.font"
	local mattext = require "soluna.material.text"
	local fontid

	-- Desktop: try common system fonts
	local ok, sysfont = pcall(require, "soluna.font.system")
	if ok then
		local candidates = { "Arial", "DejaVu Sans", "Liberation Sans", "Helvetica Neue", "Helvetica" }
		for _, name in ipairs(candidates) do
			local ok2, data = pcall(sysfont.ttfdata, name)
			if ok2 and data then
				font.import(data)
				fontid = font.name ""   -- last imported
				if fontid then break end
			end
		end
	end

	-- WASM: try bundled font files
	if not fontid then
		local ok2, file = pcall(require, "soluna.file")
		if ok2 then
			local paths = { "asset/font/arial.ttf", "asset/font/SourceHanSansSC-Regular.ttf" }
			for _, path in ipairs(paths) do
				local data = file.load(path)
				if data then
					font.import(data)
					fontid = font.name ""
					if fontid then break end
				end
			end
		end
	end

	if fontid then
		local fontcobj = font.cobj()
		-- size=24, color=opaque white, alignment=center-horizontal+center-vertical
		local block = mattext.block(fontcobj, fontid, 24, 0xFFFFFFFF, "CV")
		return block("play", BTN_W, BTN_H)
	end
end

local ok, result = pcall(try_load_font)
if ok then text_label = result end

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
	if text_label then
		batch:add(text_label, bx, by)
	end
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