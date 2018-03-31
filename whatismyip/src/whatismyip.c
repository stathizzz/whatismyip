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
#include "ftp.h"

/*
 * Get the server response in a file or in memory .
 * It delays the actual destination file creation until the first write
 * callback so that it won't create an empty file in case the remote file
 * doesn't exist or something else fails.
 * Upload the file on a ftp server via ftp, or download a file from a ftp server.
 */

#define NUMBER_OF_OFFSETS 4
#define MIN_REQUIRED_IP_LENGTH 16
#define MIN_REQUIRED_REGEX_RESULT_LENGTH 100

static Sll head;
static locale_struct lang_globals;
static size_t realsize;

static size_t curl_fwrite_callback(void *buffer, size_t size, size_t nmemb, void *stream)
{
  ftp_file_t out = stream;
  size_t numbytes;
  
  if(out && !out->stream) 
  {
    /* open file for writing */
	out->stream = fopen(out->filename, "wb");
    if(!out->stream)
	{
#if DEBUG
		fprintf(stdout, " failure, can't open file to write: %d\n", GetLastError());
#endif
		return -1; 
	}
  }
  numbytes = fwrite(buffer, size, nmemb, out->stream);

  return numbytes;
};

static size_t write_memory_callback(void *ptr, size_t size, size_t nmemb, void **data)
{
	char *characters;
	Sll *new;
	

	realsize = size * nmemb;

	characters = malloc(realsize);
	if (characters == NULL) 
	{
#if DEBUG
		/* out of memory! */
		fprintf(stdout, "not enough memory (malloc returned NULL): %d\n", GetLastError());
#endif
    	exit(EXIT_FAILURE);
	}
  
	memcpy(characters, ptr, realsize);
	new = allocateNode((void *) characters);
	appendNode(&head,&new);

	return realsize;
};

WHATISMYIP_DECLARE(int) get_httpdata_in_file(const char* url, const char* filename)
{
  CURL *curl;
  CURLcode res;
  curl_status_t status = CURL_STATUS_SUCCESS;

  ftp_file_t file = malloc(sizeof(struct ftp_file ));
  file->filename = filename; /* name to store the file as if succesful */
  file->stream = NULL;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();
  if(curl)
  {
    /*
     * You better replace the URL with one that works!
     */
    curl_easy_setopt(curl, CURLOPT_URL, url);
    /* Define our callback to get called when there's data to be written */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_fwrite_callback);
    /* Set a pointer to our struct to pass to the callback */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

#if DEBUG
    /* Switch on full protocol/debug output */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

    res = curl_easy_perform(curl);

    /* always cleanup */
    curl_easy_cleanup(curl);

    if(CURLE_OK != res) 
	{
#if DEBUG
      /* we failed */
		fprintf(stderr, "error output #: %d\n", GetLastError());
#endif
		status = CURL_STATUS_FAILURE;
    }
  }

  if(file->stream)
    fclose(file->stream); /* close the local file */

  curl_global_cleanup();

  return status;
}
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	int written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}
WHATISMYIP_DECLARE(int) get_ip_from_url_in_file(const char* url, const char* filename)
{
	CURL *curl;
	CURLcode res;
#ifdef WIN32
	WSADATA wsaData;
	int initwsa;

	if ((initwsa = WSAStartup(MAKEWORD(2, 0), &wsaData)) != 0) {
		printf("WSAStartup failed: %d\n", initwsa);
		return 1;
	}
#endif
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		/* example.com is redirected, so we tell libcurl to follow redirection */
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	return 0;
	{
		CURL *curl;
		CURLcode res;
		curl_status_t status = CURL_STATUS_SUCCESS;
		char* out;
		Sll* list;
		FILE *f;

		ftp_file_t file = malloc(sizeof(struct ftp_file));
		file->filename = filename; /* name to store the file as if succesful */
		file->stream = NULL;

		out = malloc(MIN_REQUIRED_REGEX_RESULT_LENGTH);
		/* initialize the linked list */
		initList(&head);

		curl_global_init(CURL_GLOBAL_DEFAULT);

		curl = curl_easy_init();
		if (curl)
		{
			/*
			 * You better replace the URL with one that works!
			 */
			curl_easy_setopt(curl, CURLOPT_URL, url);
			/* Define our callback to get called when there's data to be written */
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
			/* Set a pointer to our struct to pass to the callback */
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

#if DEBUG
			/* Switch on full protocol/debug output */
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

			res = curl_easy_perform(curl);

			easy_extract_regex_from_sll("(([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])\\.){3}([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])", &out);

			f = fopen(filename, "wb+");
			if (!f)
			{
				fprintf(stdout, " failure, can't open file to write: %d\n", GetLastError());
				return -1;
			}
			fflush(f);


			fwrite(out, sizeof(char), strlen(out), f);

			fclose(f);

			/* always cleanup */
			curl_easy_cleanup(curl);

			if (CURLE_OK != res)
			{
#if DEBUG
				/* we failed */
				fprintf(stderr, "error output #: %d\n", GetLastError());
#endif
				status = CURL_STATUS_FAILURE;
			}
		}

		if (file->stream)
			fclose(file->stream); /* close the local file */

		curl_global_cleanup();

		//if(head)
		//free(head);

		return status;
	}
}

WHATISMYIP_DECLARE(int) curl_memwrite(Sll *data, const char *url)
{
  CURL *curl;
  curl_status_t status = CURL_STATUS_SUCCESS;

  char* chunk;

  chunk = malloc(1);  /* will be grown as needed by the realloc above */
 
  /* initialize the linked list */
  //head = NULL;
  initList(&head);

  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl = curl_easy_init();

  /* specify URL to get */
  curl_easy_setopt(curl, CURLOPT_URL, url);

  /* send all data to this function  */
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);

  /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

#if DEBUG
    /* Switch on full protocol/debug output */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

	/* get it! */
	curl_easy_perform(curl);
	
	/* cleanup curl stuff */
	curl_easy_cleanup(curl);
  
	/*
	* Now, our chunk.memory points to a memory block that is chunk.size
	* bytes big and contains the remote file.
	*
	* You should be aware of the fact that at this point we might have an
	* allocated data block, and nothing has yet deallocated that data. So when
	* you're done with it, you should free() it as a nice application.
	*/
#if DEBUG
	fprintf(stdout, "%lu bytes retrieved\n", (long)realsize);
#endif
	/* we're done with libcurl, so clean it up */
	curl_global_cleanup();

	/* export the result to data structure*/

	memcpy(data, &head, sizeof(head));

	return status;
}

WHATISMYIP_DECLARE(int) easy_curl_memwrite(const char *url)
{
  CURL *curl;
  curl_status_t status = CURL_STATUS_SUCCESS;

  char* chunk;

  chunk = malloc(1);  /* will be grown as needed by the realloc above */
 
  /* initialize the linked list */
  //head = NULL;
  initList(&head);

  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl = curl_easy_init();

  /* specify URL to get */
  curl_easy_setopt(curl, CURLOPT_URL, url);

  /* send all data to this function  */
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);

  /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

#if DEBUG
    /* Switch on full protocol/debug output */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

  /* get it! */
  curl_easy_perform(curl);

  /* cleanup curl stuff */
  curl_easy_cleanup(curl);
  
  /*
   * Now, our chunk.memory points to a memory block that is chunk.size
   * bytes big and contains the remote file.
   *
   * You should be aware of the fact that at this point we might have an
   * allocated data block, and nothing has yet deallocated that data. So when
   * you're done with it, you should free() it as a nice application.
   */
#if DEBUG
  fprintf(stdout, "%lu bytes retrieved\n", (long)realsize);
#endif

  /* we're done with libcurl, so clean it up */
  curl_global_cleanup();

  return status;
}

WHATISMYIP_DECLARE(void) set_language(const char *nationality)
{
#ifdef WIN32
#pragma setlocale(nationality)	
#else
	setlocale((LC_CTYPE, nationality);
#endif
	lang_globals.locale = nationality;
};

WHATISMYIP_DECLARE(int) easy_extract_regex_from_sll(const char *pattern, char **result)
{
	Sll *answer;
	char  *out;
	int restable[NUMBER_OF_OFFSETS];
	int res, res2, status = CURL_STATUS_SUCCESS;
	const char *regex;
	const unsigned char* tables = NULL;
	const char** errorptr;
	int* erroroffset;
	pcre* compiled;
	
	out = malloc(MIN_REQUIRED_IP_LENGTH);
		
	if (lang_globals.locale != NULL && lang_globals.locale != "")
	{
		tables = pcre_maketables();
	}
	
	compiled = pcre_compile(pattern, 0 , &errorptr, &erroroffset, tables);

	if (errorptr && erroroffset)
	{
#if DEBUG
		fprintf(stdout, "error compiling regular expression: %d\n", GetLastError());
#endif
		return CURL_STATUS_FAILURE;
	}
	
	answer = &head;

	while(answer)
	{
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
		
		answer = answer->next;
	}


	if (res == -1)
	{
#if DEBUG
		fprintf(stdout, "failed to find a match!\n");
#endif
		status = CURL_STATUS_FAILURE;
		out = "Pattern not found";
		goto end;
	}
	else if (res < 0)
	{
#if DEBUG
		fprintf(stdout, "error occured on running: %d\n!", GetLastError());
#endif		
		status = CURL_STATUS_FAILURE;
		out = "Pattern not found";
		goto end;
	}

	
	
	/* Get the first (1) string found on first match (0)*/
	res2 =  pcre_copy_substring(answer->data, restable, 1, 0, out, MIN_REQUIRED_IP_LENGTH); 
	if (res2 == (int)PCRE_ERROR_NOMEMORY || res2 == (int)PCRE_ERROR_NOSUBSTRING)
	{
#if DEBUG
		fprintf(stdout, "error on memory: %d\n!", GetLastError());
#endif
		out = "Error on memory";
	}
	
end:
	*result = out;
	free(compiled);
	return status;
}

WHATISMYIP_DECLARE(int) extract_regex_from_sll(void *buffer, const char *pattern, char **result)
{
	Sll *answer;
	char  *out;
	int restable[NUMBER_OF_OFFSETS];
	int res, res2, status = CURL_STATUS_SUCCESS;
	const unsigned char* tables = NULL;
	const char** errorptr;
	int* erroroffset;
	pcre* compiled;
	
	out = malloc(MIN_REQUIRED_IP_LENGTH);

	if (lang_globals.locale != NULL && lang_globals.locale != "")
	{
		tables = pcre_maketables();
	}
	
	compiled = pcre_compile(pattern, 0 , &errorptr, &erroroffset, tables);

	if (errorptr && erroroffset)
	{
#if DEBUG
		fprintf(stdout, "error compiling regular expression: %d\n", GetLastError());
#endif
		return CURL_STATUS_FAILURE;
	}
		
	answer = buffer;

	while(answer)
	{
		/* try get our string against the regular expression - no flags - single line files*/
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
		answer= answer->next;
	}
	

	if (res == -1)
	{
#if DEBUG
		fprintf(stdout, "failed to find a match!\n");
#endif
		status = CURL_STATUS_FAILURE;
		out = "Pattern not found";
		goto end;
	}
	else if (res < 0)
	{
#if DEBUG
		fprintf(stdout, "error occured on running: %d\n!", GetLastError());
#endif
		status = CURL_STATUS_FAILURE;
		out = "Pattern not found";
		goto end;
	}
	
	/* Get only one (1) string found on first match (0)*/
	res2 =  pcre_copy_substring(answer->data, restable, 1, 0, out, MIN_REQUIRED_IP_LENGTH); 
	
	if (res2 == (int)PCRE_ERROR_NOMEMORY || res2 == (int)PCRE_ERROR_NOSUBSTRING)
	{
#if DEBUG
		fprintf(stdout, "error on memory: %d\n!", GetLastError());
#endif
		out = "error on memory!";
	}

end:
	*result = out;
	free(compiled);
	return status; 
}

WHATISMYIP_DECLARE(int) get_ip_from_url(const char *url, char **ip)
{
	int status;
	Sll* mysll = malloc(sizeof(Sll));
	
	char **out = malloc(MIN_REQUIRED_REGEX_RESULT_LENGTH);

	curl_memwrite(&mysll, url);

	if (!mysll)
	{
#if DEBUG
		fprintf(stdout, "error fetching data! try calling curl_memwrite first!\n");
#endif
		return CURL_STATUS_SUCCESS;
	}

	status = extract_regex_from_sll(mysll, "(([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])\\.){3}([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])", &out);
	
	*ip = &out;

	if(mysll)
		free(mysll);

	return status;
}

WHATISMYIP_DECLARE(int) easy_get_ip_from_url(const char *url, char **ip)
{
	int status;
	
	char *out;
	out = malloc(MIN_REQUIRED_REGEX_RESULT_LENGTH);
		
	easy_curl_memwrite(url);

	//if (!head)
	{
#if DEBUG
		fprintf(stdout, "error fetching data! try calling curl_memwrite first!\n");
#endif
		return CURL_STATUS_FAILURE;
	}
		
	status = easy_extract_regex_from_sll("(([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])\\.){3}([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])", &out);
	
	*ip = &out;

	//if(head)
	//	free(head);

	return status;
}

WHATISMYIP_DECLARE(int) get_data_from_url(const char *url, const char *pattern, char **data)
{
	int status;
	Sll* mysll = malloc(1000*sizeof(Sll));

	char out[MIN_REQUIRED_REGEX_RESULT_LENGTH];

	curl_memwrite(mysll, url);

	if (!mysll)
	{
#if DEBUG
		fprintf(stdout, "error fetching data! try calling curl_memwrite first!\n");
#endif
		return CURL_STATUS_SUCCESS;
	}

	status = extract_regex_from_sll(mysll, pattern, &out);

	*data = &out;

	if (mysll)
		free(mysll);

	return status;
}

WHATISMYIP_DECLARE(int) easy_get_data_from_url(const char *url, const char * pattern, char **res)
{
	int status;

	char *out;
	out = malloc(MIN_REQUIRED_REGEX_RESULT_LENGTH);

	easy_curl_memwrite(url);

	//if (!head)
	{
#if DEBUG
		fprintf(stdout, "error fetching data! try calling curl_memwrite first!\n");
#endif
		return CURL_STATUS_FAILURE;
	}

	status = easy_extract_regex_from_sll(pattern, &out);

	*res = &out;

	//if (head)
	//	free(head);

	return status;
}

/* minimum required number of parameters */
#define MIN_REQUIRED 2

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


#if !defined(WHATISMYIP_DECLARE_STATIC)
/* main */
int main(int argc, char *argv[]) 
{
	char *output_file = NULL;
	char *upload_file = NULL;
	char *get_file = NULL;
	char *ftp_uri = NULL;
	char *ftp_file = malloc(128);
	int counter = 0;
	CURLcode status = -1;
	char* url = NULL;
	//ACTIONS actions;
	int i;

	if (argc < MIN_REQUIRED) 
	{
		return help();
	}
   
   /* iterate over all arguments */
   for (i = 1; i < (argc - 1); i++) 
   {
		if (strcmp("-u", argv[i]) == 0) 
		{
			upload_file = argv[++i];
			continue;
		}
		if (strcmp("-g", argv[i]) == 0) 
		{
			get_file = argv[++i];
			continue;
		}
		if (strcmp("-o", argv[i]) == 0) 
		{
			output_file = argv[++i];
			continue;
		}
		if (strcmp("-r", argv[i]) == 0) 
		{
			url = argv[++i];
			continue;
		}
		if (strcmp("-f", argv[i]) == 0) 
		{
			ftp_uri = argv[++i];
			continue;
		}
		return help();
   }
   if (output_file)
   {
		if (url)
		{
			get_ip_from_url_in_file(url, output_file);
		}
		else
		{
			get_ip_from_url_in_file("http://checkip.dyndns.com", output_file);
		}
   }
   

   if (upload_file)
   {
		while(status != CURLE_OK && counter < 3)
		{
			if (ftp_uri)
			{
				status = ftp_upload(ftp_uri, upload_file);
			}
			else 
			{
#ifdef DEBUG
				fprintf(stdout, "please provide an ftp uri to upload the file!\n");
				return -1;
#endif
			}
			counter++;
		}
   }
   if (get_file)
   {
		memset(ftp_file, 0, 128);
		if (ftp_uri)
		{
			memcpy(ftp_file, ftp_uri, strlen(url));
		}
		else
		{
#ifdef DEBUG
				fprintf(stdout, "please provide an ftp uri to upload the file!\n");
				return -1;
#endif
		}
		strncat(ftp_file, get_file, strlen(get_file));
		ftp_get(ftp_file);
	   
   }
   return 0;
}

#endif