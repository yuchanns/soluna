# Soluna App API

本文件覆盖游戏侧应用控制、窗口标题、图标和 IME。

## 应用控制

```lua
local app = require "soluna.app"

app.quit()
```

在输入回调中退出：

```lua
local app = require "soluna.app"

local KEY_ESCAPE <const> = 256
local KEYSTATE_PRESS <const> = 1

function callback.key(keycode, state)
	if keycode == KEY_ESCAPE and state == KEYSTATE_PRESS then
		app.quit()
	end
end
```

## 窗口标题

标题既可以在 `.game` 中设置，也可以运行时设置。

```lua
local soluna = require "soluna"

soluna.set_window_title "My Game"
```

## 图标

`soluna.set_icon(data)` 接受一个 icon 列表。每个元素包含 `data`、`width`、`height`。

```lua
local soluna = require "soluna"
local file = require "soluna.file"
local image = require "soluna.image"

local data, width, height = image.load(assert(file.load "asset/icon.png"))
local small_data, small_width, small_height = image.resize(data, width, height, 0.5)

soluna.set_icon {
	{ data = data, width = width, height = height },
	{ data = small_data, width = small_width, height = small_height },
}
```

## IME

需要文本输入时，设置 IME 字体和候选框位置。

```lua
local app = require "soluna.app"

app.set_ime_font("Game Font", 18)
app.set_ime_rect {
	x = 20,
	y = 40,
	width = 300,
	height = 28,
	text_color = 0xff000000,
}
```

清除候选框位置：

```lua
app.set_ime_rect(nil)
```
