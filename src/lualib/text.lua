local font = require "soluna.font"
local icon = require "soluna.icon"
global setmetatable, print, tonumber

local text = {}

local bundle_data

function text.init(bundle)
	bundle_data = icon.bundle(bundle)	-- prevent gc to collect bundle_data
	font.import_icon(bundle_data)
end

local colors = {
	red = "[FF0000]",
	green = "[00FF00]",
	blue = "[0000FF]",
	white = "[FFFFFF]",
	black = "[000000]",
	aqua = "[00FFFF]",
	yellow = "[FFFF00]",
	pink = "[FF00FF]",
	gray = "[808080]",
	bracket = "[bracket]",
}

local function user_color(self, name)
	if name:byte() == 99 then	-- 'c'
		local cvalue = name:sub(2)
		local c = tonumber(cvalue, 16)
		if c then
			local cname = "[" .. cvalue .. "]"
			self[name] = cname
			return cname
		end
	end
end

setmetatable(colors, { __index = user_color })

local function icon_id(name)
	local cname = colors[name]
	if cname then
		return cname
	end
	local id = icon.names[name]
	if not id then
		return "["..name.."]"
	end
	return "[i"..id.."]"
end

local function convert(tbl, key)
	local escape = key:gsub("%[%[", "[bracket]")
	local value = escape:gsub("%[(%w+)%]", icon_id)
	if escape ~= key then
		value = value:gsub("%[bracket%]", "[[")
	elseif value == key then
		-- uniforming long string
		value = key
	end
	tbl[key]=value
	return value
end

text.convert = setmetatable({}, { __mode = "kv", __index = convert })

return text
