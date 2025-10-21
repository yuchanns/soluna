---@meta

---
--- Soluna Game Engine API Reference
---
--- This file documents the Soluna API using Lua meta annotations.
---

---@class soluna
local soluna = {}

---
--- Current platform identifier
---
---@type "windows"|"macos"|"linux"|"wasm"
soluna.platform = "windows"

---
--- API version number
---
---@type integer
soluna.version_api = 0

---
--- Returns the game settings table
---
---@return table settings Game configuration from .game file
function soluna.settings() end

---
--- Sets the window title
---
---@param text string The window title text
function soluna.set_window_title(text) end

---
--- Sets the window icon
---
---@param data table Array of icon data tables with {data=..., width=..., height=...}
function soluna.set_icon(data) end

---
--- Returns the game data directory path
---
---@param name? string Project name (defaults to settings.project)
---@return string path Absolute path to game data directory
function soluna.gamedir(name) end

---
--- Loads a sprite bundle from a file
---
---@param filename string Path to sprite definition file (.dl format)
---@return table sprites Sprite bundle mapping sprite names to IDs
function soluna.load_sprites(filename) end

---@class soluna.layout
local layout = {}

---
--- Loads a layout definition from a file or table
---
--- The filename_or_list parameter can be:
--- - A string: path to a .dl layout file (will be loaded and parsed)
--- - A table: pre-parsed datalist structure
---
--- The scripts parameter is optional and provides a function table for script resolution.
---
---@param filename_or_list string|table Layout definition file path or parsed list
---@param scripts? table Script resolver function table
---@return table document Layout document object with element access by ID
function layout.load(filename_or_list, scripts) end

---
--- Calculates layout positions and dimensions
---
--- Runs the Yoga layout calculation on the document and updates all element positions.
--- Returns an array of element objects, each with x, y, w, h fields set to calculated values.
---
---@param document table Layout document from layout.load()
---@return table[] elements Array of element objects with calculated x, y, w, h positions
function layout.calc(document) end

---@class soluna.font
local font = {}

---
--- Imports a TrueType font
---
---@param data string Raw TTF font data
function font.import(data) end

---
--- Gets font ID by name
---
---@param name string Font name (empty string for last imported font)
---@return integer fontid Font ID
function font.name(name) end

---
--- Gets the font system C object
---
---@return userdata fontcobj Font system object
function font.cobj() end

---@class soluna.font.system
local font_system = {}

---
--- Loads system font data by name
---
---@param name string Font name (e.g., "Arial", "微软雅黑")
---@return string? data TTF font data or nil if not found
function font_system.ttfdata(name) end

---@class soluna.material.text
local mattext = {}

---
--- Creates a text block renderer
---
--- Returns two functions:
--- 1. block(text, width, height) - creates a renderable text sprite
--- 2. cursor(text, position, width, height) - calculates cursor position in text
---
--- Color format: RGBA as 32-bit integer. If alpha channel (high byte) is 0, defaults to 0xFF (opaque).
--- Alignment codes (case-insensitive, can be combined):
---   Horizontal: L (left), C (center), R (right)
---   Vertical: T (top), V (center), B (bottom)
---   Examples: "LT" (left-top), "CV" (center-vertical), "RB" (right-bottom)
---
---@param fontcobj userdata Font system C object from font.cobj()
---@param fontid integer Font ID from font.name()
---@param size? integer Font size in pixels (default: 16)
---@param color? integer Text color (RGBA as 0xRRGGBBAA, default: 0xff000000 = opaque black)
---@param alignment? string Alignment code (default: no alignment)
---@return fun(text: string, width: integer, height: integer): userdata block_function Creates text sprite
---@return fun(text: string, position: integer, width: integer, height: integer): integer, integer, integer, integer, integer, integer cursor_function Returns x, y, w, h, actual_position, descent
function mattext.block(fontcobj, fontid, size, color, alignment) end

---@class soluna.material.quad
local matquad = {}

---
--- Creates a colored rectangle sprite
---
--- Color format: RGBA as 32-bit integer 0xRRGGBBAA.
--- If alpha channel (high byte) is 0, it defaults to 0xFF (opaque).
---
---@param width integer Rectangle width in pixels
---@param height integer Rectangle height in pixels
---@param color integer Color in RGBA format (0xRRGGBBAA, e.g., 0xFF0000FF for opaque red)
---@return userdata sprite Sprite object for rendering with batch:add()
function matquad.quad(width, height, color) end

---@class soluna.material.mask
local matmask = {}

---@class soluna.text
local text = {}

---
--- Initializes the text system with an icon bundle
---
--- Loads an icon sprite bundle and makes it available for embedding in text via text.convert.
--- The bundle is kept in memory to prevent garbage collection.
---
---@param bundle_file string Path to icon sprite bundle file (.dl format)
function text.init(bundle_file) end

---
--- Table that converts text strings with embedded icon tags and color codes
---
--- Usage: local converted = text.convert[original_text]
---
--- Supports:
--- - Icon embedding: [icon_name] - replaced with icon from bundle loaded with text.init()
--- - Color codes: [RRGGBB] - sets text color (RGB hex, e.g., [FF0000] for red)
--- - Named colors: [red], [green], [blue], [white], [black], [aqua], [yellow], [pink], [gray]
--- - Custom hex colors: [cRRGGBB] - custom RGB (e.g., [c808080] for gray)
--- - Escape brackets: [[ - literal bracket (replaced with [bracket] internally)
---
--- The table uses weak keys/values for memory efficiency.
---
---@type table<string, string>
text.convert = {}

---@class soluna.image
local image = {}

---
--- Loads an image from binary data
---
---@param data string Image file data (PNG, JPG, etc.)
---@return userdata imagedata Image data
---@return integer width Image width
---@return integer height Image height
function image.load(data) end

---
--- Resizes an image by scale factors
---
--- The image data can be either RGBA (4 channels) or grayscale (1 channel).
--- Size is validated: for RGBA data must be width*height*4, for grayscale must be width*height.
---
---@param data userdata Image data (external string from image.load)
---@param width integer Source image width
---@param height integer Source image height
---@param scale_x number Horizontal scale factor (e.g., 0.5 for half width)
---@param scale_y? number Vertical scale factor (default: same as scale_x)
---@return userdata imagedata Resized image data
---@return integer width New width (width * scale_x, rounded)
---@return integer height New height (height * scale_y, rounded)
function image.resize(data, width, height, scale_x, scale_y) end

---@class soluna.file
local file = {}

---
--- Loads a file's contents
---
---@param filename string File path
---@return string? content File contents or nil on error
function file.load(filename) end

---
--- Gets file attributes
---
---@param filename string File path
---@return table? attributes File attributes or nil
function file.attributes(filename) end

---
--- Checks if a local file exists
---
---@param filename string File path
---@return boolean exists True if file exists
function file.local_exist(filename) end

---
--- Loads a local file's contents
---
---@param filename string File path
---@return string? content File contents or nil
function file.local_load(filename) end

---
--- Iterates over directory entries
---
---@param path string Directory path
---@return function iterator Iterator function
function file.dir(path) end

---@class soluna.lfs
local lfs = {}

---
--- Gets file attributes
---
---@param filename string File path
---@return table? attributes File attributes or nil
function lfs.attributes(filename) end

---
--- Iterates over directory entries
---
---@param path string Directory path
---@return function iterator Iterator function
function lfs.dir(path) end

---@class soluna.datalist
local datalist = {}

---
--- Parses datalist format data
---
---@param data string Datalist format text
---@return table parsed Parsed data structure
function datalist.parse(data) end

---
--- Quotes a string for datalist format
---
---@param str string String to quote
---@return string quoted Quoted string
function datalist.quote(str) end

---@class Batch
local batch = {}

---
--- Adds a sprite to the render batch
---
--- The sprite can be:
--- - A sprite ID (number) from a loaded sprite bundle
--- - A material userdata (from mattext.block or matquad.quad)
--- - A string for batch commands
---
---@param sprite number|userdata|string Sprite ID, material object, or command string
---@param x? number X position (default: 0)
---@param y? number Y position (default: 0)
function batch:add(sprite, x, y) end

---
--- Creates or closes a transformation layer
---
--- Layers apply scale, rotation, and translation transformations to all sprites
--- added while the layer is active. Layers can be nested.
---
--- Usage:
--- - batch:layer() with no args: closes the current layer
--- - batch:layer(rotation): applies rotation only (scale=1, x=0, y=0)
--- - batch:layer(x, y): applies translation only (scale=1, rotation=0)
--- - batch:layer(scale, x, y): applies scale and translation (rotation=0)
--- - batch:layer(scale, rotation, x, y): applies all transformations
---
---@overload fun(self: Batch)
---@overload fun(self: Batch, rotation: number)
---@overload fun(self: Batch, x: number, y: number)
---@overload fun(self: Batch, scale: number, x: number, y: number)
---@param scale number Scale factor (cannot be 0)
---@param rotation number Rotation in radians
---@param x number X translation
---@param y number Y translation
function batch:layer(scale, rotation, x, y) end

---@class Args
---@field width integer Current window width
---@field height integer Current window height
---@field batch Batch Render batch object

---@class Callback
local callback = {}

---
--- Called every frame
---
---@param count integer Frame number
function callback.frame(count) end

---
--- Called on keyboard events
---
---@param keycode integer Key code
---@param state integer 0=release, 1=press, 2=repeat
function callback.key(keycode, state) end

---
--- Called on character input events
---
---@param char string UTF-8 character
function callback.char(char) end

---
--- Called on mouse button events
---
---@param button integer 0=left, 1=right, 2=middle
---@param state integer 0=release, 1=press
function callback.mouse_button(button, state) end

---
--- Called on mouse movement
---
---@param x integer Mouse X position
---@param y integer Mouse Y position
function callback.mouse_move(x, y) end

---
--- Called on mouse wheel scroll
---
---@param dx number Horizontal scroll delta
---@param dy number Vertical scroll delta
function callback.mouse_scroll(dx, dy) end

return soluna
