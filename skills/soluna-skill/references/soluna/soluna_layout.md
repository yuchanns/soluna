# Soluna Layout API

本文件覆盖 `soluna.layout` 的游戏侧用法。layout 用于把 `.dl` 布局数据转换成带位置和尺寸的元素列表。

## 加载 Layout

```lua
local layout = require "soluna.layout"

local document = layout.load "asset/layout/hud.dl"
```

也可以传入已解析的数据 table：

```lua
local datalist = require "soluna.datalist"
local layout = require "soluna.layout"
local file = require "soluna.file"

local data = datalist.parse(assert(file.load "asset/layout/hud.dl"))
local document = layout.load(data)
```

## Script Resolver

layout 支持传入 script resolver table，用于生成或替换布局片段。

```lua
local scripts = {}

function scripts.track(name)
	return {
		"title",
		{
			text = "Track " .. name,
			text_align = "C",
			size = 18,
			width = 120,
		},
	}
end

local document = layout.load("asset/layout/hud.dl", scripts)
```

## 计算布局

```lua
local elements = layout.calc(document)
```

返回元素包含计算后的：

- `x`
- `y`
- `w`
- `h`

常见绘制方式：

```lua
local matquad = require "soluna.material.quad"

for _, obj in ipairs(elements) do
	if obj.background then
		batch:add(matquad.quad(obj.w, obj.h, obj.background), obj.x, obj.y)
	end
end
```

## 更新布局数据

修改 document 后，应重新 `layout.calc(document)`。

```lua
document.screen.width = args.width
document.screen.height = args.height
local elements = layout.calc(document)
```

对频繁更新的 UI，缓存 layout document 和计算结果，只在窗口尺寸、语言、字体或布局输入变化时失效。
