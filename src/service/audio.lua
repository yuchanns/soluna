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
	print("[audio] play: id=" .. tostring(id) .. " file=" .. tostring(BANK[id]))
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
	print("[audio] service init: loading " .. tostring(filename))
	local bank, ret = load_bundle(filename)
	print("[audio] service init: bank loaded, " .. #bank .. " sounds")
	for i, v in ipairs(bank) do
		print("[audio] service init: bank[" .. i .. "]=" .. tostring(v))
	end
	local d = ltask.call(ltask.queryservice "render", "audio_engine")
	print("[audio] service init: audio_engine ptr=" .. tostring(d))
	if not d then
		print("[audio] service init: no audio engine, audio disabled")
		return {}
	else
		-- todo : load file list
		BANK = bank
		DEVICE = d
		local inject = ltask.dispatch()
		for k, v in pairs(api) do
			inject[k] = v
		end
		print("[audio] service init: OK, sounds ready")
		return ret
	end
end

function S.deinit()
	DEVICE = nil
end

return S
