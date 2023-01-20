//------------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//------------------------------------------------------------------------------
#ifndef OPENRP1210_CORE_H__
#define OPENRP1210_CORE_H__

#include "OpenRP1210/OpenRP1210.h"
#include <stddef.h>

typedef struct S_Handle_t
{
	unsigned int HandleFlags;
	void *Target;
	void (*ReleaseCallback)(ORP_HANDLE handle);
}S_Handle;

typedef void (*HandleReleaseCallback)(ORP_HANDLE handle);

#ifdef _DEBUG
	#define rp_malloc(size) rp_dbg_malloc(size, __FILE__, __LINE__)
	#define rp_mallocZ(size) rp_dbg_mallocZ(size, __FILE__, __LINE__)
#else
	#define rp_malloc(size) rp_rel_malloc(size)
	#define rp_mallocZ(size) rp_rel_mallocZ(size)
#endif

void *rp_rel_malloc(size_t size);
void *rp_rel_mallocZ(size_t size);

void *rp_dbg_malloc(size_t size, const char *filename, int line);
void *rp_dbg_mallocZ(size_t size, const char *filename, int line);

void rp_free(void *ptr);

S_Handle *rp_CreateHandle(void *target, HandleReleaseCallback releaseCallback);
S_Handle *rp_CopyToHandle(void *target, size_t size, HandleReleaseCallback releaseCallback);

void *rp_HandleToTarget(ORP_HANDLE handle);

char *rp_Concat(char *buf, size_t len, const char *text, ...);

#endif
