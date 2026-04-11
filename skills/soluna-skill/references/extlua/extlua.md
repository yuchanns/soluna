# extlua C 插件参考

extlua 允许游戏开发者在不修改 Soluna 引擎源码的情况下，为自己的游戏添加 C 模块。游戏通过 `.game` 配置预加载插件库，Lua 代码再像加载普通模块一样 `require` 这些 C 模块。新 material 也可以通过 extlua 提供：C 插件暴露 helper 和 draw object，Lua material adapter 接入 Soluna render service。

## 适用场景

使用 extlua 的典型场景：

- 游戏需要调用平台 SDK、原生库或已有 C/C++ 代码。
- Lua 层性能不足，需要把小而稳定的热点逻辑放到 C。
- 游戏希望以插件方式分发可选 ext 功能。
- 游戏需要自定义 shader、pipeline、instance buffer 或 material stream。

不建议把普通 gameplay 逻辑过早放进 C。优先保持 Lua 侧规则清晰，只把边界明确、接口稳定、测试容易覆盖的部分做成插件。

## 项目结构

一个插件通常包含：

```text
mygame/
├── main.game
├── main.lua
├── ext/
│   └── myplugin.c
└── build/
```

普通插件 C 文件需要包含 Lua 头文件，并声明 Soluna extlua 提供的初始化函数：

```c
#include <lua.h>
#include <lauxlib.h>

LUA_API void luaapi_init(lua_State *L);

#if defined(_WIN32)
#define EXTLUA_EXPORT __declspec(dllexport)
#else
#define EXTLUA_EXPORT __attribute__((visibility("default")))
#endif
```

如果插件要使用 Sokol 或 Soluna material API，还需要包含对应桥接头并初始化：

```c
#include "sokol/sokol_gfx.h"
#include "solunaapi.h"

void sokolapi_init(lua_State *L);
void solunaapi_init(lua_State *L);
```

## C 插件入口

插件需要导出 `.game` 中 `extlua_entry` 指定的函数。该函数先调用 `luaapi_init(L)`，再返回一个 table。table 的 key 是 Lua 模块名，value 是对应的 `luaopen_*` 函数。

```c
#include <lua.h>
#include <lauxlib.h>

LUA_API void luaapi_init(lua_State *L);

#if defined(_WIN32)
#define EXTLUA_EXPORT __declspec(dllexport)
#else
#define EXTLUA_EXPORT __attribute__((visibility("default")))
#endif

static int
lhello(lua_State *L) {
	lua_pushstring(L, "Hello from extlua");
	return 1;
}

static int
luaopen_ext_hello(lua_State *L) {
	luaL_Reg lib[] = {
		{ "hello", lhello },
		{ NULL, NULL },
	};
	luaL_newlib(L, lib);
	return 1;
}

EXTLUA_EXPORT int
extlua_init(lua_State *L) {
	luaapi_init(L);
	luaL_Reg libs[] = {
		{ "ext.hello", luaopen_ext_hello },
		{ NULL, NULL },
	};
	luaL_newlib(L, libs);
	return 1;
}
```

`extlua_init` 这个名字不是固定协议的一部分；它只需要和 `.game` 中的 `extlua_entry` 一致。

## `.game` 配置

通过 `extlua_preload` 指定要预加载的插件库名，通过 `extlua_entry` 指定入口符号。

```text
entry : main.lua
extlua_entry : extlua_init
extlua_preload : myplugin
```

`extlua_preload` 可以是单个库名，也可以是多个库名组成的列表。runtime 会按 `package.cpath` 查找动态库，并调用 `package.loadlib(path, extlua_entry)`。

如果动态库不在默认查找路径里，在启动时提供 `cpath`：

```bash
soluna main.game cpath="./?.so;./?.dll;./?.dylib"
```

按平台调整扩展名和路径分隔。wasm 侧加载 side module 时通常使用类似：

```text
cpath=/data/?.wasm
```

## Extlua Material 配置

自定义 material 需要同时配置 C 插件和 Lua material adapter。

```text
entry : extlua.lua
extlua_entry : extlua_init
extlua_preload : sample
extlua_material : custom
extlua_material_path : extlua/material/?.lua
```

- `extlua_preload`：加载动态库，入口返回 table 中应包含游戏侧 `require` 的 C 模块。
- `extlua_material`：单个 material 名称或名称列表。runtime 会为它们分配 material id，外部 material id 从 `256` 开始。
- `extlua_material_path`：用 `file.searchpath(name, path)` 查找 Lua adapter，例如 `custom` 会命中 `extlua/material/custom.lua`。

内置 material 使用较小 id；extlua material 从 `256` 起是为了和内置 material 分开。extlua material id 由引擎分配，并通过 Lua adapter 的 `ctx.id` 提供；插件侧应把这个 id 记录在自己的模块状态或 material state 中，生成 stream 和提交 material 时都使用该 id。

## Lua 使用

预加载成功后，Lua 代码像普通模块一样 `require`：

```lua
local hello = require "ext.hello"

print(hello.hello())
```

模块名必须和 C 入口返回表中的 key 一致。

## 构建要点

插件编译需要：

- include Lua headers。
- 编译并链接 Soluna 的 `extlua/extlua.c`。
- 导出 `.game` 中指定的入口符号。
- 入口函数先调用 `luaapi_init(L)`，让插件使用 Soluna runtime 提供的 Lua C API。
- 如果使用 Sokol 或 Soluna material API，还要编译并链接 `extlua/sokolapi.c`、`extlua/solunaapi.c`，并 include `3rd` 与 `extlua`。

最小构建形态：

```bash
cc -shared -fPIC \
  -I path/to/soluna/3rd/lua \
  path/to/soluna/extlua/extlua.c \
  ext/myplugin.c \
  -o myplugin.so
```

macOS 输出通常是 `.dylib`，Windows 输出通常是 `.dll`。实际命令应按编译器、平台和项目构建系统调整。

如果项目使用 `luamake`，插件目标应把 `extlua/extlua.c` 和插件源码一起放进动态库目标：

```lua
local lm = require "luamake"

lm:dll "myplugin" {
	sources = {
		"soluna/extlua/extlua.c",
		"ext/myplugin.c",
	},
	includes = {
		"soluna/3rd/lua",
	},
}
```

自定义 material 需要把 Soluna SDK 提供的 bridge 源文件和插件源码打进同一个动态库。

```lua
local lm = require "luamake"

local soluna = lm:path "path/to/soluna-sdk"

lm:dll "myplugin" {
	sources = {
		soluna / "extlua/extlua.c",
		soluna / "extlua/sokolapi.c",
		soluna / "extlua/solunaapi.c",
		"ext/myplugin.c",
	},
	includes = {
		soluna / "3rd/lua",
		soluna / "3rd",
		soluna / "extlua",
	},
}
```

## 多模块插件

一个动态库可以注册多个 Lua 模块：

```c
EXTLUA_EXPORT int
extlua_init(lua_State *L) {
	luaapi_init(L);
	luaL_Reg libs[] = {
		{ "ext.audio_codec", luaopen_ext_audio_codec },
		{ "ext.pathfinding", luaopen_ext_pathfinding },
		{ NULL, NULL },
	};
	luaL_newlib(L, libs);
	return 1;
}
```

Lua 侧分别加载：

```lua
local codec = require "ext.audio_codec"
local pathfinding = require "ext.pathfinding"
```

## Lua Material Adapter 文件

`extlua_material_path` 命中的 Lua 文件运行在 render service 中。例如：

```text
extlua_material : custom
extlua_material_path : extlua/material/?.lua
```

对应 adapter 文件是：

```text
extlua/material/custom.lua
```

render service 会加载 `extlua_material_path` 命中的 Lua 文件，并把 context 作为 `...` 传入。adapter 必须把 `ctx.id` 传给插件并返回 material table。下面示例假设 `register(ctx.id)` 会把引擎分配的 material id 记录到自定义材质模块中，供后续 stream helper 和 submit 使用。

```lua
-- extlua/material/custom.lua
local render = require "soluna.render"
local custommat = require "ext.material.custom"

local ctx = ...
local state = ctx.state

custommat.register(ctx.id)

local inst_buffer = render.buffer {
	type = "vertex",
	usage = "stream",
	label = "extlua-custom-instance",
	size = custommat.instance_size * ctx.settings.draw_instance,
}

local bindings = render.bindings()
bindings:vbuffer(0, inst_buffer)
bindings:sampler(0, state.default_sampler)

local cobj = custommat.new {
	inst_buffer = inst_buffer,
	bindings = bindings,
	uniform = state.uniform,
	sprite_bank = ctx.arg.bank_ptr,
	tmp_buffer = ctx.tmp_buffer,
}

local material = {}

function material.reset()
	cobj:reset()
end

function material.submit(ptr, n)
	cobj:submit(ptr, n)
end

function material.draw(ptr, n, tex)
	bindings:view(1, state.views[tex + 1])
	cobj:draw(ptr, n, tex)
end

return material
```

adapter 的职责：

- 读取 `ctx.id`，并通过插件自定义 API 让 C 插件记录这个 material id。
- 创建 render service 侧资源，例如 instance buffer、bindings、uniform 引用和 C material userdata。
- 返回 material table，至少实现 `submit(ptr, n)` 和 `draw(ptr, n, tex)`。
- 如果 C material 有 per-frame offset、staging buffer 或状态缓存，实现 `reset()`。

context 常用字段：

- `ctx.id`：runtime 分配的 material id，插件侧应记录这个 id，并在生成 stream 与提交 material 时使用。
- `ctx.state`：render service state，例如 `default_sampler`、`views`、`uniform`。
- `ctx.settings`：合并后的 `.game` settings。
- `ctx.arg.bank_ptr`：sprite bank lightuserdata，C 侧用来查 sprite rect。
- `ctx.tmp_buffer`：无 metatable userdata，适合作为 submit 阶段的临时 staging memory。
- `ctx.render` / `ctx.font`：render 和 font 相关 runtime 对象。

adapter 返回的 table 必须有 `submit(ptr, n)` 和 `draw(ptr, n, tex)`；`reset()` 可选，但有 instance buffer offset 或 per-frame 状态时应实现。

## Material C API

使用 Soluna material API 的插件入口应初始化三组 API：

```c
EXTLUA_EXPORT int
extlua_init(lua_State *L) {
	luaapi_init(L);
	sokolapi_init(L);
	solunaapi_init(L);
	luaL_Reg libs[] = {
		{ "ext.material.custom", luaopen_ext_material_custom },
		{ NULL, NULL },
	};
	luaL_newlib(L, libs);
	return 1;
}
```

插件模块通常暴露：

- 一个 registration API：名称由插件自定，用于记录 runtime 分配的 material id。
- `new(options)`：创建 material userdata，持有 pipeline、buffer、bindings、uniform、sprite bank、tmp buffer 等对象。
- 一个或多个 helper，例如 `sprite(sprite_id, options)`，返回可传给 `batch:add` 的 packed stream string。

核心 C API：

```c
soluna_material_error soluna_material_push_stream(
	int material_id,
	int count,
	size_t payload_size,
	soluna_material_stream_write_func write,
	void *ud,
	struct soluna_material_stream *out);

soluna_material_error soluna_material_submit(
	const void *stream,
	int prim_n,
	int material_id,
	int batch_n,
	void *ud,
	soluna_material_submit_func submit);

int soluna_material_stream_read(
	struct soluna_material_stream_context ctx,
	int index,
	size_t payload_size,
	void *payload,
	struct soluna_material_stream_data *out);
```

写入 packed stream：

- `soluna_material_push_stream` 分配 `out->data`，写入 `count` 个 stream item；成功后应把这段内存交给 Lua external string，最终用 `soluna_material_stream_free` 释放。
- `write(ud, index, item)` 填 `item->x`、`item->y`、`item->sprite` 和可选 `item->payload`。
- `item->sprite` 在 C API 中是 0-based sprite index；Lua 游戏侧拿到的 sprite id 通常是 1-based，传入前要减一。
- `payload_size` 必须能放入一个 material stream item 的 payload 区；超限会返回 `"Invalid material payload size"`。

提交 stream：

- material userdata 的 `submit(ptr, n)` 通常调用 `soluna_material_submit(ptr, n, material_id, batch_n, userdata, submit_callback)`。
- `batch_n` 是单次 callback 最大 primitive 数；runtime 会分块调用 callback。
- callback 内通过 `soluna_material_stream_read(ctx, index, payload_size, &payload, &item)` 读数据。
- 发现格式错误时调用 `soluna_material_stream_error(ctx, "message")` 并返回；错误会传播回 `soluna_material_submit`。
- 如果需要停止后续读取，可检查 `soluna_material_stream_failed(ctx)`。

渲染辅助：

- `soluna_material_sprite_rect(bank, sprite, &rect)` 查询 0-based sprite 的 texture、uv、尺寸和 origin。
- `soluna_material_bindings(bindings)` 把 Lua `render.bindings()` 包装对象转换成 `sg_bindings`，再传给 `sg_apply_bindings`。

## Minimal Material Helper Skeleton

下面是自包含的 C helper 骨架，展示 material id 注册、packed stream 写入、Lua external string 释放、以及插件入口注册。实际 material 还需要补上 `new`、`submit`、`draw` 和 pipeline 逻辑。

```c
#include <lua.h>
#include <lauxlib.h>

#include "solunaapi.h"

LUA_API void luaapi_init(lua_State *L);
void sokolapi_init(lua_State *L);
void solunaapi_init(lua_State *L);

#if defined(_WIN32)
#define EXTLUA_EXPORT __declspec(dllexport)
#else
#define EXTLUA_EXPORT __attribute__((visibility("default")))
#endif

struct material_payload {
	float value;
};

struct stream_write_context {
	int sprite;
	struct material_payload payload;
};

static int material_id = 0;

static void *
free_material_stream(void *ud, void *ptr, size_t osize, size_t nsize) {
	(void)ud;
	(void)osize;
	if (nsize == 0) {
		soluna_material_stream_free(ptr);
	}
	return NULL;
}

static int
lregister(lua_State *L) {
	int id = luaL_checkinteger(L, 1);
	if (id <= 0) {
		return luaL_error(L, "Invalid material id %d", id);
	}
	material_id = id;
	return 0;
}

static void
write_stream(void *ud, int index, struct soluna_material_stream_item *item) {
	struct stream_write_context *ctx = (struct stream_write_context *)ud;
	(void)index;
	item->x = 0.0f;
	item->y = 0.0f;
	item->sprite = ctx->sprite;
	item->payload = &ctx->payload;
}

static int
lmaterial_sprite(lua_State *L) {
	if (material_id <= 0) {
		return luaL_error(L, "Material is not registered");
	}

	struct stream_write_context ctx = {
		.sprite = luaL_checkinteger(L, 1) - 1,
		.payload = {
			.value = (float)luaL_optnumber(L, 2, 1.0),
		},
	};

	struct soluna_material_stream stream;
	soluna_material_error err = soluna_material_push_stream(
		material_id,
		1,
		sizeof(ctx.payload),
		write_stream,
		&ctx,
		&stream);
	if (err != NULL) {
		return luaL_error(L, "%s", err);
	}

	lua_pushexternalstring(L, stream.data, stream.size, free_material_stream, NULL);
	return 1;
}

static int
luaopen_ext_material_custom(lua_State *L) {
	luaL_Reg libs[] = {
		{ "register", lregister },
		{ "sprite", lmaterial_sprite },
		{ NULL, NULL },
	};
	luaL_newlib(L, libs);
	return 1;
}

EXTLUA_EXPORT int
extlua_init(lua_State *L) {
	luaapi_init(L);
	sokolapi_init(L);
	solunaapi_init(L);
	luaL_Reg libs[] = {
		{ "ext.material.custom", luaopen_ext_material_custom },
		{ NULL, NULL },
	};
	luaL_newlib(L, libs);
	return 1;
}
```

## 验证

先用一个无副作用函数验证加载链路：

```lua
local mod = require "ext.hello"
assert(mod.hello() == "Hello from extlua")
```

常见错误检查：

- `Can't load extlua ...`：动态库名不在 `package.cpath` 可查找范围内，或文件扩展名不匹配。
- `Need C function`：`extlua_entry` 对应符号没有导出，或函数签名不正确。
- `Invalid external libs, maybe lua version mismatch`：入口没有返回模块表，或没有先调用 `luaapi_init(L)`。
- `No preload extlua 'name'`：Lua `require` 的模块名不在入口返回表中。
- `soluna ext api version mismatch`：插件调用了 `solunaapi_init(L)`，但插件头文件和 runtime 的 `SOLUNA_EXT_API_VERSION` 不一致。
- `Invalid material id` / `... material is not registered`：插件没有记录 `ctx.id`，或生成 stream / 提交 material 时使用了无效 id。
- `Invalid material marker`：提交的 stream 不是当前 material id 生成的，通常是混用了不同 material helper 的 stream。
- `Invalid material primitive count`：C material 对 primitive 数量有分组要求，例如某些 quad-like material 需要 4 个 item。
