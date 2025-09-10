local lm = require "luamake"
local fs = require "bee.filesystem"

local function compile_shader(src, target, lang)
  lm:runlua {
    script = lm.basedir .. "/clibs/sokol/shader2c.lua",
    inputs = lm.basedir .. "/" .. src,
    outputs = lm.basedir .. "/" .. target,
    args = {
     "$in",
      "$out",
      lang,
    },
  }
end


for path in fs.pairs("src") do
  local lang = lm.os == "windows" and "hlsl4" or
    lm.os == "macos" and "metal_macos" or "unknown"
  if path:extension() == ".glsl" then
    local base = path:stem():string()
    compile_shader(path:string(), lm.builddir .. "/" .. base .. ".glsl.h", lang)
  end
end
