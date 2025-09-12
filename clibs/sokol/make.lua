local lm = require "luamake"
local fs = require "bee.filesystem"

local deps = {}

local function compile_shader(src, name, lang)
  local dep = name .. "_shader"
  deps[#deps+1] = dep
  local target = lm.builddir .. "/" .. name
  lm:runlua (dep) {
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
    lm.os == "macos" and "metal_macos" or
    lm.os == "linux" and "glsl430" or "unknown"
  if path:extension() == ".glsl" then
    local base = path:stem():string()
    compile_shader(path:string(), base .. ".glsl.h", lang)
  end
end

lm:phony "compile_shaders" {
  deps = deps,
}
