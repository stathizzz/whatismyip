/***************************************************************************
*                                  _   _ ____  _
*  Project                     ___| | | |  _ \| |
*                             / __| | | | |_) | |
*                            | (__| |_| |  _ <| |___
*                             \___|\___/|_| \_\_____|
*
* Copyright (C) 1998 - 2017, Daniel Stenberg, <daniel@haxx.se>, et al.
*
* This software is licensed as described in the file COPYING, which
* you should have received as part of this distribution. The terms
* are also available at https://curl.haxx.se/docs/copyright.html.
*
* You may opt to use, copy, modify, merge, publish, distribute and/or sell
* copies of the Software, and permit persons to whom the Software is
* furnished to do so, under the terms of the COPYING file.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
***************************************************************************/
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
#include <stdio.h>
#include <string.h>

#include <curl/curl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "whatismyip.h"


/* This is an example showing how to get a single file from an FTP server.
* It delays the actual destination file creation until the first write
* callback so that it won't create an empty file in case the remote file
* doesn't exist or something else fails.
*/
static size_t ftp_download_callback(void *buffer, size_t size, size_t nmemb, void *stream)
{
	struct ftp_file *out = (struct ftp_file *)stream;
	if (out && !out->stream) {
		/* open file for writing */
		out->stream = fopen(out->filename, "wb");
		if (!out->stream)
			return -1; /* failure, can't open file to write */
	}
	return fwrite(buffer, size, nmemb, out->stream);
}

/* NOTE: if you want this example to work on Windows with libcurl as a
DLL, you MUST also provide a read callback with CURLOPT_READFUNCTION.
Failing to do so will give you a crash since a DLL may not use the
variable's memory when passed in to it from an app like this. */
static size_t ftp_upload_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	curl_off_t nread;
	/* in real-world cases, this would probably get this data differently
	as this fread() stuff is exactly what the library already would do
	by default internally */
	size_t retcode = fread(ptr, size, nmemb, stream);

	nread = (curl_off_t)retcode;

	fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T " bytes from file\n", nread);
	return retcode;
}

WHATISMYIP_DECLARE(int) ftp_upload(char *url, const char *filepath, char *newname)
{
	CURL *curl;
	CURLcode res;
	FILE *hd_src;
	struct stat file_info;
	curl_off_t fsize;
	char buf1[MAX_PATH];
	char buf2[MAX_PATH];
	struct curl_slist *headerlist = NULL;

#ifdef WIN32
	char *filename;
	(filename = strrchr(filepath, '/')) ? ++filename : (filename = filepath);
#else
	char *filename = basename(filepath);
#endif
	int len = snprintf(buf1, strlen("RNFR "), "RNFR ");
	snprintf(buf1 + len, MAX_PATH, filename);

	int len2 = snprintf(buf2, strlen("RNTO "), "RNTO ");
	snprintf(buf2 + len2, MAX_PATH, newname);

	/* get the file size of the local file */
	if (stat(filepath, &file_info)) {
		WriteToLog("Couldn't open '%s': %s\n", filepath, strerror(errno));
		return 1;
	}
	fsize = (curl_off_t)file_info.st_size;

	WriteToLog("Local file size: %" CURL_FORMAT_CURL_OFF_T " bytes.\n", fsize);

	/* get a FILE * of the same file */
	hd_src = fopen(filepath, "rb");

	/* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */
	curl = curl_easy_init();
	if (curl) {
		/* build a list of commands to pass to libcurl */
		headerlist = curl_slist_append(headerlist, buf1);
		headerlist = curl_slist_append(headerlist, buf2);

		/* we want to use our own read function */
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, ftp_upload_callback);

		/* enable uploading */
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

		/* specify target */
		curl_easy_setopt(curl, CURLOPT_URL, url);

		/* pass in that last of FTP commands to run after the transfer */
		curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);

		/* now specify which file to upload */
		curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);

		/* Set the size of the file to upload (optional).  If you give a *_LARGE
		option you MUST make sure that the type of the passed-in argument is a
		curl_off_t. If you use CURLOPT_INFILESIZE (without _LARGE) you must
		make sure that to pass in a type 'long' argument. */
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
			(curl_off_t)fsize);

		/* Now run off and do what you've been told! */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));

		/* clean up the FTP commands list */
		curl_slist_free_all(headerlist);

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	fclose(hd_src); /* close the local file */

	curl_global_cleanup();
	return 0;
}

WHATISMYIP_DECLARE(CURLcode) ftp_get(const char *url, const char *filename)
{
	CURL *curl;
	CURLcode res;
	struct ftp_file ftpfile;
	
	ftpfile.filename = filename;
	ftpfile.stream = NULL;
	
	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if(curl) 
	{
		/*
		* You better replace the URL with one that works!
		*/
		char fullurl[MAX_PATH];
		int len = snprintf(fullurl, MAX_PATH, url);
		fullurl[len] = '/';
		snprintf(fullurl + len + 1, MAX_PATH, filename);

		curl_easy_setopt(curl, CURLOPT_URL,	fullurl); 
		/* Define our callback to get called when there's data to be written */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ftp_download_callback);
		/* Set a pointer to our struct to pass to the callback */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);
#ifdef _DEBUG
		/* Switch on full protocol/debug output */
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
		res = curl_easy_perform(curl);

		/* always cleanup */
		curl_easy_cleanup(curl);

		if(CURLE_OK != res) 
		{
			/* we failed */
			fprintf(stderr, "curl told us %d\n", res);	
		}
    }
	if(ftpfile.stream)
		fclose(ftpfile.stream); /* close the local file */

	curl_global_cleanup();

	return res;
};
