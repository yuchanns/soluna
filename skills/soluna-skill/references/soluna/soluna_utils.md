# Soluna Utility API

本文件覆盖游戏侧可能直接使用的辅助模块：crypt、url、coroutine、lfs、zip 和 image.sdf。文件、图片和 datalist 见 `soluna_assets.md`。

## Crypt

```lua
local crypt = require "soluna.crypt"
```

常用函数：

- `crypt.hexencode(data)`：二进制转小写 hex。
- `crypt.sha1(data)`：计算 SHA-1，返回 20 字节二进制 hash。

示例：

```lua
local crypt = require "soluna.crypt"

local digest = crypt.hexencode(crypt.sha1("save-data"))
```

## URL

```lua
local url = require "soluna.url"

url.open("https://example.com")
```

适合打开游戏官网、隐私政策、反馈链接等外部页面。

## Coroutine

```lua
local coroutine = require "soluna.coroutine"
```

Soluna 提供 coroutine 模块，用法接近 Lua coroutine：

```lua
local co = coroutine.create(function()
	coroutine.yield("WAIT")
	return "DONE"
end)

local ok, value = coroutine.resume(co)
```

复杂多帧流程可以在项目自己的 flow/state 模块中封装 coroutine，不要把 coroutine 调度逻辑散落到所有 gameplay 文件里。

## LFS

```lua
local lfs = require "soluna.lfs"
```

常用函数：

- `lfs.personaldir()`：用户个人目录。
- `lfs.currentdir()`：当前目录。
- `lfs.chdir(path)`：切换目录。
- `lfs.dir(path)`：遍历目录。
- `lfs.attributes(filename)`：读取文件属性。
- `lfs.realpath(path)`：解析真实路径。
- `lfs.mkdir(path)`：创建目录。

保存文件时优先结合 `soluna.gamedir()`，避免把存档写进安装目录：

```lua
local soluna = require "soluna"
local lfs = require "soluna.lfs"

local dir = soluna.gamedir()
lfs.mkdir(dir .. "/save")
```

## Zip

```lua
local zip = require "soluna.zip"
```

游戏发布打包优先使用 shell 或项目构建脚本。`soluna.zip` 更适合运行时需要读取、压缩或解压 zip 数据的高级场景。

常用函数：

- `zip.open(filename, mode)`：打开 zip 文件，`mode` 为 `"r"` 或 `"w"`。
- `zip.list(data_or_filename)`：读取 zip 文件列表。
- `zip.compress(data)`：压缩二进制数据。
- `zip.uncompress(data)`：解压二进制数据。

## Image SDF

```lua
local sdf = require "soluna.image.sdf"
```

`soluna.image.sdf` 主要用于 SDF/icon 相关资源处理。普通游戏图片加载使用 `soluna.image`；只有项目明确使用 SDF 图像或 icon bundle 生成流程时再使用该模块。

常用函数：

- `sdf.load(data)`：加载 SDF image 数据。
- `sdf.bundle(list)`：把 icon 列表打包成 bundle 数据。
- `sdf.save(data)`：保存 SDF image 数据，通常用于调试或工具链。
