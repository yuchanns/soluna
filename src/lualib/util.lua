local table = table

global setmetatable

local util = {}

local func_chain = {}; func_chain.__index = func_chain

function func_chain:add(f)
	table.insert(self, f)
end

function func_chain:__call()
	for i = 1, #self do
		local f = self[i]
		f()
	end
end

function util.func_chain()
	return setmetatable({}, func_chain)
end

return util
