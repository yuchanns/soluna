local ltask = require "ltask"
local audio = require "soluna.audio"
local file = require "soluna.file"
local datalist = require "soluna.datalist"

global print, assert, setmetatable, tostring, error, ipairs

local DEVICE, BANK

local function log(...)
	print("[audio.service]", ...)
end

local api = {}

local play = audio.play

-- play
api[true] = function(id)
	if not DEVICE then
		log("play skipped: device not initialized", id)
		return
	end
	local filename = BANK and BANK[id]
	if not filename then
		log("play skipped: unknown sound id", id)
		return
	end
	log("play request", id, filename)
	play(DEVICE, filename)
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
	log("loading sound bundle", filename)
	local b = datalist.parse(file.load(filename))
	local bank = {}
	local map = {}
	for i, v in ipairs(b) do
		bank[i] = assert(v.filename)
		map[assert(v.name)] = i
	end
	log("sound bundle ready", #bank)
	return bank, map
end

function S.init(filename)
	assert(DEVICE == nil)
	DEVICE = false
	log("audio service init", filename)
	local bank, ret = load_bundle(filename)
	local d = ltask.call(ltask.queryservice "render", "audio_engine")
	if not d then
		log("audio engine not available")
		return {}
	else
		-- todo : load file list
		BANK = bank
		DEVICE = d
		log("audio engine ready", DEVICE)
		local inject = ltask.dispatch()
		for k, v in pairs(api) do
			inject[k] = v
		end
		return ret
	end
end

function S.deinit()
	log("audio service deinit")
	DEVICE = nil
end

return S
