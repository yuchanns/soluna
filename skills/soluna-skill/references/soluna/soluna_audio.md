# Soluna Audio API

本文件覆盖游戏侧音频接口。游戏通常只需要 `soluna.load_sounds` 和 `soluna.play_sound`，不直接使用底层 `soluna.audio` 模块。

## Sound Bundle

用 datalist 声明声音名和文件路径：

```text
--
name : click
filename : asset/sounds/click.wav
--
name : confirm
filename : asset/sounds/confirm.wav
```

路径按项目资产组织决定。

## 加载声音

启动时加载 sound bundle：

```lua
local soluna = require "soluna"

soluna.load_sounds "asset/sounds.dl"
```

`load_sounds` 只建立声音名到文件路径的映射；实际播放时按名称触发。

## 播放声音

```lua
local soluna = require "soluna"

soluna.play_sound "click"
```

常见用法是在输入确认、按钮点击、状态变化时播放：

```lua
function callback.mouse_button(button, state)
	if button == 0 and state == 0 then
		soluna.play_sound "confirm"
	end
end
```

## 平台注意事项

- wasm 上音频播放通常需要用户交互后才能被浏览器允许。
- 声音文件必须被打包到 runtime 可访问的位置。
- 不要在每次播放前重复 `load_sounds`；初始化阶段加载一次即可。
