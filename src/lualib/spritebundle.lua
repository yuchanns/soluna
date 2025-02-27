local image = require "soluna.image"
local file = require "soluna.file"
local datalist = require "soluna.datalist"

local M = {}

local function load_bundle(filename)
	local b = datalist.parse(file.loader(filename))
	return b
end

local function crop(item, filecache)
	local c = filecache[item.filename]
	if c == nil then
		error("No file : " .. item.filename)
	end
	local x = item.cx
	local y = item.cy
	local w = item.cw
	local h = item.ch
	
	local offx = item.x or 0
	local offy = item.y or 0
	if offx < 0 then
		offx = - c.w * offx // 1 | 0
	end
	if offy < 0 then
		offy = - c.h * offy // 1 | 0
	end
	local cx, cy, cw, ch = image.crop(c.data, c.w, c.h, x, y, w, h)
	offx = offx - cx
	offy = offy - cy
	
	item.x = offx
	item.y = offy
	item.cx = cx + (x or 0)
	item.cy = cy + (y or 0)
	item.cw = cw
	item.ch = ch
end

function M.load(filecache, filename)
	local path = filename:match "(.*[/\\])[^/\\]+$"
	local v = load_bundle(filename)
	for idx, item in ipairs(v) do
		local fname = item.filename or "Need filename for item " .. idx
		if path then
			item.filename = path .. fname
		end
		crop(item, filecache)
	end
	return v
end

function M.loadimage(filecache, filename)
	local content = file.load(filename)
	if not content then
		if not filecache.__missing[filename] then
			filecache.__missing[filename] = true
			print("Missing file : " .. filename)
		end
		return
	end
	local data, w, h = image.load(content)
	if data == nil then
		if not filecache.__missing[filename] then
			filecache.__missing[filename] = true
			print("Invalid image : " .. filename .. "(" .. w .. ")")
		end
		return
	end
	local r = { data = data, w = w, h = h }
	filecache[filename] = r
	return r
end

return M
