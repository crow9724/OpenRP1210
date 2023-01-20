//------------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//------------------------------------------------------------------------------
#ifndef OPENRP1210_COMMON_H__
#define OPENRP1210_COMMON_H__

#if defined _WIN32 || defined _WIN64
	#define TLS __declspec(thread)

	#ifdef _DEBUG
		// enable memory leak detection for MS CRT
		// use _CrtDumpMemoryLeaks to check for leaks
		#define _CRTDBG_MAP_ALLOC
		#include <stdlib.h>
		#include <crtdbg.h>
	#endif
#elif defined __linux__
	#define TLS __thread
#else
	#error Platform not supported
#endif

#ifdef _MSC_VER
	#define rp_strtok strtok_s
#elif __GNUC__
	#define _GNU_SOURCE        // enable GNU extensions to get *_r functions
	#define rp_strtok strtok_r
	#define rp_strerror strerror_r
#else
	#error rp_strtok not defined for platform.
#endif

#include "OpenRP1210/Core.h"
#include "OpenRP1210/common/Error.h"

#endif
