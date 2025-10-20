# Soluna 快速开始

本指南将帮助你开始 Soluna 游戏引擎开发。

## 前置条件

- **Windows**: GCC (MinGW) 或 MSVC 编译器
- **macOS/Linux**: GCC 或 Clang 编译器
- **所有平台**: luamake 构建工具

## 构建 Soluna

Soluna 使用 luamake 作为构建系统。构建方法：

```bash
luamake
```

编译后的可执行文件将根据平台和构建模式放置在 `bin/` 目录中。

## 第一个 Soluna 程序

### Hello World

创建文件 `hello.lua`:

```lua
print("Hello World")
```

运行它：

```bash
bin/soluna.exe entry=hello.lua
```

### 创建窗口

创建文件 `window.lua`:

```lua
local soluna = require "soluna"

-- 设置窗口标题
soluna.set_window_title("我的第一个游戏")

-- 定义回调函数
local callback = {}

function callback.frame(count)
    -- 此函数每帧调用一次
    -- count 是帧号
end

return callback
```

运行它：

```bash
bin/soluna.exe entry=window.lua
```

### 使用游戏配置文件

创建 `mygame.game`:

```
title : 我的第一个游戏
width : 1280
height : 720
entry : main.lua
```

创建 `main.lua`:

```lua
local soluna = require "soluna"

local callback = {}

function callback.frame(count)
    -- 你的游戏逻辑
end

function callback.key(keycode, state)
    -- 处理键盘输入
    -- state: 0=释放, 1=按下, 2=重复
    if state == 1 then
        print("按键:", keycode)
    end
end

return callback
```

运行你的游戏：

```bash
bin/soluna.exe mygame.game
```

## 项目结构

典型的 Soluna 项目结构：

```
mygame/
├── mygame.game          # 游戏配置
├── main.lua             # 入口点
├── asset/               # 游戏资产
│   ├── sprites.dl       # 精灵定义
│   ├── images/          # 图像文件
│   └── fonts/           # 字体文件
└── scripts/             # 游戏脚本
    ├── player.lua
    ├── enemy.lua
    └── ui.lua
```

## 加载精灵

### 定义精灵包

创建 `asset/sprites.dl`:

```
sprite1 :
    filename : sprite1.png
    x : 0
    y : 0
```

### 加载和显示精灵

```lua
local soluna = require "soluna"

soluna.set_window_title("精灵示例")

-- 加载精灵包
local sprites = soluna.load_sprites("asset/sprites.dl")

local callback = {}
local args = ...
local batch = args.batch

function callback.frame(count)
    -- 在屏幕中心绘制精灵
    local x = args.width / 2
    local y = args.height / 2
    batch:add(sprites.sprite1, x, y, 1, 0)
end

return callback
```

## 基本文本渲染

```lua
local soluna = require "soluna"
local font = require "soluna.font"
local mattext = require "soluna.material.text"

local args = ...
local batch = args.batch

-- 初始化字体
local sysfont = require "soluna.font.system"
font.import(sysfont.ttfdata("微软雅黑"))
local fontid = font.name("")
local fontcobj = font.cobj()

-- 创建文本块
local block, cursor = mattext.block(fontcobj, fontid, 32, 0xffffff, "CV")
local label = block("你好，Soluna!", 200, 50)

local callback = {}

function callback.frame(count)
    batch:add(label, 100, 100)
end

return callback
```

## 处理输入

### 键盘输入

```lua
function callback.key(keycode, state)
    -- state: 0=释放, 1=按下, 2=重复
    if state == 1 then
        if keycode == 256 then  -- ESC
            -- 退出游戏
        elseif keycode == 32 then  -- 空格
            -- 跳跃
        end
    end
end
```

### 鼠标输入

```lua
function callback.mouse_button(button, state, x, y)
    -- button: 0=左键, 1=右键, 2=中键
    -- state: 0=释放, 1=按下
    if button == 0 and state == 1 then
        print("点击位置:", x, y)
    end
end

function callback.mouse_move(x, y)
    -- 鼠标移动到 (x, y)
end
```

## 使用布局系统

```lua
local layout = require "soluna.layout"
local datalist = require "soluna.datalist"

-- 定义布局
local layout_def = [[
id : container
width : 800
height : 600
direction : column
gap : 10
header :
    height : 60
    background : 0xff0000ff
content :
    flex : 1
    background : 0xff00ff00
]]

local dom = layout.load(datalist.parse_list(layout_def))

-- 计算布局
local elements = layout.calc(dom)

-- 绘制布局
for _, obj in ipairs(elements) do
    if obj.background then
        local quad = matquad.quad(obj.w, obj.h, obj.background)
        batch:add(quad, obj.x, obj.y)
    end
end
```

## 下一步

- 探索 [API 参考](api-reference.md)获取详细的 API 文档
- 查看[示例](examples.md)获取更高级的示例
- 学习 [Deep Future](https://github.com/cloudwu/deepfuture) 游戏源代码了解实际使用

## 常用按键码

- ESC: 256
- 空格: 32
- 回车: 257
- 左箭头: 263
- 右箭头: 262
- 上箭头: 265
- 下箭头: 264
- A-Z: 65-90
- 0-9: 48-57

## 故障排除

### 构建错误

如果遇到构建错误：

1. 确保安装了正确的编译器
2. 检查所有子模块是否已初始化：`git submodule update --init --recursive`
3. 清理并重新构建：`luamake rebuild`

### 运行时错误

- 检查所有资产路径是否正确
- 确保在使用前正确加载字体
- 验证所需的服务是否已初始化

更多帮助，请参考 `test/` 目录中的示例。
