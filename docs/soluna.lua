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
--- Loads a layout definition from a file
---
---@param filename string Path to layout definition file
---@param scripts? table Script resolver table
---@return table document Layout document object
function layout.load(filename, scripts) end

---
--- Calculates layout positions and dimensions
---
---@param document table Layout document from layout.load()
---@return table[] elements Array of element objects with x, y, w, h fields
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
---@param fontcobj userdata Font system C object from font.cobj()
---@param fontid integer Font ID from font.name()
---@param size integer Font size in pixels
---@param color integer Text color (RGBA hex, 0 for default white)
---@param alignment string Alignment code ("L","C","R","T","V","B","H", e.g., "CV" for center)
---@return fun(text: string, width: integer, height: integer): userdata block_function Text block creator
---@return fun(text: string, position: integer, width: integer, height: integer): integer, integer, integer, integer, integer cursor_function Cursor position calculator
function mattext.block(fontcobj, fontid, size, color, alignment) end

---@class soluna.material.quad
local matquad = {}

---
--- Creates a colored rectangle sprite
---
---@param width integer Rectangle width
---@param height integer Rectangle height
---@param color integer Color in RGBA hex format (0xRRGGBBAA)
---@return userdata sprite Sprite object for rendering
function matquad.quad(width, height, color) end

---@class soluna.material.mask
local matmask = {}

---@class soluna.text
local text = {}

---
--- Initializes the text system with an icon bundle
---
---@param bundle_file string Path to icon sprite bundle file
function text.init(bundle_file) end

---
--- Table that converts text strings with embedded icon tags and color codes
---
--- Supports:
--- - Icon embedding: [icon_name]
--- - Color codes: [FF0000] for RGB hex
--- - Named colors: [red], [green], [blue], [white], [black], etc.
--- - Custom hex: [c808080] for custom RGB
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
--- Resizes an image by a scale factor
---
---@param data userdata Image data
---@param width integer Source image width
---@param height integer Source image height
---@param scale number Scale factor (e.g., 0.5 for half size)
---@return userdata imagedata Resized image data
---@return integer width New width
---@return integer height New height
function image.resize(data, width, height, scale) end

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
---@param sprite userdata Sprite object
---@param x number X position
---@param y number Y position
---@param scale? number Scale factor (default: 1)
---@param rotation? number Rotation in radians (default: 0)
---@param color? integer Color tint (RGBA hex)
function batch:add(sprite, x, y, scale, rotation, color) end

---
--- Sets the render layer
---
---@param layer integer Layer index
function batch:layer(layer) end

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
