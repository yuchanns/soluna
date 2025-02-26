local file = require "soluna.file"
local image = require "soluna.image"

local loader = file.loader "asset/avatar.png"

print_r(image.info(loader))


