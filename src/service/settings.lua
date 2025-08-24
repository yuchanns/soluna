local ltask = require "ltask"
local datalist = require "soluna.datalist"
local source = require "soluna.embedsource"
local lfs = require "soluna.lfs"
local file = require "soluna.file"

global type, error, pairs, assert

local S = {}

local setting

local function patch(s, k, v)
	if type(k) == "number" then
		-- ignore
		return
	end
	local branch, key = k:match "^([^.]+)%.(.+)"
	if branch then
		local tree = s[branch]
		if tree == nil then
			tree = {}
			s[branch] = tree
		elseif type(tree) ~= "table" then
			error ("Conflict setting key : " .. k)
		end
		s = tree
		k = key
	end
	
	if type(v) == "table" then
		local orig_v = s[k]
		if orig_v == nil then
			s[k] = v
		elseif type(orig_v) == "table" then
			for sub_k,sub_v in pairs(v) do
				patch(orig_v, sub_k, sub_v)
			end
		else
			error ("Conflict setting key : " .. k)
		end
	else
		s[k] = v
	end
end

local function settings_filename(filename)
	if filename then
		local realname = lfs.realpath(filename)
		local curpath = realname:match "(.*)[/\\][^/\\]+$"
		if curpath then
			lfs.chdir(curpath)
		end
		return realname
	end
	if file.exist "main.game" then
		return "main.game"
	end
end

function S.init(args)
	assert(setting == nil)
	local default_settings = datalist.parse(source.data.settingdefault)
	local realname = settings_filename(args[1])
	if realname then
		local loader = file.loader(realname)
		local game_settings = datalist.parse(loader)
		for k,v in pairs(game_settings) do
			patch(default_settings, k,v)
		end
	end
	for k,v in pairs(args) do
		if type(k) == "string" then
			patch(default_settings, k,v)
		end
	end
	setting = default_settings
end

function S.get()
	return setting
end

return S
