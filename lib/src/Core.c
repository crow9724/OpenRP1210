//------------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//------------------------------------------------------------------------------
#include "OpenRP1210/Common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define HANDLE_FLAGS_NONE 0
#define HANDLE_FLAGS_FREETARGET 1

#ifdef _DEBUG
	#if defined _WIN32 || defined _WIN64
		#define DBG_MALLOC(size, block, filename, line) _malloc_dbg(size, block, filename, line)
	#else
		#define DBG_MALLOC(size, block, filename, line) malloc(size)
	#endif
#else
	#define DBG_MALLOC(size, block, filename, line) malloc(size)
#endif

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void *rp_dbg_malloc(size_t size, const char *filename, int line)
{
	void *p = DBG_MALLOC(size, _NORMAL_BLOCK, filename, line);

	if(!p)
		rp_SetLastError(ORP_ERR_MEM_ALLOC, NULL);

	return p;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void *rp_rel_malloc(size_t size)
{
	void *p = malloc(size);

	if(!p)
		rp_SetLastError(ORP_ERR_MEM_ALLOC, NULL);

	return p;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void *rp_dbg_mallocZ(size_t size, const char *filename, int line)
{
	void *p = DBG_MALLOC(size, _NORMAL_BLOCK, filename, line);

	if(p)
		memset(p, 0, size);
	else
		rp_SetLastError(ORP_ERR_MEM_ALLOC, NULL);

	return p;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void *rp_rel_mallocZ(size_t size)
{
	void *p = malloc(size);

	if(p)
		memset(p, 0, size);
	else
		rp_SetLastError(ORP_ERR_MEM_ALLOC, NULL);

	return p;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_free(void *ptr)
{
	free(ptr);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_Handle *rp_CreateHandle(void *target, HandleReleaseCallback releaseCallback)
{
	S_Handle *handle = rp_malloc(sizeof(S_Handle));
	if(handle)
	{
		memset(handle, 0, sizeof(S_Handle));
		handle->Target = target;
		handle->ReleaseCallback = releaseCallback;
		handle->HandleFlags = HANDLE_FLAGS_NONE;
	}
	else
		rp_SetLastError(ORP_ERR_MEM_ALLOC, NULL);

	return handle;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_Handle *rp_CopyToHandle(void *target, size_t size, HandleReleaseCallback releaseCallback)
{
	S_Handle *handle = NULL;

	void *targetCopy = rp_malloc(size);
	if(targetCopy)
	{
		memcpy(targetCopy, target, size); // only shallow copy
		handle = rp_CreateHandle(targetCopy, releaseCallback);

		if(handle)
			handle->HandleFlags |= HANDLE_FLAGS_FREETARGET;
	}
	else
		rp_SetLastError(ORP_ERR_MEM_ALLOC, NULL);

	return handle;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rpFreeHandle(ORP_HANDLE handle)
{
	if(handle)
	{
		S_Handle *sHandle = handle;

		if(sHandle->ReleaseCallback)
			sHandle->ReleaseCallback(sHandle);

		if(sHandle->HandleFlags & HANDLE_FLAGS_FREETARGET)
			rp_free(sHandle->Target);

		rp_free(sHandle);
	}
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void *rp_HandleToTarget(ORP_HANDLE handle)
{
	return ((S_Handle *)handle)->Target;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
char *rp_Concat(char *buf, size_t len, const char *text, ...)
{
	va_list args;
	va_start(args, text);

	vsnprintf(buf, len, text, args);

	va_end(args);
	return buf;
}