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
#include <exports.h>
#include <curl/curl.h>
#include <stdlib.h>

#define SLEEP_TIME 300000
#ifdef WIN32
#define HOME "WINDIR"
#define LOGFILE "\\whatismyip.log"
#else
#define HOME "/"
#define LOGFILE "/whatismyip.log"
#endif

SERVICE_STATUS          ServiceStatus; 
SERVICE_STATUS_HANDLE   hStatus; 


void  ServiceMain(int argc, char** argv); 
void  ControlHandler(DWORD request); 
int InitService();
int WriteToLog(char* str);

int WriteToLog(char* str)
{
	FILE* log;
	char *path;
	LPSTR *pa[100];
	GetEnvironmentVariable(HOME, pa,100);
	strncat(pa, LOGFILE, strlen(LOGFILE));
	log = fopen(pa, "a+");
	if (log == NULL)
		return -1;
	fprintf(log, "%s\n", str);
	fclose(log);
	return 0;
}

void ServiceMain(int argc, char** argv) 
{ 
	int error; 
	MEMORYSTATUS memory;
	int result;
	
	CURLcode status ;
	int counter = 0;
	status = (CURLcode)-1;

	ServiceStatus.dwServiceType = SERVICE_WIN32; 
	ServiceStatus.dwCurrentState = SERVICE_START_PENDING; 
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	ServiceStatus.dwWin32ExitCode = 0; 
	ServiceStatus.dwServiceSpecificExitCode = 0; 
	ServiceStatus.dwCheckPoint = 0; 
	ServiceStatus.dwWaitHint = 0; 
 
	hStatus = RegisterServiceCtrlHandler("MemoryStatus", (LPHANDLER_FUNCTION)ControlHandler); 
	if (hStatus == (SERVICE_STATUS_HANDLE)0) 
	{ 
		// Registering Control Handler failed
		WriteToLog("RegisterServiceCtrlHandler started.");
		return; 
	}  
	
	// We report the running status to SCM. 
	ServiceStatus.dwCurrentState = SERVICE_RUNNING; 
	SetServiceStatus (hStatus, &ServiceStatus);
	
	WriteToLog("Monitoring started!");
	 
	// The worker loop of a service
	while (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
	{
		int mystatus;
		GlobalMemoryStatus(&memory);
		
		/* whatismyip stuff */
		if (mystatus = get_ip_from_url_in_file("http://checkip.dyndns.com", "ip.log") == CURL_STATUS_FAILURE) 
		{
			result = WriteToLog("Error creating temporary file for upload!");
			goto end;
		};
   
		while(status != CURLE_OK && counter < 3)
		{
			status = ftp_upload("ftp://put_your_public_folder_here", "ip.log");
			_sleep(100);
			counter++;
		}		
		result = (status != CURLE_OK)?  WriteToLog("Error uploading file!") : WriteToLog("Successfully uploaded file on ftp!");
				
	end:
		if (result)
		{
			ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
			ServiceStatus.dwWin32ExitCode = -1; 
			SetServiceStatus(hStatus, &ServiceStatus);
			return;
		}
		_sleep(SLEEP_TIME);
	}
	return; 
}


void ControlHandler(DWORD request) 
{ 
   switch(request) 
   { 
      case SERVICE_CONTROL_STOP: 
         WriteToLog("Monitoring stopped.");
		 
         ServiceStatus.dwWin32ExitCode = 0; 
         ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
         SetServiceStatus (hStatus, &ServiceStatus);
		 return;  
      case SERVICE_CONTROL_SHUTDOWN: 
         WriteToLog("Monitoring stopped.");

         ServiceStatus.dwWin32ExitCode = 0; 
         ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
         SetServiceStatus (hStatus, &ServiceStatus);
         return; 
        
      default:
         break;
    } 
 
    // Report current status
    SetServiceStatus (hStatus, &ServiceStatus);
 
    return; 
}

void main() 
{
   SERVICE_TABLE_ENTRY ServiceTable[2];
   ServiceTable[0].lpServiceName = "MemoryStatus";
   ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

   ServiceTable[1].lpServiceName = NULL;
   ServiceTable[1].lpServiceProc = NULL;
   // Start the control dispatcher thread for our service
   StartServiceCtrlDispatcher(ServiceTable);  
}