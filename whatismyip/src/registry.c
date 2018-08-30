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

#define REG_PATH TEXT("Software\\Microsoft\\")##SERVICE_NAME
#define REG_WPATH TEXT(L"Software\\Microsoft\\")##SERVICE_NAME

void writeToReg(LPCTSTR value, ULONG type, LPCTSTR data) {

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

void writeToRegW(LPCWSTR value, ULONG type, LPCWSTR data) {

	HKEY hKey;

	LONG openRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, REG_WPATH, 0, KEY_ALL_ACCESS, &hKey);
	if (openRes != ERROR_SUCCESS) {
		openRes = RegCreateKeyExW(HKEY_LOCAL_MACHINE, REG_WPATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
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

void readFromReg(LPCTSTR value, ULONG *type, BYTE data[BUFSIZ]) {

	HKEY key;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PATH, 0, KEY_ALL_ACCESS, &key) != ERROR_SUCCESS)
	{
		WriteToLog("Unable to open registry key.\n");
		return;
	}

	DWORD bufferSize = BUFSIZ;
	RegQueryValueEx(key, value, NULL, type, data, &bufferSize);
}

void readFromRegW(LPCWSTR value, ULONG *type, BYTE data[BUFSIZ]) {

	HKEY key;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, REG_WPATH, 0, KEY_ALL_ACCESS, &key) != ERROR_SUCCESS)
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
			writeToReg("-d", REG_SZ, out->dropbox_token);
			i++;
			continue;
		}
		if (strcmp("-dd", argv[i]) == 0) {
			snprintf(out->dropbox_down_filename, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg("-dd", REG_SZ, out->dropbox_down_filename);
			i++;
			continue;
		}
		if (strcmp("-du", argv[i]) == 0) {
			snprintf(out->dropbox_up_filename, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg("-du", REG_SZ, out->dropbox_up_filename);
			i++;
			continue;
		}
		if (strcmp("-dm", argv[i]) == 0) {
			out->dropbox_up_mstsc = TRUE;
			writeToReg("-dm", REG_BINARY, &out->dropbox_up_mstsc);
			continue;
		}

		if (strcmp("-u", argv[i]) == 0) {
			snprintf(out->upload_file, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg("-u", REG_SZ, out->upload_file);
			i++;
			continue;
		}
		if (strcmp("-g", argv[i]) == 0) {
			snprintf(out->get_file, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg("-g", REG_SZ, out->get_file);
			i++;
			continue;
		}
		if (strcmp("-o", argv[i]) == 0) {
			snprintf(out->output_file, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg("-o", REG_SZ, out->output_file);
			i++;
			continue;
		}
		if (strcmp("-r", argv[i]) == 0) {
			snprintf(out->url, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg("-r", REG_SZ, out->url);
			i++;
			continue;
		}
		if (strcmp("-f", argv[i]) == 0) {
			snprintf(out->ftp_uri, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg("-f", REG_SZ, out->ftp_uri);
			i++;
			continue;
		}
		if (strcmp("--passwords", argv[i]) == 0) {
			//todo
			int Size = lstrlenA(argv[i + 1]);
			MultiByteToWideChar(CP_ACP, 0, argv[i + 1], Size, out->passwords, Size);
			writeToRegW(L"--passwords", REG_MULTI_SZ, out->passwords);
			i++;
			continue;
		}

		if (strcmp("--friendlyNIC", argv[i]) == 0) {
			int siz = lstrlenA(argv[i + 1]);
			MultiByteToWideChar(CP_ACP, 0, argv[i + 1], siz, out->friendly_nic_name, siz);
			writeToRegW(L"--friendlyNIC", REG_SZ, out->friendly_nic_name);
			i++;
			continue;
		}
		return help();
	}
}

WHATISMYIP_DECLARE(void) readArgsFromReg(WHATISMYIP_ARGS *out) {

	ULONG type = REG_NONE;
	readFromReg("-d", &type, out->dropbox_token);
	readFromReg("-dd", &type, out->dropbox_down_filename);
	readFromReg("-du", &type, out->dropbox_up_filename);
	readFromReg("-dm", &type, &out->dropbox_up_mstsc);

	readFromReg("-u", &type, out->upload_file);
	readFromReg("-g", &type, out->get_file);
	readFromReg("-o", &type, out->output_file);

	readFromReg("-r", &type, out->url);
	readFromReg("-f", &type, out->ftp_uri);

	WCHAR buf[4192];
	readFromRegW(L"--passwords", &type, buf);
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
	readFromRegW(L"--friendlyNIC", &type, out->friendly_nic_name);
}

