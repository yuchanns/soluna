#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)

#include <windows.h>
#define MAX_NAME 1024

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
    int ok = 0;
    HGDIOBJ oldobj = SelectObject(hdc, hfont);
	uint32_t tags[2] = {0x66637474/*ttcf*/, 0};
	int i;
	for (i=0;i<2;i++) {
		uint32_t tag = tags[i];
		DWORD bytes = GetFontData(hdc, tag, 0, 0, 0);
        if (bytes != GDI_ERROR) {
			void *buf = lua_newuserdatauv(L, bytes, 0);
			bytes = GetFontData(hdc, tag, 0, buf, bytes);
			if (bytes != GDI_ERROR) {
				ok = 1;
				break;
			}
		}
	}
	SelectObject(hdc, oldobj);
	DeleteObject(hfont);
	DeleteDC(hdc);
	if (!ok) {
		return luaL_error(L, "Read font data failed");
	}
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

