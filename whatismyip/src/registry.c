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
#include "whatismyip.h"

void writeToReg(HKEY context, LPCTSTR path, LPCTSTR value, ULONG type, LPCTSTR data) {

	HKEY hKey;

	LONG openRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PATH, 0, KEY_ALL_ACCESS, &hKey);
	if (openRes != ERROR_SUCCESS) {
		openRes = RegCreateKeyEx(HKEY_LOCAL_MACHINE, REG_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
		if (openRes != ERROR_SUCCESS) {
			WriteToLog("Error opening a key. Do this from an elevated account.\n");
			return;
		}
	}

	size_t len = type == REG_SZ ? strlen(data) + 1 : sizeof(data);
	LONG setRes = RegSetValueEx(hKey, value, 0, type, (LPBYTE)data, len);
	if (setRes != ERROR_SUCCESS) {
		WriteToLog("Error writing to Registry key %s.\n", value);
	}

	RegCloseKey(hKey);
}

void writeToRegW(HKEY context, LPCWSTR path, LPCWSTR value, ULONG type, LPCWSTR data) {

	HKEY hKey;

	LONG openRes = RegOpenKeyExW(context, path, 0, KEY_ALL_ACCESS, &hKey);
	if (openRes != ERROR_SUCCESS) {
		openRes = RegCreateKeyExW(context, path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
		if (openRes != ERROR_SUCCESS) {
			WriteToLog("Error opening a key. Do this from an elevated account.\n");
			return;
		}
	}

	size_t len = type == REG_SZ ? wcslen(data) + 1 : sizeof(data);
	LONG setRes = RegSetValueExW(hKey, value, 0, type, (LPBYTE)data, len*sizeof(WCHAR));
	if (setRes != ERROR_SUCCESS) {
		WriteToLog("Error writing to Registry key %s.\n", value);
	}

	RegCloseKey(hKey);
}

void readFromReg(HKEY context, LPCTSTR path, LPCTSTR value, ULONG *type, BYTE data[BUFSIZ]) {

	HKEY key;
	if (RegOpenKeyEx(context, path, 0, KEY_ALL_ACCESS, &key) != ERROR_SUCCESS)
	{
		WriteToLog("Unable to open registry key.\n");
		return;
	}

	DWORD bufferSize = BUFSIZ;
	RegQueryValueEx(key, value, NULL, type, data, &bufferSize);
}

void readFromRegW(HKEY context, LPCWSTR path, LPCWSTR value, ULONG *type, BYTE data[BUFSIZ]) {

	HKEY key;
	if (RegOpenKeyExW(context, path, 0, KEY_ALL_ACCESS, &key) != ERROR_SUCCESS)
	{
		WriteToLog("Unable to open registry key.\n");
		return;
	}

	DWORD bufferSize = BUFSIZ;
	RegQueryValueExW(key, value, NULL, type, data, &bufferSize);
}

WHATISMYIP_DECLARE(void) formatArgsAndSaveOnReg(int argc, char *argv[], WHATISMYIP_ARGS *out) {

	int counter = 0;
	CURLcode status = -1;
	int i;

	/* iterate over all arguments */
	for (i = 1; i < argc; i++)
	{
		if (strcmp("-d", argv[i]) == 0) {
			snprintf(out->dropbox_token, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg(HKEY_LOCAL_MACHINE, REG_PATH, "-d", REG_SZ, out->dropbox_token);
			i++;
			continue;
		}
		if (strcmp("-dd", argv[i]) == 0) {
			snprintf(out->dropbox_down_filename, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg(HKEY_LOCAL_MACHINE, REG_PATH, "-dd", REG_SZ, out->dropbox_down_filename);
			i++;
			continue;
		}
		if (strcmp("-du", argv[i]) == 0) {
			snprintf(out->dropbox_up_filename, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg(HKEY_LOCAL_MACHINE, REG_PATH, "-du", REG_SZ, out->dropbox_up_filename);
			i++;
			continue;
		}
		if (strcmp("-dm", argv[i]) == 0) {
			out->dropbox_up_mstsc = TRUE;
			writeToReg(HKEY_LOCAL_MACHINE, REG_PATH, "-dm", REG_BINARY, &out->dropbox_up_mstsc);
			continue;
		}

		if (strcmp("-u", argv[i]) == 0) {
			snprintf(out->upload_file, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg(HKEY_LOCAL_MACHINE, REG_PATH, "-u", REG_SZ, out->upload_file);
			i++;
			continue;
		}
		if (strcmp("-g", argv[i]) == 0) {
			snprintf(out->get_file, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg(HKEY_LOCAL_MACHINE, REG_PATH, "-g", REG_SZ, out->get_file);
			i++;
			continue;
		}
		if (strcmp("-o", argv[i]) == 0) {
			snprintf(out->output_file, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg(HKEY_LOCAL_MACHINE, REG_PATH, "-o", REG_SZ, out->output_file);
			i++;
			continue;
		}
		if (strcmp("-r", argv[i]) == 0) {
			snprintf(out->url, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg(HKEY_LOCAL_MACHINE, REG_PATH, "-r", REG_SZ, out->url);
			i++;
			continue;
		}
		if (strcmp("-f", argv[i]) == 0) {
			snprintf(out->ftp_uri, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg(HKEY_LOCAL_MACHINE, REG_PATH, "-f", REG_SZ, out->ftp_uri);
			i++;
			continue;
		}
		if (strcmp("--passwords", argv[i]) == 0) {
			//todo
			int Size = lstrlenA(argv[i + 1]);
			MultiByteToWideChar(CP_ACP, 0, argv[i + 1], Size, out->passwords, Size);
			writeToRegW(HKEY_LOCAL_MACHINE, REG_WPATH, L"--passwords", REG_MULTI_SZ, out->passwords);
			i++;
			continue;
		}

		if (strcmp("--friendlyNIC", argv[i]) == 0) {
			int siz = lstrlenA(argv[i + 1]);
			MultiByteToWideChar(CP_ACP, 0, argv[i + 1], siz, out->friendly_nic_name, siz);
			writeToRegW(HKEY_LOCAL_MACHINE, REG_WPATH, L"--friendlyNIC", REG_SZ, out->friendly_nic_name);
			i++;
			continue;
		}
		return help();
	}
}

WHATISMYIP_DECLARE(void) readArgsFromReg(WHATISMYIP_ARGS *out) {

	ULONG type = REG_NONE;
	readFromReg(HKEY_LOCAL_MACHINE, REG_PATH, "-d", &type, out->dropbox_token);
	readFromReg(HKEY_LOCAL_MACHINE, REG_PATH, "-dd", &type, out->dropbox_down_filename);
	readFromReg(HKEY_LOCAL_MACHINE, REG_PATH, "-du", &type, out->dropbox_up_filename);
	readFromReg(HKEY_LOCAL_MACHINE, REG_PATH, "-dm", &type, &out->dropbox_up_mstsc);

	readFromReg(HKEY_LOCAL_MACHINE, REG_PATH, "-u", &type, out->upload_file);
	readFromReg(HKEY_LOCAL_MACHINE, REG_PATH, "-g", &type, out->get_file);
	readFromReg(HKEY_LOCAL_MACHINE, REG_PATH, "-o", &type, out->output_file);

	readFromReg(HKEY_LOCAL_MACHINE, REG_PATH, "-r", &type, out->url);
	readFromReg(HKEY_LOCAL_MACHINE, REG_PATH, "-f", &type, out->ftp_uri);

	WCHAR buf[4192];
	readFromRegW(HKEY_LOCAL_MACHINE, REG_WPATH, L"--passwords", &type, buf);
	if (type == REG_MULTI_SZ) {
		WCHAR *context = buf;
		for (int i = 0; i < sizeof(out->passwords) / sizeof(out->passwords[0]); ++i) {
			WCHAR *tok = wcstok(context, "/0", &context);
			if (tok) {
				memcpy(out->passwords[i], tok, sizeof(WCHAR)*wcslen(tok));
				if (!*context) context++;
			}
			else {
				break;
			}
		}

	}
	readFromRegW(HKEY_LOCAL_MACHINE, REG_WPATH, L"--friendlyNIC", &type, out->friendly_nic_name);
}

