local datalist = require "soluna.datalist"
local source = require "soluna.embedsource"

-- todo : read app settings from filesystem

return datalist.parse(source.data.settingdefault)
