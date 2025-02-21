local spritepack = require "soluna.spritepack"

local pack = spritepack.init(128)

local rects = {
	{ 64, 32 },
	{ 128, 128 },
}

for i, v in ipairs(rects) do
	pack:add(i, v[1], v[2])
end

local r = pack:run(4096)

for id,v in pairs(r) do
	local x, y = v >> 32 , v & 0xffffffff
	local r = rects[id]
	print("id = ", id , "coord = ", x, y, "rect = ", r[1], r[2])
end

