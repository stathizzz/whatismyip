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
#ifndef _WIFIF_H_
# define _WIFI_H_

#include <stdio.h>
#include <windows.h>
#include <wininet.h>
#include <wlanapi.h>

typedef struct wifi_s
{
	HANDLE MyHandle;
	DWORD NumberOfItems;
	WCHAR InterfaceDescription[BUFSIZ];
	WLAN_INTERFACE_STATE CurrentState;
	GUID MyGuid;
	//PWSTR Profil;
	//LPCWSTR	Network;
	WCHAR pwds[16][128];
} wifi_t;

#ifdef __cplusplus
extern "C" {
#endif
	/* WIRELESS */
	WLAN_INTERFACE_STATE check_wifi_status(wchar_t name[]);
	DWORD wifi_create_config(const WCHAR passwords[][128], wifi_t *config);
	DWORD wifi_destroy_config(wifi_t config);
	DWORD wifi_scan_networks(wifi_t config, PWLAN_AVAILABLE_NETWORK_LIST *networks);
	DWORD wifi_connect_to_network(wifi_t config, WLAN_AVAILABLE_NETWORK network, WLAN_CONNECTION_PARAMETERS *parameters);
	DWORD wifi_disconnect(wifi_t config, WLAN_CONNECTION_PARAMETERS parameters);
	
#ifdef __cplusplus
}
#endif
#endif