# Soluna WebAssembly 部署参考

Soluna wasm 部署的目标是把浏览器 runtime、游戏 zip 和启动页放到同一个静态站点中。部署可以手动完成，也可以用 GitHub Actions 自动完成。

## 产物

浏览器部署至少需要这些文件：

```text
site/
├── index.html
├── runtime/
│   ├── soluna.js
│   ├── soluna.wasm
│   └── main.zip
└── asset/
```

`main.zip` 至少包含：

```text
main.game
main.lua
```

如果游戏把资产放进 zip，zip 内路径必须与 Lua 代码加载路径一致：

```text
asset/sprites.dl
asset/font/GameFont.ttf
```

也可以把资产拆成多个 zip，并在启动参数里用 `zipfile=` 依次挂载。

## 手动部署

1. 获取 wasm runtime。

可以从 Soluna releases 下载 wasm 产物，或从源码构建：

```bash
git clone --recursive https://github.com/cloudwu/soluna
cd soluna
luamake -mode release -compiler emcc
```

构建后找到 `soluna.js` 和 `soluna.wasm`，复制到静态站点的 `runtime/` 目录。

2. 打包游戏。

在游戏项目根目录执行：

```bash
zip -qr main.zip \
  main.game \
  main.lua \
  asset \
  game
```

按项目实际目录调整文件列表，然后复制到 `site/runtime/main.zip`。

3. 编写启动页。

启动页需要加载 `soluna.js`，创建 canvas，把 `main.zip` 写入 Emscripten 文件系统，并传入 `zipfile=` 参数。具体导出函数名可能随 Soluna runtime 版本变化；编写时以当前 `soluna.js` 的实际导出为准。

```html
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Soluna Game</title>
  <style>
    html, body {
      margin: 0;
      width: 100%;
      height: 100%;
      background: #000;
    }
    canvas {
      display: block;
      width: 100vw;
      height: 100vh;
    }
  </style>
</head>
<body>
  <canvas id="canvas"></canvas>
  <script type="module">
    import createApp from "./runtime/soluna.js";

    const canvas = document.getElementById("canvas");
    const mainZip = new Uint8Array(await (await fetch("./runtime/main.zip")).arrayBuffer());

    await createApp({
      canvas,
      arguments: [
        "zipfile=/data/main.zip",
      ],
      preRun(module) {
        module.FS_createPath("/", "data", true, true);
        module.FS.writeFile("/data/main.zip", mainZip, { canOwn: true });
      },
    });
  </script>
</body>
</html>
```

如果 runtime 封装层不是 `default export`，先检查 `soluna.js` 的导出形状，再调整 import 和调用方式。

4. 配置静态服务器。

服务器应正确服务 wasm MIME type：

```text
application/wasm
```

浏览器要求 cross-origin isolation，配置 HTTP header：

```text
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

无法配置 header 的静态托管，可以在启动 runtime 前注册 COI service worker。service worker 会影响缓存与刷新行为，部署后用隐私窗口或清理站点数据验证。

5. 本地验证。

不要直接用 `file://` 打开页面。用本地 HTTP server 验证：

```bash
cd site
python3 -m http.server 8080
```

打开：

```text
http://127.0.0.1:8080/
```

## GitHub Actions 部署

自动部署一般分四步：checkout 游戏项目、按 Soluna README 的方式调用官方 action 构建 runtime、打包游戏、上传静态站点。

推荐先 checkout 固定 commit 的 Soluna 到子目录，再调用 `./soluna/.github/actions/soluna`。这个 action 会输出 native runtime、wasm runtime 和 JS glue 路径。

```yaml
name: Deploy Web

on:
  workflow_dispatch:
  push:
    branches: [main]

permissions:
  contents: read
  pages: write
  id-token: write

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v6

      - uses: actions/checkout@v6
        with:
          repository: cloudwu/soluna
          ref: <fixed-soluna-commit>
          path: soluna
          submodules: recursive

      - uses: ./soluna/.github/actions/soluna
        id: soluna
        with:
          soluna_path: soluna

      - name: Prepare site
        run: |
          mkdir -p site/runtime
          cp "${{ steps.soluna.outputs.SOLUNA_JS_PATH }}" site/runtime/soluna.js
          cp "${{ steps.soluna.outputs.SOLUNA_WASM_PATH }}" site/runtime/soluna.wasm
          cp web/index.html site/index.html
          zip -qr site/runtime/main.zip main.game main.lua asset game

      - uses: actions/upload-pages-artifact@v4
        with:
          path: site

  deploy:
    needs: build
    runs-on: ubuntu-latest
    environment:
      name: github-pages
      url: ${{ steps.deployment.outputs.page_url }}
    steps:
      - id: deployment
        uses: actions/deploy-pages@v4
```

按项目实际目录调整：

- `web/index.html`
- `main.game`
- `main.lua`
- `asset`
- `game`
- `<fixed-soluna-commit>`
- `site/runtime`

## 部署检查表

- `index.html` 能加载到 `runtime/soluna.js`。
- `soluna.js` 能加载同目录或配置路径下的 `soluna.wasm`。
- `main.zip` 中包含 `main.game` 和入口 Lua 文件。
- `zipfile=` 参数中的路径与写入 Emscripten 文件系统的路径一致。
- Lua 中加载的 asset 路径与 zip 内路径一致。
- wasm 字体已打包，不依赖系统字体。
- 静态服务器为 `.wasm` 返回 `application/wasm`。
- 需要 cross-origin isolation 时，header 或 COI service worker 已生效。
