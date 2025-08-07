-- To run this sample :
-- bin/soluna.exe entry=test/window.lua
-- bin/soluna.exe test/window.game
local soluna = require "soluna"

soluna.set_window_title "Soluna Sample"

local callback = {}

function callback.frame(count)
end

return callback

