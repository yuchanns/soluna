local lm = require "luamake"
local fs = require "bee.filesystem"

lm.rootdir = lm.basedir .. "/3rd/yoga"

lm:source_set "yoga_src" {
  sources = {
    "yoga/*.cpp",
    "yoga/*/*.cpp",
  },
  includes = {
    lm.rootdir,
  }
}

local function compile_lua_code(script, src, target)
  local outputs = lm.basedir .. "/" .. target
  lm:runlua {
    script = lm.basedir .. "/clibs/yoga/runlua.lua",
    deps = {
      "yoga_src",
      "lua",
    },
    inputs = lm.basedir .. "/" .. src,
    outputs = outputs,
    args = {
      lm.bindir,
      lm.basedir .. "/" .. script,
      "$in",
      "$out",
    },
  }
end

local lua_code_src = {
  "3rd/ltask/service",
  "3rd/ltask/lualib",
  "src/service",
  "src/lualib",
}

for _, dir in ipairs(lua_code_src) do
  for path in fs.pairs(dir) do
    if path:extension() == ".lua" then
      local base = path:stem():string()
      compile_lua_code("script/lua2c.lua", path:string(), lm.builddir .. "/" .. base .. ".lua.h")
    end
  end
end

for path in fs.pairs("src/data") do
  if path:extension() == ".dl" then
    local base = path:stem():string()
    compile_lua_code("script/datalist2c.lua", path:string(), lm.builddir .. "/" .. base .. ".dl.h")
  end
end
