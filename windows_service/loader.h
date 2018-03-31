/*
* Copyright (c) 2013-2018, Sfecas D. Efstathios <stathizzz@gmail.com>.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the WhatIsMyIp Project nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/
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
