-- To run this sample :
-- bin/soluna.exe entry=test/sprite.lua
local soluna = require "soluna"
local ltask = require "ltask"
local mask = require "soluna.material.mask"

soluna.set_window_title "soluna sprite sample"
local sprites = soluna.load_sprites "asset/sprites.dl"

local args = ...
local batch = args.batch

local callback = {}

function callback.frame(count)
	batch:add(mask.mask(sprites.avatar, 0x40000000), args.width / 2, args.height/2, 1, 0)
end

return callback

