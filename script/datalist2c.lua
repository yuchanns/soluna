local dlsrc, cname = ...

local f = assert(io.open(dlsrc,"rb"))
local bin = f:read "a"
f:close()

local code = [[
static const unsigned char dl_$name[] = {
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
	name = cname:match "([^/]+)%.dl%.h$",
	bytes = string.gsub(bin, ".", tohex),	
}

local source = string.gsub(code, "$(%w+)", p)

local f = assert(io.open(cname, "w"))
f:write(source)
f:close()