# Soluna Material API

本文件覆盖 Soluna 内置 material：纯色 quad、mask、packed stream，以及绘制缓存建议。

## Quad

```lua
local matquad = require "soluna.material.quad"

local rect = matquad.quad(80, 24, 0xff30ff30)
batch:add(rect, 20, 20)
```

颜色使用 ARGB：`0xAARRGGBB`。

稳定 quad 应缓存：

```lua
local matquad = require "soluna.material.quad"
local quad_cache = {}

local function quad(width, height, color)
	local by_width = quad_cache[width]
	if by_width == nil then
		by_width = {}
		quad_cache[width] = by_width
	end
	local by_height = by_width[height]
	if by_height == nil then
		by_height = {}
		by_width[height] = by_height
	end
	local cached = by_height[color]
	if cached == nil then
		cached = matquad.quad(width, height, color)
		by_height[color] = cached
	end
	return cached
end
```

## Mask

`soluna.material.mask` 用一个颜色 tint/mask 处理已有 sprite。

```lua
local mask = require "soluna.material.mask"

local tinted = mask.mask(sprites.player, 0x80ffffff)
batch:add(tinted, 100, 100)
```

适合 hover、disabled、selection 等视觉状态。

## Packed Stream

`batch:add` 接受 material helper 返回的 packed stream string。stream 适合表示一个 material command 内部的多个 primitive，例如一个 quad 由 4 个 item 组成。

```lua
local stream = matquad.quad(80, 24, 0xff30ff30)
batch:add(stream, 20, 20)
```

稳定 stream 应缓存，尤其是每帧重复绘制的 quad、mask 或 text block。

## Batch Layer

material 创建绘制对象，最终都通过 batch 提交。

```lua
batch:layer(2, 100, 100)
batch:add(sprite)
batch:layer()
```

保持 layer 成对闭合。复杂嵌套应写 helper 或局部封装，避免漏闭合。
