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
#include "spoofMac.h"
#include "spoofHost.h"
#include "exports.hpp"

#pragma comment(lib, "kernel32")

extern "C" WHATISMYIP_DECLARE(void) getMAC(int type, char mac[], char localip[]) {

	IP_ADAPTER_INFO tmp;
	DWORD dwBufLen = 0;

	// Make an initial call to GetAdaptersInfo to get the necessary size into the dwBufLen variable
	GetAdaptersInfo(&tmp, &dwBufLen);

	PIP_ADAPTER_INFO AdapterInfo = (PIP_ADAPTER_INFO)malloc(dwBufLen);

	if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR) {
		PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;// Contains pointer to current adapter info
		do {
			if (pAdapterInfo->Type == type) {
				switch (pAdapterInfo->Type) {
				case MIB_IF_TYPE_OTHER:
					// Other
					break;
				case MIB_IF_TYPE_ETHERNET:
					// Ethernet
					break;
				case 71:
					//Wi-Fi
					WriteToLog((char *)"Wi-Fi\n");
					if (stricmp(pAdapterInfo->IpAddressList.IpAddress.String, "0.0.0.0")) {
						snprintf(mac, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
							pAdapterInfo->Address[0], pAdapterInfo->Address[1],
							pAdapterInfo->Address[2], pAdapterInfo->Address[3],
							pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
						snprintf(localip, strlen(pAdapterInfo->IpAddressList.IpAddress.String) + 1, pAdapterInfo->IpAddressList.IpAddress.String);
					}
					goto end;
					break;
				default:
					WriteToLog((char *)"Unknown type %ld\n", pAdapterInfo->Type);
					break;
				}
			}
			pAdapterInfo = pAdapterInfo->Next;
		} while (pAdapterInfo);
	}
end:
	free(AdapterInfo);

	WriteToLog((char *)"Local IP Address: %s\n", localip);
	WriteToLog((char *)"MAC address: %s\n", mac);
}

WHATISMYIP_DECLARE(BOOL) winMacSpoofer_getMac(std::string *mac) {

	try {
		*mac = spoofMac::getCurrentMAcAddress();
	}
	catch (...) {
		return FALSE;
	}
	return TRUE;
}


WHATISMYIP_DECLARE(std::string) winMacSpoofer_getRandomMac() {

	return spoofMac::randomizeMAC();
}

WHATISMYIP_DECLARE(BOOL) winMacSpoofer_getNicFriendlyName(std::wstring *nic) {

	try {
		*nic = spoofMac::getNicFriendlyName();
		return TRUE;
	}
	catch (...) {
		return FALSE;
	}
}

WHATISMYIP_DECLARE(LONG) winMacSpoofer_changeMac(std::string newmac) {

	return spoofMac::setNewMac(newmac);
}

WHATISMYIP_DECLARE(void) winMacSpoofer_netshRestart() {

	return spoofMac::netshRestart();
}

WHATISMYIP_DECLARE(BOOL) winMacSpoofer_getHost(std::wstring *host) {

	try {
		*host = spoofHost::getHostName();
	}
	catch (...) {
		return FALSE;
	}
	return TRUE;
}

WHATISMYIP_DECLARE(std::string) winMacSpoofer_getRandomHost() {

	char newValArray[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
		, 'G', 'H', 'I', 'J','K','L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' };

	std::string newMac = "DESKTOP_";

	for (int i = 0; i < 7; i++) {

		newMac += newValArray[rand() % 36];
	}

	return newMac;
}

WHATISMYIP_DECLARE(LONG) winMacSpoofer_changeHost(std::string newName) {

	LONG retval = spoofHost::setNewHostName(newName);
	if (retval == ERROR_SUCCESS) {
		SetComputerName(newName.c_str());
		SetComputerNameEx(ComputerNamePhysicalDnsHostname, newName.c_str());
		WriteToLog((char *)"Next time the computer is restarted, it will have the %s name\n", newName.c_str());
	}
	return retval;
}
