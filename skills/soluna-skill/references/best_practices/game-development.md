# Soluna 游戏开发参考

## 项目初始化

Soluna 游戏项目的最小单位是 `.game` 环境文件、入口 Lua 文件和资产目录。runtime 可以来自预编译二进制，也可以来自项目内的 Soluna 源码构建产物。

预编译 runtime 适合普通游戏项目：

```bash
soluna main.game
```

源码 runtime 适合固定 commit、从源码获得 runtime 或构建 wasm：

```bash
git submodule add https://github.com/cloudwu/soluna soluna
git submodule update --init --recursive
cd soluna
luamake
```

构建后仍然从游戏项目目录运行游戏自己的 `.game` 文件：

```bash
path/to/soluna main.game
```

## Runtime 模型

Soluna 游戏从入口 Lua 文件返回 callback table。引擎会调用这些 callback 处理 frame 渲染、键盘输入、鼠标输入、触摸输入、字符输入和窗口尺寸变化。API 细节见 `references/soluna/` 下按模块拆分的 reference。

基础游戏入口：

```lua
local soluna = require "soluna"
local app = require "soluna.app"
local matquad = require "soluna.material.quad"

local args = ...
local batch = args.batch

local KEY_ESCAPE <const> = 256
local KEYSTATE_PRESS <const> = 1
local COLOR_RED <const> = 0xffff3030

local rect = matquad.quad(48, 48, COLOR_RED)
local callback = {}

soluna.set_window_title "Minimal"

function callback.frame(count)
	batch:add(rect, 32, 32)
end

function callback.key(keycode, state)
	if keycode == KEY_ESCAPE and state == KEYSTATE_PRESS then
		app.quit()
	end
end

return callback
```

## 环境文件

游戏用 `.game` 文件声明入口与窗口配置。引擎有自己的默认环境；游戏项目只需要覆盖自身需要的项。

常用配置：

```text
entry : main.lua
project : mygame
service_path : "./?.lua"
width : 1024
height : 768
high_dpi : true
window_title : "My Game"
background : 0xff000000
```

启动方式：

```bash
soluna
soluna main.zip
soluna main.game
soluna entry=main.lua
soluna zipfile=/data/main.zip:/data/asset.zip
```

## 绘制

`args.batch` 接受 sprite ID、material userdata 和打包后的 draw stream。

```lua
local args = ...
local batch = args.batch

batch:add(sprite, x, y)
batch:layer(2, 100, 100)
batch:add(sprite)
batch:layer()
```

保持 layer 成对闭合。嵌套 transform 复杂时，优先写小 helper：

```lua
local function with_layer(batch, ...)
	batch:layer(...)
	return function()
		batch:layer()
	end
end

local close = with_layer(batch, 2, 100, 100)
batch:add(sprite)
close()
```

## Service Batch

可独立推进的特效、后台动画或调试覆盖层可以放到 service 中。service 可以创建自己的 batch，注册到 render service，然后提交 draw stream；主入口只负责发送事件和每帧状态。

```lua
local ltask = require "ltask"
local spritemgr = require "soluna.spritemgr"

local batches = {
	spritemgr.newbatch(),
	spritemgr.newbatch(),
}

local render
local batch_id
local current_index = 1
local current_batch = batches[current_index]
local ticking = false
local quit = false
local S = {}

function S.init()
	render = ltask.uniqueservice "render"
	batch_id = ltask.call(render, "register_batch", ltask.self())
	current_batch:reset()

	ltask.fork(function()
		while not quit do
			if not ticking then
				ticking = true
				ltask.send(ltask.self(), "tick")
			end
			ltask.call(render, "submit_batch", batch_id, current_batch:ptr())
		end
	end)
end

function S.tick()
	local index = current_index == 1 and 2 or 1
	local batch = batches[index]
	batch:reset()
	draw(batch)
	current_index = index
	current_batch = batch
	ticking = false
end

return S
```

如果 service 的 batch 需要位于主 batch 之后，不要在入口文件加载时立即初始化 service；在第一帧里执行一次 `init`，使 `register_batch` 发生在主 batch 注册之后。

```lua
local ltask = require "ltask"

local worker = ltask.spawn "worker"
local init

function init()
	ltask.call(worker, "init")
	init = nil
end

function callback.frame(count)
	if init then
		init()
	end
	ltask.send(worker, "frame", {
		count = count,
	})
end
```

service 提交 batch 时优先使用双 batch。一个 batch 交给 render service 等待消费时，service 在另一个 batch 上构建下一帧 draw stream，避免提交中的 batch 被继续写入。

## Quad 与文本

稳定的 quad 和 text sprite 应缓存。每帧重复创建 material object、格式化缓存 key 或拼接大量字符串都是可避免的开销。

```lua
local matquad = require "soluna.material.quad"
local mattext = require "soluna.material.text"
local font = require "soluna.font"
local file = require "soluna.file"

local quad_cache = {}
local text_cache = {}

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

local function label(cache_key, block, text, width, height)
	local cached = text_cache[cache_key]
	if cached == nil then
		cached = block(text, width, height)
		text_cache[cache_key] = cached
	end
	return cached
end

font.import(assert(file.load "asset/font/GameFont.ttf"))
local fontid = assert(font.name "Game Font")
local block = mattext.block(font.cobj(), fontid, 16, 0xffffffff, "LT")
```

wasm 字体加载模式：

```lua
local soluna = require "soluna"
local font = require "soluna.font"
local file = require "soluna.file"

local function init_font()
	if soluna.platform == "wasm" then
		local data = assert(file.load "asset/font/GameFont.ttf")
		font.import(data)
		return assert(font.name "Game Font")
	end

	local sysfont = require "soluna.font.system"
	for _, name in ipairs { "Arial", "Helvetica", "Microsoft YaHei" } do
		local ok, data = pcall(sysfont.ttfdata, name)
		if ok and data then
			font.import(data)
			local id = font.name(name)
			if id then
				return id
			end
		end
	end

	error "No available font"
end
```

## Sprite 与资产

sprite bundle 在初始化时加载一次。路径由项目自己的资产组织决定：

```lua
local soluna = require "soluna"

local sprites = soluna.load_sprites "asset/sprites.dl"

function callback.frame()
	batch:add(sprites.player, 320, 240)
end
```

图标可以从 image 数据生成：

```lua
local soluna = require "soluna"
local image = require "soluna.image"
local file = require "soluna.file"

local data, width, height = image.load(assert(file.load "asset/icon.png"))
local small_data, small_width, small_height = image.resize(data, width, height, 0.25)

soluna.set_icon {
	{ data = data, width = width, height = height },
	{ data = small_data, width = small_width, height = small_height },
}
```

## Layout 与文本转换

较大的 UI 优先使用 `.dl` layout 文件，并向 `layout.load` 传入 script resolver。

```lua
local layout = require "soluna.layout"
local matquad = require "soluna.material.quad"

local document = layout.load("asset/layout/hud.dl", scripts.hud)
local elements = layout.calc(document)

for _, obj in ipairs(elements) do
	if obj.background then
		batch:add(matquad.quad(obj.w, obj.h, obj.background), obj.x, obj.y)
	end
end
```

text tag 或内嵌图标需要先用项目实际的 icon sprite bundle 初始化。下面的路径只是示例：

```lua
local text = require "soluna.text"

text.init "asset/icons.dl"
local converted = text.convert["[red]Danger"]
```

## 游戏 Zip 打包

Soluna 游戏可以发布为 `main.zip`。打包是项目构建步骤，优先使用系统工具或项目自己的构建脚本，不需要在游戏运行时调用引擎 zip API。按需要包含入口文件、`.game` 文件、Lua 模块、资产、localization、service 代码和 license 文件。

```bash
zip -qr main.zip \
  main.game \
  main.lua \
  asset \
  game \
  LICENSE
```

按项目实际目录调整文件列表。确认 zip 内路径与运行时加载路径一致：

```bash
unzip -l main.zip
soluna main.zip
```

## 验证

优先使用用户已有的 Soluna 二进制或项目脚本验证游戏：

```bash
soluna main.game
```

如果项目通过源码 runtime 获取二进制，则先按项目记录的构建命令更新 runtime，再运行游戏自己的 `.game` 文件。
