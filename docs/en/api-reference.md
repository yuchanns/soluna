# Soluna API Reference

Complete API reference for the Soluna game engine.

## Core Module: `soluna`

The main module that provides core functionality.

```lua
local soluna = require "soluna"
```

### Functions

#### `soluna.platform`
Returns the current platform as a string.

**Returns:**
- `string`: One of `"windows"`, `"macos"`, `"linux"`, or `"wasm"`

**Example:**
```lua
if soluna.platform == "windows" then
    print("Running on Windows")
end
```

#### `soluna.version`
Returns the engine version string.

**Returns:**
- `string`: Version identifier (e.g., "001a1b2c3d")

#### `soluna.settings()`
Returns the game settings table.

**Returns:**
- `table`: Settings table containing configuration values

**Example:**
```lua
local settings = soluna.settings()
print("Project:", settings.project)
print("Width:", settings.width)
print("Height:", settings.height)
```

#### `soluna.set_window_title(text)`
Sets the window title.

**Parameters:**
- `text` (string): The new window title

**Example:**
```lua
soluna.set_window_title("My Awesome Game")
```

#### `soluna.set_icon(data)`
Sets the window icon.

**Parameters:**
- `data`: Icon image data

#### `soluna.gamedir([name])`
Returns the game data directory path.

**Parameters:**
- `name` (string, optional): Project name (defaults to settings.project)

**Returns:**
- `string`: Absolute path to game data directory

**Platform-specific locations:**
- Windows: `My Games/{name}/`
- macOS/Linux: `.local/share/{name}/`
- WASM: `persistent/games/{name}/`

**Example:**
```lua
local savedir = soluna.gamedir()
-- Write save file
local file = require "soluna.file"
file.save(savedir .. "save.dat", data)
```

#### `soluna.load_sprites(filename)`
Loads a sprite bundle from a file.

**Parameters:**
- `filename` (string): Path to sprite definition file (.dl format)

**Returns:**
- `table`: Sprite bundle table mapping sprite names to IDs

**Example:**
```lua
local sprites = soluna.load_sprites("asset/sprites.dl")
local player_sprite = sprites.player
```

#### `soluna.gamepad_init()`
Initializes gamepad support and returns the gamepad state table.

**Returns:**
- `table`: Gamepad state table updated every frame

**Gamepad State Fields:**
- `button_a`, `button_b`, `button_x`, `button_y` (boolean)
- `button_back`, `button_start` (boolean)
- `dpad_up`, `dpad_down`, `dpad_left`, `dpad_right` (boolean)
- `shoulder_left`, `shoulder_right` (boolean)
- `axis_left_x`, `axis_left_y` (number): Left stick position (-1 to 1)
- `axis_right_x`, `axis_right_y` (number): Right stick position (-1 to 1)
- `trigger_left`, `trigger_right` (number): Trigger pressure (0 to 1)

**Example:**
```lua
local gamepad = soluna.gamepad_init()

function callback.frame(count)
    if gamepad.button_a then
        player_jump()
    end
    player.x = player.x + gamepad.axis_left_x * 5
end
```

---

## Module: `soluna.layout`

Provides flexbox-style layout functionality using Yoga.

```lua
local layout = require "soluna.layout"
```

### Functions

#### `layout.load(filename_or_list, [scripts])`
Loads a layout definition from a file or table.

**Parameters:**
- `filename_or_list` (string|table): Layout definition file path or parsed list
- `scripts` (function, optional): Script resolver function

**Returns:**
- `document`: Layout document object

**Layout Properties:**
- `id` (string): Element identifier
- `width`, `height` (number): Fixed dimensions
- `flex` (number): Flex grow factor
- `direction` (string): `"row"` or `"column"`
- `gap` (number): Space between children
- `padding` (number): Inner padding
- `margin` (number): Outer margin
- `background` (number): Background color (RGBA hex)
- `image` (string): Background image sprite name
- `text` (string): Text content
- `region` (table): Clickable region definition

**Example:**
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
Calculates layout positions and dimensions.

**Parameters:**
- `document`: Layout document returned by `layout.load()`

**Returns:**
- `array`: Array of element objects with calculated positions

**Element Fields:**
- `x`, `y` (number): Position
- `w`, `h` (number): Dimensions
- `background` (number, optional): Background color
- `image` (string, optional): Image sprite name
- `text` (string, optional): Text content
- `region` (table, optional): Region definition

**Example:**
```lua
local elements = layout.calc(dom)

for _, obj in ipairs(elements) do
    if obj.background then
        batch:add(quad(obj.w, obj.h, obj.background), obj.x, obj.y)
    end
end
```

### Document Object

Returned by `layout.load()`.

#### `document[id]`
Access element by ID.

**Returns:**
- `element`: Element object or nil

**Example:**
```lua
local root = dom.root
root.width = 1024  -- Update width
```

### Element Object

Accessed via document.

#### `element:update(attr)`
Updates element attributes.

**Parameters:**
- `attr` (table): Attribute table

**Example:**
```lua
dom.header:update({ height = 120, background = 0xff0000ff })
```

#### `element:get()`
Gets element's calculated position and size.

**Returns:**
- `x`, `y`, `w`, `h` (numbers): Position and dimensions

**Example:**
```lua
local x, y, w, h = dom.header:get()
```

---

## Module: `soluna.font`

Font management and text rendering.

```lua
local font = require "soluna.font"
```

### Functions

#### `font.import(data)`
Imports a TrueType font.

**Parameters:**
- `data` (string): Raw TTF font data

**Example:**
```lua
local sysfont = require "soluna.font.system"
font.import(sysfont.ttfdata("Arial"))
```

#### `font.name(name)`
Gets font ID by name.

**Parameters:**
- `name` (string): Font name (empty string for last imported font)

**Returns:**
- `number`: Font ID

**Example:**
```lua
local fontid = font.name("")  -- Get last imported font
```

#### `font.cobj()`
Gets the font system C object.

**Returns:**
- `userdata`: Font system object

#### `font.texture_size`
Current font texture atlas size.

**Returns:**
- `number`: Texture size in pixels

#### `font.import_icon(bundle)`
Imports icon sprites for text rendering.

**Parameters:**
- `bundle`: Icon sprite bundle

---

## Module: `soluna.font.system`

System font access.

```lua
local sysfont = require "soluna.font.system"
```

### Functions

#### `sysfont.ttfdata(name)`
Loads system font data by name.

**Parameters:**
- `name` (string): Font name

**Returns:**
- `string`: TTF font data or nil

**Common Font Names:**
- Windows: "Arial", "Times New Roman", "Courier New", "微软雅黑"
- macOS: "Helvetica", "Times", "Courier"
- Linux: "DejaVu Sans", "Liberation Sans"

---

## Module: `soluna.material.text`

Text rendering materials.

```lua
local mattext = require "soluna.material.text"
```

### Functions

#### `mattext.block(fontcobj, fontid, size, color, alignment)`
Creates a text block renderer.

**Parameters:**
- `fontcobj`: Font system C object from `font.cobj()`
- `fontid` (number): Font ID from `font.name()`
- `size` (number): Font size in pixels
- `color` (number): Text color (RGBA hex, 0 for default white)
- `alignment` (string): Alignment code
  - `"L"` or `"T"`: Left/Top
  - `"C"`: Center
  - `"R"` or `"B"`: Right/Bottom
  - `"V"`: Vertical center
  - `"H"`: Horizontal center
  - Example: `"CV"` = Center horizontally, Vertical center

**Returns:**
- `block_function`: Function to create text blocks
- `cursor_function`: Function to calculate cursor position

**Example:**
```lua
local fontcobj = font.cobj()
local fontid = font.name("")
local block, cursor = mattext.block(fontcobj, fontid, 24, 0xffffffff, "LT")

-- Create text label
local label = block("Hello World", 200, 50)

-- Use in frame callback
function callback.frame(count)
    batch:add(label, 100, 100)
end
```

#### Block Function

The function returned by `mattext.block()`.

**Signature:** `block(text, width, height) -> sprite`

**Parameters:**
- `text` (string): Text to render
- `width`, `height` (number): Text box dimensions

**Returns:**
- Sprite object for rendering

#### Cursor Function

The second function returned by `mattext.block()`.

**Signature:** `cursor(text, position, width, height) -> x, y, w, h, actual_pos`

**Parameters:**
- `text` (string): Text content
- `position` (number): Cursor position (character index)
- `width`, `height` (number): Text box dimensions

**Returns:**
- `x`, `y` (number): Cursor position
- `w`, `h` (number): Cursor dimensions
- `actual_pos` (number): Clamped cursor position

---

## Module: `soluna.material.quad`

Colored rectangle rendering.

```lua
local matquad = require "soluna.material.quad"
```

### Functions

#### `matquad.quad(width, height, color)`
Creates a colored rectangle sprite.

**Parameters:**
- `width`, `height` (number): Rectangle dimensions
- `color` (number): Color in RGBA hex format (0xRRGGBBAA)

**Returns:**
- Sprite object for rendering

**Example:**
```lua
-- Red rectangle with 50% opacity
local red_box = matquad.quad(100, 50, 0xff000080)
batch:add(red_box, 10, 10)

-- Blue background
local bg = matquad.quad(800, 600, 0x0000ffff)
batch:add(bg, 0, 0)
```

---

## Module: `soluna.material.mask`

Mask rendering for sprite clipping.

```lua
local maskmat = require "soluna.material.mask"
```

---

## Module: `soluna.image`

Image loading and manipulation.

```lua
local image = require "soluna.image"
```

### Functions

#### `image.load(data)`
Loads an image from binary data.

**Parameters:**
- `data` (string): Image file data (PNG, JPG, etc.)

**Returns:**
- `data`, `width`, `height`: Image data and dimensions

#### `image.load_alpha(data)`
Loads an image with alpha channel processing.

**Parameters:**
- `data` (string): Image file data

**Returns:**
- `data`, `width`, `height`: Image data and dimensions

#### `image.new(width, height)`
Creates a new blank image.

**Parameters:**
- `width`, `height` (number): Image dimensions

**Returns:**
- `image`: Image object

#### `image.crop(data, width, height, x, y, w, h)`
Crops an image region.

**Parameters:**
- `data`: Image data
- `width`, `height` (number): Source image dimensions
- `x`, `y`, `w`, `h` (number): Crop rectangle

**Returns:**
- `x`, `y`, `w`, `h` (number): Actual cropped region

---

## Module: `soluna.file`

File I/O operations.

```lua
local file = require "soluna.file"
```

### Functions

#### `file.load(filename)`
Loads a file's contents.

**Parameters:**
- `filename` (string): File path

**Returns:**
- `string`: File contents or nil on error

**Example:**
```lua
local content = file.load("data/config.json")
if content then
    -- Parse and use content
end
```

#### `file.save(filename, data)`
Saves data to a file.

**Parameters:**
- `filename` (string): File path
- `data` (string): Data to write

**Returns:**
- `boolean`: Success status

#### `file.searchpath(name, path)`
Searches for a file in multiple paths.

**Parameters:**
- `name` (string): File name
- `path` (string): Search path (semicolon-separated)

**Returns:**
- `string`: Found file path or nil

---

## Module: `soluna.lfs`

File system operations.

```lua
local lfs = require "soluna.lfs"
```

### Functions

#### `lfs.mkdir(path)`
Creates a directory.

**Parameters:**
- `path` (string): Directory path

**Returns:**
- `boolean`: Success status

#### `lfs.personaldir()`
Returns the user's home directory.

**Returns:**
- `string`: Home directory path

---

## Module: `soluna.datalist`

Data serialization format parser.

```lua
local datalist = require "soluna.datalist"
```

### Functions

#### `datalist.parse(data)`
Parses datalist format data.

**Parameters:**
- `data` (string): Datalist format text

**Returns:**
- `table`: Parsed data structure

#### `datalist.parse_list(data)`
Parses datalist format as a list.

**Parameters:**
- `data` (string): Datalist format text

**Returns:**
- `array`: Parsed list structure

---

## Module: `ltask`

Multithreading and message passing.

```lua
local ltask = require "ltask"
```

### Functions

#### `ltask.call(service, method, ...)`
Makes a synchronous call to a service.

**Parameters:**
- `service`: Service address
- `method` (string): Method name
- `...`: Method arguments

**Returns:**
- Return values from the service method

#### `ltask.send(service, method, ...)`
Sends an asynchronous message to a service.

**Parameters:**
- `service`: Service address
- `method` (string): Method name
- `...`: Method arguments

#### `ltask.uniqueservice(name)`
Gets or creates a unique service instance.

**Parameters:**
- `name` (string): Service name

**Returns:**
- Service address

#### `ltask.queryservice(name)`
Queries for a service by name.

**Parameters:**
- `name` (string): Service name

**Returns:**
- Service address or nil

---

## Callback Functions

Your game script should return a table with callback functions.

### `callback.frame(count)`
Called every frame.

**Parameters:**
- `count` (number): Frame number

### `callback.key(keycode, state)`
Called on keyboard events.

**Parameters:**
- `keycode` (number): Key code
- `state` (number): 0=release, 1=press, 2=repeat

### `callback.mouse_button(button, state, x, y)`
Called on mouse button events.

**Parameters:**
- `button` (number): 0=left, 1=right, 2=middle
- `state` (number): 0=release, 1=press
- `x`, `y` (number): Mouse position

### `callback.mouse_move(x, y)`
Called on mouse movement.

**Parameters:**
- `x`, `y` (number): Mouse position

### `callback.mouse_wheel(dx, dy)`
Called on mouse wheel scroll.

**Parameters:**
- `dx`, `dy` (number): Scroll delta

### `callback.window_resize(width, height)`
Called when window is resized.

**Parameters:**
- `width`, `height` (number): New window dimensions

### `callback.touch(id, phase, x, y)`
Called on touch events (mobile/tablet).

**Parameters:**
- `id` (number): Touch ID
- `phase` (string): Touch phase
- `x`, `y` (number): Touch position

---

## Arguments Table

The `args` table is passed to your entry script and contains:

- `args.width` (number): Current window width
- `args.height` (number): Current window height
- `args.batch`: Render batch object for drawing

**Example:**
```lua
local args = ...

function callback.frame(count)
    local center_x = args.width / 2
    local center_y = args.height / 2
    args.batch:add(sprite, center_x, center_y)
end
```

---

## Batch Object

The batch object is used for rendering sprites and primitives.

### Methods

#### `batch:add(sprite, x, y, [scale], [rotation], [color])`
Adds a sprite to the render batch.

**Parameters:**
- `sprite`: Sprite object (from sprite bundle, text block, or quad)
- `x`, `y` (number): Position
- `scale` (number, optional): Scale factor (default: 1)
- `rotation` (number, optional): Rotation in radians (default: 0)
- `color` (number, optional): Color tint (RGBA hex)

**Example:**
```lua
-- Draw at position with default scale and rotation
batch:add(sprite, 100, 100)

-- Draw scaled 2x
batch:add(sprite, 100, 100, 2)

-- Draw rotated 45 degrees
batch:add(sprite, 100, 100, 1, math.pi / 4)

-- Draw with red tint
batch:add(sprite, 100, 100, 1, 0, 0xff0000ff)
```

---

## Color Format

Colors in Soluna use 32-bit RGBA hexadecimal format: `0xRRGGBBAA`

- `RR`: Red channel (00-FF)
- `GG`: Green channel (00-FF)
- `BB`: Blue channel (00-FF)
- `AA`: Alpha channel (00-FF), where FF is opaque and 00 is transparent

**Examples:**
```lua
local red = 0xff0000ff      -- Solid red
local green = 0x00ff00ff    -- Solid green
local blue = 0x0000ffff     -- Solid blue
local white = 0xffffffff    -- Solid white
local black = 0x000000ff    -- Solid black
local transparent = 0x00000000  -- Fully transparent
local semi_red = 0xff000080     -- 50% transparent red
```

---

## Sprite Bundle Format (.dl)

Sprite bundles define sprite metadata in datalist format:

```
sprite_name :
    filename : image.png
    x : 0           # Offset X (optional)
    y : 0           # Offset Y (optional)
    cx : 0          # Crop start X
    cy : 0          # Crop start Y
    cw : 32         # Crop width
    ch : 32         # Crop height

multi_sprite :
    filename : spritesheet.png
    size : 32x32    # Size of each sprite
    number : 10     # Number of sprites in a row (or 5x2 for grid)
    gap : 1x1       # Gap between sprites (optional)
    x : 0           # Offset (optional)
    y : 0
```

**Multi-sprite Example:**
```
player_walk :
    filename : player.png
    size : 32x48
    number : 8      # 8 frames in a row
    gap : 2         # 2 pixel gap between frames
```

This creates an array `sprites.player_walk[1]` through `sprites.player_walk[8]`.

---

## Configuration File Format (.game)

Game configuration files use datalist format:

```
title : My Game
width : 1280
height : 720
entry : main.lua
project : mygame
max_sprite : 4096
texture_size : 2048
```

**Configuration Options:**
- `title` (string): Window title
- `width`, `height` (number): Window dimensions
- `entry` (string): Entry Lua script
- `project` (string): Project name for save data
- `max_sprite` (number): Maximum sprite count (default: 4096)
- `texture_size` (number): Texture atlas size (default: 2048)

---

For more examples and advanced usage, see the [Examples](examples.md) documentation and the [Deep Future](https://github.com/cloudwu/deepfuture) game source code.
