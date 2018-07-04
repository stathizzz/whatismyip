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
#include "whatismyip.h"

void getMAC(int type, char mac[], char localip[]) {

	IP_ADAPTER_INFO tmp;
	DWORD dwBufLen = 0;

	// Make an initial call to GetAdaptersInfo to get the necessary size into the dwBufLen variable
	GetAdaptersInfo(&tmp, &dwBufLen);

	PIP_ADAPTER_INFO AdapterInfo = malloc(dwBufLen);

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
					WriteToLog("Wi-Fi\n");
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
					WriteToLog("Unknown type %ld\n", pAdapterInfo->Type);
					break;
				}
			}
			pAdapterInfo = pAdapterInfo->Next;
		} while (pAdapterInfo);
	}
end:
	free(AdapterInfo);

	WriteToLog("Local IP Address: %s\n", localip);
	WriteToLog("MAC address: %s\n", mac);
}



