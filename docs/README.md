# Soluna Game Engine Documentation / Soluna 游戏引擎文档

Complete documentation for the Soluna game engine in both English and Chinese.

完整的 Soluna 游戏引擎文档，提供英文和中文版本。

## Documentation Structure / 文档结构

### English Documentation / 英文文档

Located in [`docs/en/`](en/)

- **[README.md](en/README.md)** - Introduction and overview of Soluna
- **[Getting Started](en/getting-started.md)** - Installation guide and first project
- **[API Reference](en/api-reference.md)** - Complete API documentation
- **[Architecture](en/architecture.md)** - Engine architecture and internals
- **[Examples](en/examples.md)** - Code examples and tutorials

### Chinese Documentation / 中文文档

位于 [`docs/zh/`](zh/)

- **[README.md](zh/README.md)** - Soluna 简介和概述
- **[快速开始](zh/getting-started.md)** - 安装指南和第一个项目
- **[API 参考](zh/api-reference.md)** - 完整 API 文档
- **[架构](zh/architecture.md)** - 引擎架构和内部机制
- **[示例](zh/examples.md)** - 代码示例和教程

## Quick Links / 快速链接

### For Beginners / 新手入门

- English: Start with [Getting Started Guide](en/getting-started.md)
- 中文: 从[快速开始指南](zh/getting-started.md)开始

### For Developers / 开发者

- English: Refer to [API Reference](en/api-reference.md)
- 中文: 参考 [API 参考](zh/api-reference.md)

### For Contributors / 贡献者

- English: Read [Architecture Documentation](en/architecture.md)
- 中文: 阅读[架构文档](zh/architecture.md)

## What is Soluna? / 什么是 Soluna？

**English:**

Soluna is a lightweight, high-performance 2D game engine that leverages Lua for scripting and provides built-in multithreading capabilities through ltask. It's designed to be simple yet powerful, making it ideal for developing cross-platform 2D games.

Key features:
- Lua scripting with multithreading support
- Cross-platform (Windows, macOS, Linux, WebAssembly)
- Modern rendering with Sokol
- Yoga-based layout engine
- Efficient sprite and text rendering

**中文:**

Soluna 是一个轻量级、高性能的 2D 游戏引擎，它利用 Lua 进行脚本编写，并通过 ltask 提供内置的多线程功能。它设计简单而强大，非常适合开发跨平台 2D 游戏。

主要特性：
- 支持多线程的 Lua 脚本
- 跨平台（Windows、macOS、Linux、WebAssembly）
- 使用 Sokol 的现代渲染
- 基于 Yoga 的布局引擎
- 高效的精灵和文本渲染

## External Resources / 外部资源

- **Repository / 仓库**: [github.com/yuchanns/soluna](https://github.com/yuchanns/soluna)
- **Example Game / 示例游戏**: [Deep Future](https://github.com/cloudwu/deepfuture) - A complete game built with Soluna / 使用 Soluna 构建的完整游戏

## Building Soluna / 构建 Soluna

**English:**

```bash
# Clone the repository
git clone https://github.com/yuchanns/soluna.git
cd soluna

# Initialize submodules
git submodule update --init --recursive

# Build (Windows with MinGW)
make

# Build (Windows with MSVC)
make CC=cl

# Build (macOS/Linux)
make
```

**中文:**

```bash
# 克隆仓库
git clone https://github.com/yuchanns/soluna.git
cd soluna

# 初始化子模块
git submodule update --init --recursive

# 构建（Windows 使用 MinGW）
make

# 构建（Windows 使用 MSVC）
make CC=cl

# 构建（macOS/Linux）
make
```

## Contributing to Documentation / 为文档贡献

**English:**

We welcome contributions to improve the documentation! Please:
1. Keep both English and Chinese versions in sync
2. Follow the existing documentation style
3. Include code examples where appropriate
4. Test all code examples before submitting

**中文:**

我们欢迎为改进文档做出贡献！请：
1. 保持英文和中文版本同步
2. 遵循现有的文档风格
3. 在适当的地方包含代码示例
4. 在提交前测试所有代码示例

## License / 许可证

See the LICENSE file in the repository root for licensing information.

有关许可信息，请参见仓库根目录中的 LICENSE 文件。

---

**Need help? / 需要帮助？**

- Check the [examples](en/examples.md) / 查看[示例](zh/examples.md)
- Study the [Deep Future](https://github.com/cloudwu/deepfuture) game source code / 学习 [Deep Future](https://github.com/cloudwu/deepfuture) 游戏源代码
- Look at the test files in the `test/` directory / 查看 `test/` 目录中的测试文件
