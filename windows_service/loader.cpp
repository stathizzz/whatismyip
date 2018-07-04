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
#include "loader.h"

#if defined WIN32
/*use 
* extern "C" { 
* on functions loaded by dll
*/

LOADER_DECLARE(void) destroy_dll(dso_lib_t *lib)
{
	if (lib && *lib) 
	{
		FreeLibrary(*lib);
		*lib = NULL;
	}
}

LOADER_DECLARE(dso_lib_t) open_dll(const char *path, int global, char **err)
{
	HINSTANCE lib;

	lib = LoadLibraryExA(path, NULL, 0);

	if (!lib) 
	{
		LoadLibraryExA(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	}

	if (!lib) 
	{
		DWORD error = GetLastError();
		fprintf(stdout, "dll open error [%ul]\n", error);
		sprintf(*err, "dll open error [%ul]\n", error);
	}
	return lib;
}

LOADER_DECLARE(dso_func_t) func_sym_dll(dso_lib_t lib, const char *sym, char **err)
{
	FARPROC func = GetProcAddress(lib, sym);
	if (!func) 
	{
		DWORD error = GetLastError();
		fprintf(stdout, "dll sym error [%ul]\n", error);
		sprintf(*err, "dll sym error [%ul]\n", error);
	}
	return (dso_func_t) func;
}

LOADER_DECLARE(void *) data_sym_dll(dso_lib_t lib, const char *sym, char **err)
{
	FARPROC addr = GetProcAddress(lib, sym);
	if (!addr) 
	{
		DWORD error = GetLastError();
		fprintf(stdout, "dll sym error [%ul]\n", error);
		sprintf(*err, "dll sym error [%ul]\n", error);
	}
	return (void *) (intptr_t) addr;
}


#else
/*
** {========================================================================
** This is an implementation of loadlib based on the dlfcn interface.
** The dlfcn interface is available in Linux, SunOS, Solaris, IRIX, FreeBSD,
** NetBSD, AIX 4.2, HPUX 11, and  probably most other Unix flavors, at least
** as an emulation layer on top of native functions.
** =========================================================================
*/


#include <dlfcn.h>

void destroy_dll(switch_dso_lib_t *lib)
{
	if (lib && *lib) {
		dlclose(*lib);
		*lib = NULL;
	}
}

switch_dso_lib_t open_dll(const char *path, int global, char **err)
{
	void *lib;

	if (global) {
		lib = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
	} else {
		lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	}

	if (lib == NULL) {
		const char *dlerr = dlerror();
		/* Work around broken uclibc returning NULL on both dlopen() and dlerror() */
		if (dlerr) {
			*err = strdup(dlerr);
		} else {
			*err = strdup("Unknown error");
		}
	}
	return lib;
}

switch_dso_func_t func_sym_dll(switch_dso_lib_t lib, const char *sym, char **err)
{
	void *func = dlsym(lib, sym);
	if (!func) {
		*err = strdup(dlerror());
	}
	return (switch_dso_func_t) (intptr_t) func;
}

void *data_sym_dll(switch_dso_lib_t lib, const char *sym, char **err)
{
	void *addr = dlsym(lib, sym);
	if (!addr) {
		char *err_str = NULL;
		dlerror();

		if (!(addr = dlsym(lib, sym))) {
			err_str = (char *)dlerror();
		}

		if (err_str) {
			*err = strdup(err_str);
		}
	}
	return addr;
}

#endif

//int main(int argc, char *argv[])
//{
//	char *error;
//	char *mydll_name;
//	dso_lib_t mydll;
//	dso_func_t myfunc;
//
//	if (argv[1] == NULL)
//	{
//		mydll_name = "whatismyip.dll";
//	}
//	else
//	{
//		mydll_name = argv[1];
//	}
//
//	mydll = open_dll(mydll_name, 1, &error);
//
//
//	//sleep(1000);
//	///....
//	myfunc = func_sym_dll(mydll, "get_ip_from_url_in_file", &error); //("http://checkip.dyndns.com", "my_ip.log");
//
//	destroy_dll(&mydll);
//
//	return 0;
//}