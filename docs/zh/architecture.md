# Soluna 引擎架构

本文档描述了 Soluna 游戏引擎的内部架构。

## 概述

Soluna 建立在使用 ltask 库的面向服务架构上，用于多线程和消息传递。引擎将关注点分离到不同的服务中，这些服务异步通信，允许高效的并行处理。

## 核心组件

### 1. 主线程（原生）

主线程运行原生应用程序循环并处理：
- 窗口管理和事件处理
- 图形上下文初始化（通过 Sokol）
- 输入设备轮询
- 帧同步
- 事件分发到 Lua 运行时

**关键文件:**
- `src/entry.c`: 应用程序入口点和主循环
- `src/appevent.h`: 事件定义

### 2. 引导和初始化

引导过程：
1. 初始化 ltask 运行时
2. 设置根服务
3. 创建核心服务（timer, log, loader）
4. 加载嵌入的 Lua 代码
5. 启动游戏入口脚本

**关键文件:**
- `src/lualib/main.lua`: 引导逻辑
- `src/embedlua.c`: 嵌入式 Lua 脚本管理

### 3. 服务架构

服务是通过消息传递通信的独立 Lua 执行上下文。

#### 核心服务

**定时器服务**
- 调度延迟和定期任务
- 管理超时事件
- ltask 库的一部分

**日志服务**
- 集中日志输出
- 线程安全的日志消息处理
- ltask 库的一部分

**加载器服务** (`src/service/loader.lua`)
- 异步资产加载
- 精灵包管理
- 纹理图集打包
- 图像缓存

**渲染服务** (`src/service/render.lua`)
- 渲染管道管理
- 材质系统
- 字体纹理管理
- 批次提交协调

**设置服务** (`src/service/settings.lua`)
- 配置管理
- 游戏设置存储

**手柄服务** (`src/service/gamepad.lua`)
- 手柄状态轮询
- 输入状态分发

**启动服务** (`src/service/start.lua`)
- 用户游戏脚本执行
- 回调管理
- 游戏循环协调

## 渲染系统

### 渲染管道

1. **批次收集**: 游戏代码通过 `batch:add()` 将精灵添加到批次
2. **批次提交**: 批次提交到渲染服务
3. **材质处理**: 不同的材质处理它们的批次
4. **GPU 上传**: 顶点数据上传到 GPU
5. **绘制调用**: Sokol 渲染帧
6. **字体更新**: 如果需要，更新字体纹理图集

### 材质系统

Soluna 使用基于材质的渲染方法，有四种内置材质：

**默认材质** (`src/material_default.c`)
- 基本纹理精灵渲染
- 来自纹理图集的 UV 映射
- 颜色着色支持

**文本材质** (`src/material_text.c`)
- SDF（有符号距离场）文本渲染
- 动态字体图集
- 任何比例下的平滑文本

**四边形材质** (`src/material_quad.c`)
- 纯色矩形
- UI 元素和背景
- 不需要纹理

**遮罩材质** (`src/material_mask.c`)
- 精灵遮罩和裁剪
- 复杂形状渲染

### 着色器系统

着色器用 GLSL 编写，并使用 sokol-shdc 编译为平台特定格式：

**着色器文件:**
- `src/texquad.glsl`: 纹理四边形渲染
- `src/colorquad.glsl`: 彩色四边形渲染
- `src/sdftext.glsl`: SDF 文本渲染
- `src/maskquad.glsl`: 遮罩渲染

**编译:**
```bash
sokol-shdc --input src/texquad.glsl --output build/texquad.glsl.h --slang hlsl4 --format sokol
```

编译后的头文件包含在相应的材质 C 文件中。

## 布局引擎

### Yoga 集成

Soluna 集成了 Facebook 的 Yoga 库用于类 flexbox 布局。

**布局流程:**
1. 解析布局定义（datalist 格式）
2. 创建 Yoga 节点树
3. 设置节点属性（width, height, flex 等）
4. 计算布局（`YGNodeCalculateLayout`）
5. 提取计算后的位置和大小
6. 生成渲染列表

**关键文件:**
- `src/lualib/layout.lua`: 布局 API
- `src/luayoga.c`: Yoga Lua 绑定
- `src/yogaone.cpp`: Yoga 库集成

### 布局文档模型

布局文档使用类似 DOM 的结构：
- **文档**: 根容器，保存所有元素
- **元素**: 带有 ID 的单个布局节点
- **属性**: 如 width、height、flex、padding 等属性
- **子元素**: 嵌套的布局节点

元素可以动态访问和修改：
```lua
local dom = layout.load(definition)
dom.header.height = 120  -- 更新元素
local elements = layout.calc(dom)  -- 重新计算
```

## 字体系统

### TrueType 字体渲染

Soluna 使用自定义 TrueType 渲染器，带有 SDF（有符号距离场）生成，以在任何比例下实现平滑文本。

**字体管道:**
1. 通过 `font.import()` 加载 TTF 数据
2. 使用 TrueType 解析器生成字形位图
3. 使用距离变换转换为 SDF
4. 将字形打包到纹理图集中
5. 使用 SDF 着色器渲染

**关键文件:**
- `src/truetype.c`: TrueType 字体解析
- `src/sdfimage.c`: SDF 生成
- `src/font.c`: 字体 API
- `src/font_manager.c`: 字体图集管理
- `src/lualib/fontmgr.lua`: 字体管理器服务

### 字体图集

字体图集是一个动态纹理，根据需要增长：
- 初始大小：可配置（默认 1024x1024）
- 布局：基于行的打包
- 更新：增量式，仅在添加新字形时
- 提交：与渲染服务协调

### 系统字体访问

平台特定的字体枚举：

**Windows:**
- 使用 Win32 字体枚举 API
- 从 Windows 字体目录读取字体文件
- 支持 TrueType 和 OpenType 字体

**macOS/Linux:**
- 扫描系统字体目录
- 解析字体配置文件

**关键文件:**
- `src/font_system.c`: 系统字体访问

## 精灵管理

### 精灵库

精灵库管理精灵元数据和纹理打包：

**数据结构:**
- 精灵 ID → 元数据（大小、偏移、纹理坐标）
- 纹理 ID → 图集区域分配
- 纹理打包的 LRU 缓存

**操作:**
- `add(w, h, x, y)`: 向库中添加精灵
- `touch(id)`: 标记精灵为已使用（LRU）
- `pack()`: 将精灵打包到纹理图集
- `atlas(texid)`: 获取纹理的图集布局

**关键文件:**
- `src/spritemgr.c`: 精灵管理器 C 实现
- `src/lualib/spritebundle.lua`: 精灵包加载器

### 纹理打包

精灵使用简单的基于行的算法打包到纹理图集中：
1. 按高度对精灵排序
2. 打包成行，当宽度超出时换到新行
3. 如果需要，分配新纹理
4. 更新精灵 UV 坐标

## 图像处理

### 图像加载

通过 stb_image 支持多种图像格式：
- PNG（带透明度）
- JPEG
- BMP
- TGA

**Alpha 通道处理:**
- 标准加载：按原样保留 alpha
- Alpha 加载：将 RGB 预乘以 alpha

**关键文件:**
- `src/image.c`: 图像加载和处理

### 图像操作

- **加载**: 从内存解码图像
- **裁剪**: 提取子矩形，自动裁剪透明像素
- **画布**: 创建可绘制区域
- **Blit**: 复制图像数据
- **写入**: 将图像保存到文件

## 数据序列化

### Datalist 格式

Soluna 使用名为"datalist"的自定义文本格式进行配置和数据：

**语法:**
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

**解析器:**
- 基于缩进的结构
- 冒号分隔的键值对
- 自动类型推断（数字、字符串）
- 嵌套对象和数组

**关键文件:**
- `3rd/datalist/datalist.c`: 解析器实现

## 多线程模型

### ltask 概述

ltask 是 Lua 的轻量级任务库：
- **隔离的服务**: 每个服务在自己的 Lua 状态中运行
- **消息传递**: 服务通过消息通信
- **异步/同步调用**: 支持异步和同步调用
- **无共享状态**: 服务之间没有共享内存

**服务类型:**
- **唯一服务**: 具有单个实例的命名服务
- **匿名服务**: 未命名，可以有多个实例

### 消息流

示例：加载精灵

1. 游戏代码调用 `soluna.load_sprites("sprites.dl")`
2. 请求发送到加载器服务
3. 加载器服务：
   - 加载和解析精灵包
   - 创建精灵库条目
   - 如果需要，打包精灵
4. 请求发送到渲染服务以分配 GPU 资源
5. 响应返回给游戏代码，带有精灵 ID

### 服务发现

服务可以通过以下方式定位：
- `ltask.uniqueservice(name)`: 获取/创建唯一服务
- `ltask.queryservice(name)`: 查找现有服务
- 在消息中传递服务地址

## 构建系统

### 构建过程

Soluna 使用 Make 和 luamake 构建系统：

**Makefile**（面向 Windows）：
1. 构建独立的 Lua 解释器
2. 将 Lua 脚本编译为 C 头文件
3. 将着色器编译为 C 头文件
4. 编译 C/C++ 源代码
5. 链接最终可执行文件

**luamake**（跨平台）：
- 现代的基于 Lua 的构建系统
- 更好的跨平台支持
- 用于 macOS、Linux 和 WASM 构建

### 代码生成

**Lua 脚本嵌入:**
```bash
lua script/lua2c.lua input.lua output.lua.h
```

这将 Lua 脚本转换为嵌入到可执行文件中的 C 字符串文字。

**Datalist 嵌入:**
```bash
lua script/datalist2c.lua input.dl output.dl.h
```

类似于 Lua 嵌入，但用于 datalist 文件。

**着色器编译:**
```bash
sokol-shdc --input shader.glsl --output shader.glsl.h --slang hlsl4 --format sokol
```

将 GLSL 着色器编译为 Sokol 着色器格式。

## 平台支持

### 平台抽象

Sokol 为以下提供跨平台抽象：
- **图形**: D3D11 (Windows), Metal (macOS), OpenGL (Linux), WebGL (WASM)
- **窗口**: 原生窗口创建和管理
- **输入**: 键盘、鼠标、触摸事件
- **时间**: 高分辨率计时器

### 平台特定代码

**Windows** (`src/winfile.c`):
- UTF-8 文件路径处理
- 系统字体枚举
- 注册表访问

**WASM** (`src/wasm/`):
- 浏览器集成
- 文件系统仿真（通过 Emscripten）
- WebGL 渲染

### 构建目标

- **Windows**: 原生 Win32 可执行文件 (x86/x64)
- **macOS**: 原生 Cocoa 应用程序
- **Linux**: 原生 X11/Wayland 应用程序
- **WASM**: 用于浏览器的 WebAssembly 模块

## 内存管理

### 资源生命周期

- **Lua 状态**: 由 Lua 垃圾回收
- **C 资源**: 手动管理，仔细跟踪生命周期
- **GPU 资源**: 在关闭或显式释放时释放

### 内存池

- **精灵库**: 预分配的精灵元数据
- **批次缓冲区**: 可重用的顶点缓冲区
- **字体图集**: 动态增长，永不缩小

## 性能考虑

### 优化策略

1. **批量渲染**: 通过批处理精灵最小化绘制调用
2. **纹理图集**: 减少纹理切换
3. **SDF 文本**: 无需多个字体大小的平滑文本
4. **多线程**: 并行资产加载和处理
5. **脏状态跟踪**: 只更新改变的内容

### 瓶颈

常见性能瓶颈：
- **GPU 上传**: 大量纹理更新
- **绘制调用**: 太多渲染批次
- **Lua GC**: 过度垃圾生成
- **文件 I/O**: 同步文件加载

### 性能分析

使用平台特定工具：
- **Windows**: Visual Studio Profiler, PIX
- **macOS**: Instruments
- **Linux**: perf, Valgrind
- **所有平台**: Lua 性能分析钩子

## 扩展点

### 添加自定义材质

1. 编写 GLSL 着色器
2. 使用 sokol-shdc 编译
3. 创建 C 材质实现
4. 在渲染服务中注册材质
5. 导出 Lua API

### 添加自定义服务

1. 在 `src/service/` 中编写服务 Lua 脚本
2. 添加到 `main.lua` 中的引导列表
3. 实现服务分发表
4. 通过 `soluna` 模块导出 API

### 自定义文件格式

1. 编写解析器（C 或 Lua）
2. 添加到加载器服务
3. 导出加载函数
4. 记录格式规范

## 调试

### 日志记录

启用调试日志：
```lua
-- 在引导配置中
core = {
    debuglog = "=",  -- stdout
    -- 或
    debuglog = "debug.log",  -- 文件
}
```

### 服务调试

通过添加打印语句或使用 ltask 调试工具调试特定服务：
```lua
-- 在服务中
print("服务收到:", method, ...)
```

### 图形调试

- 使用 Sokol 调试功能
- 启用验证层
- 使用平台工具捕获帧（RenderDoc, Xcode 等）

## 未来方向

潜在改进领域：
- **3D 渲染**: 添加 3D 精灵支持
- **粒子系统**: 内置粒子效果
- **物理**: 集成物理引擎
- **音频**: 添加音频支持
- **网络**: 多人游戏功能
- **编辑器**: 可视化游戏编辑器

## 参考

- [ltask](https://github.com/cloudwu/ltask): 多线程库
- [Sokol](https://github.com/floooh/sokol): 图形库
- [Yoga](https://github.com/facebook/yoga): 布局引擎
- [stb_image](https://github.com/nothings/stb): 图像加载
- [Deep Future](https://github.com/cloudwu/deepfuture): 使用 Soluna 的示例游戏
