#ifndef __EXPORTS_H
#define __EXPORTS_H

#include <stdio.h>

#ifdef WIN32

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <assert.h>
#include <stdint.h>

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

enum curl_status
{
	CURL_STATUS_SUCCESS = 0,
	CURL_STATUS_FAILURE = -1
};

typedef struct ftp_file *ftp_file_t;
typedef struct locale_struct locale_struct;
typedef enum curl_status curl_status_t;

#ifdef __cplusplus
extern "C" {
#endif
//
/*
* Extract the first regex value from a linked list <buffer> to <result>
*/
WHATISMYIP_DECLARE(int) extract_regex_from_sll(void* buffer, const char* pattern, char** result);

/*
* Write the contents of the <url> to a single linked list <data>
*/
WHATISMYIP_DECLARE(int) curl_memwrite(void *data, const char* url);

/*
* Set the locale to be used, so that characters retrieved are in the desired locale compatibility
*/
WHATISMYIP_DECLARE(void) set_language(const char* nationality);

/*
* Write the contents of the html page located at <url> to memory
*/
WHATISMYIP_DECLARE(int) easy_curl_memwrite(const char *url);

/*
* Extract the first regex <pattern> match to <result> from a secured memory buffer (written with  easy_curl_memwrite)
*/
WHATISMYIP_DECLARE(int) easy_extract_regex_from_sll(const char *pattern, char **result);

/*
* Gets your public <ip> address from the specified <url> website echoing it back 
* This method is equivalent to easy_get_ip_from_url. Only difference is the way the ip is fetched. 
*/
WHATISMYIP_DECLARE(int) get_ip_from_url(const char* url, char** ip);

/*
* Gets your public <ip> address from the specified <url> website echoing it back 
* This method is equivalent to get_ip_from_url. Only difference is the way the ip is fetched. 
*/
WHATISMYIP_DECLARE(int) easy_get_ip_from_url(const char* url, char** ip);

/*
* Gets your public ip address  in a file with name <filename> from the specified <url> website echoing it back
*/
WHATISMYIP_DECLARE(int) get_ip_from_url_in_file(const char* url, const char* filename);

/*
* Posts the data from the specified <url> in a file with name <filename>
*/
WHATISMYIP_DECLARE(int) get_httpdata_in_file(const char* url, const char* filename);

/*
* Upload the file on an ftp server defined by the url_no_file uri.
* Args:
* @url_no_file: the uri of the ftp server, written as ftp://user:password@ftp.server.com/
* @file: the path to the file to upload 
*/
WHATISMYIP_DECLARE(int) ftp_upload(char *url_no_file, char* file);

/*
* Download the file from a ftp server 
* Args:
* @full_url: the full uri of the ftp server, written as ftp://user:password@ftp.server.com/thefile
*/
WHATISMYIP_DECLARE(int) ftp_get(char *full_url);

#ifdef __cplusplus
}
#endif
#endif