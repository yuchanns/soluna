.PHONY : all clean shader

BUILD=build
BIN=bin
APPNAME=soluna.exe
CC=gcc
CFLAGS=-Wall -O2
LDFLAGS=-Wl,-subsystem,windows -lkernel32 -luser32 -lshell32 -lgdi32 -ldxgi -ld3d11 $(LUALIB) -lwinmm -lws2_32 -lntdll
LUA_EXE=$(BUILD)/lua.exe
SHDC=sokol-shdc.exe

all : $(BIN)/$(APPNAME)

3RDINC=-I3rd

LUAINC=-I3rd/lua
LUASRC:=$(wildcard 3rd/lua/*.c 3rd/lua/*.h)

$(LUA_EXE) : $(LUASRC)
	$(CC) $(CFLAGS) -o $@ 3rd/lua/onelua.c -DMAKE_LUA -std=c99 -lm

COMPILE_C=$(CC) $(CFLAGS) -c -o $@ $<
COMPILE_LUA=$(LUA_EXE) script/lua2c.lua $< $@

LUA_O=$(BUILD)/onelua.o

$(LUA_O) : $(LUASRC)
	$(CC) $(CFLAGS) -c -o $@ 3rd/lua/onelua.c -DMAKE_LIB -std=c99 -lm

SHADER_SRC=$(wildcard src/*.glsl)
SHADER_O=$(patsubst src/%.glsl,$(BUILD)/%.glsl.h,$(SHADER_SRC))
SHADERINC=-I$(BUILD)

$(BUILD)/%.glsl.h : src/%.glsl
	$(SHDC) --input $< --output $@ --slang hlsl4 --format sokol

shader : $(SHADER_O)

MAIN_FULL=$(wildcard src/*.c)
MAIN_C=$(notdir $(MAIN_FULL))
MAIN_O=$(patsubst %.c,$(BUILD)/soluna_%.o,$(MAIN_C))

$(MAIN_O) : $(SHADER_O)

LTASK_FULL=$(wildcard 3rd/ltask/src/*.c)
LTASK_C=$(notdir $(LTASK_FULL))
LTASK_O=$(patsubst %.c,$(BUILD)/ltask_%.o,$(LTASK_C))

LTASK_LUASRC=\
  3rd/ltask/service/root.lua\
  3rd/ltask/service/timer.lua\
  $(wildcard 3rd/ltask/lualib/*.lua src/lualib/*.lua src/service/*.lua)

LTASK_LUACODE=$(patsubst %.lua, $(BUILD)/%.lua.h, $(notdir $(LTASK_LUASRC)))

$(LTASK_LUACODE) : | $(LUA_EXE)

$(BUILD)/%.lua.h : 3rd/ltask/service/%.lua
	$(COMPILE_LUA)

$(BUILD)/%.lua.h : 3rd/ltask/lualib/%.lua
	$(COMPILE_LUA)

$(BUILD)/%.lua.h : src/lualib/%.lua
	$(COMPILE_LUA)

$(BUILD)/%.lua.h : src/service/%.lua
	$(COMPILE_LUA)

$(BUILD)/soluna_embedlua.o : src/embedlua.c $(LTASK_LUACODE)
	$(COMPILE_C) -I$(BUILD) $(LUAINC)

$(BUILD)/soluna_%.o : src/%.c
	$(COMPILE_C) $(LUAINC) $(3RDINC) $(SHADERINC) -Wno-unknown-pragmas
	
$(BUILD)/ltask_%.o : 3rd/ltask/src/%.c
	$(COMPILE_C) $(LUAINC) -D_WIN32_WINNT=0x0601 -DLTASK_EXTERNAL_OPENLIBS=soluna_openlibs
	
DATALIST_O=$(BUILD)/datalist.o

$(DATALIST_O) : 3rd/datalist/datalist.c
	$(COMPILE_C) $(LUAINC)

$(BIN)/$(APPNAME): $(MAIN_O) $(LTASK_O) $(LUA_O) $(DATALIST_O)
	$(CC) -o $@ $^ $(LDFLAGS)
	
clean :
	rm -f $(BIN)/*.exe $(BUILD)/*.o $(BUILD)/*.h
