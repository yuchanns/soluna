# Lua 最佳实践参考

本文总结 Soluna 游戏的 Lua 最佳实践，重点是可维护的模块边界、状态组织、输入归属、渲染分层和数据驱动 UI。

## 模块边界

较大游戏应按职责拆分模块。下面是常见职责示例，目录名不构成约定，实际项目可以按自己的领域命名：

- 输入：键盘、鼠标、触摸、focus、click、gesture 状态。
- 规则：游戏规则、结算、可行动作判断、持久化数据结构。
- 状态流：游戏阶段、流程迁移、多帧交互。
- 渲染：draw list、动画、UI region、widget、tips、button。
- 资产：sprite、layout datalist、gameplay datalist、字体、音频。
- 本地化：语言数据、文本转换、字体选择。
- service：按需放置后台服务。

入口文件负责组装模块并返回 callback，不应承载细节规则。下面的模块名只是占位示例，实际项目应使用自己的命名。

```lua
local soluna = require "soluna"
local flow = require "game.flow"
local renderer = require "game.renderer"
local mouse = require "game.input.mouse"
local keyboard = require "game.input.keyboard"

local args = ...
local callback = {}

renderer.init {
	batch = args.batch,
	width = args.width,
	height = args.height,
}

function callback.frame(count)
	local x, y = mouse.sync(count)
	renderer.set_mouse(x, y)
	flow.update()
	renderer.draw(count)
	mouse.frame()
end

keyboard.setup(callback)

return callback
```

## Require 与 Global

`require` 放在文件顶部。高频标准库绑定为 local。

```lua
local cards = require "game.rules.cards"
local flow = require "game.flow"
local util = require "game.util"
local table = table
local math = math

global pairs, ipairs, assert, error, tostring
```

`global` 声明用于记录有意使用的全局依赖。新增或移除全局使用时，应保持它准确。

## 常量

模块常量使用大写名称。runtime 支持且值不可变时使用 `<const>`。

```lua
local DURATION <const> = 30
local INV_DURATION <const> = 1 / DURATION
local KEY_ESCAPE <const> = 256
```

## 状态机

较复杂的 Soluna gameplay 可以建模为 coroutine 驱动的命名状态。这样可以让跨多帧 UI 交互保持可读，同时仍然逐帧推进。

```lua
local coroutine = require "soluna.coroutine"

local flow = {}
local STATE
local CURRENT = {
	state = nil,
	thread = nil,
}

function flow.load(states)
	STATE = states
end

function flow.enter(state, args)
	local f = STATE[state] or error("Missing state " .. tostring(state))
	CURRENT.state = state
	CURRENT.thread = coroutine.create(function()
		local next_state, next_args = f(args)
		return "NEXT", next_state, next_args
	end)
end

function flow.sleep(tick)
	coroutine.yield("SLEEP", tick)
end
```

当游戏逻辑需要多帧决策、动画、等待或输入循环时，使用这种风格。小型 arcade 示例用 `callback.frame` 中的简单 `update_game()` 通常更合适。

## 输入归属

callback 只负责归一化原始引擎事件，并把事件转发给输入模块。

```lua
local mouse_btn = {
	[0] = "left",
	[1] = "right",
	[2] = "mid",
}

function callback.mouse_move(x, y)
	mouse.mouse_move(x, y)
end

function callback.mouse_button(btn, state)
	mouse.mouse_button(mouse_btn[btn], state == 1)
end
```

focus、click、press、gesture 等状态应集中在输入模块中，不要散落在规则、渲染或流程模块中。

## 缓存模式

对 layout、draw list、quad、text label 等稳定派生对象，使用 lazy table cache。

```lua
local function cache(create)
	local meta = {}
	function meta:__index(key)
		local value = create(key)
		self[key] = value
		return value
	end
	return setmetatable({}, meta)
end

local doms = cache(function(name)
	return layout.load("asset/layout/" .. name .. ".dl", scripts[name])
end)
```

输入变化时只失效受影响的 cache。

```lua
function widget.set(dom, attribs)
	local d = doms[dom]
	layout_pos[dom] = nil
	for id, patch in pairs(attribs) do
		local obj = d[id]
		for key, value in pairs(patch) do
			obj[key] = value
		end
	end
end
```

## 数据驱动 UI

优先使用 `.dl` layout 和 script resolver，而不是在 Lua 中硬编码所有 UI 坐标。

```lua
local scripts = {}

function scripts.track(name)
	return {
		"name",
		{
			text = "hud." .. name .. ".logo",
			text_align = "C",
			size = 18,
			width = 24,
		},
	}
end
```

Lua 负责绑定行为、解析 localization key、组装 draw list。layout data 负责几何与稳定展示属性。

## 渲染模块

渲染模块通常暴露类似 lifecycle 的操作：

```lua
local M = {}

function M.init(args)
end

function M.flush(...)
end

function M.draw(count)
end

function M.change_font(font_id)
end

return M
```

card、map、track、button、tips、progress 等子系统应提供小而明确的函数来修改渲染状态。规则或流程代码不要在命名操作更清楚时直接改渲染内部状态。

## 流程模块

流程或阶段模块应返回函数或职责窄的 table。阶段函数可以用 `flow.sleep(0)` 等到下一帧，同时轮询输入。

```lua
local flow = require "game.flow"
local mouse = require "game.input.mouse"

return function(args)
	local focus_state = {}
	while true do
		if mouse.get(focus_state) then
			-- update hover feedback
		end
		local object, region = mouse.click(focus_state, "left")
		if object and region == "hand" then
			break
		end
		flow.sleep(0)
	end
	return flow.state.next
end
```

## 错误处理

对非法状态名、缺失资源和不可能规则，应尽早失败。

```lua
local f = STATE[state] or error("Missing state " .. tostring(state))
local value_card = card.draw_discard() or error "No cards in draw pile"
```

用 `assert` 表达必需的初始化顺序：

```lua
assert(STATE, "Call flow.load() first")
assert(CURRENT.thread == nil, "Running state")
```

## 命名与格式

- local 变量和函数使用 `snake_case`。
- 模块 table 常用 `M`、领域名词或导出的领域名称。
- 常量使用大写。
- 优先使用领域含义清楚的 table literal，避免大型匿名 tuple。
- 注释保持少量，只解释规则存在的原因。
- 不要把规则决策藏进渲染代码；规则模块计算可能性，渲染模块负责绘制。

## 何时不要使用复杂架构

coroutine 状态机、渲染 region 系统和独立 UI 框架适合多阶段流程、复杂交互、动画等待、可复用界面组件或大量数据驱动布局。若游戏只有少量状态、输入简单、渲染对象有限，单入口 callback table 加少量 local helper function 更易维护。
