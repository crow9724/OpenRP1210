//------------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//------------------------------------------------------------------------------
#include "OpenRP1210/platform/Platform.h"
#include "OpenRP1210/Common.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/stat.h>

static const char  gSYS_HOME_DIR[] = "/";
static const int gSYS_HOME_DIR_LEN = sizeof(gSYS_HOME_DIR) / sizeof(gSYS_HOME_DIR[0]);

static const char  gSYS_DIR[] = "/";
static const int gSYS_DIR_LEN = sizeof(gSYS_DIR) / sizeof(gSYS_DIR[0]);

static const char  gUSER_HOME_DIR[] = "/home"; // not necessarily true
static const int gUSER_HOME_DIR_LEN = sizeof(gUSER_HOME_DIR) / sizeof(gUSER_HOME_DIR[0]);

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
unsigned int rp_GetDirectory(E_SpecialDir directory, char *buf, unsigned int len)
{
	// the length of the directory will be returned when buf=NULL or if len < min req'd length
    // some of these make no sense in a linux context, but needed for Windows
    unsigned int r = 0;

    if(directory == SD_OSHOME)
    {
        r = gSYS_HOME_DIR_LEN;

        if(len >= r)
            strncpy(buf, gSYS_HOME_DIR, len);      
    }
    else if(directory == SD_SYS)
    {
        r = gSYS_DIR_LEN;

        if(len >= r)
            strncpy(buf, gSYS_DIR, len);
    }
    else if(directory == SD_USERHOME)
    {
        r = gUSER_HOME_DIR_LEN;

        if(len >= r)
            strncpy(buf, gUSER_HOME_DIR, len);
    }

    if(r == 0)
        rp_SetLastError(ORP_ERR_BAD_ARG, " Undefined special directory.");

    return r;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
uint64_t rp_GetFileSize(const char *file)
{
    struct stat s;

    if(stat(file, &s) == -1)
	{
		rp_SetLastError(ORP_ERR_FILE_NOT_FOUND, NULL);
		return 0;
	}
    else
        return s.st_size;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
unsigned int rp_GetSpecialDir(E_SpecialDir directory, char *buf, unsigned int len)
{
	unsigned int dirLen = rp_GetDirectory(directory, buf, len);

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
	return '/';
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
unsigned long rp_GetLastSystemError(void)
{
	return errno;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rp_GetSystemErrorText(unsigned long errCode, char *buf, unsigned long *len)
{
    memset(buf, 0, *len);
    rp_strerror(errCode, buf, *len);
    *len = strlen(buf);

    return ORP_ERR_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_CloseLib(ORP_HANDLE hLib)
{
    assert(hLib != NULL);

    void *handle = rp_HandleToTarget(hLib);
    dlclose(handle);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_HANDLE rp_OpenLib(const char *path)
{
    assert(path != NULL);

    ORP_HANDLE orpHandle = NULL;
    void *handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);

	if(handle)
		orpHandle = rp_CreateHandle(handle, rp_CloseLib);
	else
    {
        const char *e = dlerror();
		rp_SetLastError(ORP_ERR_GENERAL, "Failed to load library: %s. dlerror=%s.", path, e ? e : "");
    }

    return orpHandle;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void *rp_GetLibSymbol(ORP_HANDLE hLib, const char *symbol)
{
    assert(hLib != NULL);

    void *handle = rp_HandleToTarget(hLib);

    dlerror(); // clear any errors
    void *s = dlsym(handle, symbol);
    char *e = dlerror();

    if(e)
        rp_SetLastError(ORP_ERR_GENERAL, "Failed to load symbol: %s. dlerror=%s.", symbol, e);

    return s;
}