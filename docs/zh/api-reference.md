# Soluna API 参考

Soluna 游戏引擎的完整 API 参考。

## 核心模块：`soluna`

提供核心功能的主模块。

```lua
local soluna = require "soluna"
```

### 函数

#### `soluna.platform`
返回当前平台字符串。

**返回值:**
- `string`: `"windows"`, `"macos"`, `"linux"` 或 `"wasm"` 之一

**示例:**
```lua
if soluna.platform == "windows" then
    print("在 Windows 上运行")
end
```

#### `soluna.version`
返回引擎版本字符串。

**返回值:**
- `string`: 版本标识符 (例如 "001a1b2c3d")

#### `soluna.settings()`
返回游戏设置表。

**返回值:**
- `table`: 包含配置值的设置表

**示例:**
```lua
local settings = soluna.settings()
print("项目:", settings.project)
print("宽度:", settings.width)
print("高度:", settings.height)
```

#### `soluna.set_window_title(text)`
设置窗口标题。

**参数:**
- `text` (string): 新的窗口标题

**示例:**
```lua
soluna.set_window_title("我的超级游戏")
```

#### `soluna.set_icon(data)`
设置窗口图标。

**参数:**
- `data`: 图标图像数据

#### `soluna.gamedir([name])`
返回游戏数据目录路径。

**参数:**
- `name` (string, 可选): 项目名称（默认为 settings.project）

**返回值:**
- `string`: 游戏数据目录的绝对路径

**平台特定位置:**
- Windows: `My Games/{name}/`
- macOS/Linux: `.local/share/{name}/`
- WASM: `persistent/games/{name}/`

**示例:**
```lua
local savedir = soluna.gamedir()
-- 写入保存文件
local file = require "soluna.file"
file.save(savedir .. "save.dat", data)
```

#### `soluna.load_sprites(filename)`
从文件加载精灵包。

**参数:**
- `filename` (string): 精灵定义文件的路径 (.dl 格式)

**返回值:**
- `table`: 精灵包表，将精灵名称映射到 ID

**示例:**
```lua
local sprites = soluna.load_sprites("asset/sprites.dl")
local player_sprite = sprites.player
```

---

## 模块：`soluna.layout`

提供使用 Yoga 的类 flexbox 布局功能。

```lua
local layout = require "soluna.layout"
```

### 函数

#### `layout.load(filename_or_list, [scripts])`
从文件或表加载布局定义。

**参数:**
- `filename_or_list` (string|table): 布局定义文件路径或解析后的列表
- `scripts` (function, 可选): 脚本解析器函数

**返回值:**
- `document`: 布局文档对象

**布局属性:**
- `id` (string): 元素标识符
- `width`, `height` (number): 固定尺寸
- `flex` (number): 弹性增长因子
- `direction` (string): `"row"` 或 `"column"`
- `gap` (number): 子元素之间的间距
- `padding` (number): 内边距
- `margin` (number): 外边距
- `background` (number): 背景颜色 (RGBA 十六进制)
- `image` (string): 背景图像精灵名称
- `text` (string): 文本内容
- `region` (table): 可点击区域定义

**示例:**
```lua
local datalist = require "soluna.datalist"

local layout_def = [[
id : root
width : 800
height : 600
direction : column
header :
    height : 100
    background : 0xff0000ff
content :
    flex : 1
    direction : row
    sidebar :
        width : 200
        background : 0xff00ff00
    main :
        flex : 1
        background : 0xffff0000
]]

local dom = layout.load(datalist.parse_list(layout_def))
```

#### `layout.calc(document)`
计算布局位置和尺寸。

**参数:**
- `document`: 由 `layout.load()` 返回的布局文档

**返回值:**
- `array`: 包含计算后位置的元素对象数组

**元素字段:**
- `x`, `y` (number): 位置
- `w`, `h` (number): 尺寸
- `background` (number, 可选): 背景颜色
- `image` (string, 可选): 图像精灵名称
- `text` (string, 可选): 文本内容
- `region` (table, 可选): 区域定义

**示例:**
```lua
local elements = layout.calc(dom)

for _, obj in ipairs(elements) do
    if obj.background then
        batch:add(quad(obj.w, obj.h, obj.background), obj.x, obj.y)
    end
end
```

### 文档对象

由 `layout.load()` 返回。

#### `document[id]`
通过 ID 访问元素。

**返回值:**
- `element`: 元素对象或 nil

**示例:**
```lua
local root = dom.root
root.width = 1024  -- 更新宽度
```

### 元素对象

通过文档访问。

#### `element:update(attr)`
更新元素属性。

**参数:**
- `attr` (table): 属性表

**示例:**
```lua
dom.header:update({ height = 120, background = 0xff0000ff })
```

#### `element:get()`
获取元素计算后的位置和大小。

**返回值:**
- `x`, `y`, `w`, `h` (numbers): 位置和尺寸

**示例:**
```lua
local x, y, w, h = dom.header:get()
```

---

## 模块：`soluna.font`

字体管理和文本渲染。

```lua
local font = require "soluna.font"
```

### 函数

#### `font.import(data)`
导入 TrueType 字体。

**参数:**
- `data` (string): 原始 TTF 字体数据

**示例:**
```lua
local sysfont = require "soluna.font.system"
font.import(sysfont.ttfdata("微软雅黑"))
```

#### `font.name(name)`
通过名称获取字体 ID。

**参数:**
- `name` (string): 字体名称（空字符串表示最后导入的字体）

**返回值:**
- `number`: 字体 ID

**示例:**
```lua
local fontid = font.name("")  -- 获取最后导入的字体
```

#### `font.cobj()`
获取字体系统 C 对象。

**返回值:**
- `userdata`: 字体系统对象

#### `font.texture_size`
当前字体纹理图集大小。

**返回值:**
- `number`: 纹理大小（像素）

#### `font.import_icon(bundle)`
导入用于文本渲染的图标精灵。

**参数:**
- `bundle`: 图标精灵包

---

## 模块：`soluna.text`

文本转换和富文本图标嵌入。

```lua
local text = require "soluna.text"
```

### 函数

#### `text.init(bundle_file)`
使用图标包初始化文本系统。

**参数:**
- `bundle_file` (string): 图标精灵包文件路径

**示例:**
```lua
local text = require "soluna.text"
text.init("asset/icons.dl")
```

#### `text.convert`
转换带有嵌入图标标签和颜色代码的文本字符串的表。

**用法:**
```lua
local converted = text.convert["你好 [icon_name] 世界"]
```

文本转换支持：
- 图标嵌入：`[icon_name]` 从加载的包嵌入图标
- 颜色代码：`[FF0000]` 设置文本颜色（RGB 十六进制）
- 命名颜色：`[red]`、`[green]`、`[blue]`、`[white]`、`[black]` 等
- 自定义十六进制颜色：`[c808080]` 用于自定义 RGB 值

**示例:**
```lua
local text = require "soluna.text"
text.init("asset/icons.dl")

-- 带图标和颜色的文本
local label = "生命值: [heart_icon] [red]100[white]"
local converted = text.convert[label]
-- 使用 mattext.block 使用转换后的文本
```

---

## 模块：`soluna.font.system`

系统字体访问。

```lua
local sysfont = require "soluna.font.system"
```

### 函数

#### `sysfont.ttfdata(name)`
按名称加载系统字体数据。

**参数:**
- `name` (string): 字体名称

**返回值:**
- `string`: TTF 字体数据或 nil

**常见字体名称:**
- Windows: "Arial", "Times New Roman", "Courier New", "微软雅黑", "宋体"
- macOS: "Helvetica", "Times", "Courier", "PingFang SC"
- Linux: "DejaVu Sans", "Liberation Sans", "Noto Sans CJK SC"

---

## 模块：`soluna.material.text`

文本渲染材质。

```lua
local mattext = require "soluna.material.text"
```

### 函数

#### `mattext.block(fontcobj, fontid, size, color, alignment)`
创建文本块渲染器。

**参数:**
- `fontcobj`: 来自 `font.cobj()` 的字体系统 C 对象
- `fontid` (number): 来自 `font.name()` 的字体 ID
- `size` (number): 字体大小（像素）
- `color` (number): 文本颜色（RGBA 十六进制，0 表示默认白色）
- `alignment` (string): 对齐代码
  - `"L"` 或 `"T"`: 左/上
  - `"C"`: 居中
  - `"R"` 或 `"B"`: 右/下
  - `"V"`: 垂直居中
  - `"H"`: 水平居中
  - 示例: `"CV"` = 水平居中，垂直居中

**返回值:**
- `block_function`: 创建文本块的函数
- `cursor_function`: 计算光标位置的函数

**示例:**
```lua
local fontcobj = font.cobj()
local fontid = font.name("")
local block, cursor = mattext.block(fontcobj, fontid, 24, 0xffffffff, "LT")

-- 创建文本标签
local label = block("你好世界", 200, 50)

-- 在帧回调中使用
function callback.frame(count)
    batch:add(label, 100, 100)
end
```

#### 块函数

由 `mattext.block()` 返回的函数。

**签名:** `block(text, width, height) -> sprite`

**参数:**
- `text` (string): 要渲染的文本
- `width`, `height` (number): 文本框尺寸

**返回值:**
- 用于渲染的精灵对象

#### 光标函数

由 `mattext.block()` 返回的第二个函数。

**签名:** `cursor(text, position, width, height) -> x, y, w, h, actual_pos`

**参数:**
- `text` (string): 文本内容
- `position` (number): 光标位置（字符索引）
- `width`, `height` (number): 文本框尺寸

**返回值:**
- `x`, `y` (number): 光标位置
- `w`, `h` (number): 光标尺寸
- `actual_pos` (number): 限制后的光标位置

---

## 模块：`soluna.material.quad`

彩色矩形渲染。

```lua
local matquad = require "soluna.material.quad"
```

### 函数

#### `matquad.quad(width, height, color)`
创建彩色矩形精灵。

**参数:**
- `width`, `height` (number): 矩形尺寸
- `color` (number): RGBA 十六进制格式的颜色 (0xRRGGBBAA)

**返回值:**
- 用于渲染的精灵对象

**示例:**
```lua
-- 50% 不透明度的红色矩形
local red_box = matquad.quad(100, 50, 0xff000080)
batch:add(red_box, 10, 10)

-- 蓝色背景
local bg = matquad.quad(800, 600, 0x0000ffff)
batch:add(bg, 0, 0)
```

---

## 模块：`soluna.material.mask`

用于精灵裁剪的遮罩渲染。

```lua
local maskmat = require "soluna.material.mask"
```

---

## 模块：`soluna.image`

图像加载和处理。

```lua
local image = require "soluna.image"
```

### 函数

#### `image.load(data)`
从二进制数据加载图像。

**参数:**
- `data` (string): 图像文件数据 (PNG, JPG 等)

**返回值:**
- `data`, `width`, `height`: 图像数据和尺寸

#### `image.load_alpha(data)`
加载带有 alpha 通道处理的图像。

**参数:**
- `data` (string): 图像文件数据

**返回值:**
- `data`, `width`, `height`: 图像数据和尺寸

#### `image.resize(data, width, height, scale)`
按比例因子缩放图像。

**参数:**
- `data`: 图像数据
- `width`, `height` (number): 源图像尺寸
- `scale` (number): 缩放因子（例如，0.5 表示一半大小，0.25 表示四分之一大小）

**返回值:**
- `data`, `width`, `height`: 缩放后的图像数据和新尺寸

**示例:**
```lua
local file = require "soluna.file"
local c = file.load("asset/icon.png")
local data, w, h = image.load(c)
-- 创建一半大小的版本
local mid_data, mid_w, mid_h = image.resize(data, w, h, 0.5)
```

#### `image.new(width, height)`
创建新的空白图像。

**参数:**
- `width`, `height` (number): 图像尺寸

**返回值:**
- `image`: 图像对象

#### `image.crop(data, width, height, x, y, w, h)`
裁剪图像区域。

**参数:**
- `data`: 图像数据
- `width`, `height` (number): 源图像尺寸
- `x`, `y`, `w`, `h` (number): 裁剪矩形

**返回值:**
- `x`, `y`, `w`, `h` (number): 实际裁剪区域

---

## 模块：`soluna.file`

文件 I/O 操作。

```lua
local file = require "soluna.file"
```

### 函数

#### `file.load(filename)`
加载文件内容。

**参数:**
- `filename` (string): 文件路径

**返回值:**
- `string`: 文件内容，出错时返回 nil

**示例:**
```lua
local content = file.load("data/config.json")
if content then
    -- 解析和使用内容
end
```

#### `file.save(filename, data)`
将数据保存到文件。

**参数:**
- `filename` (string): 文件路径
- `data` (string): 要写入的数据

**返回值:**
- `boolean`: 成功状态

#### `file.searchpath(name, path)`
在多个路径中搜索文件。

**参数:**
- `name` (string): 文件名
- `path` (string): 搜索路径（分号分隔）

**返回值:**
- `string`: 找到的文件路径或 nil

---

## 模块：`soluna.lfs`

文件系统操作。

```lua
local lfs = require "soluna.lfs"
```

### 函数

#### `lfs.mkdir(path)`
创建目录。

**参数:**
- `path` (string): 目录路径

**返回值:**
- `boolean`: 成功状态

#### `lfs.personaldir()`
返回用户的主目录。

**返回值:**
- `string`: 主目录路径

---

## 模块：`soluna.datalist`

数据序列化格式解析器。

```lua
local datalist = require "soluna.datalist"
```

### 函数

#### `datalist.parse(data)`
解析 datalist 格式数据。

**参数:**
- `data` (string): Datalist 格式文本

**返回值:**
- `table`: 解析后的数据结构

#### `datalist.parse_list(data)`
将 datalist 格式解析为列表。

**参数:**
- `data` (string): Datalist 格式文本

**返回值:**
- `array`: 解析后的列表结构

---

## 回调函数

你的游戏脚本应返回一个包含回调函数的表。

### `callback.frame(count)`
每帧调用。

**参数:**
- `count` (number): 帧号

### `callback.key(keycode, state)`
键盘事件时调用。

**参数:**
- `keycode` (number): 键码
- `state` (number): 0=释放, 1=按下, 2=重复

### `callback.mouse_button(button, state, x, y)`
鼠标按钮事件时调用。

**参数:**
- `button` (number): 0=左键, 1=右键, 2=中键
- `state` (number): 0=释放, 1=按下
- `x`, `y` (number): 鼠标位置

### `callback.mouse_move(x, y)`
鼠标移动时调用。

**参数:**
- `x`, `y` (number): 鼠标位置

### `callback.mouse_wheel(dx, dy)`
鼠标滚轮滚动时调用。

**参数:**
- `dx`, `dy` (number): 滚动增量

### `callback.window_resize(width, height)`
窗口大小调整时调用。

**参数:**
- `width`, `height` (number): 新的窗口尺寸

### `callback.touch(id, phase, x, y)`
触摸事件时调用（移动设备/平板）。

**参数:**
- `id` (number): 触摸 ID
- `phase` (string): 触摸阶段
- `x`, `y` (number): 触摸位置

---

## 参数表

`args` 表被传递给你的入口脚本，包含：

- `args.width` (number): 当前窗口宽度
- `args.height` (number): 当前窗口高度
- `args.batch`: 用于绘制的渲染批次对象

**示例:**
```lua
local args = ...

function callback.frame(count)
    local center_x = args.width / 2
    local center_y = args.height / 2
    args.batch:add(sprite, center_x, center_y)
end
```

---

## 批次对象

批次对象用于渲染精灵和图元。

### 方法

#### `batch:add(sprite, x, y, [scale], [rotation], [color])`
向渲染批次添加精灵。

**参数:**
- `sprite`: 精灵对象（来自精灵包、文本块或四边形）
- `x`, `y` (number): 位置
- `scale` (number, 可选): 缩放因子（默认：1）
- `rotation` (number, 可选): 旋转角度（弧度）（默认：0）
- `color` (number, 可选): 颜色着色（RGBA 十六进制）

**示例:**
```lua
-- 以默认缩放和旋转在位置绘制
batch:add(sprite, 100, 100)

-- 绘制缩放 2x
batch:add(sprite, 100, 100, 2)

-- 绘制旋转 45 度
batch:add(sprite, 100, 100, 1, math.pi / 4)

-- 绘制红色着色
batch:add(sprite, 100, 100, 1, 0, 0xff0000ff)
```

---

## 颜色格式

Soluna 中的颜色使用 32 位 RGBA 十六进制格式：`0xRRGGBBAA`

- `RR`: 红色通道 (00-FF)
- `GG`: 绿色通道 (00-FF)
- `BB`: 蓝色通道 (00-FF)
- `AA`: Alpha 通道 (00-FF)，其中 FF 是不透明，00 是透明

**示例:**
```lua
local red = 0xff0000ff      -- 纯红色
local green = 0x00ff00ff    -- 纯绿色
local blue = 0x0000ffff     -- 纯蓝色
local white = 0xffffffff    -- 纯白色
local black = 0x000000ff    -- 纯黑色
local transparent = 0x00000000  -- 完全透明
local semi_red = 0xff000080     -- 50% 透明的红色
```

---

## 精灵包格式 (.dl)

精灵包以 datalist 格式定义精灵元数据：

```
sprite_name :
    filename : image.png
    x : 0           # 偏移 X（可选）
    y : 0           # 偏移 Y（可选）
    cx : 0          # 裁剪起始 X
    cy : 0          # 裁剪起始 Y
    cw : 32         # 裁剪宽度
    ch : 32         # 裁剪高度

multi_sprite :
    filename : spritesheet.png
    size : 32x32    # 每个精灵的大小
    number : 10     # 一行中的精灵数量（或 5x2 表示网格）
    gap : 1x1       # 精灵之间的间隙（可选）
    x : 0           # 偏移（可选）
    y : 0
```

**多精灵示例:**
```
player_walk :
    filename : player.png
    size : 32x48
    number : 8      # 一行 8 帧
    gap : 2         # 帧之间 2 像素间隙
```

这会创建数组 `sprites.player_walk[1]` 到 `sprites.player_walk[8]`。

---

## 配置文件格式 (.game)

游戏配置文件使用 datalist 格式：

```
title : 我的游戏
width : 1280
height : 720
entry : main.lua
project : mygame
max_sprite : 4096
texture_size : 2048
```

**配置选项:**
- `title` (string): 窗口标题
- `width`, `height` (number): 窗口尺寸
- `entry` (string): 入口 Lua 脚本
- `project` (string): 保存数据的项目名称
- `max_sprite` (number): 最大精灵数量（默认：4096）
- `texture_size` (number): 纹理图集大小（默认：2048）

---

更多示例和高级用法，请参见[示例](examples.md)文档和 [Deep Future](https://github.com/cloudwu/deepfuture) 游戏源代码。
