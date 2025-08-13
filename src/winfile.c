#include <stdio.h>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)

#include <windows.h>

FILE *
fopen_utf8(const char *filename, const char *mode) {
	WCHAR filenameW[FILENAME_MAX + 0x200 + 1];
	int n = MultiByteToWideChar(CP_UTF8,0,(const char*)filename,-1,filenameW,FILENAME_MAX + 0x200);
	if (n == 0)
		return NULL;
	WCHAR modeW[128];
	n = MultiByteToWideChar(CP_UTF8,0,(const char*)mode,-1,modeW, 127);
	if (n == 0)
		return NULL;
	return _wfopen(filenameW, modeW);
}

#else

FILE *
fopen_utf8(const char *filename, const char *mode) {
	return fopen(filename, mode);
}

#endif