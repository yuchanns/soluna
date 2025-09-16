local lm = require "luamake"
local fs = require "bee.filesystem"

local plat = (function ()
    if lm.os == "windows" then
        if lm.compiler == "gcc" then
            return "mingw"
        end
        if lm.cc == "clang-cl" then
            return "clang-cl"
        end
        return "msvc"
    end
    return lm.os
end)()

lm.platform = plat
lm.basedir = lm:path "."
lm.bindir = ("bin/%s/%s"):format(plat, lm.mode)

lm:conf({
  cxx = "c++20",
  clang = {
    c = "c11",
  },
  flags = {
    "-O2",
  },
  msvc = {
    c = "c11",
    flags = {
      "-W3",
      "-utf-8",
      "-experimental:c11atomics",
      "/wd4244",
      "/wd4267",
      "/wd4305",
      "/wd4996",
      "/wd4018",
      "/wd4113",
    },
    defines = {
      "_CRT_SECURE_NO_WARNINGS",
      "_CRT_NONSTDC_NO_DEPRECATE",
      "_CRT_SECURE_NO_DEPRECATE"
    },
  },
  mingw = {
    c = "c99",
  },
  gcc = {
    c = "c11",
    flags = {
      "-Wall",
    },
    defines = {
      "_POSIX_C_SOURCE=199309L",
      "_GNU_SOURCE",
    },
    links = {
      "m",
      lm.os ~= "windows" and "fontconfig",
    },
  },
  defines = {
    -- "SOKOL_DEBUG",
  }
})

for path in fs.pairs(lm.basedir .. "/clibs") do
  if fs.exists(path / "make.lua") then
    local name = path:stem():string()
    local makefile = ("%s/clibs/%s/make.lua"):format(lm.basedir, name)
    lm:import(makefile)
  end
end

lm:exe "soluna" {
  deps = {
    "lua_src",
    "soluna_src",
    "ltask_src",
    "yoga_src",
    "compile_lua_code",
    "compile_shaders",
    "datalist_src",
  },
}

lm:phony "precompile" {
  deps = {
    "compile_shaders",
    "compile_lua_code",
  },
}
