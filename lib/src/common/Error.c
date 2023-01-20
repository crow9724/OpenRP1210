//------------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//------------------------------------------------------------------------------
#include "OpenRP1210/Common.h"
#include "OpenRP1210/platform/Platform.h"
#include <string.h>
#include <stdio.h>

#define MAX_ERROR_LEN 512
#define _ERR_MAX -(ORP_ERR_LENGTH - 1)
#define _NO_ERR_TXT "No error."

const char gErrorText[_ERR_MAX][MAX_ERROR_LEN] =
{
	_NO_ERR_TXT,
	"Invalid argument. ",
	"Memory allocation failure. ",
	"File not found. ",
	"A file I/O error has occurred. ",
	"Value out of range. ",
	"An error has occurred. ",
	"Illegal INI section name. ",
	"Illegal INI character encountered. ",
	"Illegal INI key name. ",
	"INI key not found. ",
	"INI key value not found. ",
	"INI Section not found. ",
	"A system call error has occurred. ",
	"Invalid length. "
};

TLS int gLastError;
TLS char gLastErrorText[MAX_ERROR_LEN] = _NO_ERR_TXT;

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
unsigned long rp_errSetText(ORP_ERR error, const char *msg, va_list *args)
{
	int i = 0, j = 0;
	unsigned long k = 0;
	int e = -error;

	memset(gLastErrorText, 0, MAX_ERROR_LEN);

	if(e >= ORP_ERR_NO_ERROR && e < _ERR_MAX)
		i = snprintf(gLastErrorText, MAX_ERROR_LEN, "%s", gErrorText[e]);
	else
		i = snprintf(gLastErrorText, MAX_ERROR_LEN, "%s", "Description not available. ");

	if(i < 0)
		i = 0;

	if(msg && MAX_ERROR_LEN - i > 0)
	{
		if(args)
			j = vsnprintf(gLastErrorText + i, MAX_ERROR_LEN - i, msg, *args);
		else
			j = snprintf(gLastErrorText + i, MAX_ERROR_LEN - i, "%s. ", msg);

		if(j < 0)
			j = 0;
	}

	if(error == ORP_ERR_SYSTEM)
	{
		k = MAX_ERROR_LEN - (i + j);
		if(k > 0)
			rp_GetSystemErrorText(rp_GetLastSystemError(), gLastErrorText + i + j, &k);
		
		if(k < 0)
			k = 0;
	}

	return i + j + k;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rp_ClearLastError(void)
{
	return rp_SetLastError(ORP_ERR_NO_ERROR, "");
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rp_SetLastErrorVA(ORP_ERR error, const char *msg, va_list *args)
{
	gLastError = error;
	rp_errSetText(error, msg, args);

	return error;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rp_SetLastError(ORP_ERR error, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	rp_SetLastErrorVA(error, msg, &args);

	va_end(args);
	return error;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_AppendLastError(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	size_t sz = strlen(gLastErrorText);

	if(sz < MAX_ERROR_LEN)
		vsnprintf(gLastErrorText + sz, MAX_ERROR_LEN - sz, msg, args);

	va_end(args);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rpGetLastError(void)
{
	return gLastError;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
const char *rpGetLastErrorDesc(void)
{
	return gLastErrorText;
}
