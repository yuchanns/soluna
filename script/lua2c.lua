local luasrc, cname = ...

local s = assert(loadfile(luasrc))
local bin = string.dump(s)

local code = [[
static const unsigned char luasrc_$name[] = {
	$bytes
};
]]

local count = 0
local function tohex(c)
	local b = string.format("0x%02x,", c:byte())
	count = count + 1
	if count == 16 then
		b = b .. "\n\t"
		count = 0
	end
	return b
end

local p = {
	name = cname:match "([^/]+)%.lua%.h$",
	bytes = string.gsub(bin .. "\0" , ".", tohex),	
}

local source = string.gsub(code, "$(%w+)", p)

local f = assert(io.open(cname, "w"))
f:write(source)
f:close()