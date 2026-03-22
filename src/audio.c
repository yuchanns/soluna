#include <lua.h>
#include <lauxlib.h>
#include <stdio.h>

#include "zipreader.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define MA_NO_WIN32_FILEIO
#define MA_NO_MP3
#define MA_NO_FLAC
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

FILE * fopen_utf8(const char *filename, const char *mode);

static ma_result
vfs_open_local(ma_vfs* pVFS, const char* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile) {
	FILE* pFileStd;
	const char* pOpenModeStr;

	MA_ASSERT(pFilePath != NULL);
	MA_ASSERT(openMode  != 0);
	MA_ASSERT(pFile     != NULL);

	(void)pVFS;

	if ((openMode & MA_OPEN_MODE_READ) != 0) {
		if ((openMode & MA_OPEN_MODE_WRITE) != 0) {
			pOpenModeStr = "r+";
		} else {
			pOpenModeStr = "rb";
		}
	} else {
		pOpenModeStr = "wb";
	}
	
	pFileStd = fopen_utf8(pFilePath, pOpenModeStr);
	
	if (pFileStd == NULL) {
		return MA_ERROR;
	}

    *pFile = pFileStd;

    return MA_SUCCESS;
}

struct custom_vfs {
	ma_default_vfs base;
	struct zipreader_name *zipnames;
};

struct custom_engine {
	struct ma_engine engine;
	struct ma_resource_manager rm;
	struct custom_vfs vfs;
};

#if defined(__EMSCRIPTEN__)
EM_JS(void, soluna_webaudio_resume_on_gesture, (int device_index), {
	if (typeof document === "undefined") return;
	try {
		var miniaudio = window.miniaudio;
		if (!miniaudio || typeof miniaudio.get_device_by_index !== "function") {
			return;
		}
		var device = miniaudio.get_device_by_index(device_index);
		if (!device || !device.webaudio || typeof device.webaudio.resume !== "function") {
			return;
		}
		var ctx = device.webaudio;
		var activated = false;
		var activate = function() {
			if (activated) return;
			activated = true;
			["click", "touchend", "keydown"].forEach(function(et) {
				document.removeEventListener(et, activate, true);
			});
			/* Prime audio graph: play a tiny silent buffer to kick-start
			   the audio pipeline (required by some browsers, e.g. Safari). */
			try {
				var buf = ctx.createBuffer(1, 1, ctx.sampleRate || 44100);
				var src = ctx.createBufferSource();
				src.buffer = buf;
				src.connect(ctx.destination);
				src.start(0);
			} catch(e) {}
			/* Resume the context if it is not already running. */
			ctx.resume().then(function() {
				/* After resume succeeds, disconnect and reconnect the
				   ScriptProcessorNode so the browser restarts the
				   onaudioprocess callback reliably. */
				if (device.scriptNode) {
					try {
						device.scriptNode.disconnect();
						device.scriptNode.connect(ctx.destination);
					} catch(e) {}
				}
			}).catch(function(err) {
				console.error("[audio] AudioContext resume failed", err);
			});
		};
		["click", "touchend", "keydown"].forEach(function(et) {
			document.addEventListener(et, activate, true);
		});
	} catch (err) {
		console.error("[audio] Failed to install resume handler", err);
	}
});

static void
inject_webaudio_resume(struct ma_engine *engine) {
	ma_device *device;
	if (engine == NULL)
		return;
	device = ma_engine_get_device(engine);
	if (device == NULL || device->pContext == NULL)
		return;
	if (device->pContext->backend != ma_backend_webaudio)
		return;
	if (device->webaudio.deviceIndex < 0)
		return;
	soluna_webaudio_resume_on_gesture(device->webaudio.deviceIndex);
}
#endif

static ma_result
zr_open(ma_vfs* pVFS, const char* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile) {
	struct custom_vfs *vfs = (struct custom_vfs *)pVFS;
	if (openMode != MA_OPEN_MODE_READ)
		return MA_NOT_IMPLEMENTED;
	zipreader_file zf = zipreader_open(vfs->zipnames, pFilePath);
	if (zf == NULL) {
		printf("[audio] zr_open: FAILED for '%s'\n", pFilePath);
		return MA_ERROR;
	}
	*pFile = (ma_vfs_file)zf;
	return MA_SUCCESS;
}

static ma_result
zr_close(ma_vfs* pVFS, ma_vfs_file file) {
	(void)pVFS;
	zipreader_close((zipreader_file)file);
	return MA_SUCCESS;
}

static ma_result
zr_read(ma_vfs* pVFS, ma_vfs_file file, void* pDst, size_t sizeInBytes, size_t* pBytesRead) {
	(void)pVFS;
	int bytes = (int)sizeInBytes;
	if (bytes!= sizeInBytes || bytes < 0)
		return MA_OUT_OF_RANGE;
	int rd = zipreader_read((zipreader_file)file, pDst, bytes);
	if (rd < 0)
		return MA_IO_ERROR;
	*pBytesRead = rd;
	return MA_SUCCESS;
}

static ma_result
zr_seek(ma_vfs* pVFS, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin) {
	(void)pVFS;
	int whence;
	switch (origin) {
	case ma_seek_origin_start :
		whence = SEEK_SET;
		break;
	case ma_seek_origin_current :
		whence = SEEK_CUR;
		break;
	case ma_seek_origin_end :
		whence = SEEK_END;
		break;
	default :
		return MA_INVALID_ARGS;
	}
	if (zipreader_seek((zipreader_file)file, offset, whence) != 0) {
		return MA_ERROR;
	}
	return MA_SUCCESS;
}

static ma_result
zr_tell(ma_vfs* pVFS, ma_vfs_file file, ma_int64* pCursor) {
	(void)pVFS;
	*pCursor = zipreader_tell((zipreader_file)file);
	if (*pCursor < 0)
		return MA_ERROR;
	return MA_SUCCESS;
}

static ma_result
zr_info(ma_vfs* pVFS, ma_vfs_file file, ma_file_info* pInfo) {
	(void)pVFS;
	pInfo->sizeInBytes = zipreader_size((zipreader_file)file);
	return MA_SUCCESS;
}

static int
laudio_init(lua_State *L) {
	lua_settop(L, 1);
	struct custom_engine *e = (struct custom_engine *)lua_newuserdatauv(L, sizeof(*e), 1);
	
	ma_default_vfs_init(&e->vfs.base, NULL);
	e->vfs.base.cb.onOpen = vfs_open_local;
	e->vfs.zipnames = NULL;
	
	if (lua_isuserdata(L, 1)) {
		e->vfs.zipnames = lua_touserdata(L, 1);
		e->vfs.base.cb.onOpen = zr_open;
		e->vfs.base.cb.onOpenW = NULL;
		e->vfs.base.cb.onClose = zr_close;
		e->vfs.base.cb.onRead = zr_read;
		e->vfs.base.cb.onWrite = NULL;
		e->vfs.base.cb.onSeek = zr_seek;
		e->vfs.base.cb.onTell = zr_tell;
		e->vfs.base.cb.onInfo = zr_info;
		lua_pushvalue(L, 1);
		lua_setiuservalue(L, -2, 1);
	}
	
    ma_resource_manager_config config = ma_resource_manager_config_init();
	config.pVFS = &e->vfs;
	
	ma_result r = ma_resource_manager_init(&config, &e->rm);
	if (r != MA_SUCCESS) {
		return luaL_error(L, "ma_resource_manager_init() error : %s", ma_result_description(r));
	}
		
	ma_engine_config ec = ma_engine_config_init();
	ec.pResourceManager = &e->rm;
	r = ma_engine_init(&ec, &e->engine);
	if (r != MA_SUCCESS) {
		return luaL_error(L, "ma_engine_init() error : %s", ma_result_description(r));
	}
#if defined(__EMSCRIPTEN__)
	inject_webaudio_resume(&e->engine);
#endif
	lua_pushlightuserdata(L, (void *)e);
	
	return 2;
}

static int
laudio_deinit(lua_State *L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	ma_engine *engine = (ma_engine *)lua_touserdata(L, 1);
	ma_engine_uninit(engine);

	return 0;
}

/*
// todo : call ma_sound_init_from_file()

static int
laudio_load(lua_State *L) {
	return 0;
}

static int
laudio_unload(lua_State *L) {
	return 0;
}
*/

static int
laudio_play(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	ma_engine *engine = (ma_engine *)lua_touserdata(L, 1);
	const char *filename = luaL_checkstring(L, 2);
	
	ma_result r = ma_engine_play_sound(engine, filename, NULL);
	if (r != MA_SUCCESS) {
		printf("[audio] play: FAILED for '%s': %s\n", filename, ma_result_description(r));
	}
	return 0;
}

int
luaopen_soluna_audio(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "init", laudio_init },
		{ "deinit", laudio_deinit },
		{ "play", laudio_play },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
