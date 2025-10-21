# Getting Started with Soluna

This guide will help you get started with Soluna game engine development.

## Getting Soluna

Download pre-built binaries from the [Nightly Releases](https://github.com/yuchanns/soluna/releases/tag/nightly).

Extract the downloaded archive and you'll find the `soluna` executable.

## Your First Soluna Program

### Hello World

Create a file `hello.lua`:

```lua
print("Hello World")
```

Run it:

```bash
soluna entry=hello.lua
```

### Creating a Window

Create a file `window.lua`:

```lua
local soluna = require "soluna"

-- Set window title
soluna.set_window_title("My First Game")

-- Define callback functions
local callback = {}

function callback.frame(count)
    -- This function is called every frame
    -- count is the frame number
end

return callback
```

Run it:

```bash
soluna entry=window.lua
```

### Using a Game Configuration File

Create `mygame.game`:

```
title : My First Game
width : 1280
height : 720
entry : main.lua
```

Create `main.lua`:

```lua
local soluna = require "soluna"

local callback = {}

function callback.frame(count)
    -- Your game logic here
end

function callback.key(keycode, state)
    -- Handle keyboard input
    -- state: 0=release, 1=press, 2=repeat
    if state == 1 then
        print("Key pressed:", keycode)
    end
end

return callback
```

Run your game:

```bash
soluna mygame.game
```

## Project Structure

A typical Soluna project structure:

```
mygame/
├── mygame.game          # Game configuration
├── main.lua             # Entry point
├── asset/               # Game assets
│   ├── sprites.dl       # Sprite definitions
│   ├── images/          # Image files
│   └── fonts/           # Font files
└── scripts/             # Game scripts
    ├── player.lua
    ├── enemy.lua
    └── ui.lua
```

## Loading Sprites

### Define Sprite Bundle

Create `asset/sprites.dl`:

```
sprite1 :
    filename : sprite1.png
    x : 0
    y : 0
```

### Load and Display Sprites

```lua
local soluna = require "soluna"

soluna.set_window_title("Sprite Example")

-- Load sprite bundle
local sprites = soluna.load_sprites("asset/sprites.dl")

local callback = {}
local args = ...
local batch = args.batch

function callback.frame(count)
    -- Draw sprite at center of screen
    local x = args.width / 2
    local y = args.height / 2
    batch:add(sprites.sprite1, x, y, 1, 0)
end

return callback
```

## Basic Text Rendering

```lua
local soluna = require "soluna"
local font = require "soluna.font"
local mattext = require "soluna.material.text"

local args = ...
local batch = args.batch

-- Initialize font
local sysfont = require "soluna.font.system"
font.import(sysfont.ttfdata("Arial"))
local fontid = font.name("")
local fontcobj = font.cobj()

-- Create text block
local block, cursor = mattext.block(fontcobj, fontid, 32, 0xffffff, "CV")
local label = block("Hello, Soluna!", 200, 50)

local callback = {}

function callback.frame(count)
    batch:add(label, 100, 100)
end

return callback
```

## Handling Input

### Keyboard Input

```lua
function callback.key(keycode, state)
    -- state: 0=release, 1=press, 2=repeat
    if state == 1 then
        if keycode == 256 then  -- ESC
            -- Exit game
        elseif keycode == 32 then  -- SPACE
            -- Jump
        end
    end
end
```

### Mouse Input

```lua
function callback.mouse_button(button, state, x, y)
    -- button: 0=left, 1=right, 2=middle
    -- state: 0=release, 1=press
    if button == 0 and state == 1 then
        print("Clicked at:", x, y)
    end
end

function callback.mouse_move(x, y)
    -- Mouse moved to (x, y)
end
```



## Using Layout System

```lua
local layout = require "soluna.layout"
local datalist = require "soluna.datalist"

-- Define layout
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

-- Calculate layout
local elements = layout.calc(dom)

-- Draw layout
for _, obj in ipairs(elements) do
    if obj.background then
        local quad = matquad.quad(obj.w, obj.h, obj.background)
        batch:add(quad, obj.x, obj.y)
    end
end
```

## Next Steps

- Check out [Examples](examples.md) for more advanced examples
- Refer to the API documentation in [soluna.lua](../soluna.lua)

## Common Keycodes

- ESC: 256
- SPACE: 32
- Enter: 257
- Arrow Left: 263
- Arrow Right: 262
- Arrow Up: 265
- Arrow Down: 264
- A-Z: 65-90
- 0-9: 48-57

## Troubleshooting

### Runtime Errors

- Check that all asset paths are correct
- Ensure fonts are properly loaded before use

For more help, refer to the examples in the `test/` directory.
