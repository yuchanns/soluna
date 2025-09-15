local lm = require "luamake"

lm.rootdir = lm.basedir

lm:source_set "soluna_src" {
  sources = {
    "src/*.c",
  },
  includes = {
    "build",
    "3rd/lua",
    "3rd",
    "3rd/yoga",
  },
  macos = {
    frameworks = {
      "IOKit",
      "CoreText",
      "CoreFoundation",
      "Foundation",
      "Cocoa",
      "Metal",
      "MetalKit",
      "QuartzCore",
    },
    flags = {
      "-x objective-c",
    },
  },
  linux = {
    links = {
      "pthread",
      "dl",
      "GL",
      "X11",
      "Xrandr",
      "Xi",
      "Xxf86vm",
      "Xcursor",
      "GLU",
      "asound",
    },
  },
  msvc = {
    ldflags = {
      "-SUBSYSTEM:WINDOWS",
      "xinput.lib",
      "Ws2_32.lib",
      "ntdll.lib",
    },
  },
  mingw = {
    links = {
      "kernel32",
      "user32",
      "shell32",
      "gdi32",
      "dxgi",
      "d3d11",
      "winmm",
      "ws2_32",
      "ntdll",
      "xinput",
    },
    flags = {
      "-Wl,subsystem,windows",
    },
  },
}
