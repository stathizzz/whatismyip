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
#include <stdlib.h>
#include "wifi.h"
#include "exports.h"

#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Wininet.lib")

extern int WriteToLog(char* format, ...);

DWORD wifi_create_config(const WCHAR passwords[][128], wifi_t *config)
{
	PWLAN_INTERFACE_INFO_LIST InterfaceList = NULL;
	DWORD Error = ERROR_SUCCESS;
	HANDLE Handle = NULL;
	DWORD Version = 0;

	if ((Error = WlanOpenHandle(WLAN_API_VERSION, NULL, &Version, &Handle)) != ERROR_SUCCESS) {
		WriteToLog("WlanOpenHandle failed with error %d", Error);
		return Error;
	}

	config->MyHandle = Handle;

	if ((Error = WlanEnumInterfaces(config->MyHandle, NULL, &InterfaceList)) != ERROR_SUCCESS)
		WriteToLog("WlanOpenHandle failed with error %d", Error);
	else
	{
		config->NumberOfItems = InterfaceList->dwNumberOfItems;
		config->MyGuid = InterfaceList->InterfaceInfo->InterfaceGuid;
		config->CurrentState = InterfaceList->InterfaceInfo->isState;
		memcpy(config->InterfaceDescription, InterfaceList->InterfaceInfo->strInterfaceDescription, wcslen(InterfaceList->InterfaceInfo->strInterfaceDescription) * sizeof(WCHAR));
	}

	for (int i = 0; i < 16; ++i) {
		if (!passwords || !*passwords[i]) break;
		memcpy(config->pwds[i], passwords[i], wcslen(passwords[i]) * sizeof(WCHAR));
	}
	return Error;
}

DWORD wifi_destroy_config(wifi_t config) {

	return WlanCloseHandle(config.MyHandle, NULL);
}

DWORD wifi_scan_networks(wifi_t config, PWLAN_AVAILABLE_NETWORK_LIST *networks)
{
	DWORD Error = ERROR_SUCCESS;

	if ((Error = WlanGetAvailableNetworkList(config.MyHandle, &config.MyGuid, 0x00000001, NULL, networks)) != ERROR_SUCCESS)
		WriteToLog("WlanGetAvailableNetworkList failed with error %d\n", Error);

	return Error;
}

DWORD wifi_set_profile(wifi_t config, WLAN_AVAILABLE_NETWORK net, const WCHAR *password, WLAN_REASON_CODE *code) {

	LPCWSTR tmpTempl1 =
		L"<?xml version=\"1.0\" encoding=\"US-ASCII\"?>"
		L"<WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\">"
		L"<name>";
	LPCWSTR tmpTempl2 =
		L"</name>"
		L"<SSIDConfig>"
		L"<SSID>"
		/*L"<hex>";
		L"</hex>"*/
		L"<name>";
	LPCWSTR tmpTempl3 =
		L"</name>"
		L"</SSID>"
		L"</SSIDConfig>"
		L"<connectionType>ESS</connectionType>"
		L"<connectionMode>auto</connectionMode>"
		L"<autoSwitch>false</autoSwitch>"
		L"<MSM>"
		L"<security>"
		L"<authEncryption>"
		L"<authentication>"; //WPA
	LPCWSTR tmpTempl4 =
		L"</authentication>"
		L"<encryption>"; //TKIP
	LPCWSTR tmpTempl5 =
		L"</encryption>"
		L"<useOneX>false</useOneX>"
		L"</authEncryption>"
		L"<sharedKey>"
		L"<keyType>passPhrase</keyType>"
		L"<protected>";
	LPCWSTR tmpTempl6 =
		L"</protected>"
		L"<keyMaterial>";
	LPCWSTR tmpTempl7 =
		L"</keyMaterial>"
		L"</sharedKey>"
		L"</security>"
		L"</MSM>"
		L"<MacRandomization xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v3\">"
		L"<enableRandomization>true</enableRandomization>"
		L"<randomizationSeed>";
	LPCWSTR tmpTempl8 =
		L"</randomizationSeed>"
		L"</MacRandomization>"
		L"</WLANProfile>";

	WCHAR templateProfile[4 * BUFSIZ] = { 0 };
	WCHAR profilename[32] = { 0 };
	BOOL wideProfileNamePresent = net.strProfileName && *net.strProfileName;
	WCHAR *protect;
	WCHAR randval[32];

	wcsncat(templateProfile, tmpTempl1, wcslen(tmpTempl1));
	if (wideProfileNamePresent != TRUE) {
		int Size = lstrlenA(net.dot11Ssid.ucSSID);
		MultiByteToWideChar(CP_ACP, 0, net.dot11Ssid.ucSSID, Size, profilename, Size);
	}
	else {
		memcpy(profilename, net.strProfileName, wcslen(net.strProfileName) * sizeof(WCHAR));
	}
	wcsncat(templateProfile, profilename, wcslen(profilename));

	wcsncat(templateProfile, tmpTempl2, wcslen(tmpTempl2));
	wcsncat(templateProfile, profilename, wcslen(profilename));

	wcsncat(templateProfile, tmpTempl3, wcslen(tmpTempl3));
	LPWSTR authalgo = L"";
	switch (net.dot11DefaultAuthAlgorithm) {
	case DOT11_AUTH_ALGO_80211_OPEN:
		authalgo = L"open";
		break;
	case DOT11_AUTH_ALGO_WPA:
		authalgo = L"WPA";
		break;
	case DOT11_AUTH_ALGO_WPA_PSK:
		authalgo = L"WPAPSK";
		break;
	case DOT11_AUTH_ALGO_WPA_NONE:
		authalgo = L"none";
		break;
	case DOT11_AUTH_ALGO_RSNA:
		authalgo = L"WPA2";
		break;
	case DOT11_AUTH_ALGO_RSNA_PSK:
		authalgo = L"WPA2PSK";
		break;
	case DOT11_AUTH_ALGO_80211_SHARED_KEY:
		authalgo = L"80211_SHARED_KEY";
		break;
	case DOT11_AUTH_ALGO_IHV_START:
		authalgo = "IHV_START";
		break;
	case DOT11_AUTH_ALGO_IHV_END:
		authalgo = "IHV_END";
		break;
	default:
		authalgo = L"open";
		break;
	}
	wcsncat(templateProfile, authalgo, wcslen(authalgo));

	wcsncat(templateProfile, tmpTempl4, wcslen(tmpTempl4));
	LPWSTR encr = L"";
	switch (net.dot11DefaultCipherAlgorithm) {
	case DOT11_CIPHER_ALGO_NONE:
		encr = L"none";
		break;
	case DOT11_CIPHER_ALGO_WEP40:
		encr = L"WEP40";
		break;
	case DOT11_CIPHER_ALGO_TKIP:
		encr = L"TKIP";
		break;
	case DOT11_CIPHER_ALGO_CCMP:
		encr = L"AES";
		break;
	case DOT11_CIPHER_ALGO_WEP104:
		encr = L"WEP104";
		break;
	case DOT11_CIPHER_ALGO_BIP:
		encr = L"BIP";
		break;
	case DOT11_CIPHER_ALGO_GCMP:
		encr = L"GCMP";
		break;
	case DOT11_CIPHER_ALGO_WPA_USE_GROUP:
		encr = L"WPA_USE_GROUP";
		break;
	case DOT11_CIPHER_ALGO_WEP:
		encr = L"WEP";
		break;
	case DOT11_CIPHER_ALGO_IHV_START:
		encr = L"IHV_START";
		break;
	case DOT11_CIPHER_ALGO_IHV_END:
		encr = L"IHV_END";
		break;
	default:
		encr = L"";
		break;
	}
	wcsncat(templateProfile, encr, wcslen(encr));

	wcsncat(templateProfile, tmpTempl5, wcslen(tmpTempl5));
	//protect = (net.dot11DefaultAuthAlgorithm == DOT11_AUTH_ALGO_80211_OPEN) ? L"false" : L"true:";
	protect = L"false";
	wcsncat(templateProfile, protect, wcslen(protect));

	wcsncat(templateProfile, tmpTempl6, wcslen(tmpTempl6));
	wcsncat(templateProfile, password, wcslen(password));

	wcsncat(templateProfile, tmpTempl7, wcslen(tmpTempl7));

	_i64tow((rand() * rand()) % 9000000000 + 1000000000, randval, 10);
	wcsncat(templateProfile, randval, sizeof(long long));

	wcsncat(templateProfile, tmpTempl8, wcslen(tmpTempl8));

	return WlanSetProfile(config.MyHandle, &config.MyGuid, 0, templateProfile, NULL, TRUE, NULL, code);
}

DWORD wifi_connect_to_network(wifi_t config, WLAN_AVAILABLE_NETWORK net)
{
	DWORD Error = ERROR_SUCCESS;
	WLAN_CONNECTION_PARAMETERS Connect = { 0 };
	WCHAR profilename[32] = { 0 };

	BOOL wideProfileNamePresent = net.strProfileName && *net.strProfileName;
	if (wideProfileNamePresent != TRUE) {
		int Size = lstrlenA(net.dot11Ssid.ucSSID);
		MultiByteToWideChar(CP_ACP, 0, net.dot11Ssid.ucSSID, Size, profilename, Size);
	}
	else {
		memcpy(profilename, net.strProfileName, wcslen(net.strProfileName) * sizeof(WCHAR));
	}
	Connect.wlanConnectionMode = net.bSecurityEnabled == TRUE ? wlan_connection_mode_profile : wlan_connection_mode_discovery_unsecure;
	Connect.strProfile = profilename;
	Connect.pDot11Ssid = &net.dot11Ssid;
	Connect.pDesiredBssidList = NULL;
	Connect.dot11BssType = net.dot11BssType;
	Connect.dwFlags = net.dwFlags;

	for (int i = 0; i < sizeof(config.pwds) / sizeof(config.pwds[0]); ++i) {
		if (!*config.pwds[i]) break;
		WLAN_REASON_CODE code;
		if (Error = wifi_set_profile(config, net, config.pwds[i], &code) != ERROR_SUCCESS) {
			WCHAR tmp[128] = { 0 };
			CHAR ansitmp[256] = { 0 };
			WlanReasonCodeToString(code, 128, tmp, NULL);
			size_t sizeRequired = WideCharToMultiByte(CP_UTF8, 0, tmp, wcslen(tmp), NULL, 0, NULL, NULL);
			WideCharToMultiByte(CP_UTF8, 0, tmp, wcslen(tmp), ansitmp, sizeRequired, NULL, NULL);
			WriteToLog("Unable to set profile for \"%ws\". Error: %s\n", Connect.strProfile, ansitmp);
		}
		if (Error = WlanConnect(config.MyHandle, &config.MyGuid, &Connect, NULL) != ERROR_SUCCESS)
			switch (Error)
			{
			case ERROR_INVALID_HANDLE:
				WriteToLog("Wlan failed to connect on %ws with error %d - Invalid handle\n", Connect.strProfile, Error);
				break;
			case ERROR_ACCESS_DENIED:
				WriteToLog("Wlan failed to connect on %ws with error %d - Accecss Denied\n", Connect.strProfile, Error);
				break;
			case ERROR_INVALID_PARAMETER:
				WriteToLog("Wlan failed to connect on %ws with error %d - Invalid parameter\n", Connect.strProfile, Error);
				break;
			default:
				WriteToLog("Wlan failed to connect on %ws with error %d\n", Connect.strProfile, Error);
				break;
			}
		else
			break;
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
		return wlan_interface_state_not_ready;
	}
	dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
	if (dwResult != ERROR_SUCCESS) {
		WriteToLog("WlanEnumInterfaces failed with error: %u\n", dwResult);
		WlanCloseHandle(hClient, NULL);
		// FormatMessage can be used to find out why the function failed
		return wlan_interface_state_not_ready;
	}
	else {
		WriteToLog("Num Entries: %lu\n", pIfList->dwNumberOfItems);
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
	WlanCloseHandle(hClient, NULL);

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

void toggle_wifi_windows10() {

	/* simulate opening the network wifi settings menu in win 10 and press the spacebar to toggle wifi on or off (not known)*/
	TCHAR runme[MAX_PATH];
	TCHAR driveLetter[3];
	TCHAR directory[MAX_PATH];
	TCHAR FinalPath[MAX_PATH];
	if (!GetModuleFileName(NULL, runme, MAX_PATH)) {
		WriteToLog("Cannot install service! Error: (%s)\n", strerror(GetLastError()));
		return;
	}
	_splitpath(runme, driveLetter, directory, NULL, NULL);
	memset(runme, 0, MAX_PATH);
	snprintf(runme, MAX_PATH, "%s%s%s", driveLetter, directory, "runme.vbs");

	WriteToLog("Open ms-settings window\n");
	system("start ms-settings:network-wifi");

	_sleep(5000);

	FILE *f = fopen(runme, "w");
	if (f) {
		TCHAR buf[MAX_PATH];
		char *script = "WScript.CreateObject(\"WScript.Shell\").SendKeys \" \"";
		fwrite(script, sizeof(char), strlen(script) + 1, f);
		fclose(f);
		WriteToLog("Running script %s\n", runme);
		snprintf(buf, MAX_PATH, "cscript /nologo %s", runme);
		/* following method is gonna fail for a windows service - no keyboard or mouse events possible */
		system(buf);
	}
	else {
		/* following method is gonna fail for a windows service - no keyboard or mouse events possible */
		INPUT space = { 0 };
		space.type = INPUT_KEYBOARD;
		space.ki.wVk = VK_SPACE;
		space.ki.dwFlags = 0;
		/* following method is gonna fail for a windows service - no keyboard or mouse events possible */
		SendInput(1, &space, sizeof(INPUT));
		_sleep(300);
		space.ki.dwFlags = KEYEVENTF_KEYUP;
		/* following method is gonna fail for a windows service - no keyboard or mouse events possible */
		SendInput(1, &space, sizeof(INPUT));
	}
}

WHATISMYIP_DECLARE(void) wifi_try_connect(const WCHAR *wifiName, const WCHAR passwords[][128]) {

#define GOOGLE_URL "https://www.google.com"
	BOOL try_toggle = FALSE;
	for (;;) {

		if (InternetCheckConnectionA(GOOGLE_URL, FLAG_ICC_FORCE_CONNECTION, 0)) break;

		BOOL should_break = FALSE;
		wifi_t wifi_config = { 0 };
		if (wifi_create_config(passwords, &wifi_config) != ERROR_SUCCESS) {
			_sleep(5000);
			continue;
		}

		PWLAN_AVAILABLE_NETWORK_LIST networks = NULL;
		if (wifi_scan_networks(wifi_config, &networks) != ERROR_SUCCESS)
			//DO NOTHING
			;

		if (!networks) {
			WCHAR NicName[MAX_PATH] = { 0 };
			WLAN_INTERFACE_STATE state = check_wifi_status(NicName);
			if (state != wlan_interface_state_connected) {
				WCHAR buf[BUFSIZ] = { 0 };
				WriteToLog("Wifi NIC %ws is not connected to the system\n", NicName);
				if (!*NicName) {
					WriteToLog("Enabling %ws interface through netsh\n", wifiName);
					_snwprintf(buf, BUFSIZ, L"netsh interface set interface \"%s\" enabled", wifiName);
					_wsystem(buf);
				}
				else {
					if (try_toggle) {

						toggle_wifi_windows10();
					}
					else {
						WriteToLog("Enabling %ws interface through netsh\n", wifiName);
						_snwprintf(buf, BUFSIZ, L"netsh interface set interface \"%s\" enabled", wifiName);
						WriteToLog("%ws\n", buf);
						_wsystem(buf);
						_sleep(300);
						memset(buf, 0, BUFSIZ);
						WriteToLog("Enabling %ws network adapter through WMI\n", NicName);
						_snwprintf(buf, BUFSIZ, L"wmic path win32_networkadapter where name=\"%s\" call enable", NicName);
						WriteToLog("%ws\n", buf);
						_wsystem(buf);
					}
					try_toggle = !try_toggle;
				}
#ifdef _DEBUG
				_sleep(1000);
#else
				_sleep(10000);
#endif
			}
			goto end;
		}
		WriteToLog("Networks found!\n");
		//sort networks based on signal strength
		qsort(networks->Network, networks->dwNumberOfItems, sizeof(networks->Network[0]), wlan_network_strength_comparator);

		for (int i = 0; i < networks->dwNumberOfItems; ++i)
		{
			DWORD err = wifi_connect_to_network(wifi_config, networks->Network[i]);
			if (err == ERROR_SUCCESS) {
#ifdef _DEBUG
				_sleep(5000);
#else
				_sleep(30000);
#endif
				if (InternetCheckConnectionA(GOOGLE_URL, FLAG_ICC_FORCE_CONNECTION, 0)) {
					should_break = TRUE;
					break;
				}

			}
			else
				_sleep(1000);
		}
	end:
		wifi_destroy_config(wifi_config);

		if (should_break) break;

		_sleep(20);
	}
}