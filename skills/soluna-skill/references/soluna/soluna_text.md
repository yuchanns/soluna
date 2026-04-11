# Soluna Text API

本文件覆盖字体导入、系统字体、文本 material 和 tagged text 转换。

## 字体

```lua
local font = require "soluna.font"
```

常用函数：

- `font.import(data)`：导入 TTF 数据。
- `font.name(name)`：按字体名取 font id。
- `font.cobj()`：取得文本 material 需要的 font object。

wasm 上应随游戏打包字体：

```lua
local font = require "soluna.font"
local file = require "soluna.file"

font.import(assert(file.load "asset/font/GameFont.ttf"))
local fontid = assert(font.name "Game Font")
```

桌面端可尝试系统字体：

```lua
local font = require "soluna.font"
local sysfont = require "soluna.font.system"

local data = assert(sysfont.ttfdata "Arial")
font.import(data)
local fontid = assert(font.name "Arial")
```

## 文本 Material

```lua
local mattext = require "soluna.material.text"
local font = require "soluna.font"

local block = mattext.block(font.cobj(), fontid, 18, 0xffffffff, "LT")
local label = block("Hello", 160, 32)
batch:add(label, 20, 20)
```

`mattext.block` 返回两个函数：

- `block(text, width, height)`：创建可绘制文本对象。
- `cursor(text, position, width, height)`：计算文本光标位置。

Alignment 可组合：

- `L` left，`C` center，`R` right。
- `T` top，`V` vertical center，`B` bottom。

示例：

```lua
local title_block = mattext.block(font.cobj(), fontid, 32, 0xffffd040, "CV")
```

alignment 作用于 `block(text, width, height)` 传入的布局盒子，不会根据 `batch:add(label, x, y)` 的坐标自动推断目标区域。

说明：

- `mattext.block(..., "CV")` 表示在文本 block 自身的 `width` / `height` 盒子内水平和垂直居中。
- `batch:add(label, x, y)` 只决定这个文本 block 的左上角放到哪里，不会额外参与居中计算。
- 需要把按钮文字或面板文字居中时，应把按钮或面板的完整尺寸传给 `block(text, width, height)`。
- 如果只传很小的 `height`，再手动用 `y + offset` 修位置，通常会破坏垂直居中，表现为文字看起来偏上、偏下，或者压到边框。

正确做法：

```lua
local button_block = mattext.block(font.cobj(), fontid, 16, 0xffffffff, "CV")
local button_label = button_block("START", button_w, button_h)

batch:add(button_label, button_x, button_y)
```

错误做法：

```lua
local button_block = mattext.block(font.cobj(), fontid, 16, 0xffffffff, "CV")
local button_label = button_block("START", button_w, 12)

batch:add(button_label, button_x, button_y + 6)
```

## 文本缓存

稳定文本应缓存，避免每帧创建新 text sprite。

```lua
local text_cache = {}

local function label(key, block, text, width, height)
	local cached = text_cache[key]
	if cached == nil then
		cached = block(text, width, height)
		text_cache[key] = cached
	end
	return cached
end
```

## Tagged Text

```lua
local text = require "soluna.text"

text.init "asset/icons.dl"
local converted = text.convert["[red]Danger"]
```

`text.init(bundle_file)` 使用项目实际 icon sprite bundle 初始化。路径不是固定约定。

常见 tag：

- `[icon_name]`：插入 icon bundle 中的 icon。
- `[RRGGBB]`：切换颜色。
- `[red]`、`[green]`、`[blue]`、`[white]`、`[black]`、`[aqua]`、`[yellow]`、`[pink]`、`[gray]`：命名颜色。
- `[cRRGGBB]`：自定义 RGB。
- `[[`：字面量左中括号。
