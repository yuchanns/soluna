# Soluna Input API

本文件覆盖游戏侧键盘、字符、鼠标和触摸输入 callback。

## 键盘

```lua
function callback.key(keycode, state)
end
```

`state`：

- `0`：release。
- `1`：press。
- `2`：repeat。

示例：

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

## 字符输入

```lua
function callback.char(codepoint)
end
```

`codepoint` 是 Unicode codepoint。文本输入应使用 `utf8.char(codepoint)` 转成 UTF-8 字符串，并过滤控制字符；不要用 `key` callback 手写普通字符输入。

## 鼠标

```lua
function callback.mouse_move(x, y)
end

function callback.mouse_button(button, state)
end

function callback.mouse_scroll(dx, dy)
end
```

`button`：

- `0`：left。
- `1`：right。
- `2`：middle。

`state`：

- `0`：release。
- `1`：press。

## Touch

```lua
function callback.touch_begin(x, y)
end

function callback.touch_moved(x, y)
end

function callback.touch_end(x, y)
end

function callback.touch_cancelled()
end
```

触摸输入常用于 wasm 和移动类平台。游戏可以把 touch 和 mouse 归一成同一套 pointer 状态。
