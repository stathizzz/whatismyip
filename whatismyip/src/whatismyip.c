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
#include "exports.h"
#include "whatismyip.h"
#include "wifi.h"

#ifdef WIN32

#pragma comment(lib,"WS2_32")
#pragma comment(lib,"winmm")
#pragma comment(lib, "IPHLPAPI")
#pragma comment(lib, "Wldap32")
#pragma comment(lib, "Normaliz")

#endif

FILE _iob[3] = { NULL, NULL, NULL };
FILE * __cdecl __iob_func(void) { return _iob; }

extern void writeToReg(LPCTSTR value, LONG type, LPCTSTR data);
extern void readFromReg(LPCTSTR value, BYTE data[]);

/*
* Get the server response in a file or in memory .
* It delays the actual destination file creation until the first write
* callback so that it won't create an empty file in case the remote file
* doesn't exist or something else fails.
* Upload the file on a ftp server via ftp, or download a file from a ftp server.
*/
char *logfile_ = NULL;

void InitLog(const char *path) {

	logfile_ = path;
}

int WriteToLog(char* format, ...)
{
	FILE* log;
	log = fopen(logfile_, "a+");
	if (log == NULL) {
		printf("Error %s \n", strerror(errno));
		return -1;
	}
	va_list argptr;
	va_start(argptr, format);
	vfprintf(log, format, argptr);
	va_end(argptr);
	//fprintf(log, "%s\n", format);
	fclose(log);
	return 0;
}

#define NUMBER_OF_OFFSETS 4
#define MIN_REQUIRED_IP_LENGTH 16
#define MIN_REQUIRED_REGEX_RESULT_LENGTH 100
/* minimum required number of parameters */
#define MIN_REQUIRED 2

static locale_struct lang_globals;

size_t curl_fwrite_callback(void *buffer, size_t size, size_t nmemb, void *stream)
{
	ftp_file_t out = stream;
	size_t numbytes;

	if (out && !out->stream)
	{
		/* open file for writing */
		out->stream = fopen(out->filename, "wb");
		if (!out->stream)
		{
#ifdef _DEBUG
			WriteToLog("failure, can't open file to write: %d\n", GetLastError());
#endif
			return -1;
		}
	}
	numbytes = fwrite(buffer, size, nmemb, out->stream);

	return numbytes;
};

size_t curl_memwrite_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t realsize = size * nmemb;

	/* in our case we pass only char arrays, so there is no need to pass the size value */
	if (userdata) {
		userdata = curl_slist_append(userdata, (char *)ptr);
	}

	return realsize;
};

WHATISMYIP_DECLARE(void) set_language(const char *nationality)
{

#ifdef WIN32
#pragma setlocale(nationality)	
#else
	setlocale((LC_CTYPE, nationality);
#endif
	lang_globals.locale = (char *)nationality;
};

WHATISMYIP_DECLARE(CURLcode) easy_get_ip(const char* url, const char* filename)
{
	CURL *curl = NULL;
	CURLcode res = -1;
	char out[BUFSIZ];
	FILE *f = NULL;
	struct ftp_file file;
	struct curl_slist head = { 0, 0 };

	file.filename = filename; /* name to store the file as if succesful */
	file.stream = NULL;

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if (curl) {
		/*
		* You better replace the URL with one that works!
		*/
		curl_easy_setopt(curl, CURLOPT_URL, url);

		/* Define our callback to get called when there's data to be written */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_memwrite_callback);
		/* Set a pointer to our struct to pass to the callback */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &head);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);

		/* some servers don't like requests that are made without a user-agent
		field, so we provide one */
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

#ifdef _DEBUG
		/* Switch on full protocol/debug output */
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
		res = curl_easy_perform(curl);
		if (CURLE_OK != res)
		{
			/* we failed */
			WriteToLog("error output #: %d\n", GetLastError());
			goto end;
		}
		memset(out, 0, sizeof(out));
		if (FALSE == easy_extract_regex_from_sll(&head, "(([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])\\.){3}([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])", out))
		{
			WriteToLog("failure, can't find regex ip: %d\n", GetLastError());
			res = CURLE_OBSOLETE40;
			goto end;
		}
		WriteToLog("Public IP Address: %s\n", out);

		f = fopen(filename, "wb+");
		if (!f)
		{
			WriteToLog("failure, can't open file to write: %d\n", GetLastError());
			res = CURLE_OBSOLETE44;
			goto end;
		}
		fflush(f);

		fwrite(out, sizeof(char), strlen(out), f);

		fclose(f);
	}
	else {
		WriteToLog("Curl could not be initialized\n");
	}
end:
	if (file.stream)
		fclose(file.stream); /* close the local file */

	curl_slist_free_all(head.next);

	/* always cleanup */
	curl_easy_cleanup(curl);

	curl_global_cleanup();

	return res;

}

WHATISMYIP_DECLARE(BOOL) easy_extract_regex_from_sll(struct curl_slist *head, const char *pattern, char result[])
{
	int restable[NUMBER_OF_OFFSETS] = { 0 };
	int res, res2, status = TRUE;
	const char *regex;
	const unsigned char* tables = NULL;
	const char** errorptr;
	int* erroroffset;
	pcre* compiled;

	if (lang_globals.locale != NULL && lang_globals.locale != "")
	{
		tables = pcre_maketables();
	}

	compiled = pcre_compile(pattern, 0, &errorptr, &erroroffset, tables);

	if (errorptr && erroroffset)
	{
#ifdef _DEBUG
		WriteToLog("error compiling regular expression: %d\n", GetLastError());
#endif
		return FALSE;
	}

	struct curl_slist *answer = head;
	while (answer) {

		if (answer->data) {
			/* execute the regular expression against our string - no flags */
			res = pcre_exec(compiled, NULL, answer->data, strlen(answer->data), 0, 0, restable, sizeof(restable) / sizeof(restable[0]));
			if (res < 0)
			{
				/* try get our string against the regular expression - no flags - multi line files*/
				res = pcre_exec(compiled, NULL, answer->data, strlen(answer->data), 0, PCRE_MULTILINE, restable, sizeof(restable) / sizeof(restable[0]));
			}
			if (res < 0)
			{
				/* try get our string against the regular expression - no flags - UTF8 files*/
				res = pcre_exec(compiled, NULL, answer->data, strlen(answer->data), 0, PCRE_UTF8, restable, sizeof(restable) / sizeof(restable[0]));
			}
			if (res < 0)
			{
				/* try get our string against the regular expression - no flags - any newline character files*/
				res = pcre_exec(compiled, NULL, answer->data, strlen(answer->data), 0, PCRE_NEWLINE_ANY, restable, sizeof(restable) / sizeof(restable[0]));
			}
			if (res < 0)
			{
				/* try get our string against the regular expression - no flags - javascript files*/
				res = pcre_exec(compiled, NULL, answer->data, strlen(answer->data), 0, PCRE_JAVASCRIPT_COMPAT, restable, sizeof(restable) / sizeof(restable[0]));
			}
			if (res < 0)
			{
				/* try get our string against the regular expression - no flags - dotall files*/
				res = pcre_exec(compiled, NULL, answer->data, strlen(answer->data), 0, PCRE_DOTALL, restable, sizeof(restable) / sizeof(restable[0]));
			}

			/* escape on first match */
			if (res >= 0)
				break;
		}

		answer = answer->next;
	}

	if (res == -1)
	{
		WriteToLog("failed to find a match!\n");
		status = FALSE;
		snprintf(result, strlen("Pattern not found") + 1, "Pattern not found");
		goto end;
	}
	else if (res < 0)
	{
		WriteToLog("error occured on running: %d\n!", GetLastError());
		status = FALSE;
		snprintf(result, strlen("Pattern not found") + 1, "Pattern not found");
		goto end;
	}

	/* Get the first (1) string found on first match (0)*/
	res2 = pcre_copy_substring(answer->data, restable, 2, 0, result, BUFSIZ);
	if (res2 == (int)PCRE_ERROR_NOMEMORY || res2 == (int)PCRE_ERROR_NOSUBSTRING)
	{
		WriteToLog("error on memory: %d\n!", GetLastError());
		snprintf(result, strlen("Error on memory") + 1, "Error on memory");
	}

end:

	free(compiled);
	return status;
}

WHATISMYIP_DECLARE(CURLcode) easy_get_data(const char *url, const char * pattern, char out[])
{
	CURL *curl = NULL;
	CURLcode res = -1;
	struct curl_slist userdata = { 0, 0 };

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if (curl) {
		/*
		* You better replace the URL with one that works!
		*/
		curl_easy_setopt(curl, CURLOPT_URL, url);

		/* Define our callback to get called when there's data to be written */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_memwrite_callback);
		/* Set a pointer to our struct to pass to the callback */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &userdata);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);

		/* some servers don't like requests that are made without a user-agent
		field, so we provide one */
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

#ifdef _DEBUG
		/* Switch on full protocol/debug output */
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
		res = curl_easy_perform(curl);
		if (CURLE_OK != res)
		{
			/* we failed */
			WriteToLog("error output #: %d\n", GetLastError());
			goto end;
		}

		if (FALSE == easy_extract_regex_from_sll(&userdata, pattern, out))
		{
			WriteToLog("failure, can't find regex ip: %d\n", GetLastError());
			res = CURLE_OBSOLETE29;
			goto end;
		}

	}
	else {
		WriteToLog("Curl could not be initialized\n");
	}
end:
	curl_slist_free_all(userdata.next);
	/* always cleanup */
	curl_easy_cleanup(curl);

	curl_global_cleanup();

	return res;

}

/* display usage */
int help() {
	printf("Usage: whatismyip [-u <arg0>] [-g <arg1>] [-o <arg2>] [-u <arg3>] [-f <arg4>]\n");
	printf("\t-u: upload the file <arg0> to an ftp account given by the ftp uri <arg4> \n");
	printf("\t-g: get the file <arg1> via ftp from specified uri <arg4>\n");
	printf("\t-o: get your ip on local file <arg2>. If -r switch also specified, it tries to retrieve the url from the url <arg3> ip provider. If not, it connects to http://checkip.dyndns.com \n");
	printf("\t-r: the url to pass\n");
	printf("\t-f: the ftp uri to pass\n");
	return 1;
}

WHATISMYIP_DECLARE(void) formatArgsAndSaveOnReg(int argc, char *argv[], WHATISMYIP_ARGS *out) {

	int counter = 0;
	CURLcode status = -1;
	int i;

	if (argc < MIN_REQUIRED)
	{
		return help();
	}

	/* iterate over all arguments */
	for (i = 1; i < argc; i++)
	{
		if (strcmp("-d", argv[i]) == 0)
		{
			snprintf(out->dropbox_token, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg("-d", REG_SZ, out->dropbox_token);
			i++;
			continue;
		}
		if (strcmp("-dd", argv[i]) == 0)
		{
			snprintf(out->dropbox_down_filename, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg("-dd", REG_SZ, out->dropbox_down_filename);
			i++;
			continue;
		}
		if (strcmp("-du", argv[i]) == 0)
		{
			snprintf(out->dropbox_up_filename, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg("-du", REG_SZ, out->dropbox_up_filename);
			i++;
			continue;
		}
		if (strcmp("-dm", argv[i]) == 0)
		{
			out->dropbox_up_mstsc = TRUE;
			writeToReg("-dm", REG_BINARY, &out->dropbox_up_mstsc);
			continue;
		}

		if (strcmp("-u", argv[i]) == 0)
		{
			snprintf(out->upload_file, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg("-u", REG_SZ, out->upload_file);
			i++;
			continue;
		}
		if (strcmp("-g", argv[i]) == 0)
		{
			snprintf(out->get_file, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg("-g", REG_SZ, out->get_file);
			i++;
			continue;
		}
		if (strcmp("-o", argv[i]) == 0)
		{
			snprintf(out->output_file, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg("-o", REG_SZ, out->output_file);
			i++;
			continue;
		}
		if (strcmp("-r", argv[i]) == 0)
		{
			snprintf(out->url, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg("-r", REG_SZ, out->url);
			i++;
			continue;
		}
		if (strcmp("-f", argv[i]) == 0)
		{
			snprintf(out->ftp_uri, strlen(argv[i + 1]) + 1, argv[i + 1]);
			writeToReg("-f", REG_SZ, out->ftp_uri);
			i++;
			continue;
		}

		return help();
	}

	if (!out->url) {
		snprintf(out->url, strlen(SAFE_URL) + 1, SAFE_URL);
		writeToReg("-r", REG_SZ, out->url);
	}
}

WHATISMYIP_DECLARE(void) readArgsFromReg(WHATISMYIP_ARGS *out) {

	readFromReg("-d", out->dropbox_token);
	readFromReg("-dd", out->dropbox_down_filename);
	readFromReg("-du", out->dropbox_up_filename);
	readFromReg("-dm", &out->dropbox_up_mstsc);

	readFromReg("-u", out->upload_file);
	readFromReg("-g", out->get_file);
	readFromReg("-o", out->output_file);

	readFromReg("-r", out->url);
	readFromReg("-f", out->ftp_uri);

}

#ifndef WHATISMYIP_DECLARE_STATIC
/* main */
int main(int argc, char *argv[])
{
	if (argc < MIN_REQUIRED)
	{
		return help();
	}

	WHATISMYIP_ARGS formatted = { 0 };
	formatArgsAndSaveOnReg(argc, argv, &formatted);

	InitLog(SERVICE_NAME".log");

	wifi_try_connect();

	if (formatted.dropbox_token) {
		if (formatted.dropbox_up_mstsc)
			dropbox_upload_mstsc(formatted.dropbox_token, formatted.url);

		if (formatted.dropbox_down_filename)
			dropbox_download(formatted.dropbox_token, formatted.dropbox_down_filename);

		if (formatted.dropbox_up_filename)
			dropbox_upload(formatted.dropbox_token, "test.txt", "hehe", 4);
	}

	if (formatted.output_file) {
		easy_get_ip(formatted.url, formatted.output_file);
	}

	if (formatted.upload_file) {

		if (formatted.ftp_uri)
		{
			ftp_upload(formatted.ftp_uri, formatted.upload_file, "test");
		}
		else
		{
#ifdef DEBUG
			printf("please provide an ftp uri to upload the file!\n");
#endif
		}

	}

	if (formatted.get_file)
	{
		if (!formatted.ftp_uri)
		{
#ifdef DEBUG
			printf("please provide an ftp uri to upload the file!\n");
#endif
		}

		ftp_get(formatted.ftp_uri, formatted.get_file);

	}
	return 0;
}

#endif