# Soluna 示例和教程

本文档提供了使用 Soluna 进行常见游戏开发任务的实用示例和教程。

## 目录

1. [基本窗口](#基本窗口)
2. [精灵渲染](#精灵渲染)
3. [文本渲染](#文本渲染)
4. [布局系统](#布局系统)
5. [输入处理](#输入处理)
6. [动画](#动画)
7. [UI 元素](#ui-元素)
8. [保存和加载数据](#保存和加载数据)
9. [完整小游戏](#完整小游戏)

---

## 基本窗口

创建一个带有彩色背景的简单窗口。

**main.lua:**
```lua
local soluna = require "soluna"
local matquad = require "soluna.material.quad"

soluna.set_window_title("基本窗口")

local args = ...
local batch = args.batch

local callback = {}

function callback.frame(count)
    -- 绘制蓝色背景
    local bg = matquad.quad(args.width, args.height, 0x4488ffff)
    batch:add(bg, 0, 0)
end

return callback
```

---

## 精灵渲染

加载和显示带有移动的精灵。

**sprites.dl:**
```
player :
    filename : player.png
    cx : 0
    cy : 0
    cw : 32
    ch : 48
```

**main.lua:**
```lua
local soluna = require "soluna"

soluna.set_window_title("精灵示例")

local sprites = soluna.load_sprites("asset/sprites.dl")
local args = ...
local batch = args.batch

local player_x = 100
local player_y = 100
local speed = 5

local callback = {}

function callback.frame(count)
    batch:add(sprites.player, player_x, player_y)
end

function callback.key(keycode, state)
    if state == 1 then  -- 按下
        if keycode == 265 then  -- 上
            player_y = player_y - speed
        elseif keycode == 264 then  -- 下
            player_y = player_y + speed
        elseif keycode == 263 then  -- 左
            player_x = player_x - speed
        elseif keycode == 262 then  -- 右
            player_x = player_x + speed
        end
    end
end

return callback
```

---

## 文本渲染

显示带有自定义字体和格式的文本。

**main.lua:**
```lua
local soluna = require "soluna"
local font = require "soluna.font"
local mattext = require "soluna.material.text"
local matquad = require "soluna.material.quad"

soluna.set_window_title("文本示例")

local args = ...
local batch = args.batch

-- 加载系统字体
local sysfont = require "soluna.font.system"
local fontdata = sysfont.ttfdata("微软雅黑")
if not fontdata then
    fontdata = sysfont.ttfdata("Noto Sans CJK SC")  -- Linux 备用
end

if fontdata then
    font.import(fontdata)
end

local fontid = font.name("")
local fontcobj = font.cobj()

-- 创建不同对齐方式的文本块
local title_block = mattext.block(fontcobj, fontid, 48, 0xff0000ff, "CH")
local body_block = mattext.block(fontcobj, fontid, 24, 0x000000ff, "LT")

local title = title_block("Soluna 引擎", 600, 80)
local body = body_block("这是一个文本渲染示例。\n你可以渲染多行文本\n使用不同的大小和颜色。", 600, 200)

local callback = {}

function callback.frame(count)
    -- 背景
    local bg = matquad.quad(args.width, args.height, 0xffffffff)
    batch:add(bg, 0, 0)
    
    -- 顶部居中标题
    batch:add(title, (args.width - 600) / 2, 50)
    
    -- 标题下方的正文
    batch:add(body, (args.width - 600) / 2, 150)
end

return callback
```

---

## 布局系统

使用 Yoga 布局引擎创建响应式 UI。

**main.lua:**
```lua
local soluna = require "soluna"
local layout = require "soluna.layout"
local datalist = require "soluna.datalist"
local matquad = require "soluna.material.quad"

soluna.set_window_title("布局示例")

local args = ...
local batch = args.batch

-- 定义布局
local layout_def = [[
id : root
direction : column
header :
    height : 80
    background : 0xff2c3e50
content :
    flex : 1
    direction : row
    sidebar :
        width : 200
        background : 0xff34495e
    main :
        flex : 1
        padding : 20
        background : 0xffecf0f1
footer :
    height : 60
    background : 0xff2c3e50
]]

local dom = layout.load(datalist.parse_list(layout_def))

local function calc_layout()
    dom.root.width = args.width
    dom.root.height = args.height
    return layout.calc(dom)
end

local elements = calc_layout()

local callback = {}

function callback.frame(count)
    for _, obj in ipairs(elements) do
        if obj.background then
            local quad = matquad.quad(obj.w, obj.h, obj.background)
            batch:add(quad, obj.x, obj.y)
        end
    end
end

function callback.window_resize(w, h)
    args.width = w
    args.height = h
    elements = calc_layout()
end

return callback
```

---

## 输入处理

处理键盘、鼠标和触摸输入。

**main.lua:**
```lua
local soluna = require "soluna"
local matquad = require "soluna.material.quad"
local font = require "soluna.font"
local mattext = require "soluna.material.text"

soluna.set_window_title("输入示例")

local args = ...
local batch = args.batch

-- 设置字体
local sysfont = require "soluna.font.system"
font.import(sysfont.ttfdata("微软雅黑") or sysfont.ttfdata("Noto Sans CJK SC"))
local fontid = font.name("")
local fontcobj = font.cobj()
local text_block = mattext.block(fontcobj, fontid, 20, 0x000000ff, "LT")

-- 状态
local mouse_x, mouse_y = 0, 0
local mouse_pressed = false
local last_key = "无"
local clicks = {}

local callback = {}

function callback.frame(count)
    -- 背景
    local bg = matquad.quad(args.width, args.height, 0xffffffff)
    batch:add(bg, 0, 0)
    
    -- 鼠标光标
    local cursor_color = mouse_pressed and 0xff0000ff or 0x000000ff
    local cursor = matquad.quad(10, 10, cursor_color)
    batch:add(cursor, mouse_x - 5, mouse_y - 5)
    
    -- 信息文本
    local info = string.format(
        "鼠标: (%d, %d) %s\n上次按键: %s\n点击: %d",
        mouse_x, mouse_y,
        mouse_pressed and "按下" or "",
        last_key,
        #clicks
    )
    local label = text_block(info, 400, 100)
    batch:add(label, 20, 20)
    
    -- 点击标记
    for i, click in ipairs(clicks) do
        local alpha = math.max(0, 255 - (count - click.frame) * 5)
        if alpha > 0 then
            local color = (alpha << 24) | 0x00ff00
            local marker = matquad.quad(20, 20, color)
            batch:add(marker, click.x - 10, click.y - 10)
        end
    end
end

function callback.mouse_move(x, y)
    mouse_x = x
    mouse_y = y
end

function callback.mouse_button(button, state, x, y)
    if button == 0 then  -- 左键
        mouse_pressed = (state == 1)
        if state == 1 then
            table.insert(clicks, {x = x, y = y, frame = 0})
        end
    end
end

function callback.key(keycode, state)
    if state == 1 then
        last_key = tostring(keycode)
    end
end

return callback
```

---

## 动画

创建精灵动画。

**sprites.dl:**
```
walk :
    filename : character_walk.png
    size : 32x48
    number : 8
    gap : 0
```

**main.lua:**
```lua
local soluna = require "soluna"

soluna.set_window_title("动画示例")

local sprites = soluna.load_sprites("asset/sprites.dl")
local args = ...
local batch = args.batch

local player_x = 400
local player_y = 300
local frame = 1
local frame_counter = 0
local frame_delay = 5  -- 动画更新之间的帧数

local callback = {}

function callback.frame(count)
    frame_counter = frame_counter + 1
    if frame_counter >= frame_delay then
        frame_counter = 0
        frame = frame + 1
        if frame > #sprites.walk then
            frame = 1
        end
    end
    
    -- 绘制动画精灵
    batch:add(sprites.walk[frame], player_x, player_y)
end

return callback
```

---

## UI 元素

创建交互式 UI 元素。

**main.lua:**
```lua
local soluna = require "soluna"
local matquad = require "soluna.material.quad"
local font = require "soluna.font"
local mattext = require "soluna.material.text"

soluna.set_window_title("UI 示例")

local args = ...
local batch = args.batch

-- 设置字体
local sysfont = require "soluna.font.system"
font.import(sysfont.ttfdata("微软雅黑") or sysfont.ttfdata("Noto Sans CJK SC"))
local fontid = font.name("")
local fontcobj = font.cobj()
local text_block = mattext.block(fontcobj, fontid, 24, 0xffffffff, "CV")

-- 按钮状态
local buttons = {
    {id = "btn1", x = 100, y = 100, w = 200, h = 50, text = "按钮 1", hovered = false, pressed = false},
    {id = "btn2", x = 100, y = 170, w = 200, h = 50, text = "按钮 2", hovered = false, pressed = false},
    {id = "btn3", x = 100, y = 240, w = 200, h = 50, text = "按钮 3", hovered = false, pressed = false},
}

local mouse_x, mouse_y = 0, 0

local function point_in_rect(px, py, rx, ry, rw, rh)
    return px >= rx and px <= rx + rw and py >= ry and py <= ry + rh
end

local callback = {}

function callback.frame(count)
    -- 背景
    local bg = matquad.quad(args.width, args.height, 0x2c3e50ff)
    batch:add(bg, 0, 0)
    
    -- 绘制按钮
    for _, btn in ipairs(buttons) do
        -- 更新悬停状态
        btn.hovered = point_in_rect(mouse_x, mouse_y, btn.x, btn.y, btn.w, btn.h)
        
        -- 按钮颜色
        local color
        if btn.pressed then
            color = 0x27ae60ff
        elseif btn.hovered then
            color = 0x3498dbff
        else
            color = 0x34495eff
        end
        
        -- 绘制按钮背景
        local button_bg = matquad.quad(btn.w, btn.h, color)
        batch:add(button_bg, btn.x, btn.y)
        
        -- 绘制按钮文本
        local label = text_block(btn.text, btn.w, btn.h)
        batch:add(label, btn.x, btn.y)
    end
end

function callback.mouse_move(x, y)
    mouse_x = x
    mouse_y = y
end

function callback.mouse_button(button, state, x, y)
    if button == 0 then  -- 左键
        if state == 1 then  -- 按下
            for _, btn in ipairs(buttons) do
                if point_in_rect(x, y, btn.x, btn.y, btn.w, btn.h) then
                    print("按钮点击:", btn.text)
                    btn.pressed = true
                end
            end
        else  -- 释放
            for _, btn in ipairs(buttons) do
                btn.pressed = false
            end
        end
    end
end

return callback
```

---

## 保存和加载数据

保存和加载游戏数据。

**main.lua:**
```lua
local soluna = require "soluna"
local file = require "soluna.file"
local datalist = require "soluna.datalist"

soluna.set_window_title("保存/加载示例")

local args = ...

-- 获取保存目录
local savedir = soluna.gamedir()
local savefile = savedir .. "save.txt"

-- 游戏状态
local score = 0
local level = 1

local function save_game()
    local save_data = string.format([[
score : %d
level : %d
]], score, level)
    
    file.save(savefile, save_data)
    print("游戏已保存到", savefile)
end

local function load_game()
    local data = file.load(savefile)
    if data then
        local parsed = datalist.parse(data)
        score = parsed.score or 0
        level = parsed.level or 1
        print("游戏已加载:", "score=" .. score, "level=" .. level)
    else
        print("未找到保存文件")
    end
end

-- 启动时加载
load_game()

local callback = {}

function callback.frame(count)
    -- 游戏逻辑
    if count % 60 == 0 then
        score = score + 10
    end
end

function callback.key(keycode, state)
    if state == 1 then
        if keycode == 83 then  -- S 键
            save_game()
        elseif keycode == 76 then  -- L 键
            load_game()
        end
    end
end

return callback
```

---

## 完整小游戏

一个简单的收集游戏，带有计分和 UI。

**sprites.dl:**
```
player :
    filename : player.png
    cx : 0
    cy : 0
    cw : 32
    ch : 32

coin :
    filename : coin.png
    cx : 0
    cy : 0
    cw : 16
    ch : 16
```

**game.game:**
```
title : 收集游戏
width : 800
height : 600
entry : main.lua
project : collectgame
```

**main.lua:**
```lua
local soluna = require "soluna"
local matquad = require "soluna.material.quad"
local font = require "soluna.font"
local mattext = require "soluna.material.text"

soluna.set_window_title("收集游戏")

local sprites = soluna.load_sprites("asset/sprites.dl")
local args = ...
local batch = args.batch

-- 设置字体
local sysfont = require "soluna.font.system"
font.import(sysfont.ttfdata("微软雅黑") or sysfont.ttfdata("Noto Sans CJK SC"))
local fontid = font.name("")
local fontcobj = font.cobj()
local text_block = mattext.block(fontcobj, fontid, 32, 0xffffffff, "LT")

-- 游戏状态
local player = {x = 400, y = 300, speed = 5}
local coins = {}
local score = 0
local game_time = 30 * 60  -- 30 秒，60 FPS

-- 生成硬币
for i = 1, 10 do
    table.insert(coins, {
        x = math.random(50, 750),
        y = math.random(50, 550),
        active = true
    })
end

local function check_collision(x1, y1, w1, h1, x2, y2, w2, h2)
    return x1 < x2 + w2 and x1 + w1 > x2 and
           y1 < y2 + h2 and y1 + h1 > y2
end

local callback = {}

function callback.frame(count)
    -- 背景
    local bg = matquad.quad(args.width, args.height, 0x87ceebff)
    batch:add(bg, 0, 0)
    
    -- 更新游戏计时器
    game_time = math.max(0, game_time - 1)
    
    -- 绘制硬币
    for _, coin in ipairs(coins) do
        if coin.active then
            batch:add(sprites.coin, coin.x, coin.y)
            
            -- 检查与玩家的碰撞
            if check_collision(player.x - 16, player.y - 16, 32, 32,
                              coin.x - 8, coin.y - 8, 16, 16) then
                coin.active = false
                score = score + 10
                print("得分:", score)
            end
        end
    end
    
    -- 绘制玩家
    batch:add(sprites.player, player.x - 16, player.y - 16)
    
    -- 绘制 UI
    local ui_bg = matquad.quad(args.width, 50, 0x80000000)
    batch:add(ui_bg, 0, 0)
    
    local score_text = string.format("得分: %d  时间: %d", score, math.ceil(game_time / 60))
    local label = text_block(score_text, 400, 50)
    batch:add(label, 20, 10)
    
    -- 游戏结束
    if game_time == 0 then
        local gameover = text_block("游戏结束!", 400, 100)
        batch:add(gameover, args.width / 2 - 200, args.height / 2 - 50)
    end
end

function callback.key(keycode, state)
    if state == 1 or state == 2 then  -- 按下或重复
        if keycode == 265 then  -- 上
            player.y = math.max(16, player.y - player.speed)
        elseif keycode == 264 then  -- 下
            player.y = math.min(args.height - 16, player.y + player.speed)
        elseif keycode == 263 then  -- 左
            player.x = math.max(16, player.x - player.speed)
        elseif keycode == 262 then  -- 右
            player.x = math.min(args.width - 16, player.x + player.speed)
        end
    end
end

return callback
```

---

## 更多示例

更多高级示例，请查看：

- **测试目录**: Soluna 仓库中的 `test/` 目录包含许多示例脚本
- **Deep Future**: [github.com/cloudwu/deepfuture](https://github.com/cloudwu/deepfuture) - 使用 Soluna 构建的游戏

## 提示和最佳实践

1. **使用批处理**: 将绘制调用分组以获得更好的性能
2. **缓存资源**: 一次加载精灵和字体，重复使用它们
3. **优化布局**: 仅在需要时重新计算布局
4. **处理窗口大小调整**: 实现 `callback.window_resize`
5. **在目标平台上测试**: 在目标平台上测试

使用 Soluna 愉快地开发游戏！
