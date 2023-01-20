//------------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//------------------------------------------------------------------------------
/// @file
/// @author Chris Anthony
/// @brief Specifies RP1210 specific functionality.
//------------------------------------------------------------------------------
#ifndef OPENRP1210_RP1210_H__
#define OPENRP1210_RP1210_H__

#if defined _WIN32 || defined _WIN64
	#include <Windows.h>
	#define DLLEXPORT OpenRP1210API

	#define RP1210_EXPORT_CLIENTCONNECT       "RP1210_ClientConnect"
	#define RP1210_EXPORT_CLIENTDISCONNECT    "RP1210_ClientDisconnect"
	#define RP1210_EXPORT_GETERRORMSG         "RP1210_GetErrorMsg"
	#define RP1210_EXPORT_GETHARDWARESTATUS   "RP1210_GetHardwareStatus"
	#define RP1210_EXPORT_GETLASTERRORMSG     "RP1210_GetLastErrorMsg"
	#define RP1210_EXPORT_READDETAILEDVERSION "RP1210_ReadDetailedVersion"
	#define RP1210_EXPORT_READMESSAGE         "RP1210_ReadMessage"
	#define RP1210_EXPORT_READVERSION         "RP1210_ReadVersion"
	#define RP1210_EXPORT_SENDCOMMAND         "RP1210_SendCommand"
	#define RP1210_EXPORT_SENDMESSAGE         "RP1210_SendMessage"

	// stdcall definitions
	#define RP1210_EXPORT_CLIENTCONNECT_ALT       "_RP1210_ClientConnect@24"
	#define RP1210_EXPORT_CLIENTDISCONNECT_ALT    "_RP1210_ClientDisconnect@4"
	#define RP1210_EXPORT_GETERRORMSG_ALT         "_RP1210_GetErrorMsg@8"
	#define RP1210_EXPORT_GETHARDWARESTATUS_ALT   "_RP1210_GetHardwareStatus@16"
	#define RP1210_EXPORT_GETLASTERRORMSG_ALT     "_RP1210_GetLastErrorMsg@12"
	#define RP1210_EXPORT_READDETAILEDVERSION_ALT "_RP1210_ReadDetailedVersion@16"
	#define RP1210_EXPORT_READMESSAGE_ALT         "_RP1210_ReadMessage@16"
	#define RP1210_EXPORT_READVERSION_ALT         "_RP1210_ReadVersion@16"
	#define RP1210_EXPORT_SENDCOMMAND_ALT         "_RP1210_SendCommand@16"
	#define RP1210_EXPORT_SENDMESSAGE_ALT         "_RP1210_SendMessage@20"

#elif defined __linux__
	// RP1210 spec defines some functions in terms of Windows types
	#define HWND unsigned long
	#define WINAPI
    #define DLLEXPORT

	#define RP1210_EXPORT_CLIENTCONNECT       "RP1210_ClientConnect"
	#define RP1210_EXPORT_CLIENTDISCONNECT    "RP1210_ClientDisconnect"
	#define RP1210_EXPORT_GETERRORMSG         "RP1210_GetErrorMsg"
	#define RP1210_EXPORT_GETHARDWARESTATUS   "RP1210_GetHardwareStatus"
	#define RP1210_EXPORT_GETLASTERRORMSG     "RP1210_GetLastErrorMsg"
	#define RP1210_EXPORT_READDETAILEDVERSION "RP1210_ReadDetailedVersion"
	#define RP1210_EXPORT_READMESSAGE         "RP1210_ReadMessage"
	#define RP1210_EXPORT_READVERSION         "RP1210_ReadVersion"
	#define RP1210_EXPORT_SENDCOMMAND         "RP1210_SendCommand"
	#define RP1210_EXPORT_SENDMESSAGE         "RP1210_SendMessage"

	#define RP1210_EXPORT_CLIENTCONNECT_ALT       NULL
	#define RP1210_EXPORT_CLIENTDISCONNECT_ALT    NULL
	#define RP1210_EXPORT_GETERRORMSG_ALT         NULL
	#define RP1210_EXPORT_GETHARDWARESTATUS_ALT   NULL
	#define RP1210_EXPORT_GETLASTERRORMSG_ALT     NULL
	#define RP1210_EXPORT_READDETAILEDVERSION_ALT NULL
	#define RP1210_EXPORT_READMESSAGE_ALT         NULL
	#define RP1210_EXPORT_READVERSION_ALT         NULL
	#define RP1210_EXPORT_SENDCOMMAND_ALT         NULL
	#define RP1210_EXPORT_SENDMESSAGE_ALT         NULL
#else
	#error Platform not supported
#endif

#ifdef RP1210_VERSION
	#if RP1210_VERSION == RP1210_VERSION_A
		#define RP1210_VERSION_STRING "RP1210A"
		#include "RP1210A.h"
	#elif RP1210_VERSION == RP1210_VERSION_B
		#define RP1210_VERSION_STRING "RP1210B"
		#include "RP1210B.h"
	#elif RP1210_VERSION == RP1210_VERSION_C
		#define RP1210_VERSION_STRING "RP1210C"
		#include "RP1210C.h"
	#endif
#else
	#error RP1210 Version not defined.
#endif

/////////////////////////////////////////////////////////////////////////////////
/// @brief An RP1210 context encapsulates all information required to call RP1210
///        functions within a vendor driver.
///
/// The context is what actually exposes RP1210 functionality to a client. A
/// context can be obtained directy via rpGetContext. A context can also be loaded
/// globally using rpLoadContext.
/// 
/// After loading a global context, direct calls to RP1210 functions such as
/// RP1210_ClientConnect can be made.
/////////////////////////////////////////////////////////////////////////////////
struct S_RP1210Context
{
	ORP_HANDLE Context;
	short (WINAPI *RP1210_ClientConnect)(HWND, short, const char *, long, long, short);
	short (WINAPI *RP1210_ClientDisconnect)(short);
	short (WINAPI *RP1210_SendMessage)(short, char *, short, short, short);
	short (WINAPI *RP1210_ReadMessage)(short, char *, short, short);
	short (WINAPI *RP1210_SendCommand)(short, short, char *, short);
	short (WINAPI *RP1210_ReadVersion)(char *, char *, char *, char *);
	short (WINAPI *RP1210_GetHardwareStatus)(short, char *, short, short);
	short (WINAPI *RP1210_GetErrorMsg)(short, char *);

	#if RP1210_VERSION >= RP1210_VERSION_B
		short (WINAPI *RP1210_ReadDetailedVersion)(short, char *, char *, char *);
		short (WINAPI *RP1210_GetLastErrorMsg)(short, int *, char *, short);
	#endif
};

#endif
