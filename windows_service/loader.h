#ifndef __LOADER_H
#define __LOADER_H

#ifdef __cplusplus
	extern "C" {
#endif

#include <stdio.h>

#ifdef WIN32

#if defined(LOADER_DECLARE_STATIC)
#define LOADER_DECLARE(type)			type __stdcall
#define LOADER_DECLARE_NONSTD(type)	type __cdecl
#define LOADER_DECLARE_DATA
#elif defined(LOADER_EXPORTS)
#define LOADER_DECLARE(type)			__declspec(dllexport) type __stdcall
#define LOADER_DECLARE_NONSTD(type)	__declspec(dllexport) type __cdecl
#define LOADER_DECLARE_DATA			__declspec(dllexport)
#else
#define LOADER_DECLARE(type)			__declspec(dllimport) type __stdcall
#define LOADER_DECLARE_NONSTD(type)	__declspec(dllimport) type __cdecl
#define LOADER_DECLARE_DATA			__declspec(dllimport)
#endif

#else

#define O_BINARY 0
#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(LOADER_API_VISIBILITY)
#define LOADER_DECLARE(type)		__attribute__((visibility("default"))) type
#define LOADER_DECLARE_NONSTD(type)	__attribute__((visibility("default"))) type
#define LOADER_DECLARE_DATA		__attribute__((visibility("default")))
#define LOADER_MOD_DECLARE(type)	__attribute__((visibility("default"))) type
#define LOADER_MOD_DECLARE_NONSTD(type)	__attribute__((visibility("default"))) type
#define LOADER_MOD_DECLARE_DATA		__attribute__((visibility("default")))
#define LOADER_DECLARE_CLASS		__attribute__((visibility("default")))
#else
#define LOADER_DECLARE(type)		type
#define LOADER_DECLARE_NONSTD(type)	type
#define LOADER_DECLARE_DATA
#define LOADER_MOD_DECLARE(type)	type
#define LOADER_MOD_DECLARE_NONSTD(type)	type
#define LOADER_MOD_DECLARE_DATA
#define LOADER_DECLARE_CLASS
#endif

#endif



typedef int (*dso_func_t) (void);

#ifdef WIN32
#include <windows.h>
typedef HINSTANCE dso_lib_t;
#else
typedef void *dso_lib_t;
#endif

typedef void *dso_data_t;

LOADER_DECLARE(void) destroy_dll(dso_lib_t *lib);
LOADER_DECLARE(dso_lib_t) open_dll(const char *path, int global, char **err);
LOADER_DECLARE(dso_func_t) func_sym_dll(dso_lib_t lib, const char *sym, char **err);
LOADER_DECLARE(void *) data_sym_dll(dso_lib_t lib, const char *sym, char **err);

#endif

#ifdef __cplusplus
	}
#endif
