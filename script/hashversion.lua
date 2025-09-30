local function get_rev()
	local f = io.popen "git rev-parse HEAD"
	local rev = f:read "a"
	f:close()
	return rev:match "[%da-f]*"
end

local version = get_rev()
print("SOLUNA_HASH_VERSION", version)

local f = assert(io.open "src/version.h")
local text = f:read "a"
text = text:gsub('(SOLUNA_HASH_VERSION%s+")([%da-f]*)(")', "%1".. version .. "%3")
print(text)
f:close()
local f = assert(io.open("src/version.h", "wb"))
f:write(text)
f:close()
