//------------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//------------------------------------------------------------------------------
#ifndef OPENRP1210_PLATFORM_H__
#define OPENRP1210_PLATFORM_H__

#include "OpenRP1210/OpenRP1210.h"
#include <stdint.h>

typedef enum E_SpecialDir_t
{
	SD_OSHOME,
	SD_USERHOME,
	SD_SYS
}E_SpecialDir;

uint64_t rp_GetFileSize(const char* file);
unsigned int rp_GetSpecialDir(E_SpecialDir directory, char *buf, unsigned int len);
char rp_PathSeparator(void);

unsigned long rp_GetLastSystemError(void);
ORP_ERR rp_GetSystemErrorText(unsigned long errCode, char *buf, unsigned long *len);

ORP_HANDLE rp_OpenLib(const char *path);
void *rp_GetLibSymbol(ORP_HANDLE hLib, const char *symbol);

#endif
