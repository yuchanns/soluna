local font = require "soluna.font"
local icon = require "soluna.icon"

local text = {}

function text.init(bundle)
	font.import_icon(icon.bundle(bundle))
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
}

local function icon_id(name)
	local id = icon.names[name]
	if not id then
		return colors[name] or ("["..name.."]")
	end
	return "[i"..id.."]"
end

local function convert(tbl, key)
	local value = key:gsub("%[(%l+)%]", icon_id)
	if value == key then
		-- uniforming long string
		value = key
	end
	tbl[key]=value
	return value
end

text.convert = setmetatable({}, { __mode = "kv", __index = convert })

return text

