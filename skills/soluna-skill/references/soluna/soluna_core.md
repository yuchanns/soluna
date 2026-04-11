# Soluna Core API

本文件覆盖游戏开发最常用的入口、`.game` 配置、callback、启动参数和 batch 绘制表面。

## `.game` 配置

`.game` 文件声明入口、项目名和窗口基础配置。
字符串值如果包含空格，必须使用双引号。

```text
entry : main.lua
project : mygame
width : 640
height : 480
high_dpi : false
window_title : "My Game"
background : 0xff000000
```

常用字段：

- `entry`：入口 Lua 文件。
- `project`：项目名，影响 `soluna.gamedir()` 默认参数。
- `width` / `height`：初始窗口尺寸。
- `high_dpi`：是否启用高 DPI。
- `window_title`：窗口标题。
- `background`：背景色，ARGB。
- `service_path`：需要自定义 service 搜索路径时使用。
- `extlua_entry` / `extlua_preload`：加载 extlua C 插件时使用。
- `extlua_material` / `extlua_material_path`：加载 extlua Lua material adapter 时使用。`extlua_material` 可以是单个名称或名称列表；runtime 按 `extlua_material_path` 搜索对应 Lua 文件。

运行：

```bash
soluna main.game
```

也可以直接传 zip：

```bash
soluna main.zip
```

## 入口 Lua 文件

入口文件返回 callback table。`...` 是启动参数 table，常用字段是 `width`、`height`、`batch`。

```lua
local soluna = require "soluna"
local app = require "soluna.app"

local args = ...
local batch = args.batch

local callback = {}

function callback.frame(count)
end

function callback.key(keycode, state)
end

return callback
```

## Callback

游戏只需要实现实际用到的 callback。

```lua
function callback.frame(count)
end

function callback.key(keycode, state)
end

function callback.char(codepoint)
end

function callback.mouse_button(button, state)
end

function callback.mouse_move(x, y)
end

function callback.mouse_scroll(dx, dy)
end

function callback.touch_begin(x, y)
end

function callback.touch_end(x, y)
end

function callback.touch_moved(x, y)
end

function callback.touch_cancelled()
end

function callback.window_resize(width, height)
end
```

输入约定：

- `key(state)`：`0` release，`1` press，`2` repeat。
- `mouse_button(button)`：`0` left，`1` right，`2` middle。
- `mouse_button(state)`：`0` release，`1` press。

## Batch

`args.batch` 是游戏主要绘制表面。

```lua
batch:add(sprite, x, y)
batch:add(material, x, y)
batch:add(stream, x, y)
```

`sprite` 可以是：

- `soluna.load_sprites` 返回的 sprite id。
- material 模块创建的 userdata。
- material 模块返回的 packed draw stream。

extlua material 通常让游戏侧 helper 返回 packed draw stream，再通过 `batch:add(stream, x, y)` 提交。batch 会把 `x`/`y` 和当前 layer transform 应用到 stream 中的每个 item。

Layer 用于变换，必须成对闭合。

```lua
batch:layer(2, 100, 100)
batch:add(sprite)
batch:layer()
```

常见调用形式：

```lua
batch:layer()
batch:layer(rotation)
batch:layer(x, y)
batch:layer(scale, x, y)
batch:layer(scale, rotation, x, y)
```

## `soluna`

```lua
local soluna = require "soluna"
```

常用字段和函数：

- `soluna.platform`：`"windows"`、`"macos"`、`"linux"`、`"wasm"`。
- `soluna.version`：runtime version string。
- `soluna.version_api`：API version number。
- `soluna.settings()`：返回 `.game` 配置合并后的 settings table。
- `soluna.gamedir(name)`：返回游戏数据目录。
- `soluna.load_sprites(filename)`：加载 sprite bundle。
- `soluna.preload(spr)`：预加载内存 RGBA 数据生成的 sprite。详见 `soluna_assets.md`。
- `soluna.load_sounds(filename)`：加载 sound bundle。详见 `soluna_audio.md`。
- `soluna.play_sound(name)`：播放已加载 sound。详见 `soluna_audio.md`。

示例：

```lua
local soluna = require "soluna"

local settings = soluna.settings()
local sprites = soluna.load_sprites "asset/sprites.dl"

if soluna.platform == "wasm" then
	print("running in browser")
end
```
