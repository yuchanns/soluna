local lm = require "luamake"

lm.rootdir = lm.basedir .. "/3rd/lua"

lm:source_set "lua_src" {
  sources = {
    "onelua.c",
    lm.os == "windows" and lm.basedir .. "src/winfile.c",
  },
  defines = {
    "MAKE_LIB",
  },
  windows = {
    defines = {
      "fopen=fopen_utf8",
    },
  },
}

lm:exe "lua" {
  sources = {
    "onelua.c",
    lm.os == "windows" and lm.basedir .. "src/winfile.c",
  },
  defines = {
    "MAKE_LUA",
  },
  windows = {
    defines = {
      "fopen=fopen_utf8",
    },
  },
}
