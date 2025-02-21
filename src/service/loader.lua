local image = require "soluna.image"

local missing = {}

local function fetchfile(filecache, filename)
	local f = io.open(filename, "rb")
	if not f then
		if not missing[filename] then
			missing[filename] = true
			print("Missing file : " .. filename)
		end
		return
	end
	local content = f:read "a"
	f:close()
	local data, w, h = image.load(content)
	if data == nil then
		if not missing[filename] then
			missing[filename] = true
			print("Invalid image : " .. filename .. "(" .. w .. ")")
		end
		return
	end
	local r = { data = data, w = w, h = h }
	filecache[filename] = r
	return r
end

local filecache = setmetatable({} , { __mode = "kv", __index = fetchfile })

local S = {}

local sprite = {}

function S.load(filename, offx, offy, x, y, w, h)
	local c = filecache[filename]
	if c == nil then
		return
	end
	local index = image.makeindex(x, y, w, h)
	if offx < 0 then
		offx = - c.w * offx // 1 | 0
	end
	if offy < 0 then
		offy = - c.h * offy // 1 | 0
	end
	local obj = c[index]
	if obj then
		assert(obj.x == offx and obj.y == offy)
		return obj.id
	end
	local cx, cy, cw, ch = image.crop(c.data, c.w, c.h, x, y, w, h)
	local id = #sprite+1
	sprite[id] = {
		filename = filename,
		index = index,
		id = id,
		x = offx,
		y = offy,
		cx = cx,
		cy = cy,
		cw = cw,
		ch = ch,
	}
	return id
end

function S.write(id, filename)
	local obj = sprite[id]
	assert(obj.cx)
	local c = filecache[obj.filename]
	local data = image.canvas(c.data, c.w, c.h, obj.cx, obj.cy, obj.cw, obj.ch)
	local img = image.new(obj.cw, obj.ch)
	image.blit(img:canvas(), data)
	img:write(filename)
end

return S