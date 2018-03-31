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
#include "ftp.h"
/* NOTE: if you want this example to work on Windows with libcurl as a
   DLL, you MUST also provide a read callback with CURLOPT_READFUNCTION.
   Failing to do so will give you a crash since a DLL may not use the
   variable's memory when passed in to it from an app like this. */ 
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
  /* in real-world cases, this would probably get this data differently
     as this fread() stuff is exactly what the library already would do
     by default internally */ 
  size_t retcode = fread(ptr, size, nmemb, stream);
#ifdef _DEBUG 
  fprintf(stderr, "*** We read %d bytes from file\n", retcode);
#endif
  return retcode;
}
 
WHATISMYIP_DECLARE(CURLcode) ftp_upload(char *url_no_file, char* file)
{
	CURL *curl;
	CURLcode res;
	FILE *hd_src;
	struct stat file_info;
	curl_off_t fsize;
	struct curl_slist *headerlist=NULL;
	
	/* reformat the url + file path*/
	char *url = malloc(128);
	memset(url, 0, 128);
	memcpy(url, url_no_file, strlen(url_no_file));
	if ( strrchr(url, '/') != url + strlen(url) - 1 )
	{
		strncat(url, "/", 1);
	}
	strncat(url, file, strlen(file));
	
	/* get the file size of the local file */ 
	if(stat(file, &file_info))
	{
#ifdef _DEBUG
		printf("Couldnt open '%s': %s\n", file, strerror(errno));
#endif
		return (CURLcode)1;
	}
	fsize = (curl_off_t)file_info.st_size;
#ifdef _DEBUG
	printf("Local file size: %d"  " bytes.\n", fsize);
#endif
	/* get a FILE * of the same file */ 
	hd_src = fopen(file, "rb");
 
	/* In windows, this will init the winsock stuff */ 
	curl_global_init(CURL_GLOBAL_ALL);
 
	/* get a curl handle */ 
	curl = curl_easy_init();
	if(curl) 
	{
		/* we want to use our own read function */ 
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
 
		/* enable uploading */ 
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
 
		/* specify target */ 
		curl_easy_setopt(curl,CURLOPT_URL, url);
 
		/* pass in that last of FTP commands to run after the transfer */ 
		curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);
 
		/* now specify which file to upload */ 
		curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
 
		/* Set the size of the file to upload (optional).  If you give a *_LARGE
			option you MUST make sure that the type of the passed-in argument is a
			curl_off_t. If you use CURLOPT_INFILESIZE (without _LARGE) you must
			make sure that to pass in a type 'long' argument. */ 
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);
 
		/* Now run off and do what you've been told! */ 
		res = curl_easy_perform(curl);
 
		/* clean up the FTP commands list */ 
		curl_slist_free_all (headerlist);
 
		/* always cleanup */ 
		curl_easy_cleanup(curl);
    }
    fclose(hd_src); /* close the local file */ 
 
    curl_global_cleanup();
  
	return res;
}

/*
 * This is an example showing how to get a single file from an FTP server.
 * It delays the actual destination file creation until the first write
 * callback so that it won't create an empty file in case the remote file
 * doesn't exist or something else fails.
 */

struct FtpFile {
  const char *filename;
  FILE *stream;
};

static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
  struct FtpFile *out=(struct FtpFile *)stream;
  if(out && !out->stream) {
    /* open file for writing */
    out->stream=fopen(out->filename, "wb");
    if(!out->stream)
      return -1; /* failure, can't open file to write */
  }
  return fwrite(buffer, size, nmemb, out->stream);
}


WHATISMYIP_DECLARE(CURLcode) ftp_get(char *full_url)
{
	CURL *curl;
	CURLcode res;
	struct FtpFile ftpfile;//
	char *file;

	file = strrchr(full_url, '/');
	file++;

	ftpfile.filename = file;
	ftpfile.stream = NULL;
	
	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if(curl) 
	{
		/*
		* You better replace the URL with one that works!
		*/
		curl_easy_setopt(curl, CURLOPT_URL,	full_url); 
		/* Define our callback to get called when there's data to be written */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
		/* Set a pointer to our struct to pass to the callback */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);
#ifdef _DEBUG
		/* Switch on full protocol/debug output */
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
		res = curl_easy_perform(curl);

		/* always cleanup */
		curl_easy_cleanup(curl);

#ifdef _DEBUG
		if(CURLE_OK != res) 
		{
			/* we failed */
			fprintf(stderr, "curl told us %d\n", res);	
		}
#endif
    }
	if(ftpfile.stream)
		fclose(ftpfile.stream); /* close the local file */

	curl_global_cleanup();

	return res;
};
