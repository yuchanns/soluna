#include <lua.h>
#include <lauxlib.h>

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
	if (typeof document === "undefined") {
		console.log("[Soluna Audio] Document not available, skipping resume handler");
		return;
	}
	console.log("[Soluna Audio] Installing WebAudio resume handler for device index:", device_index);
	try {
		const miniaudio = window.miniaudio;
		if (!miniaudio || typeof miniaudio.get_device_by_index !== "function") {
			console.warn("[Soluna Audio] window.miniaudio not available or get_device_by_index not a function");
			return;
		}
		const device = miniaudio.get_device_by_index(device_index);
		if (!device) {
			console.warn("[Soluna Audio] Device not found for index:", device_index);
			return;
		}
		if (!device.webaudio || typeof device.webaudio.resume !== "function") {
			console.warn("[Soluna Audio] Device.webaudio not available or resume not a function");
			return;
		}
		const ctx = device.webaudio;
		console.log("[Soluna Audio] AudioContext state:", ctx.state);
		const resume = () => {
			console.log("[Soluna Audio] Resume triggered, current state:", ctx.state);
			if (ctx.state === "running") {
				console.log("[Soluna Audio] AudioContext already running");
				return;
			}
			const p = ctx.resume();
			if (p && typeof p.catch === "function") {
				p.then(() => {
					console.log("[Soluna Audio] AudioContext resumed successfully, new state:", ctx.state);
				}).catch((err) => {
					console.error("[Soluna Audio] Failed to resume AudioContext", err);
				});
			}
		};
		["click", "touchend", "keydown"].forEach((event_type) => {
			document.addEventListener(event_type, resume, { once: true });
			console.log("[Soluna Audio] Registered listener for:", event_type);
		});
	} catch (err) {
		console.error("[Soluna Audio] Failed to install WebAudio resume handler", err);
	}
});

static void
inject_webaudio_resume(struct ma_engine *engine) {
	ma_device *device;
	if (engine == NULL) {
		EM_ASM(console.error("[Soluna Audio] inject_webaudio_resume: engine is NULL"););
		return;
	}
	device = ma_engine_get_device(engine);
	if (device == NULL || device->pContext == NULL) {
		EM_ASM(console.error("[Soluna Audio] inject_webaudio_resume: device or context is NULL"););
		return;
	}
	if (device->pContext->backend != ma_backend_webaudio) {
		EM_ASM(console.log("[Soluna Audio] inject_webaudio_resume: backend is not WebAudio"););
		return;
	}
	if (device->webaudio.deviceIndex < 0) {
		EM_ASM(console.error("[Soluna Audio] inject_webaudio_resume: invalid device index"););
		return;
	}
	EM_ASM(console.log("[Soluna Audio] inject_webaudio_resume: calling soluna_webaudio_resume_on_gesture"););
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
#if defined(__EMSCRIPTEN__)
		EM_ASM({console.error("[Soluna Audio] zr_open: failed to open file:", UTF8ToString($0));}, pFilePath);
#endif
		return MA_ERROR;
	}
#if defined(__EMSCRIPTEN__)
	EM_ASM({console.log("[Soluna Audio] zr_open: successfully opened file:", UTF8ToString($0));}, pFilePath);
#endif
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
#if defined(__EMSCRIPTEN__)
		EM_ASM(console.log("[Soluna Audio] laudio_init: using zip VFS"););
#endif
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
	} else {
#if defined(__EMSCRIPTEN__)
		EM_ASM(console.log("[Soluna Audio] laudio_init: using local file VFS"););
#endif
	}

    ma_resource_manager_config config = ma_resource_manager_config_init();
	config.pVFS = &e->vfs;

	ma_result r = ma_resource_manager_init(&config, &e->rm);
	if (r != MA_SUCCESS) {
#if defined(__EMSCRIPTEN__)
		EM_ASM({console.error("[Soluna Audio] laudio_init: ma_resource_manager_init error:", UTF8ToString($0));}, ma_result_description(r));
#endif
		return luaL_error(L, "ma_resource_manager_init() error : %s", ma_result_description(r));
	}

	ma_engine_config ec = ma_engine_config_init();
	ec.pResourceManager = &e->rm;
	r = ma_engine_init(&ec, &e->engine);
	if (r != MA_SUCCESS) {
#if defined(__EMSCRIPTEN__)
		EM_ASM({console.error("[Soluna Audio] laudio_init: ma_engine_init error:", UTF8ToString($0));}, ma_result_description(r));
#endif
		return luaL_error(L, "ma_engine_init() error : %s", ma_result_description(r));
	}
#if defined(__EMSCRIPTEN__)
	EM_ASM(console.log("[Soluna Audio] laudio_init: engine initialized, injecting WebAudio resume handler"););
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

#if defined(__EMSCRIPTEN__)
	EM_ASM({console.log("[Soluna Audio] laudio_play: attempting to play file:", UTF8ToString($0));}, filename);
#endif
	ma_result r = ma_engine_play_sound(engine, filename, NULL);
	if (r != MA_SUCCESS) {
#if defined(__EMSCRIPTEN__)
		EM_ASM({console.error("[Soluna Audio] laudio_play: ma_engine_play_sound error:", UTF8ToString($0));}, ma_result_description(r));
#endif
	} else {
#if defined(__EMSCRIPTEN__)
		EM_ASM({console.log("[Soluna Audio] laudio_play: successfully started playing:", UTF8ToString($0));}, filename);
#endif
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
