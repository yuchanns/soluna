local soluna = require "soluna"
local matquad = require "soluna.material.quad"
local mattext = require "soluna.material.text"
local font = require "soluna.font"
local file = require "soluna.file"

print(string.format("[Audio Test] Platform: %s", soluna.platform))
print("[Audio Test] Loading sounds...")
soluna.load_sounds "asset/sounds.dl"
print("[Audio Test] Sounds loaded successfully")
soluna.set_window_title "Soluna sound sample"

local args = ...
local batch = args.batch
local screen_w = args.width
local screen_h = args.height

local BUTTON_W <const> = 180
local BUTTON_H <const> = 64

local state = {
	mouse_x = screen_w // 2,
	mouse_y = screen_h // 2,
}

local function font_init()
	if soluna.platform == "wasm" then
		local bundled_path = "asset/font/SourceHanSansSC-Regular.ttf"
		local bundled_data = file.load(bundled_path)
		if bundled_data then
			font.import(bundled_data)
			local bundled_id = font.name "Source Han Sans SC Regular"
			if bundled_id then
				return bundled_id
			end
		end
	end

	local sysfont = require "soluna.font.system"
	local candidates = {
		"WenQuanYi Micro Hei",
		"Microsoft YaHei",
		"Yuanti SC",
		"Source Han Sans SC Regular",
	}
	for _, name in ipairs(candidates) do
		local ok, data = pcall(sysfont.ttfdata, name)
		if ok and data then
			font.import(data)
			local fontid = font.name(name)
			if fontid then
				return fontid
			end
		end
	end
	error "No available system font for audio sample"
end

local fontid = font_init()
local fontcobj = font.cobj()
local text_block = mattext.block(fontcobj, fontid, 28, 0xffffffff, "C")
local play_label = text_block("Play", BUTTON_W, BUTTON_H)

local function button_rect()
	local x = (screen_w - BUTTON_W) // 2
	local y = (screen_h - BUTTON_H) // 2
	return x, y, BUTTON_W, BUTTON_H
end

local function inside_button(x, y)
	local bx, by, bw, bh = button_rect()
	return x >= bx and x <= bx + bw and y >= by and y <= by + bh
end

local function trigger_play()
	print("[Audio Test] Button clicked, playing sound 'bloop'")
	soluna.play_sound "bloop"
	print("[Audio Test] play_sound call completed")
end

local callback = {}

function callback.window_resize(w, h)
	screen_w = w
	screen_h = h
end

function callback.mouse_move(x, y)
	state.mouse_x = x
	state.mouse_y = y
end

function callback.mouse_button(button, key_state)
	if button ~= 0 or key_state ~= 1 then
		return
	end
	if inside_button(state.mouse_x, state.mouse_y) then
		trigger_play()
	end
end

function callback.touch_end(x, y)
	if inside_button(x, y) then
		trigger_play()
	end
end

function callback.frame()
	local bx, by, bw, bh = button_rect()
	local hovered = inside_button(state.mouse_x, state.mouse_y)
	local color = hovered and 0x3a74d9ff or 0x245bb8ff
	batch:add(matquad.quad(bw, bh, color), bx, by)
	batch:add(play_label, bx, by)
end

return callback
