# Soluna Engine Architecture

This document describes the internal architecture of the Soluna game engine.

## Overview

Soluna is built on a service-oriented architecture using the ltask library for multithreading and message passing. The engine separates concerns into different services that communicate asynchronously, allowing for efficient parallel processing.

## Core Components

### 1. Main Thread (Native)

The main thread runs the native application loop and handles:
- Window management and event processing
- Graphics context initialization (via Sokol)
- Input device polling
- Frame synchronization
- Event dispatching to the Lua runtime

**Key Files:**
- `src/entry.c`: Application entry point and main loop
- `src/appevent.h`: Event definitions

### 2. Bootstrap and Initialization

The bootstrap process:
1. Initializes the ltask runtime
2. Sets up the root service
3. Creates core services (timer, log, loader)
4. Loads embedded Lua code
5. Starts the game entry script

**Key Files:**
- `src/lualib/main.lua`: Bootstrap logic
- `src/embedlua.c`: Embedded Lua script management

### 3. Service Architecture

Services are independent Lua execution contexts that communicate via message passing.

#### Core Services

**Timer Service**
- Schedules delayed and periodic tasks
- Manages timeout events
- Part of ltask library

**Log Service**
- Centralizes logging output
- Thread-safe log message handling
- Part of ltask library

**Loader Service** (`src/service/loader.lua`)
- Asynchronous asset loading
- Sprite bundle management
- Texture atlas packing
- Image caching

**Render Service** (`src/service/render.lua`)
- Rendering pipeline management
- Material system
- Font texture management
- Batch submission coordination

**Settings Service** (`src/service/settings.lua`)
- Configuration management
- Game settings storage

**Gamepad Service** (`src/service/gamepad.lua`)
- Gamepad state polling
- Input state distribution

**Start Service** (`src/service/start.lua`)
- User game script execution
- Callback management
- Game loop coordination

## Rendering System

### Rendering Pipeline

1. **Batch Collection**: Game code adds sprites to batch via `batch:add()`
2. **Batch Submission**: Batches are submitted to render service
3. **Material Processing**: Different materials process their batches
4. **GPU Upload**: Vertex data uploaded to GPU
5. **Draw Calls**: Sokol renders the frame
6. **Font Updates**: Font texture atlas updated if needed

### Material System

Soluna uses a material-based rendering approach with four built-in materials:

**Default Material** (`src/material_default.c`)
- Basic textured sprite rendering
- UV mapping from texture atlas
- Color tinting support

**Text Material** (`src/material_text.c`)
- SDF (Signed Distance Field) text rendering
- Dynamic font atlas
- Smooth text at any scale

**Quad Material** (`src/material_quad.c`)
- Solid color rectangles
- UI elements and backgrounds
- No texture required

**Mask Material** (`src/material_mask.c`)
- Sprite masking and clipping
- Complex shape rendering

### Shader System

Shaders are written in GLSL and compiled to platform-specific formats using sokol-shdc:

**Shader Files:**
- `src/texquad.glsl`: Textured quad rendering
- `src/colorquad.glsl`: Colored quad rendering
- `src/sdftext.glsl`: SDF text rendering
- `src/maskquad.glsl`: Mask rendering

**Compilation:**
```bash
sokol-shdc --input src/texquad.glsl --output build/texquad.glsl.h --slang hlsl4 --format sokol
```

The compiled headers are included in the corresponding material C files.

## Layout Engine

### Yoga Integration

Soluna integrates Facebook's Yoga library for flexbox-style layouts.

**Layout Flow:**
1. Parse layout definition (datalist format)
2. Create Yoga node tree
3. Set node properties (width, height, flex, etc.)
4. Calculate layout (`YGNodeCalculateLayout`)
5. Extract computed positions and sizes
6. Generate render list

**Key Files:**
- `src/lualib/layout.lua`: Layout API
- `src/luayoga.c`: Yoga Lua bindings
- `src/yogaone.cpp`: Yoga library integration

### Layout Document Model

Layout documents use a DOM-like structure:
- **Document**: Root container, holds all elements
- **Elements**: Individual layout nodes with IDs
- **Attributes**: Properties like width, height, flex, padding
- **Children**: Nested layout nodes

Elements can be accessed and modified dynamically:
```lua
local dom = layout.load(definition)
dom.header.height = 120  -- Update element
local elements = layout.calc(dom)  -- Recalculate
```

## Font System

### TrueType Font Rendering

Soluna uses a custom TrueType renderer with SDF (Signed Distance Field) generation for smooth text at any scale.

**Font Pipeline:**
1. Load TTF data via `font.import()`
2. Generate glyph bitmaps using TrueType parser
3. Convert to SDF using distance transform
4. Pack glyphs into texture atlas
5. Render using SDF shader

**Key Files:**
- `src/truetype.c`: TrueType font parsing
- `src/sdfimage.c`: SDF generation
- `src/font.c`: Font API
- `src/font_manager.c`: Font atlas management
- `src/lualib/fontmgr.lua`: Font manager service

### Font Atlas

The font atlas is a dynamic texture that grows as needed:
- Initial size: Configurable (default 1024x1024)
- Layout: Row-based packing
- Updates: Incremental, only when new glyphs are added
- Submission: Coordinated with render service

### System Font Access

Platform-specific font enumeration:

**Windows:**
- Uses Win32 font enumeration API
- Reads font files from Windows font directory
- Supports TrueType and OpenType fonts

**macOS/Linux:**
- Scans system font directories
- Parses font configuration files

**Key Files:**
- `src/font_system.c`: System font access

## Sprite Management

### Sprite Bank

The sprite bank manages sprite metadata and texture packing:

**Data Structure:**
- Sprite ID → Metadata (size, offset, texture coords)
- Texture ID → Atlas region allocations
- LRU cache for texture packing

**Operations:**
- `add(w, h, x, y)`: Add sprite to bank
- `touch(id)`: Mark sprite as used (LRU)
- `pack()`: Pack sprites into texture atlas
- `atlas(texid)`: Get atlas layout for texture

**Key Files:**
- `src/spritemgr.c`: Sprite manager C implementation
- `src/lualib/spritebundle.lua`: Sprite bundle loader

### Texture Packing

Sprites are packed into texture atlases using a simple row-based algorithm:
1. Sort sprites by height
2. Pack into rows, wrapping to new row when width exceeded
3. Allocate new texture if needed
4. Update sprite UV coordinates

## Image Processing

### Image Loading

Supports multiple image formats via stb_image:
- PNG (with transparency)
- JPEG
- BMP
- TGA

**Alpha Channel Handling:**
- Standard loading: Preserves alpha as-is
- Alpha loading: Pre-multiplies RGB by alpha

**Key Files:**
- `src/image.c`: Image loading and processing

### Image Operations

- **Load**: Decode image from memory
- **Crop**: Extract sub-rectangle with auto-cropping of transparent pixels
- **Canvas**: Create drawable region
- **Blit**: Copy image data
- **Write**: Save image to file

## Data Serialization

### Datalist Format

Soluna uses a custom text-based format called "datalist" for configuration and data:

**Syntax:**
```
key : value
nested_object :
    child_key : child_value
    another_key : value
array_item :
    index : 0
array_item :
    index : 1
```

**Parser:**
- Indentation-based structure
- Colon-separated key-value pairs
- Automatic type inference (number, string)
- Nested objects and arrays

**Key Files:**
- `3rd/datalist/datalist.c`: Parser implementation
- `src/lualib/datalist.lua` (if exists): Lua bindings

## Multithreading Model

### ltask Overview

ltask is a lightweight task library for Lua:
- **Isolated Services**: Each service runs in its own Lua state
- **Message Passing**: Services communicate via messages
- **Async/Sync Calls**: Support for both asynchronous and synchronous calls
- **No Shared State**: No shared memory between services

**Service Types:**
- **Unique Services**: Named services with single instance
- **Anonymous Services**: Unnamed, can have multiple instances

### Message Flow

Example: Loading sprites

1. Game code calls `soluna.load_sprites("sprites.dl")`
2. Request sent to loader service
3. Loader service:
   - Loads and parses sprite bundle
   - Creates sprite bank entries
   - Packs sprites if needed
4. Request sent to render service to allocate GPU resources
5. Response returned to game code with sprite IDs

### Service Discovery

Services can be located by:
- `ltask.uniqueservice(name)`: Get/create unique service
- `ltask.queryservice(name)`: Find existing service
- Service addresses passed in messages

## Build System

### Build Process

Soluna uses luamake as its build system:

1. Build standalone Lua interpreter
2. Compile Lua scripts to C headers
3. Compile shaders to C headers
4. Compile C/C++ sources
5. Link final executable

To build the project:
```bash
luamake
```

### Code Generation

**Lua Script Embedding:**
```bash
lua script/lua2c.lua input.lua output.lua.h
```

This converts Lua scripts into C string literals that are embedded in the executable.

**Datalist Embedding:**
```bash
lua script/datalist2c.lua input.dl output.dl.h
```

Similar to Lua embedding but for datalist files.

**Shader Compilation:**
```bash
sokol-shdc --input shader.glsl --output shader.glsl.h --slang hlsl4 --format sokol
```

Compiles GLSL shaders to Sokol shader format.

## Platform Support

### Platform Abstraction

Sokol provides cross-platform abstractions for:
- **Graphics**: D3D11 (Windows), Metal (macOS), OpenGL (Linux), WebGL (WASM)
- **Window**: Native window creation and management
- **Input**: Keyboard, mouse, touch events
- **Time**: High-resolution timers

### Platform-Specific Code

**Windows** (`src/winfile.c`):
- UTF-8 file path handling
- System font enumeration
- Registry access

**WASM** (`src/wasm/`):
- Browser integration
- File system emulation (via Emscripten)
- WebGL rendering

### Build Targets

- **Windows**: Native Win32 executable (x86/x64)
- **macOS**: Native Cocoa application
- **Linux**: Native X11/Wayland application
- **WASM**: WebAssembly module for browsers

## Memory Management

### Resource Lifetime

- **Lua State**: Garbage collected by Lua
- **C Resources**: Manual management with careful lifetime tracking
- **GPU Resources**: Released on shutdown or when explicitly freed

### Memory Pools

- **Sprite Bank**: Pre-allocated sprite metadata
- **Batch Buffers**: Reusable vertex buffers
- **Font Atlas**: Dynamic growth, never shrinks

## Performance Considerations

### Optimization Strategies

1. **Batch Rendering**: Batch sprites to reduce draw calls
2. **Texture Atlases**: Minimize texture switching
3. **SDF Text**: Render text at any scale without quality loss
4. **Dirty State Tracking**: Update only modified elements

### Profiling

Use platform-specific tools:
- **Windows**: Visual Studio Profiler, PIX
- **macOS**: Instruments
- **Linux**: perf, Valgrind

## Extension Points

### Adding Custom Materials

1. Write GLSL shader
2. Compile with sokol-shdc
3. Create C material implementation
4. Register material in render service
5. Export Lua API

### Adding Custom Services

1. Write service Lua script in `src/service/`
2. Add to bootstrap list in `main.lua`
3. Implement service dispatch table
4. Export API via `soluna` module

### Custom File Formats

1. Write parser (C or Lua)
2. Add to loader service
3. Export loading function
4. Document format specification

## Debugging

### Logging

Enable debug logging:
```lua
-- In bootstrap configuration
core = {
    debuglog = "=",  -- stdout
    -- or
    debuglog = "debug.log",  -- file
}
```

### Service Debugging

Debug specific services by adding print statements or using ltask debugging facilities:
```lua
-- In service
print("Service received:", method, ...)
```

### Graphics Debugging

- Use Sokol debug features
- Enable validation layers
- Capture frames with platform tools (RenderDoc, Xcode, etc.)

## References

- [Sokol](https://github.com/floooh/sokol): Graphics library
- [Yoga](https://github.com/facebook/yoga): Layout engine
- [stb_image](https://github.com/nothings/stb): Image loading
- [Deep Future](https://github.com/cloudwu/deepfuture): Example game using Soluna
