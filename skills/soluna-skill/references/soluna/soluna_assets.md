# Soluna Assets API

本文件覆盖游戏侧常用资产加载：文件、图片、sprite bundle 和 datalist。

## 文件

```lua
local file = require "soluna.file"
```

常用函数：

- `file.load(filename)`：读取资源路径中的文件，返回 string 或 nil。
- `file.exist(filename)`：检查资源路径中的文件是否存在。
- `file.attributes(filename)`：读取文件属性。
- `file.dir(path)`：遍历目录。
- `file.local_load(filename)`：读取本地文件。
- `file.local_exist(filename)`：检查本地文件。

示例：

```lua
local file = require "soluna.file"

local data = assert(file.load "asset/config.dl")
```

优先使用 `file.load` 读取游戏资源。只有明确需要访问本地文件系统时再使用 `local_*`。

## 图片

```lua
local image = require "soluna.image"
```

常用函数：

- `image.load(data)`：从 PNG/JPG 等二进制数据解码图片。
- `image.resize(data, width, height, scale_x, scale_y)`：缩放图片。

示例：

```lua
local file = require "soluna.file"
local image = require "soluna.image"

local data, width, height = image.load(assert(file.load "asset/icon.png"))
local small_data, small_width, small_height = image.resize(data, width, height, 0.5)
```

## Sprite Bundle

```lua
local soluna = require "soluna"

local sprites = soluna.load_sprites "asset/sprites.dl"
```

返回值是 name 到 sprite id 的映射：

```lua
batch:add(sprites.player, 100, 100)
```

路径和名称由项目资产组织决定，不是固定约定。

### Sprite Bundle 文件路径

说明：

- 当 `soluna.load_sprites` 读取 `.dl` 文件时，sprite 定义中的 `filename` 会相对 `.dl` 文件所在目录解析。
- `.dl` 内部不要重复写入已经用于加载 bundle 的目录前缀，否则路径会被拼接两次。
- 例如 `soluna.load_sprites "asset/pacman/sprites.dl"` 中的 sprite 文件应写成 `sprites/player.png`，不要写成 `asset/pacman/sprites/player.png`。

正确做法：

```lua
local sprites = soluna.load_sprites "asset/pacman/sprites.dl"
```

```text
--
name : player
filename : sprites/player.png
x : 0
y : 0
```

错误做法：

```lua
local sprites = soluna.load_sprites "asset/pacman/sprites.dl"
```

```text
--
name : player
filename : asset/pacman/sprites/player.png
x : 0
y : 0
```

上面的错误写法会尝试读取 `asset/pacman/asset/pacman/sprites/player.png`。

`load_sprites` 也可以接收内存中的 bundle table。这个形式常和 `soluna.preload` 搭配，用于把运行时生成的 RGBA 数据注册成 sprite。

sprite bundle 中的 `x` / `y` 语义同时适用于：

- `.dl` 文件中的 sprite 定义。
- 传给 `soluna.load_sprites` 的内存 bundle table。

说明：

- `x` / `y` 为非负数时，表示像素偏移。
- `x` / `y` 为负数时，表示相对 sprite 宽高的比例偏移，不表示负像素偏移。
- 以 sprite 中心作为 anchor 时，应写 `x = -0.5, y = -0.5`。
- 需要按像素指定偏移时，只应传非负像素值；不要把 `-(width // 2)`、`-(height // 2)` 这类负像素值直接传给 `soluna.load_sprites` 或写入 `.dl` sprite 定义。

正确做法：

```lua
local sprites = soluna.load_sprites {
	{
		name = "player",
		filename = "@player",
		x = -0.5,
		y = -0.5,
	},
}
```

错误做法：

```lua
local sprites = soluna.load_sprites {
	{
		name = "player",
		filename = "@player",
		x = -(width // 2),
		y = -(height // 2),
	},
}
```

## Preload Generated Sprites

`soluna.preload` 用于预加载内存中的 RGBA 图片数据。它不是普通文件预读接口；静态图片和 sprite atlas 仍优先用 `.dl` sprite bundle 组织。

每个 preload descriptor 包含：

- `filename`：后续 `load_sprites` 引用的虚拟文件名。项目可以自行命名，常用 `@` 前缀区分生成资产。
- `content`：RGBA 字节串。
- `w`：图片宽度。
- `h`：图片高度。

`content` 长度必须等于 `w * h * 4`。

单个生成 sprite：

```lua
local soluna = require "soluna"

soluna.preload {
	filename = "@white_pixel",
	content = string.char(255, 255, 255, 255),
	w = 1,
	h = 1,
}

local sprites = soluna.load_sprites {
	{
		name = "white_pixel",
		filename = "@white_pixel",
	},
}
```

一次预加载多个生成 sprite：

```lua
local soluna = require "soluna"

soluna.preload {
	{
		filename = "@red_pixel",
		content = string.char(255, 0, 0, 255),
		w = 1,
		h = 1,
	},
	{
		filename = "@green_pixel",
		content = string.char(0, 255, 0, 255),
		w = 1,
		h = 1,
	},
}

local sprites = soluna.load_sprites {
	{
		name = "red_pixel",
		filename = "@red_pixel",
	},
	{
		name = "green_pixel",
		filename = "@green_pixel",
	},
}
```

bundle table 中也可以为 sprite 指定 `x` / `y` 偏移，例如：

```lua
local sprites = soluna.load_sprites {
	{
		name = "player",
		filename = "@player",
		x = -0.5,
		y = -0.5,
	},
}
```

这里的 `x` / `y` 规则与前面的 `Sprite Bundle` 小节相同。

使用约束：

- 先调用 `soluna.preload`，再调用引用这些虚拟文件名的 `soluna.load_sprites`。
- 不要在每帧生成和 preload sprite；把生成结果按尺寸、颜色、状态等 key 缓存起来。
- 适合程序化 UI 形状、纯色像素、运行时生成的小图标等场景。
- 已经稳定存在于资源目录中的图片，优先写入 sprite bundle，而不是运行时 preload。

## Datalist

```lua
local datalist = require "soluna.datalist"
```

常用函数：

- `datalist.parse(data)`：解析 datalist 文本。
- `datalist.quote(str)`：生成 datalist 字符串字面量。

示例：

```lua
local file = require "soluna.file"
local datalist = require "soluna.datalist"

local rules = datalist.parse(assert(file.load "asset/rules.dl"))
```

## Datalist 字符串规则

`.dl` / `.game` 文件中的未加引号字符串不能包含空格。值里有空格时必须使用双引号。

```text
window_title : "My Game"
```

不要写成：

```text
window_title : My Game
```

路径、标识符、数字和布尔值通常不需要引号：

```text
entry : main.lua
project : mygame
high_dpi : true
background : 0xff000000
```

生成 `.dl` 内容时，含空格或特殊字符的字符串用 `datalist.quote(str)`。

```lua
local datalist = require "soluna.datalist"

local title = datalist.quote("My Game")
```
