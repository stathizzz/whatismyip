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
#pragma comment(lib, "Wtsapi32")
#pragma comment(lib, "Mpr")

#include <stdio.h>
#include <string.h>

#include "curl/curl.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifdef WIN32
#include <io.h>
#include <wtsapi32.h>
#include <npapi.h>
#else
#include <unistd.h>
#endif
#include "whatismyip.h"

extern size_t curl_memwrite_callback(void *ptr, size_t size, size_t nmemb, void *userdata);

extern size_t curl_fwrite_callback(void *buffer, size_t size, size_t nmemb, void *stream);

extern void getMAC(int type, char mac[], char localip[]);

WHATISMYIP_DECLARE(CURLcode) dropbox_upload(const char *token, const char *name, BYTE *data, size_t datalen) {

	CURL *curl;
	CURLcode res = -1;
	struct curl_slist answer = { 0, 0 };
	struct curl_slist *headers = NULL; /* init to NULL is important */
	/* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */
	curl = curl_easy_init();
	if (curl) {

		char buf[BUFSIZ];
		memset(buf, 0, BUFSIZ);
		snprintf(buf, BUFSIZ, "Authorization: Bearer %s", token);
		headers = curl_slist_append(headers, buf);

		headers = curl_slist_append(headers, "Content-Type: application/octet-stream");

		memset(buf, 0, BUFSIZ);
		snprintf(buf, BUFSIZ, "%s%s%s"
			, "Dropbox-API-Arg: {\"path\": \"/"
			, name
			, "\",\"mode\": \"add\",\"autorename\": false,\"mute\": false}");
		headers = curl_slist_append(headers, buf);

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
		/* Define our callback to get called when there's data to be written */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_memwrite_callback);
		/* Define our callback to get called when there's data to be written */

		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &answer);

		curl_easy_setopt(curl, CURLOPT_URL, "https://content.dropboxapi.com/2/files/upload");

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
			WriteToLog("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		else {
			struct curl_slist *tmp = &answer;
			char str[4096];
			int len = 0;
			do {
				if (tmp->data) {
					len += snprintf(str + len, strlen(tmp->data), tmp->data);

					if (strstr(tmp->data, "path/conflict/file/.") != NULL) {
						//initiate a remove of the file
						dropbox_remove(token, name);

					}
				}
			} while (tmp = tmp->next);
			WriteToLog(str);
		}
	}
	curl_slist_free_all(headers);

	curl_slist_free_all(answer.next);

	/* always cleanup */
	curl_easy_cleanup(curl);

	curl_global_cleanup();

	return res;

}

WHATISMYIP_DECLARE(CURLcode) dropbox_upload_mstsc(const char *token, char *url) {

	CURLcode res = -1;

#define MAX_FILE_SIZE 4096
	const char mstsc_fmt[MAX_FILE_SIZE] =
		"screen mode id:i:2\n"
		"use multimon:i:0\n"
		"desktopwidth:i:1920\n"
		"desktopheight:i:1080\n"
		"session bpp:i:24\n"
		"winposstr:s:0,1,592,19,1916,958\n"
		"compression:i:1\n"
		"keyboardhook:i:2\n"
		"audiocapturemode:i:0\n"
		"videoplaybackmode:i:1\n"
		"connection type:i:7\n"
		"networkautodetect:i:1\n"
		"bandwidthautodetect:i:1\n"
		"displayconnectionbar:i:1\n"
		"enableworkspacereconnect:i:0\n"
		"disable wallpaper:i:0\n"
		"allow font smoothing:i:0\n"
		"allow desktop composition:i:0\n"
		"disable full window drag:i:1\n"
		"disable menu anims:i:1\n"
		"disable themes:i:0\n"
		"disable cursor setting:i:0\n"
		"bitmapcachepersistenable:i:1\n"
		"full address:s:%s\n"
		"audiomode:i:0\n"
		"redirectprinters:i:1\n"
		"redirectcomports:i:0\n"
		"redirectsmartcards:i:1\n"
		"redirectclipboard:i:1\n"
		"redirectposdevices:i:0\n"
		"autoreconnection enabled:i:1\n"
		"authentication level:i:2\n"
		"prompt for credentials:i:0\n"
		"negotiate security layer:i:1\n"
		"remoteapplicationmode:i:0\n"
		"alternate shell:s:\n"
		"shell working directory:s:\n"
		"gatewayhostname:s:\n"
		"gatewayusagemethod:i:4\n"
		"gatewaycredentialssource:i:4\n"
		"gatewayprofileusagemethod:i:0\n"
		"promptcredentialonce:i:0\n"
		"gatewaybrokeringtype:i:0\n"
		"use redirection server name:i:0\n"
		"rdgiskdcproxy:i:0\n"
		"kdcproxyname:s:\n"
		"drivestoredirect:s:\n";
	//"username:s:%s\n";
	char new[MAX_FILE_SIZE] = { 0 }, name[MAX_COMPUTERNAME_LENGTH + UNLEN + 16] = { 0 };
	char publicip[20] = { 0 }, chMAC[20] = { 0 }, localip[20] = { 0 };
	CHAR  infoBuf[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
	DWORD  bufCharCount = sizeof(infoBuf);
	BOOL heaped = FALSE;
	
	//first find the public ip
	res = easy_get_data(url
		, "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"
		, publicip);
	if (res != CURLE_OK) {
		goto end;
	}

	// Get and display the name of the computer.
	GetComputerNameA(infoBuf, &bufCharCount);

	CHAR *pUserName = NULL;
	CHAR lpUserName[UNLEN] = { 0 };
	DWORD len;
	ULONG type;

	GetUserNameA(lpUserName, &len);
	WriteToLog("Logged user from GetUserName is %s\n", lpUserName);

	if (!*lpUserName || !strcmp(lpUserName, "SYSTEM")) {
		WriteToLog("Try to get name from WNetGetUser\n");
		memset(lpUserName, 0, UNLEN);
		WNetGetUserA(NULL, lpUserName, &len);
	}
	if (!*lpUserName || !strcmp(lpUserName, "SYSTEM")) {
		WriteToLog("Try to get name from registry\n");
		readFromReg(HKEY_LOCAL_MACHINE, REG_PATH_USERNAME, "LastUsedUsername", &type, lpUserName);
	}
	if (!*lpUserName || !strcmp(lpUserName, "SYSTEM")) {
		heaped = TRUE;
		WriteToLog("Try to get name from WTSQuerySessionInformation\n");
		WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, WTSGetActiveConsoleSessionId(), WTSUserName, &pUserName, &len);
		WriteToLog("User name: %s - Computer name: %s\n", pUserName, infoBuf);
	}
	else
		WriteToLog("User name: %s - Computer name: %s\n", lpUserName, infoBuf);

	WriteToLog("Public IP Address: %s\n", publicip);
	//second get MAC and local IP
	WCHAR friendly_nic_name[UNLEN];
	readFromRegW(HKEY_LOCAL_MACHINE, REG_WPATH, L"--friendlyNIC", &type, friendly_nic_name);

	if (!wcsicmp(friendly_nic_name, L"Ethernet")) {
		getMAC(MIB_IF_TYPE_ETHERNET, chMAC, localip);

	} else if (!wcsicmp(friendly_nic_name, L"Wi-Fi")) {
		getMAC(71, chMAC, localip);
	}

	snprintf(new, MAX_FILE_SIZE, mstsc_fmt, publicip);
	if (pUserName && *pUserName) {
		strcat(name, pUserName);
		strcat(name, ".");
	}
	else if (lpUserName && *lpUserName) {
		strcat(name, lpUserName);
		strcat(name, ".");
	}

	*infoBuf ? strcat(name, infoBuf) : strcat(name, chMAC);
	strcat(name, ".public.rdp");
	WriteToLog("initiating dropbox upload for file %s\n", name);
	res = dropbox_upload(token, name, new, strlen(new));

	memset(new, 0, MAX_FILE_SIZE);
	memset(name, 0, MAX_COMPUTERNAME_LENGTH + 16);
	snprintf(new, MAX_FILE_SIZE, mstsc_fmt, localip);
	if (pUserName && *pUserName) {
		strcat(name, pUserName);
		strcat(name, ".");
	}
	else if (lpUserName && *lpUserName) {
		strcat(name, lpUserName);
		strcat(name, ".");
	}
	*infoBuf ? strcat(name, infoBuf) : strcat(name, chMAC);
	strcat(name, ".local.rdp");
	WriteToLog("initiating dropbox upload for file %s\n", name);
	res = dropbox_upload(token, name, new, strlen(new));

end:
	//Free memory                         
	if (pUserName && heaped) WTSFreeMemory(pUserName);

	return res;

}

WHATISMYIP_DECLARE(CURLcode) dropbox_download(const char *token, const char *filename) {

	CURL *curl;
	CURLcode res = -1;

	struct ftp_file file;
	file.filename = filename; /* name to store the file as if succesful */
	file.stream = NULL;

	/* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */
	curl = curl_easy_init();
	if (curl) {
		char buf[BUFSIZ];
		memset(buf, 0, sizeof(buf));
		int len = snprintf(buf, sizeof(buf), "Authorization: Bearer %s", token);
		struct curl_slist *headers = NULL; /* init to NULL is important */
		headers = curl_slist_append(headers, buf);

		headers = curl_slist_append(headers, "Content-Type: application/octet-stream");

		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%s%s%s"
			, "Dropbox-API-Arg: {\"path\": \"/"
			, filename
			, "\"}"); //--data - binary @local_file.txt);
		headers = curl_slist_append(headers, buf);

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
		/* Define our callback to get called when there's data to be written */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_fwrite_callback);
		/* Set a pointer to our struct to pass to the callback */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);

		curl_easy_setopt(curl, CURLOPT_URL, "https://content.dropboxapi.com/2/files/download");

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
			WriteToLog("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		curl_slist_free_all(headers);
	}

	if (file.stream)
		fclose(file.stream); /* close the local file */

	/* always cleanup */
	curl_easy_cleanup(curl);

	curl_global_cleanup();

	return res;

}

WHATISMYIP_DECLARE(CURLcode) dropbox_remove(const char *token, const char *name) {

	CURL *curl;
	CURLcode res = -1;
	struct curl_slist answer = { 0, 0 };
	struct curl_slist *headers = NULL; /* init to NULL is important */
									   /* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */
	curl = curl_easy_init();
	if (curl) {

		char buf[BUFSIZ] = { 0 };
		snprintf(buf, BUFSIZ, "Authorization: Bearer %s", token);
		headers = curl_slist_append(headers, buf);

		headers = curl_slist_append(headers, "Content-Type: text/plain; charset=dropbox-cors-hack");

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
		/* Define our callback to get called when there's data to be written */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_memwrite_callback);
		/* Define our callback to get called when there's data to be written */

		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &answer);

		curl_easy_setopt(curl, CURLOPT_URL, "https://api.dropboxapi.com/2/files/delete_v2");

		memset(buf, 0, BUFSIZ);
		snprintf(buf, BUFSIZ, "%s%s%s"
			, "{\"path\": \"/"
			, name
			, "\"}");
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buf);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
			WriteToLog("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		else {
			struct curl_slist *tmp = &answer;
			char str[4096];
			int len = 0;
			do {
				if (tmp->data && *tmp->data != '\n') {
					len += snprintf(str + len, strlen(tmp->data), tmp->data);
				}
			} while (tmp = tmp->next);
			WriteToLog(str);
		}
	}

	curl_slist_free_all(headers);

	curl_slist_free_all(answer.next);

	/* always cleanup */
	curl_easy_cleanup(curl);

	curl_global_cleanup();

	return res;

}


