---
name: soluna
description: Soluna 2D 游戏开发指南。创建或维护 Soluna 游戏项目、Lua gameplay 代码、渲染 callback、datalist 资产、Soluna API 使用、Soluna Lua 最佳实践、extlua C 插件、游戏 zip 打包、Soluna WebAssembly 部署时使用此 skill。修改运行在 Soluna runtime 上的游戏示例，排查 Soluna callback、batch rendering、font、sprite、layout file、C extension 或 wasm 浏览器部署问题时也应使用。
---

# Soluna 游戏开发

Soluna 是 Sokol + Lua 的 2D 游戏引擎。游戏由 Lua 源码和资产组成，可以从当前目录、`main.zip`、显式 zip 路径或 `.game` 环境文件加载。

## 初始化游戏项目

创建 Soluna 游戏时，先确定 runtime 来源，再生成最小可运行结构。

### 路径一：预编译 Runtime

适用于快速原型和普通游戏项目。

1. 获取当前平台的 `soluna` 二进制：`https://github.com/cloudwu/soluna/releases`
2. 将二进制放入项目工具目录，或确保它在 `PATH` 中。
3. 创建 `main.game` 和 `main.lua`。
4. 用 `.game` 文件启动游戏。

```text
mygame/
├── main.game
├── main.lua
└── asset/
```

```text
entry : main.lua
project : mygame
width : 640
height : 480
window_title : "My Game"
background : 0xff000000
```

```bash
soluna main.game
```

### 路径二：源码 Runtime

适用于需要固定 Soluna commit、从源码获得 runtime、构建 wasm runtime、或把 runtime 构建纳入项目流程的场景。

```bash
git submodule add https://github.com/cloudwu/soluna soluna
git submodule update --init --recursive
cd soluna
luamake
```

游戏代码仍然放在游戏项目中。构建完成后，用产出的 runtime 启动游戏环境文件：

```bash
path/to/soluna main.game
```

## 快速入口

按任务选择对应的 API reference：

- 入口、`.game`、callback、batch：`references/soluna/soluna_core.md`
- 应用控制、窗口、图标、IME：`references/soluna/soluna_app.md`
- 音频：`references/soluna/soluna_audio.md`
- 键盘、鼠标和触摸输入：`references/soluna/soluna_input.md`
- 文件、图片、sprite、datalist：`references/soluna/soluna_assets.md`
- 字体、文本、富文本转换：`references/soluna/soluna_text.md`
- quad、mask、packed stream 等内置 material：`references/soluna/soluna_material.md`
- Yoga layout 与数据驱动 UI：`references/soluna/soluna_layout.md`
- crypt、url、coroutine 等辅助模块：`references/soluna/soluna_utils.md`

游戏入口代码优先采用这个形状：

```lua
local soluna = require "soluna"
local app = require "soluna.app"
local matquad = require "soluna.material.quad"

local args = ...
local batch = args.batch

local KEY_ESCAPE <const> = 256
local KEYSTATE_PRESS <const> = 1

soluna.set_window_title "Example"

local callback = {}

function callback.frame(count)
	batch:add(matquad.quad(100, 100, 0xffff3030), 20, 20)
end

function callback.key(keycode, state)
	if keycode == KEY_ESCAPE and state == KEYSTATE_PRESS then
		app.quit()
	end
end

return callback
```

## 工作流

1. 先判断任务目标：初始化游戏项目、编写 gameplay/UI、加载资产、打包游戏，还是部署到 wasm。
2. 只加载当前需要的 reference：
   - 游戏项目初始化、运行与 API 模式：`references/best_practices/game-development.md`
   - 游戏开发 API：`references/soluna/*.md`
   - Lua 最佳实践与架构：`references/best_practices/lua-style.md`
   - extlua C 插件与自定义 material：`references/extlua/extlua.md`
   - WebAssembly 部署：`references/best_practices/wasm-deployment.md`
3. 修改前先看当前任务涉及的游戏代码；若需要对齐现有写法，再阅读同目录或同模块下的既有 Lua 文件，以及当前项目中可见的示例。
4. 不要在热路径里重复创建稳定对象。文本块、quad、sprite、layout document、派生 draw list 都应按稳定输入缓存。
5. callback handler 保持小而清晰：分发输入，更新状态，再通过 batch 渲染。
6. 验证方式跟随项目的 runtime 来源：预编译 runtime 直接运行 `.game`；源码 runtime 先按项目约定获得二进制，再运行 `.game`。

```bash
soluna main.game
```

如果用户使用 submodule 源码构建，按当前平台和构建模式调整二进制路径。

## 常见任务

| 任务 | Reference |
| --- | --- |
| 编写或审查 Soluna 游戏循环 | `references/best_practices/game-development.md` |
| 添加 quad、text、sprite 或 layer 渲染 | `references/best_practices/game-development.md` |
| 处理键盘、鼠标或触摸输入 | `references/soluna/soluna_input.md` |
| 播放游戏音效 | `references/soluna/soluna_audio.md` |
| 预加载运行时生成的 sprite | `references/soluna/soluna_assets.md` |
| 使用 Soluna Lua 模块、状态和 UI 最佳实践 | `references/best_practices/lua-style.md` |
| 开发或接入 extlua C 插件、extlua material | `references/extlua/extlua.md` |
| 将游戏打包为 `main.zip` | `references/best_practices/game-development.md` |
| 部署浏览器、GitHub Pages 或 wasm runtime | `references/best_practices/wasm-deployment.md` |

## 经验规则

- `callback.frame` 是更新与渲染心跳，会收到 frame count。
- `args.batch` 是主要绘制表面；使用 `batch:add(sprite, x, y)`，并保持 `batch:layer(...)` 成对闭合。
- 颜色使用 ARGB 整数，通常写成 `0xAARRGGBB`。
- sprite bundle 使用 `soluna.load_sprites` 加载，路径应来自项目自己的资产组织。
- 使用 tagged text 或内嵌图标前，先用项目实际的 icon sprite bundle 调用 `soluna.text.init`.
- wasm 上要打包字体并通过 `soluna.file` 加载，不要依赖系统字体。
- layout、sprite 声明、规则和 localization 优先放到数据驱动的 `.dl` 文件中。
- 运行时生成的 RGBA sprite 先用 `soluna.preload` 注册虚拟文件名，再用 `soluna.load_sprites` 建立 sprite name 映射。
- 自定义渲染 material 优先走 extlua material：`.game` 配置 `extlua_material` 和 `extlua_material_path`，插件 C 侧调用 `solunaapi_init`，Lua adapter 返回 `submit(ptr, n)` 与 `draw(ptr, n, tex)`。
- 可独立推进的特效或后台绘制可以放入 service，由 service 注册自己的 render batch；初始化时机影响 batch 顺序，双 batch 可避免提交中的 batch 被继续写入。
- 较大游戏应按职责拆分模块，例如输入、规则、状态流、渲染、资产和本地化；目录名跟随项目自身约定。

## 故障排查

| 现象 | 检查点 |
| --- | --- |
| `Can't load entry main.lua` | 确认 `.game` 的 `entry`、当前目录和 zip 内容。 |
| 空白屏幕 | 确认 `callback.frame` 返回 callback table，并向 `args.batch` 写入绘制对象。 |
| wasm 上文字缺失 | 打包 TTF，并通过 `soluna.font.import` 导入。 |
| sprite 缺失 | 确认实际 sprite bundle 已打包，路径和名称与代码中的 `soluna.load_sprites`、`sprites.name` 匹配。 |
| 浏览器 runtime 无法启动 | 检查 cross-origin isolation、`soluna.js`、`soluna.wasm`、runtime zip 路径和 `zipfile=` 参数。 |
