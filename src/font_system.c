#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)

#include <windows.h>
#define MAX_NAME 1024

static void *
free_data(void *ud, void *ptr, size_t oszie, size_t nsize) {
	free(ptr);
	return NULL;
}

static int
lttfdata(lua_State *L) {
	const char * familyName = luaL_checkstring(L, 1);
	WCHAR familyNameW[MAX_NAME];
	int n = MultiByteToWideChar(CP_UTF8,0,(const char*)familyName,-1,familyNameW,MAX_NAME);
	if (n == 0 || n > LF_FACESIZE)
		return luaL_error(L, "Invalid family name %s", familyName);
	HDC hdc = CreateCompatibleDC(0);
	LOGFONTW lf;
	memset(&lf, 0, sizeof(LOGFONT));
	memcpy(lf.lfFaceName, familyNameW, n * sizeof(WCHAR));
	lf.lfCharSet = DEFAULT_CHARSET;
	HFONT hfont = CreateFontIndirectW(&lf); 
	if (!hfont) {
		DeleteDC(hdc);
		return luaL_error(L, "Create font failed: %d", GetLastError());
	}
    HGDIOBJ oldobj = SelectObject(hdc, hfont);
	uint32_t tags[2] = {0x66637474/*ttcf*/, 0};
	int i;
	DWORD bytes = 0;
	char *buf = NULL;
	for (i=0;i<2;i++) {
		uint32_t tag = tags[i];
		bytes = GetFontData(hdc, tag, 0, 0, 0);
        if (bytes != GDI_ERROR) {
			buf = malloc(bytes+1);//lua_newuserdatauv(L, bytes, 0);
			if (buf == NULL)
				return luaL_error(L, "Out of memory : sysfont");
			buf[bytes] = 0;
			bytes = GetFontData(hdc, tag, 0, (void *)buf, bytes);
			if (bytes != GDI_ERROR) {
				break;
			} else {
				free(buf);
				bytes = 0;
			}
		}
	}
	SelectObject(hdc, oldobj);
	DeleteObject(hfont);
	DeleteDC(hdc);
	if (bytes == 0) {
		return luaL_error(L, "Read font data failed");
	}
	lua_pushexternalstring(L, buf, bytes, free_data , NULL);
	return 1;
}

#else

static int
lttfdata(lua_State *) {
	return luaL_error(L, "Unsupport system font");
}

#endif

int
luaopen_font_system(lua_State *L) {
	luaL_Reg l[] = {
		{ "ttfdata", lttfdata },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}

