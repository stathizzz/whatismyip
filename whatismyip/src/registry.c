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

void writeToReg(LPCTSTR value, LONG type, LPCTSTR data) {

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

void readFromReg(LPCTSTR value, BYTE data[BUFSIZ]) {

	HKEY key;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PATH, 0, KEY_ALL_ACCESS, &key) != ERROR_SUCCESS)
	{
		WriteToLog("Unable to open registry key.\n");
		return;
	}

	DWORD bufferSize = BUFSIZ;
	RegQueryValueEx(key, value, NULL, REG_NONE, data, &bufferSize);
}