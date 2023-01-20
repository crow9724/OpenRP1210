//------------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//------------------------------------------------------------------------------
#ifndef OPENRP1210_INI_H__
#define OPENRP1210_INI_H__

#include "OpenRP1210/OpenRP1210.h"
#include "OpenRP1210/Common.h"
#include <stddef.h>
#include <stdbool.h>

typedef bool (*ENUM_KEYS_CALLBACK)(const char *key, const char *value, void *userPtr);

typedef enum E_IniDuplicateMode_t
{
	IniDuplicateMode_Ignore,
	IniDuplicateMode_Overwrite
}E_IniDuplicateMode;

typedef struct S_IniConfig_t
{
	E_IniDuplicateMode DuplicateMode;
	bool TrimKeyValues;
	bool KeysCaseInsensitive;
}S_IniConfig;

ORP_HANDLE rp_IniOpen(const char *iniPath, S_IniConfig *config);

bool rp_IniHasKey(ORP_HANDLE hIni, const char *section, const char *key);
bool rp_IniHasSection(ORP_HANDLE hIni, const char *section);

ORP_ERR rp_IniReadKey(ORP_HANDLE hIni, const char *section, const char *key, char *dest, size_t length);
ORP_ERR rp_IniReadKeyLength(ORP_HANDLE hIni, const char *section, const char *key, size_t *length);
ORP_ERR rp_IniReadInt(ORP_HANDLE hIni, const char *section, const char *key, int *value);

ORP_ERR rp_IniEnumerateKeys(ORP_HANDLE hIni, const char *section, ENUM_KEYS_CALLBACK callback, void *userPtr);

#endif
