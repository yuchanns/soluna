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
		console.log("[audio] resume handler: device_index=" + device_index);
		const miniaudio = window.miniaudio;
		if (!miniaudio || typeof miniaudio.get_device_by_index !== "function") {
			console.warn("[audio] resume handler: window.miniaudio not available");
			return;
		}
		const device = miniaudio.get_device_by_index(device_index);
		if (!device || !device.webaudio || typeof device.webaudio.resume !== "function") {
			console.warn("[audio] resume handler: device or webaudio context not available");
			return;
		}
		const ctx = device.webaudio;
		console.log("[audio] resume handler: AudioContext state=" + ctx.state);
		const resume = () => {
			console.log("[audio] gesture detected, AudioContext state=" + ctx.state);
			if (ctx.state === "running") {
				console.log("[audio] AudioContext already running");
				return;
			}
			const p = ctx.resume();
			if (p && typeof p.then === "function") {
				p.then(() => {
					console.log("[audio] AudioContext resumed OK, state=" + ctx.state);
				}).catch((err) => {
					console.error("[audio] AudioContext resume FAILED", err);
				});
			}
		};
		["click", "touchend", "keydown"].forEach((event_type) => {
			document.addEventListener(event_type, resume, { once: true });
		});
		console.log("[audio] resume handler installed for click/touchend/keydown");
	} catch (err) {
		console.error("[audio] Failed to install WebAudio resume handler", err);
	}
});

static void
inject_webaudio_resume(struct ma_engine *engine) {
	ma_device *device;
	if (engine == NULL) {
		printf("[audio] inject_webaudio_resume: engine is NULL\n");
		return;
	}
	device = ma_engine_get_device(engine);
	if (device == NULL || device->pContext == NULL) {
		printf("[audio] inject_webaudio_resume: device or context is NULL\n");
		return;
	}
	if (device->pContext->backend != ma_backend_webaudio) {
		printf("[audio] inject_webaudio_resume: backend is not webaudio (%d)\n", device->pContext->backend);
		return;
	}
	if (device->webaudio.deviceIndex < 0) {
		printf("[audio] inject_webaudio_resume: invalid deviceIndex (%d)\n", device->webaudio.deviceIndex);
		return;
	}
	printf("[audio] inject_webaudio_resume: deviceIndex=%d\n", device->webaudio.deviceIndex);
	soluna_webaudio_resume_on_gesture(device->webaudio.deviceIndex);
}
#endif

static ma_result
zr_open(ma_vfs* pVFS, const char* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile) {
	struct custom_vfs *vfs = (struct custom_vfs *)pVFS;
	if (openMode != MA_OPEN_MODE_READ)
		return MA_NOT_IMPLEMENTED;
	printf("[audio] zr_open: '%s'\n", pFilePath);
	zipreader_file zf = zipreader_open(vfs->zipnames, pFilePath);
	if (zf == NULL) {
		printf("[audio] zr_open: FAILED for '%s'\n", pFilePath);
		return MA_ERROR;
	}
	printf("[audio] zr_open: OK for '%s'\n", pFilePath);
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
		printf("[audio] init: using zip VFS (zipnames=%p)\n", (void *)e->vfs.zipnames);
	} else {
		printf("[audio] init: using local VFS (no ziplist provided, arg type=%s)\n",
			lua_typename(L, lua_type(L, 1)));
	}
	
    ma_resource_manager_config config = ma_resource_manager_config_init();
	config.pVFS = &e->vfs;
	
	ma_result r = ma_resource_manager_init(&config, &e->rm);
	if (r != MA_SUCCESS) {
		printf("[audio] init: ma_resource_manager_init FAILED: %s\n", ma_result_description(r));
		return luaL_error(L, "ma_resource_manager_init() error : %s", ma_result_description(r));
	}
	printf("[audio] init: resource manager OK\n");
		
	ma_engine_config ec = ma_engine_config_init();
	ec.pResourceManager = &e->rm;
	r = ma_engine_init(&ec, &e->engine);
	if (r != MA_SUCCESS) {
		printf("[audio] init: ma_engine_init FAILED: %s\n", ma_result_description(r));
		return luaL_error(L, "ma_engine_init() error : %s", ma_result_description(r));
	}
	printf("[audio] init: engine OK\n");
#if defined(__EMSCRIPTEN__)
	inject_webaudio_resume(&e->engine);
	printf("[audio] init: webaudio resume handler injected\n");
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
	
	printf("[audio] play: file='%s' engine=%p\n", filename, (void *)engine);
	ma_result r = ma_engine_play_sound(engine, filename, NULL);
	if (r != MA_SUCCESS) {
		printf("[audio] play: FAILED for '%s': %s\n", filename, ma_result_description(r));
	} else {
		printf("[audio] play: OK for '%s'\n", filename);
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
