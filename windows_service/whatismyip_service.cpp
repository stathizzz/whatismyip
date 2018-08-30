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
#include <curl/curl.h>
#include <stdlib.h>
#include <string>
#include "wifi.h"
#include "exports.hpp"

#define TEST 0
#define SLEEP_TIME 100000

#ifdef WIN32
#define HOME "WINDIR"
#else
#define HOME "/"
#endif
#pragma comment(lib, "Advapi32")

static SERVICE_STATUS ServiceStatus = { 0 };
static SERVICE_STATUS_HANDLE hStatus = NULL;

void __stdcall ControlHandler(DWORD request);

BOOL should_break = FALSE;

void ServiceMain(int argc, char** argv)
{
	TCHAR driveletter[MAX_PATH] = { 0 };
	TCHAR pa[MAX_PATH] = { 0 };
	
	GetEnvironmentVariable(HOME, driveletter, MAX_PATH);
	snprintf(pa, MAX_PATH, "%s\\%s.log", driveletter, SERVICE_NAME);
	
	InitLog(pa);
	WriteToLog((char *)"Log initialized.\n");

	WriteToLog((char *)"Reading arg values\n");
	

	ServiceStatus.dwServiceType = SERVICE_WIN32;
	ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	ServiceStatus.dwWin32ExitCode = 0;
	ServiceStatus.dwServiceSpecificExitCode = 0;
	ServiceStatus.dwCheckPoint = 0;
	ServiceStatus.dwWaitHint = 0;

#if !TEST
	hStatus = RegisterServiceCtrlHandler(SERVICE_NAME, (LPHANDLER_FUNCTION)&ControlHandler);
#endif

#ifdef _DEBUG
	WriteToLog((char *)"RegisterServiceCtrlHandler returned %d.\n", hStatus);
#endif

#if !TEST
	if (hStatus == NULL)
	{
		// Registering Control Handler failed
		WriteToLog((char *)"RegisterServiceCtrlHandler failed.\n");
		return;
	}
#endif
	// We report the running status to SCM. 
	ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(hStatus, &ServiceStatus);

	WriteToLog((char *)"Monitoring started!\n");

	srand(time(NULL));

	// The worker loop of a service
	while (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
	{
		WHATISMYIP_ARGS formatted = { 0 };
		formatArgsAndSaveOnReg(argc, argv, &formatted);

		if (!*formatted.url) // -r is REQUIRED 
		{
			WriteToLog((char *)"Reading registry values\n");
			//try reading values from registry
			readArgsFromReg(&formatted);

			if (!*formatted.friendly_nic_name) {
				int siz = lstrlenA("Wi-Fi");
				MultiByteToWideChar(CP_ACP, 0, "Wi-Fi", siz, (LPWSTR)formatted.friendly_nic_name, siz);
				writeToRegW(L"--friendlyNIC", REG_SZ, formatted.friendly_nic_name);
			}

			if (!*formatted.url) {
				snprintf(formatted.url, strlen(SAFE_URL) + 1, SAFE_URL);
				writeToReg("-r", REG_SZ, formatted.url);
			}
			if (should_break) {
				ServiceStatus.dwCurrentState = SERVICE_STOPPED;
				ServiceStatus.dwWin32ExitCode = -1;
				SetServiceStatus(hStatus, &ServiceStatus);
				WriteToLog((char *)"Args are not enough! Monitoring stopped.\n");
				break;
			}
			WriteToLog((char *)"Registry values were read successfully\n");
		}
		else {
			WriteToLog((char *)"Arg values were read successfully\n");
		}

		wifi_try_connect(formatted.friendly_nic_name, formatted.passwords);

		WriteToLog((char *)"\n");
		/* whatismyip stuff */
		if (*formatted.output_file && CURLE_OK != easy_get_ip(formatted.url, formatted.output_file)) {

			WriteToLog((char *)"Error creating temporary file for upload!\n");
			goto end;
		}
		if (*formatted.dropbox_token && CURLE_OK != dropbox_upload_mstsc(formatted.dropbox_token, formatted.url)) {

			WriteToLog((char *)"Error uploading file!\n");
			goto end;
		}

	end:
		WriteToLog((char *)"Going on deep sleep\n");
		_sleep(SLEEP_TIME);
	}
	WriteToLog((char *)"Monitoring stopped!\n");
}


void __stdcall ControlHandler(DWORD request)
{
	switch (request)
	{
	case SERVICE_CONTROL_STOP:
		WriteToLog((char *)"Monitoring stopped.\n");

		ServiceStatus.dwWin32ExitCode = 0;
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(hStatus, &ServiceStatus);
		return;
	case SERVICE_CONTROL_SHUTDOWN:
		WriteToLog((char *)"Monitoring shut down.\n");

		ServiceStatus.dwWin32ExitCode = 0;
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(hStatus, &ServiceStatus);
		return;

	default:
		break;
	}

	// Report current status
	WriteToLog((char *)"Service status is set to %d\n", hStatus);
	SetServiceStatus(hStatus, &ServiceStatus);

	return;
}

void main(int argc, char **argv)
{
#if TEST
	ServiceMain(argc, argv);
#else
	SERVICE_TABLE_ENTRY ServiceTable[2];
	ServiceTable[0].lpServiceName = (LPSTR)SERVICE_NAME;
	ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;
	// Start the control dispatcher thread for our service
	StartServiceCtrlDispatcher(ServiceTable);
#endif

	/*SC_HANDLE schSCManager;
	SC_HANDLE schService;
	char szPath[MAX_PATH];

	if (!GetModuleFileName(NULL, szPath, MAX_PATH)) {
		printf("Error on getting filename\n");
		return;
	}

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == schSCManager) {
		printf("Error opening SC Manager\n");
		return;
	}
	
start:
	schService = CreateService(
		schSCManager
		, SERVICE_NAME
		, SERVICE_NAME
		, SERVICE_ALL_ACCESS
		, SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS
		, SERVICE_DEMAND_START
		, SERVICE_ERROR_NORMAL
		, szPath
		, NULL, NULL, NULL, NULL, NULL);

	if (NULL == schService) {

		schService = OpenService(schSCManager, SERVICE_NAME, SERVICE_ALL_ACCESS);
		if (NULL == schService) {
			printf("Error creating or opening service\n");
			goto end;
		}
		else {
			DeleteService(schService);
			goto start;
		}
		
	}

	CloseServiceHandle(schService);

end:
	CloseServiceHandle(schSCManager);*/
}