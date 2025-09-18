local file = require "soluna.file"
local image = require "soluna.image"
local lfs = require "soluna.lfs"

local loader = file.loader "asset/avatar.png"

print_r(image.info(loader))
print(lfs.realpath ".")

for name in lfs.dir "." do
	print_r(name, lfs.attributes(name))
end
