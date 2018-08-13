#ifndef MAC_HOST_IP_H
#define MAC_HOST_IP_H

#include <string>
#include "exports.h"

std::string winMacSpoofer_getMac();

void winMacSpoofer_changeMacToRandom();

std::wstring winMacSpoofer_getHost();

void winMacSpoofer_changeHost(std::string newName = "DESKTOP-KAHIDHA");

#ifdef __cplusplus
extern "C" {
#endif
	void getMAC(int type, char mac[], char localip[]);
#ifdef __cplusplus
}
#endif
#endif
