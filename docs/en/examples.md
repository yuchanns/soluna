# Soluna Examples and Tutorials

This document provides practical examples and tutorials for common game development tasks using Soluna.

## Table of Contents

1. [Basic Window](#basic-window)
2. [Sprite Rendering](#sprite-rendering)
3. [Text Rendering](#text-rendering)
4. [Layout System](#layout-system)
5. [Input Handling](#input-handling)
6. [Animation](#animation)
7. [UI Elements](#ui-elements)
8. [Saving and Loading Data](#saving-and-loading-data)
9. [Complete Mini-Game](#complete-mini-game)

---

## Basic Window

Create a simple window with a colored background.

**main.lua:**
```lua
local soluna = require "soluna"
local matquad = require "soluna.material.quad"

soluna.set_window_title("Basic Window")

local args = ...
local batch = args.batch

local callback = {}

function callback.frame(count)
    -- Draw blue background
    local bg = matquad.quad(args.width, args.height, 0x4488ffff)
    batch:add(bg, 0, 0)
end

return callback
```

---

## Sprite Rendering

Load and display sprites with movement.

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

soluna.set_window_title("Sprite Example")

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
    if state == 1 then  -- press
        if keycode == 265 then  -- UP
            player_y = player_y - speed
        elseif keycode == 264 then  -- DOWN
            player_y = player_y + speed
        elseif keycode == 263 then  -- LEFT
            player_x = player_x - speed
        elseif keycode == 262 then  -- RIGHT
            player_x = player_x + speed
        end
    end
end

return callback
```

---

## Text Rendering

Display text with custom fonts and formatting.

**main.lua:**
```lua
local soluna = require "soluna"
local font = require "soluna.font"
local mattext = require "soluna.material.text"
local matquad = require "soluna.material.quad"

soluna.set_window_title("Text Example")

local args = ...
local batch = args.batch

-- Load system font
local sysfont = require "soluna.font.system"
local fontdata = sysfont.ttfdata("Arial")
if not fontdata then
    fontdata = sysfont.ttfdata("DejaVu Sans")  -- Linux fallback
end

if fontdata then
    font.import(fontdata)
end

local fontid = font.name("")
local fontcobj = font.cobj()

-- Create text blocks with different alignments
local title_block = mattext.block(fontcobj, fontid, 48, 0xff0000ff, "CH")
local body_block = mattext.block(fontcobj, fontid, 24, 0x000000ff, "LT")

local title = title_block("Soluna Engine", 600, 80)
local body = body_block("This is a text rendering example.\nYou can render multi-line text\nwith different sizes and colors.", 600, 200)

local callback = {}

function callback.frame(count)
    -- Background
    local bg = matquad.quad(args.width, args.height, 0xffffffff)
    batch:add(bg, 0, 0)
    
    -- Title at top center
    batch:add(title, (args.width - 600) / 2, 50)
    
    -- Body below title
    batch:add(body, (args.width - 600) / 2, 150)
end

return callback
```

---

## Layout System

Create a responsive UI with the Yoga layout engine.

**main.lua:**
```lua
local soluna = require "soluna"
local layout = require "soluna.layout"
local datalist = require "soluna.datalist"
local matquad = require "soluna.material.quad"

soluna.set_window_title("Layout Example")

local args = ...
local batch = args.batch

-- Define layout
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

## Input Handling

Handle keyboard, mouse, and touch input.

**main.lua:**
```lua
local soluna = require "soluna"
local matquad = require "soluna.material.quad"
local font = require "soluna.font"
local mattext = require "soluna.material.text"

soluna.set_window_title("Input Example")

local args = ...
local batch = args.batch

-- Setup font
local sysfont = require "soluna.font.system"
font.import(sysfont.ttfdata("Arial") or sysfont.ttfdata("DejaVu Sans"))
local fontid = font.name("")
local fontcobj = font.cobj()
local text_block = mattext.block(fontcobj, fontid, 20, 0x000000ff, "LT")

-- State
local mouse_x, mouse_y = 0, 0
local mouse_pressed = false
local last_key = "None"
local clicks = {}

local callback = {}

function callback.frame(count)
    -- Background
    local bg = matquad.quad(args.width, args.height, 0xffffffff)
    batch:add(bg, 0, 0)
    
    -- Mouse cursor
    local cursor_color = mouse_pressed and 0xff0000ff or 0x000000ff
    local cursor = matquad.quad(10, 10, cursor_color)
    batch:add(cursor, mouse_x - 5, mouse_y - 5)
    
    -- Info text
    local info = string.format(
        "Mouse: (%d, %d) %s\nLast Key: %s\nClicks: %d",
        mouse_x, mouse_y,
        mouse_pressed and "PRESSED" or "",
        last_key,
        #clicks
    )
    local label = text_block(info, 400, 100)
    batch:add(label, 20, 20)
    
    -- Click markers
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
    if button == 0 then  -- Left button
        mouse_pressed = (state == 1)
        if state == 1 then
            table.insert(clicks, {x = x, y = y, frame = 0})  -- frame will be updated
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

## Animation

Create sprite animations.

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

soluna.set_window_title("Animation Example")

local sprites = soluna.load_sprites("asset/sprites.dl")
local args = ...
local batch = args.batch

local player_x = 400
local player_y = 300
local frame = 1
local frame_counter = 0
local frame_delay = 5  -- frames between animation updates

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
    
    -- Draw animated sprite
    batch:add(sprites.walk[frame], player_x, player_y)
end

return callback
```

---

## UI Elements

Create interactive UI elements.

**main.lua:**
```lua
local soluna = require "soluna"
local layout = require "soluna.layout"
local datalist = require "soluna.datalist"
local matquad = require "soluna.material.quad"
local font = require "soluna.font"
local mattext = require "soluna.material.text"

soluna.set_window_title("UI Example")

local args = ...
local batch = args.batch

-- Setup font
local sysfont = require "soluna.font.system"
font.import(sysfont.ttfdata("Arial") or sysfont.ttfdata("DejaVu Sans"))
local fontid = font.name("")
local fontcobj = font.cobj()
local text_block = mattext.block(fontcobj, fontid, 24, 0xffffffff, "CV")

-- Button state
local buttons = {
    {id = "btn1", x = 100, y = 100, w = 200, h = 50, text = "Button 1", hovered = false, pressed = false},
    {id = "btn2", x = 100, y = 170, w = 200, h = 50, text = "Button 2", hovered = false, pressed = false},
    {id = "btn3", x = 100, y = 240, w = 200, h = 50, text = "Button 3", hovered = false, pressed = false},
}

local mouse_x, mouse_y = 0, 0

local function point_in_rect(px, py, rx, ry, rw, rh)
    return px >= rx and px <= rx + rw and py >= ry and py <= ry + rh
end

local callback = {}

function callback.frame(count)
    -- Background
    local bg = matquad.quad(args.width, args.height, 0x2c3e50ff)
    batch:add(bg, 0, 0)
    
    -- Draw buttons
    for _, btn in ipairs(buttons) do
        -- Update hover state
        btn.hovered = point_in_rect(mouse_x, mouse_y, btn.x, btn.y, btn.w, btn.h)
        
        -- Button color
        local color
        if btn.pressed then
            color = 0x27ae60ff
        elseif btn.hovered then
            color = 0x3498dbff
        else
            color = 0x34495eff
        end
        
        -- Draw button background
        local button_bg = matquad.quad(btn.w, btn.h, color)
        batch:add(button_bg, btn.x, btn.y)
        
        -- Draw button border
        local border = matquad.quad(btn.w, 2, 0xffffffff)
        batch:add(border, btn.x, btn.y)
        batch:add(border, btn.x, btn.y + btn.h - 2)
        
        local border_v = matquad.quad(2, btn.h, 0xffffffff)
        batch:add(border_v, btn.x, btn.y)
        batch:add(border_v, btn.x + btn.w - 2, btn.y)
        
        -- Draw button text
        local label = text_block(btn.text, btn.w, btn.h)
        batch:add(label, btn.x, btn.y)
    end
end

function callback.mouse_move(x, y)
    mouse_x = x
    mouse_y = y
end

function callback.mouse_button(button, state, x, y)
    if button == 0 then  -- Left button
        if state == 1 then  -- Press
            for _, btn in ipairs(buttons) do
                if point_in_rect(x, y, btn.x, btn.y, btn.w, btn.h) then
                    print("Button clicked:", btn.text)
                    btn.pressed = true
                end
            end
        else  -- Release
            for _, btn in ipairs(buttons) do
                btn.pressed = false
            end
        end
    end
end

return callback
```

---

## Saving and Loading Data

Save and load game data.

**main.lua:**
```lua
local soluna = require "soluna"
local file = require "soluna.file"
local datalist = require "soluna.datalist"

soluna.set_window_title("Save/Load Example")

local args = ...

-- Get save directory
local savedir = soluna.gamedir()
local savefile = savedir .. "save.txt"

-- Game state
local score = 0
local level = 1

local function save_game()
    local save_data = string.format([[
score : %d
level : %d
]], score, level)
    
    file.save(savefile, save_data)
    print("Game saved to", savefile)
end

local function load_game()
    local data = file.load(savefile)
    if data then
        local parsed = datalist.parse(data)
        score = parsed.score or 0
        level = parsed.level or 1
        print("Game loaded:", "score=" .. score, "level=" .. level)
    else
        print("No save file found")
    end
end

-- Load on startup
load_game()

local callback = {}

function callback.frame(count)
    -- Game logic
    if count % 60 == 0 then
        score = score + 10
    end
end

function callback.key(keycode, state)
    if state == 1 then
        if keycode == 83 then  -- S key
            save_game()
        elseif keycode == 76 then  -- L key
            load_game()
        end
    end
end

return callback
```

---

## Complete Mini-Game

A simple collecting game with scoring and UI.

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
title : Collect Game
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

soluna.set_window_title("Collect Game")

local sprites = soluna.load_sprites("asset/sprites.dl")
local args = ...
local batch = args.batch

-- Setup font
local sysfont = require "soluna.font.system"
font.import(sysfont.ttfdata("Arial") or sysfont.ttfdata("DejaVu Sans"))
local fontid = font.name("")
local fontcobj = font.cobj()
local text_block = mattext.block(fontcobj, fontid, 32, 0xffffffff, "LT")

-- Game state
local player = {x = 400, y = 300, speed = 5}
local coins = {}
local score = 0
local game_time = 30 * 60  -- 30 seconds at 60 FPS

-- Spawn coins
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
    -- Background
    local bg = matquad.quad(args.width, args.height, 0x87ceebff)
    batch:add(bg, 0, 0)
    
    -- Update game timer
    game_time = math.max(0, game_time - 1)
    
    -- Draw coins
    for _, coin in ipairs(coins) do
        if coin.active then
            batch:add(sprites.coin, coin.x, coin.y)
            
            -- Check collision with player
            if check_collision(player.x - 16, player.y - 16, 32, 32,
                              coin.x - 8, coin.y - 8, 16, 16) then
                coin.active = false
                score = score + 10
                print("Score:", score)
            end
        end
    end
    
    -- Draw player
    batch:add(sprites.player, player.x - 16, player.y - 16)
    
    -- Draw UI
    local ui_bg = matquad.quad(args.width, 50, 0x80000000)
    batch:add(ui_bg, 0, 0)
    
    local score_text = string.format("Score: %d  Time: %d", score, math.ceil(game_time / 60))
    local label = text_block(score_text, 400, 50)
    batch:add(label, 20, 10)
    
    -- Game over
    if game_time == 0 then
        local gameover = text_block("GAME OVER!", 400, 100)
        batch:add(gameover, args.width / 2 - 200, args.height / 2 - 50)
    end
end

function callback.key(keycode, state)
    if state == 1 or state == 2 then  -- Press or repeat
        if keycode == 265 then  -- UP
            player.y = math.max(16, player.y - player.speed)
        elseif keycode == 264 then  -- DOWN
            player.y = math.min(args.height - 16, player.y + player.speed)
        elseif keycode == 263 then  -- LEFT
            player.x = math.max(16, player.x - player.speed)
        elseif keycode == 262 then  -- RIGHT
            player.x = math.min(args.width - 16, player.x + player.speed)
        end
    end
end

return callback
```

---

## More Examples

For more advanced examples, check out:

- **Test Directory**: The `test/` directory in the Soluna repository contains many example scripts
- **Deep Future**: [github.com/cloudwu/deepfuture](https://github.com/cloudwu/deepfuture) - A game built with Soluna

## Tips and Best Practices

1. **Use Batching**: Group draw calls for better performance
2. **Cache Resources**: Load sprites and fonts once, reuse them
3. **Optimize Layouts**: Recalculate layouts only when needed
4. **Handle Window Resize**: Implement `callback.window_resize`
5. **Test on Target Platform**: Test on the platform you're targeting

Happy game development with Soluna!
