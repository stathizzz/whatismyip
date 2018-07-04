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
#include "wifi.h"

#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Wininet")

DWORD wifi_create_config(wifi_t *config)
{
	PWLAN_INTERFACE_INFO_LIST	InterfaceList;
	DWORD	Error = ERROR_SUCCESS;
	HANDLE	Handle = NULL;
	DWORD	Version = 0;

	if ((Error = WlanOpenHandle(WLAN_API_VERSION, NULL, &Version, &Handle)) != ERROR_SUCCESS)
		WriteToLog("WlanOpenHandle failed with error %d", Error);
	else
		config->MyHandle = Handle;

	InterfaceList = NULL;
	if ((Error = WlanEnumInterfaces(config->MyHandle, NULL, &InterfaceList)) != ERROR_SUCCESS)
		WriteToLog("WlanOpenHandle failed with error %d", Error);
	else
	{
		config->NumberOfItems = InterfaceList->dwNumberOfItems;
		config->MyGuid = InterfaceList->InterfaceInfo->InterfaceGuid;
		config->CurrentState = InterfaceList->InterfaceInfo->isState;
	}

	return Error;
}

DWORD wifi_destroy_config(wifi_t config) {

	return WlanCloseHandle(config.MyHandle, NULL);
}

DWORD wifi_scan_networks(wifi_t config, PWLAN_AVAILABLE_NETWORK_LIST *networks)
{
	DWORD Error = ERROR_SUCCESS;

	if ((WlanGetAvailableNetworkList(config.MyHandle, &config.MyGuid, 0x00000001, NULL, networks)) != ERROR_SUCCESS)
		WriteToLog("WlanGetAvailableNetworkList failed with error %d", Error);

	return Error;
}

DWORD wifi_connect_to_network(wifi_t config, WLAN_AVAILABLE_NETWORK net)
{
	DWORD Error = ERROR_SUCCESS;
	WLAN_CONNECTION_PARAMETERS Connect = { 0 };
	BSTR profilename;
	BOOL wideProfileNamePresent = *net.strProfileName != 0;
	if (wideProfileNamePresent != TRUE) {
		int Size = lstrlenA(net.dot11Ssid.ucSSID);
		profilename = SysAllocStringLen(NULL, Size);
		MultiByteToWideChar(CP_ACP, 0, net.dot11Ssid.ucSSID, Size, profilename, Size);
	}
	Connect.wlanConnectionMode = net.bSecurityEnabled == TRUE ? wlan_connection_mode_profile : wlan_connection_mode_discovery_unsecure;
	Connect.strProfile = wideProfileNamePresent != TRUE ? profilename : net.strProfileName;
	Connect.pDot11Ssid = &net.dot11Ssid;
	Connect.pDesiredBssidList = NULL;
	Connect.dot11BssType = net.dot11BssType;
	Connect.dwFlags = net.dwFlags;
	if ((Error = WlanConnect(config.MyHandle, &config.MyGuid, &Connect, NULL)) != ERROR_SUCCESS)
		switch (Error)
		{
		case ERROR_INVALID_HANDLE:
			WriteToLog("WlanConnect failed with error %d", Error);
			break;
		case ERROR_ACCESS_DENIED:
			WriteToLog("WlanConnect failed with error %d", Error);
			break;
		case ERROR_INVALID_PARAMETER:
			WriteToLog("WlanConnect failed with error %d", Error);
			break;
		default:
			WriteToLog("WlanConnect unknown error %d", Error);
		}
	
	return Error;
}

DWORD wifi_disconnect(wifi_t config) {

	return WlanDisconnect(config.MyHandle, &config.MyGuid, 0);
}

WLAN_INTERFACE_STATE check_wifi_status(wchar_t name[]) {
	// Declare and initialize variables.
	WLAN_INTERFACE_STATE state = wlan_interface_state_not_ready;
	HANDLE hClient = NULL;
	DWORD dwMaxClient = 2;
	DWORD dwCurVersion = 0;
	DWORD dwResult = 0;
	int iRet = 0;
	WCHAR GuidString[40] = { 0 };
	int i;

	/* variables used for WlanEnumInterfaces  */
	PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
	PWLAN_INTERFACE_INFO pIfInfo = NULL;

	dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
	if (dwResult != ERROR_SUCCESS) {
		WriteToLog("WlanOpenHandle failed with error: %u\n", dwResult);
		// FormatMessage can be used to find out why the function failed
		return 1;
	}
	dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
	if (dwResult != ERROR_SUCCESS) {
		WriteToLog("WlanEnumInterfaces failed with error: %u\n", dwResult);
		// FormatMessage can be used to find out why the function failed
		return 1;
	}
	else {
		WriteToLog("Num Entries: %lu\n", pIfList->dwNumberOfItems);
		WriteToLog("Current Index: %lu\n", pIfList->dwIndex);
		for (i = 0; i < (int)pIfList->dwNumberOfItems; i++) {
			pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[i];
			WriteToLog("  Interface Index[%d]:\t %lu\n", i, i);
			iRet = StringFromGUID2(&pIfInfo->InterfaceGuid, (LPOLESTR)&GuidString, 39);
			if (iRet == 0)
				WriteToLog("StringFromGUID2 failed\n");
			else {
				WriteToLog("  InterfaceGUID[%d]: %ws\n", i, GuidString);
			}
			WriteToLog("  Interface Description[%d]: %ws", i, pIfInfo->strInterfaceDescription);
			WriteToLog("\n");
			WriteToLog("  Interface State[%d]:\t ", i);
			_snwprintf(name, wcslen(pIfInfo->strInterfaceDescription), pIfInfo->strInterfaceDescription);
			state = pIfInfo->isState;
			switch (state) {
			case wlan_interface_state_not_ready:
				WriteToLog("Not ready\n");
				break;
			case wlan_interface_state_connected:
				WriteToLog("Connected\n");
				break;
			case wlan_interface_state_ad_hoc_network_formed:
				WriteToLog("First node in a ad hoc network\n");
				break;
			case wlan_interface_state_disconnecting:
				WriteToLog("Disconnecting\n");
				break;
			case wlan_interface_state_disconnected:
				WriteToLog("Not connected\n");
				break;
			case wlan_interface_state_associating:
				WriteToLog("Attempting to associate with a network\n");
				break;
			case wlan_interface_state_discovering:
				WriteToLog("Auto configuration is discovering settings for the network\n");
				break;
			case wlan_interface_state_authenticating:
				WriteToLog("In process of authenticating\n");
				break;
			default:
				WriteToLog("Unknown state %ld\n", pIfInfo->isState);
				break;
			}
			WriteToLog("\n");
		}
	}
	if (pIfList != NULL) {
		WlanFreeMemory(pIfList);
		pIfList = NULL;
	}
	return state;
}

int wlan_network_strength_comparator(const void *v1, const void *v2)
{
	const WLAN_AVAILABLE_NETWORK *p1 = (WLAN_AVAILABLE_NETWORK *)v1;
	const WLAN_AVAILABLE_NETWORK *p2 = (WLAN_AVAILABLE_NETWORK *)v2;
	if (p1->wlanSignalQuality < p2->wlanSignalQuality)
		return 1;
	else if (p1->wlanSignalQuality > p2->wlanSignalQuality)
		return -1;

	return 0;
}

void wifi_try_connect() {

#define GOOGLE_URL "https://www.google.com"
	BOOL try_toggle = FALSE;
	for (;;) {

		if (InternetCheckConnectionA(GOOGLE_URL, FLAG_ICC_FORCE_CONNECTION, 0)) break;

		BOOL should_break = FALSE;
		wifi_t wifi_config;
		wifi_create_config(&wifi_config);

		PWLAN_AVAILABLE_NETWORK_LIST networks = NULL;
		wifi_scan_networks(wifi_config, &networks);

		if (!networks) {
			printf("Wifi NIC is not connected to the system");
			wchar_t NicName[MAX_PATH] = { 0 };
			WLAN_INTERFACE_STATE state = check_wifi_status(NicName);
			if (state != wlan_interface_state_connected) {
				if (!*NicName) {
					_wsystem(L"netsh interface set interface \"Wi-Fi\" enabled");
				}
				else {
					if (try_toggle) {
						/* simulate opening the network wifi settings menu in win 10 and press the spacebar to toggle wifi on or off (not known)*/
						system("start ms-settings:network-wifi");
						_sleep(10000);
						keybd_event(VK_SPACE, 0, 0, 0);
						_sleep(1000);
						keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
						try_toggle = FALSE;
					}
					else {
						wchar_t buf[BUFSIZ] = { 0 };
						_wsystem(L"netsh interface set interface \"Wi-Fi\" enabled");
						_snwprintf(buf, BUFSIZ, L"wmic path win32_networkadapter where name=\"%s\" call enable", NicName);
						_wsystem(buf);
						try_toggle = TRUE;
					}
					
				}
				_sleep(20000);
			}
			continue;
		}
		//sort networks based on signal strength
		qsort(networks->Network, networks->dwNumberOfItems, sizeof(networks->Network[0]), wlan_network_strength_comparator);

		for (int i = 0; i < networks->dwNumberOfItems; ++i)
		{
			DWORD err = wifi_connect_to_network(wifi_config, networks->Network[i]);
			if (err == ERROR_SUCCESS) {
				_sleep(30000);

				if (InternetCheckConnectionA(GOOGLE_URL, FLAG_ICC_FORCE_CONNECTION, 0)) {
					should_break = TRUE;
					break;
				}
					
			}
		}

		wifi_destroy_config(wifi_config);

		if (should_break) break;
	}
}