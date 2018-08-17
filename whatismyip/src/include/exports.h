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
#ifndef __EXPORTS_H
#define __EXPORTS_H

#include <stdio.h>

#ifdef WIN32

#include <Windows.h>
#include <assert.h>
#include <stdint.h>
#include <iphlpapi.h>
#include <curl/curl.h>
#include <errno.h>

#if defined(WHATISMYIP_DECLARE_STATIC)
#define WHATISMYIP_DECLARE(type)			type __stdcall
#define WHATISMYIP_DECLARE_NONSTD(type)	type __cdecl
#define WHATISMYIP_DECLARE_DATA
#elif defined(WHATISMYIP_EXPORTS)
#define WHATISMYIP_DECLARE(type)			__declspec(dllexport) type __stdcall
#define WHATISMYIP_DECLARE_NONSTD(type)	__declspec(dllexport) type __cdecl
#define WHATISMYIP_DECLARE_DATA			__declspec(dllexport)
#else
#define WHATISMYIP_DECLARE(type)			__declspec(dllimport) type __stdcall
#define WHATISMYIP_DECLARE_NONSTD(type)	__declspec(dllimport) type __cdecl
#define WHATISMYIP_DECLARE_DATA			__declspec(dllimport)
#endif


#else

#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>

#define O_BINARY 0
#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(WHATISMYIP_API_VISIBILITY)
#define WHATISMYIP_DECLARE(type)		__attribute__((visibility("default"))) type
#define WHATISMYIP_DECLARE_NONSTD(type)	__attribute__((visibility("default"))) type
#define WHATISMYIP_DECLARE_DATA		__attribute__((visibility("default")))
#define WHATISMYIP_MOD_DECLARE(type)	__attribute__((visibility("default"))) type
#define WHATISMYIP_MOD_DECLARE_NONSTD(type)	__attribute__((visibility("default"))) type
#define WHATISMYIP_MOD_DECLARE_DATA		__attribute__((visibility("default")))
#define WHATISMYIP_DECLARE_CLASS		__attribute__((visibility("default")))
#else
#define WHATISMYIP_DECLARE(type)		type
#define WHATISMYIP_DECLARE_NONSTD(type)	type
#define WHATISMYIP_DECLARE_DATA
#define WHATISMYIP_MOD_DECLARE(type)	type
#define WHATISMYIP_MOD_DECLARE_NONSTD(type)	type
#define WHATISMYIP_MOD_DECLARE_DATA
#define WHATISMYIP_DECLARE_CLASS
#endif
#endif

struct ftp_file
{
	const char *filename;
	FILE *stream;
};

struct locale_struct
{
	char* locale;
};

typedef struct ftp_file *ftp_file_t;
typedef struct locale_struct locale_struct;

typedef struct WHATISMYIP_ARGS {

	char url[4096];
	char output_file[MAX_PATH];
	char upload_file[MAX_PATH];
	char get_file[MAX_PATH];
	char ftp_uri[4096];
	char ftp_file[MAX_PATH];
	char dropbox_token[BUFSIZ];
	char dropbox_down_filename[MAX_PATH];
	char dropbox_up_filename[MAX_PATH];
	BOOL dropbox_up_mstsc;
	FILE *pwd_dictionary;
	const WCHAR friendly_nic_name[128];
	const WCHAR passwords[16][128];  //can support up to 16 passwords
} WHATISMYIP_ARGS;

#define SERVICE_NAME "whatismyip"
#define SAFE_URL "http://checkip.dyndns.com"

#ifdef __cplusplus
extern "C" {
#endif

	WHATISMYIP_DECLARE(void) formatArgsAndSaveOnReg(int argc, char *argv[], WHATISMYIP_ARGS *out);

	WHATISMYIP_DECLARE(void) readArgsFromReg(WHATISMYIP_ARGS *out);

	WHATISMYIP_DECLARE(void) InitLog(const char* str);

	WHATISMYIP_DECLARE(int) WriteToLog(char* str, ...);

	WHATISMYIP_DECLARE(void) wifi_try_connect(const WCHAR *wifiName, const WCHAR passwords[][128]);
	/*
	* Set the locale to be used, so that characters retrieved are in the desired locale compatibility
	*/
	WHATISMYIP_DECLARE(void) set_language(const char* nationality);

	/*
	* Extract the first regex <pattern> match to <result> from a secured memory buffer (written with  easy_curl_memwrite)
	*/
	WHATISMYIP_DECLARE(int) easy_extract_regex_from_sll(struct curl_slist *head, const char *pattern, char result[]);

	/*
	* Gets page data from address from the specified <url> website echoing it back
	* This method is equivalent to get_data_from_url. It is a simple parser of a webpage url based on a regex.
	*/
	WHATISMYIP_DECLARE(CURLcode) easy_get_data(const char *url, const char * pattern, char res[]);

	/*
	* Gets your public ip address  in a file with name <filename> from the specified <url> website echoing it back
	*/
	WHATISMYIP_DECLARE(CURLcode) easy_get_ip(const char* url, const char* filename);

	/*
	* Upload the file on an ftp server defined by the url_no_file uri.
	* Args:
	* @url_no_file: the uri of the ftp server, written as ftp://user:password@ftp.server.com/
	* @file: the path to the file to upload
	*/
	WHATISMYIP_DECLARE(CURLcode) ftp_upload(char *url, const char *filepath, char *newname);

	/*
	* Download the file from a ftp server
	* Args:
	* @full_url: the full uri of the ftp server, written as ftp://user:password@ftp.server.com/thefile
	*/
	WHATISMYIP_DECLARE(CURLcode) ftp_get(const char *url, const char *file);

	WHATISMYIP_DECLARE(CURLcode) dropbox_upload(const char *token, const char *name, BYTE *data, size_t datalen);

	WHATISMYIP_DECLARE(CURLcode) dropbox_upload_mstsc(const char *token, char *url);

	WHATISMYIP_DECLARE(CURLcode) dropbox_download(const char *token, const char *filename);

	WHATISMYIP_DECLARE(CURLcode) dropbox_remove(const char *token, const char *name);

	WHATISMYIP_DECLARE(CURLcode) box_get_authorize();

	WHATISMYIP_DECLARE(CURLcode) box_get_tokens();

	WHATISMYIP_DECLARE(CURLcode) box_upload(const char *token, const char *name, char *data, size_t datalen);

	WHATISMYIP_DECLARE(CURLcode) box_upload_mstsc(const char *token, char *url);

	WHATISMYIP_DECLARE(CURLcode) box_download(const char *token, const char *filename);

	WHATISMYIP_DECLARE(CURLcode) onedrive_authorize();

	WHATISMYIP_DECLARE(CURLcode) onedrive_upload(const char *token, const char *name, char *data, size_t datalen);

	WHATISMYIP_DECLARE(CURLcode) onedrive_upload_mstsc(const char *token, char *url);

	WHATISMYIP_DECLARE(CURLcode) onedrive_download(const char *token, const char *filename);

#ifdef __cplusplus
}
#endif

#endif