//------------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//------------------------------------------------------------------------------
#include "OpenRP1210/platform/Platform.h"
#include "OpenRP1210/Common.h"
#include <assert.h>
#include <Windows.h>
#include <UserEnv.h>

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
unsigned int rp_GetDirectory(E_SpecialDir directory, char *buf, unsigned int len)
{
	// the length of the directory will be returned when buf=NULL and len=0
	// length returned includes the null terminator
	unsigned int r = 0;

	if(directory == SD_OSHOME)
		r = GetWindowsDirectoryA(buf, len);
	else if(directory == SD_SYS)
		r = GetSystemDirectoryA(buf, len);
	else if(directory == SD_USERHOME)
	{
		// GetProfilesDirectory doesn't appear to behave as documented on MSDN
		// if lpProfileDir = NULL, then function should set lpschSize to required length
		// however, (at least on WIN10/VS2022) setting lpProfileDir to NULL causes
		// function to return FALSE and sets LASTERROR to 87/x57 (ERROR_INVALID_PARAMETER)
		unsigned int sz = len;
		char tmpBuf = 0; // use to work-around function not respecting lpProfileDir=NULL
		BOOL b = GetProfilesDirectoryA(buf == NULL && len == 0 ? &tmpBuf : buf, &sz);

		if(!b)
			r = sz > 0 ? sz - 1 : 0;
		else
			r = sz - 1; // ignore null-term in length to normalize w/ GetWindows/SystemDirectory
	}

	if(r > 0)
	{
		if(buf != NULL)
			r++; // include null term
	}
	else
		rp_SetLastError(ORP_ERR_SYSTEM, " Error code = %d", GetLastError());

	return r;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
uint64_t rp_GetFileSize(const char *file)
{
	WIN32_FILE_ATTRIBUTE_DATA fa = { 0 };
	if (GetFileAttributesExA(file, GetFileExInfoStandard, &fa))
		return ((uint64_t)fa.nFileSizeHigh << 32LL) | fa.nFileSizeLow;
	else
	{
		rp_SetLastError(ORP_ERR_FILE_NOT_FOUND, NULL);
		return 0;
	}
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
unsigned int rp_GetSpecialDir(E_SpecialDir directory, char *buf, unsigned int len)
{
	unsigned int dirLen = rp_GetDirectory(directory, NULL, 0);

	if(!ORP_IS_ERR(rpGetLastError()) && buf)
	{
		if(len >= dirLen)
			dirLen = rp_GetDirectory(directory, buf, len);
		else
			rp_SetLastError(ORP_ERR_LENGTH, NULL);
	}

	return dirLen;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
char rp_PathSeparator(void)
{
	return '\\';
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
unsigned long rp_GetLastSystemError(void)
{
	return GetLastError();
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rp_GetSystemErrorText(unsigned long errCode, char *buf, unsigned long *len)
{
	if(buf == NULL || len == NULL)
		return ORP_ERR_BAD_ARG;
	else
	{
		DWORD r = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			                     NULL,
			                     errCode,
			                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			                     buf,
			                     *len,
			                     NULL);

		if(r == 0 && *len > 0)
			return ORP_ERR_GENERAL;
		else
		{
			*len = r - 1;
			return ORP_ERR_NO_ERROR;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_CloseLib(ORP_HANDLE hLib)
{
	assert(hLib != NULL);

	HMODULE hMod = rp_HandleToTarget(hLib);
	FreeLibrary(hMod);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_HANDLE rp_OpenLib(const char *path)
{
	assert(path != NULL);
	rp_ClearLastError();

	ORP_HANDLE handle = NULL;
	HMODULE hMod = LoadLibraryA(path);

	if(hMod)
		handle = rp_CreateHandle(hMod, rp_CloseLib);
	else
		rp_SetLastError(ORP_ERR_SYSTEM, "Failed to load library: %s. ", path);

	return handle;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void *rp_GetLibSymbol(ORP_HANDLE hLib, const char *symbol)
{
	assert(hLib != NULL);
	assert(symbol != NULL);
	rp_ClearLastError();

	HMODULE hMod = rp_HandleToTarget(hLib);
	FARPROC proc = GetProcAddress(hMod, symbol);

	if(!proc)
		rp_SetLastError(ORP_ERR_SYSTEM, "Failed to find symbol: %s", symbol);

	return proc;
}