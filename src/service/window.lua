local app = require "soluna.app"

local window = {}

function window.set_title(text)
	-- should be in a independent thread
	app.set_window_title(text)
end

return window
