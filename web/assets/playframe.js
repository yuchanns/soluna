(function () {
  const qs = (selector, root = document) => root.querySelector(selector);

  function inferBaseFromReferrer() {
    if (!document.referrer) return "";
    try {
      const ref = new URL(document.referrer);
      return inferBaseFromPath(ref.pathname);
    } catch (err) {
      return "";
    }
  }

  function inferBaseFromPath(pathname) {
    const path = pathname || "/";
    const trimmed = path.replace(/\/index\.html$/, "").replace(/\/$/, "");
    if (trimmed === "") return "";
    const segments = trimmed.split("/").filter(Boolean);
    if (segments.length === 0) return "";
    const exampleIndex = segments.indexOf("examples");
    if (exampleIndex !== -1) {
      const base = segments.slice(0, exampleIndex).join("/");
      return base ? `/${base}` : "";
    }
    if (segments[segments.length - 1] === "playframe") {
      const base = segments.slice(0, -1).join("/");
      return base ? `/${base}` : "";
    }
    return `/${segments.join("/")}`;
  }

  function getBasePath() {
    const params = new URLSearchParams(window.location.search);
    const baseParam = params.get("base");
    if (baseParam) {
      const trimmed = baseParam.replace(/\/$/, "");
      if (trimmed.startsWith("/")) {
        return trimmed;
      }
    }
    const refBase = inferBaseFromReferrer();
    if (refBase) return refBase;
    return inferBaseFromPath(window.location.pathname);
  }

  function getExampleId() {
    const params = new URLSearchParams(window.location.search);
    const value = params.get("example");
    if (!value) return null;
    const trimmed = value.trim();
    return trimmed.length > 0 ? trimmed : null;
  }

  function postMessage(type, payload) {
    if (!window.parent || window.parent === window) return;
    const message = Object.assign({ type }, payload || {});
    window.parent.postMessage(message, window.location.origin);
  }

  function setStatus(text) {
    postMessage("status", { text });
  }

  function setNote(text) {
    postMessage("note", { text: text || "" });
  }

  function appendConsole(text, isError) {
    postMessage("console", { text, isError: Boolean(isError) });
  }

  const logAudio = (...args) => {
    try {
      console.info("[audio.unlock]", ...args);
    } catch (_) {}
  };

  function setupAudioUnlock() {
    const NativeAudioContext = window.AudioContext || window.webkitAudioContext;
    if (!NativeAudioContext) {
      logAudio("no AudioContext available; skip unlock shim");
      return () => {};
    }

    logAudio("installing AudioContext unlock shim");

    const contexts = new Set();
    let lifeline = null;

    const trackContext = (ctx) => {
      if (!ctx || typeof ctx.addEventListener !== "function") return;
      const onStateChange = () => {
        logAudio("statechange", ctx.state);
        if (ctx.state === "closed") {
          contexts.delete(ctx);
          ctx.removeEventListener("statechange", onStateChange);
        }
      };
      try {
        ctx.addEventListener("statechange", onStateChange);
      } catch (_) {}
    };

    const reconnectScriptNodes = (ctx) => {
      const nodes = ctx && ctx.__solunaScriptNodes;
      if (!nodes) return;
      nodes.forEach((node) => {
        try {
          node.disconnect();
          node.connect(ctx.destination);
        } catch (_) {}
      });
    };

    const primeContext = (ctx) => {
      if (!ctx || ctx.__solunaPrimed) return;
      ctx.__solunaPrimed = true;
      try {
        const buffer = ctx.createBuffer(1, 1, ctx.sampleRate || 44100);
        const source = ctx.createBufferSource();
        source.buffer = buffer;
        source.connect(ctx.destination);
        source.start(0);
      } catch (_) {}
    };

    const wrapContext = (Ctor) => {
      const Wrapped = function (...args) {
        const ctx = new Ctor(...args);
        contexts.add(ctx);
        logAudio("AudioContext created", ctx.state);
        trackContext(ctx);
        if (!ctx.__solunaScriptNodes && typeof ctx.createScriptProcessor === "function") {
          const original = ctx.createScriptProcessor.bind(ctx);
          ctx.__solunaScriptNodes = new Set();
          ctx.createScriptProcessor = function (...spArgs) {
            const node = original(...spArgs);
            try {
              ctx.__solunaScriptNodes.add(node);
            } catch (_) {}
            return node;
          };
        }
        return ctx;
      };
      Wrapped.prototype = Ctor.prototype;
      Object.defineProperty(Wrapped.prototype, "constructor", {
        value: Wrapped,
        writable: true,
        configurable: true,
        enumerable: false,
      });
      return Wrapped;
    };

    const WrappedAudioContext = wrapContext(NativeAudioContext);
    if (window.AudioContext) {
      window.AudioContext = WrappedAudioContext;
    }
    if (window.webkitAudioContext) {
      window.webkitAudioContext = WrappedAudioContext;
    }

    const resumeAll = () => {
      if (contexts.size === 0) {
        logAudio("resumeAll: no contexts yet");
        try {
          lifeline = lifeline || new NativeAudioContext();
          contexts.add(lifeline);
          logAudio("created lifeline AudioContext", lifeline.state);
          trackContext(lifeline);
        } catch (err) {
          logAudio("failed to create lifeline AudioContext", err && err.message);
        }
      }
      contexts.forEach((ctx) => {
        if (!ctx || typeof ctx.resume !== "function") return;
        if (ctx.state === "closed") {
          logAudio("skip closed context");
          return;
        }
        if (ctx.state === "running") {
          logAudio("context already running");
          primeContext(ctx);
          reconnectScriptNodes(ctx);
          return;
        }
        try {
          logAudio("resuming AudioContext", ctx.state);
          const result = ctx.resume();
          const afterResume = () => {
            logAudio("context resumed", ctx.state);
            reconnectScriptNodes(ctx);
            primeContext(ctx);
          };
          if (result && typeof result.then === "function") {
            result.then(afterResume).catch(() => {});
          } else {
            afterResume();
          }
        } catch (_) {
          /* ignore resume errors */
        }
      });
    };

    ["pointerdown", "touchend", "click", "keydown"].forEach((type) => {
      window.addEventListener(type, resumeAll, { once: true, capture: true });
    });

    return resumeAll;
  }

  const resumeAudioUnlock = setupAudioUnlock();
  if (typeof resumeAudioUnlock === "function") {
    setTimeout(resumeAudioUnlock, 0);
  }

  function createZip(entries) {
    const CRC32_TABLE = createZip.crcTable || (createZip.crcTable = (() => {
      const table = new Uint32Array(256);
      for (let n = 0; n < 256; n++) {
        let c = n;
        for (let k = 0; k < 8; k++) {
          c = (c & 1) ? (0xedb88320 ^ (c >>> 1)) : (c >>> 1);
        }
        table[n] = c >>> 0;
      }
      return table;
    })());

    const calculateCRC32 = (data) => {
      let crc = 0xffffffff;
      for (let i = 0; i < data.length; i++) {
        crc = CRC32_TABLE[(crc ^ data[i]) & 0xff] ^ (crc >>> 8);
      }
      return (crc ^ 0xffffffff) >>> 0;
    };

    const files = entries.map((entry) => {
      const nameBytes = new TextEncoder().encode(entry.name);
      const data = entry.data;
      return {
        nameBytes,
        data,
        nameLength: nameBytes.length,
        size: data.length,
        crc32: calculateCRC32(data),
        offset: 0,
      };
    });

    let localSize = 0;
    let centralSize = 0;
    files.forEach((file) => {
      localSize += 30 + file.nameLength + file.size;
      centralSize += 46 + file.nameLength;
    });

    const totalSize = localSize + centralSize + 22;
    const output = new Uint8Array(totalSize);
    const view = new DataView(output.buffer);

    let offset = 0;
    files.forEach((file) => {
      file.offset = offset;
      view.setUint32(offset, 0x04034b50, true);
      view.setUint16(offset + 4, 20, true);
      view.setUint16(offset + 6, 0, true);
      view.setUint16(offset + 8, 0, true);
      view.setUint32(offset + 14, file.crc32, true);
      view.setUint32(offset + 18, file.size, true);
      view.setUint32(offset + 22, file.size, true);
      view.setUint16(offset + 26, file.nameLength, true);
      offset += 30;
      output.set(file.nameBytes, offset);
      offset += file.nameLength;
      output.set(file.data, offset);
      offset += file.size;
    });

    const centralDirectoryOffset = offset;
    files.forEach((file) => {
      view.setUint32(offset, 0x02014b50, true);
      view.setUint16(offset + 4, 20, true);
      view.setUint16(offset + 6, 20, true);
      view.setUint16(offset + 8, 0, true);
      view.setUint16(offset + 10, 0, true);
      view.setUint32(offset + 16, file.crc32, true);
      view.setUint32(offset + 20, file.size, true);
      view.setUint32(offset + 24, file.size, true);
      view.setUint16(offset + 28, file.nameLength, true);
      view.setUint32(offset + 42, file.offset, true);
      offset += 46;
      output.set(file.nameBytes, offset);
      offset += file.nameLength;
    });

    const centralDirectorySize = offset - centralDirectoryOffset;
    view.setUint32(offset, 0x06054b50, true);
    view.setUint16(offset + 4, 0, true);
    view.setUint16(offset + 6, 0, true);
    view.setUint16(offset + 8, files.length, true);
    view.setUint16(offset + 10, files.length, true);
    view.setUint32(offset + 12, centralDirectorySize, true);
    view.setUint32(offset + 16, centralDirectoryOffset, true);
    view.setUint16(offset + 20, 0, true);

    return output;
  }

  async function loadText(url) {
    const response = await fetch(url);
    if (!response.ok) {
      throw new Error(`Failed to load ${url}`);
    }
    return response.text();
  }

  async function loadBuffer(url) {
    const response = await fetch(url);
    if (!response.ok) {
      throw new Error(`Failed to load ${url}`);
    }
    return response.arrayBuffer();
  }

  async function loadFontZip(base) {
    const fontFiles = [
      { url: `${base}/fonts/arial.ttf`, name: "asset/font/arial.ttf" },
      { url: `${base}/fonts/SourceHanSansSC-Regular.ttf`, name: "asset/font/SourceHanSansSC-Regular.ttf" },
    ];
    const entries = [];
    for (const file of fontFiles) {
      const buf = await loadBuffer(file.url);
      entries.push({ name: file.name, data: new Uint8Array(buf) });
    }
    return createZip(entries);
  }

  function loadRuntimeScript(src) {
    return new Promise((resolve, reject) => {
      const script = document.createElement("script");
      script.src = src;
      script.onload = () => resolve();
      script.onerror = () => reject(new Error(`Failed to load ${src}`));
      document.head.appendChild(script);
    });
  }

  let resizeHandler = null;

  function setupCanvasResize(canvas) {
    if (!canvas) return;
    if (resizeHandler) {
      window.removeEventListener("resize", resizeHandler);
    }
    resizeHandler = () => {
      const rect = canvas.getBoundingClientRect();
      const ratio = window.devicePixelRatio || 1;
      canvas.width = Math.max(1, Math.floor(rect.width * ratio));
      canvas.height = Math.max(1, Math.floor(rect.height * ratio));
    };
    resizeHandler();
    window.addEventListener("resize", resizeHandler);
  }

  async function initPlay() {
    const exampleId = getExampleId();
    if (!exampleId) return;
    const base = getBasePath();

    setStatus("Loading example source...");
    const canvas = qs("#soluna-canvas");
    setupCanvasResize(canvas);

    let sourceText = "";
    try {
      sourceText = await loadText(`${base}/runtime/test/${exampleId}.lua`);
    } catch (err) {
      setStatus("Failed to load example source.");
      setNote(err.message);
      return;
    }

    if (!window.crossOriginIsolated && "serviceWorker" in navigator) {
      try {
        await navigator.serviceWorker.register(`${base}/coi-serviceworker.min.js`);
        if (!navigator.serviceWorker.controller) {
          setStatus("Reloading for cross-origin isolation...");
          window.location.reload();
          return;
        }
      } catch (err) {
        setStatus("Failed to register COI service worker.");
        setNote(err.message);
        return;
      }
    }

    setStatus("Preparing assets...");
    let assetBuffer;
    try {
      assetBuffer = await loadBuffer(`${base}/runtime/asset.zip`);
    } catch (err) {
      setStatus("Failed to load asset archive.");
      setNote(err.message);
      return;
    }

    let sampleWasmBuffer = null;
    try {
      sampleWasmBuffer = await loadBuffer(`${base}/runtime/sample.wasm`);
    } catch (err) {
      setStatus("Failed to load external module sample.wasm.");
      setNote(err.message);
      return;
    }

    setStatus("Preparing fonts...");
    let fontZip;
    try {
      fontZip = await loadFontZip(base);
    } catch (err) {
      setStatus("Failed to load font assets.");
      setNote(err.message);
      return;
    }

    const mainGame = "entry : main.lua\nhigh_dpi : true\ntext_sampler :\n  min_filter : linear\n  mag_filter : linear\nextlua_entry : extlua_init\nextlua_preload : sample\n";
    const mainLuaBytes = new TextEncoder().encode(sourceText);
    const mainGameBytes = new TextEncoder().encode(mainGame);
    const mainZip = createZip([
      { name: "main.lua", data: mainLuaBytes },
      { name: "main.game", data: mainGameBytes },
    ]);

    setStatus("Starting Soluna runtime...");

    window.Module = {
      arguments: [
        "zipfile=/data/main.zip:/data/asset.zip:/data/font.zip",
        "cpath=/data/?.wasm",
      ],
      canvas,
      print(text) {
        appendConsole(String(text || ""), false);
      },
      printErr(text) {
        appendConsole(String(text || ""), true);
      },
      locateFile(path) {
        if (path === "soluna.wasm") {
          return `${base}/runtime/soluna.wasm`;
        }
        if (path.endsWith(".wasm")) {
          return `${base}/runtime/${path}`;
        }
        return path;
      },
      preRun: [
        function () {
          Module.FS_createPath("/", "data", true, true);
          Module.addRunDependency("asset-zip");
          Module.addRunDependency("main-zip");
          Module.addRunDependency("font-zip");
          Module.addRunDependency("sample-wasm");
          Module.FS.writeFile("/data/asset.zip", new Uint8Array(assetBuffer), { canOwn: true });
          Module.FS.writeFile("/data/main.zip", mainZip, { canOwn: true });
          Module.FS.writeFile("/data/font.zip", fontZip, { canOwn: true });
          Module.FS.writeFile("/data/sample.wasm", new Uint8Array(sampleWasmBuffer), { canOwn: true });
          Module.removeRunDependency("asset-zip");
          Module.removeRunDependency("main-zip");
          Module.removeRunDependency("font-zip");
          Module.removeRunDependency("sample-wasm");
        },
      ],
      onAbort(reason) {
        setStatus("Runtime aborted.");
        setNote(String(reason || "Unknown error"));
      },
    };

    try {
      await loadRuntimeScript(`${base}/runtime/soluna.js`);
      postMessage("ready");
    } catch (err) {
      setStatus("Failed to load runtime.");
      setNote(err.message);
    }
  }

  if (document.readyState !== "loading") {
    initPlay();
  } else {
    document.addEventListener("DOMContentLoaded", initPlay);
  }
})();
