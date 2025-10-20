# Soluna Game Engine Documentation

Soluna is a framework for creating 2D games in Lua with multithreading support.

## Overview

Soluna is a lightweight, high-performance 2D game engine that leverages Lua for scripting and provides built-in multithreading capabilities through ltask. It's designed to be simple yet powerful, making it ideal for developing cross-platform 2D games.

## Features

- **Lua Scripting**: Write your game logic in Lua, a lightweight and fast scripting language
- **Multithreading**: Built-in support for concurrent operations using ltask
- **Cross-Platform**: Supports Windows, macOS, Linux, and WebAssembly (WASM)
- **Modern Rendering**: Uses Sokol for efficient graphics rendering
- **Flexible Layout**: Yoga-based layout system for UI elements
- **Sprite Management**: Efficient sprite batching and texture atlas support
- **Text Rendering**: Advanced text rendering with TrueType font support
- **Gamepad Support**: Built-in gamepad input handling
- **Resource Management**: Efficient loading and management of game assets

## Core Components

### Rendering System
- Hardware-accelerated 2D rendering using Sokol
- Material system with support for default, text, quad, and mask materials
- Sprite batching for optimal performance
- Multiple render passes and viewports

### Layout Engine
- Flexbox-style layout using Facebook's Yoga library
- Responsive UI design capabilities
- Dynamic element positioning and sizing
- Support for nested layouts

### Asset Management
- Image loading and processing
- Sprite bundle system for efficient texture packing
- Font management with TrueType support
- Data serialization using datalist format

### Multithreading
- Service-oriented architecture using ltask
- Message passing between services
- Timer service for scheduled tasks
- Asynchronous resource loading

## Architecture

Soluna follows a service-based architecture where different subsystems run as separate services communicating through ltask:

- **Render Service**: Handles all rendering operations
- **Loader Service**: Manages asset loading and caching
- **Timer Service**: Provides timing and scheduling functionality
- **Gamepad Service**: Processes gamepad input
- **Settings Service**: Manages game configuration

## Getting Started

See [Getting Started Guide](getting-started.md) for installation instructions and your first Soluna project.

## API Reference

For detailed API documentation, see [API Reference](api-reference.md).

## Examples

Check out [Examples](examples.md) for practical code samples and tutorials.

## External Resources

- [Deep Future](https://github.com/cloudwu/deepfuture) - A complete game built with Soluna that demonstrates all major features

## License

See the LICENSE file in the repository root for licensing information.
