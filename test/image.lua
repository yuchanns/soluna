local image = require "soluna.image"

local function readall(filename)
	local f = assert(io.open(filename, "rb"))
	local content = f:read "a"
	f:close()
	return content
end

local c = readall "asset/avatar.png"
print(image.info(c))
local content, w, h = image.load(c)
local x, y, cw, ch = image.crop(content, w, h)
local img = image.new(cw,ch)
local src_rect = image.canvas(content, w, h, x, y, cw, ch)
image.blit(img:canvas(), src_rect)
--img:write("crop.png")

