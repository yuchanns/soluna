local ltask = require "ltask"
local audio = require "soluna.audio"
local file = require "soluna.file"
local datalist = require "soluna.datalist"

global print, assert, setmetatable, tostring, error, ipairs

local DEVICE, BANK

local api = {}

local play = audio.play

-- play
api[true] = function(id)
	print("[Soluna Audio Lua] Playing sound id=" .. tostring(id) .. ", file=" .. tostring(BANK[id]))
	play(DEVICE, BANK[id])
end

local S = {}

global type, tonumber, error, assert, ipairs, print, pairs

for k in pairs(api) do
	S[k] = function()
		error "Init audio first"
	end
end

local M = {}

local function load_bundle(filename)
	local b = datalist.parse(file.load(filename))
	local bank = {}
	local map = {}
	for i, v in ipairs(b) do
		bank[i] = assert(v.filename)
		map[assert(v.name)] = i
	end
	return bank, map
end

function S.init(filename)
	assert(DEVICE == nil)
	DEVICE = false
	print("[Soluna Audio Lua] Initializing audio service with file: " .. filename)
	local bank, ret = load_bundle(filename)
	print("[Soluna Audio Lua] Loaded " .. #bank .. " sounds from bundle")
	local d = ltask.call(ltask.queryservice "render", "audio_engine")
	if not d then
		print("[Soluna Audio Lua] Audio engine not available")
		return {}
	else
		-- todo : load file list
		BANK = bank
		DEVICE = d
		print("[Soluna Audio Lua] Audio engine initialized, device: " .. tostring(d))
		local inject = ltask.dispatch()
		for k, v in pairs(api) do
			inject[k] = v
		end
		return ret
	end
end

function S.deinit()
	DEVICE = nil
end

return S
