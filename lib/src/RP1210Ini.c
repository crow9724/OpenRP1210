//------------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//------------------------------------------------------------------------------
#include "OpenRP1210/OpenRP1210.h"
#include "OpenRP1210/Common.h"
#include "OpenRP1210/platform/Platform.h"
#include "OpenRP1210/util/Ini.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>

#if defined _WIN32 || defined _WIN64
	#define RP1210_HOME_DIR SD_OSHOME
	#define RP1210_HOME_SUBDIR ""
	#define RP1210_ININAME "RP121032.ini"
	#define RP1210_DRIVER_EXT "dll"
	#define RP1210_IMPL_SUBDIR(x) ""
	#define RP1210_IMPL_SUBDIR_PATH_SEP ""
	#define RP1210_DRIVER_SUBDIR ""
#elif __linux__
	#define RP1210_HOME_DIR SD_USERHOME
	#define RP1210_HOME_SUBDIR "rp1210/"
	#define RP1210_ININAME "rp121032.ini"
	#define RP1210_DRIVER_EXT "so"
	#define RP1210_IMPL_SUBDIR(x) ""
	#define RP1210_IMPL_SUBDIR_PATH_SEP "/"
	#define RP1210_DRIVER_SUBDIR "so/"
#else
	#error "RP1210 constants not defined for platform."
#endif

#define MAX_RP1210_SECTION_NAME 50 // standards says name must be Device/ProtocolInformationXXXX, where X = device number

#define READ_INI_VI_FIELD(ini, vi, name) \
	vi->name = rp_ReadRP1210IniKey(ini, "VendorInformation", #name)

#define DEL_INI_FIELD(field, name) \
	if(field->name) \
		rp_free(field->name)

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
typedef struct S_ProtocolDeviceMap_t
{
	S_RP1210ProtocolInformation *Protocol;
	unsigned int ProtocolId;
	unsigned int *DeviceIds;
	unsigned int NumDevices;
}S_ProtocolDeviceMap;

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
typedef struct S_RP1210ApiImpl_t
{
	char *Name;
	char *DriverPath;
	S_RP1210VendorInformation VendorInformation;
	S_RP1210DeviceInformation **Devices;
	S_RP1210ProtocolInformation **Protocols;

	S_ProtocolDeviceMap **ProtocolDeviceMap;

	unsigned short NumDevices;
	unsigned short NumProtocols;
}S_RP1210ApiImpl;

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
typedef struct S_RP1210ApiImpls_t
{
	S_RP1210ApiImpl **Impls;
	unsigned short NumImpls;

	S_RP1210ImplLoadErr **LoadErrors;
	unsigned short NumLoadErrors;
}S_RP1210ApiImpls;

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
typedef void (*EnumIniSectionCallback)(ORP_HANDLE hIni, S_RP1210ApiImpl *impl, unsigned int sectionIndex, const char *iniSectionName);

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
long rp_strtol(const char *str)
{
	rp_ClearLastError();

	char *e = NULL;
	long l = strtol(str, &e, 0);

	if(l == 0)
	{
		if(errno != 0)
			rp_SetLastError(ORP_ERR_BAD_ARG, NULL);
	}
	else if(e == NULL)
		rp_SetLastError(ORP_ERR_BAD_ARG, NULL);

	return l;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rp_GetRp1210IniPath(char **buf, const char *append, ...)
{
	rp_SetLastError(ORP_ERR_NO_ERROR, NULL);
	va_list args;
	va_start(args, append);

	unsigned int minLength = rp_GetSpecialDir(RP1210_HOME_DIR, NULL, 0);

	if(!ORP_IS_ERR(rpGetLastError()))
	{
		int extraLen = (append ? vsnprintf(NULL, 0, append, args) : 0);
		if(extraLen < 0)  // vsnprintf can return < 0 on error
			rp_SetLastError(ORP_ERR_BAD_ARG, NULL);
		else
		{
			unsigned int fullLength = minLength + extraLen + 1;
			*buf = rp_malloc(fullLength);

			if(*buf)
			{
				minLength = rp_GetSpecialDir(RP1210_HOME_DIR, *buf, minLength);
				if(!ORP_IS_ERR(rpGetLastError()))
				{
					(*buf)[minLength - 1] = rp_PathSeparator();

					if(append)
					{
						va_end(args);
						va_start(args, append);
						vsnprintf((*buf) + minLength, fullLength, append, args);
					}
					else
						(*buf)[minLength] = 0;
				}
				else
					rp_free(*buf);
			}
		}
	}

	va_end(args);
	return rpGetLastError();
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rp_GetRp1210DriverPath(bool absPath, char **buf, const char *append, ...)
{
	rp_SetLastError(ORP_ERR_NO_ERROR, NULL);
	va_list args;
	va_start(args, append);

	unsigned int absPathLen = 0; // length of root of path if absPath requried
	if(absPath)
		absPathLen = rp_GetSpecialDir(RP1210_HOME_DIR, NULL, 0);
	
	if(!ORP_IS_ERR(rpGetLastError()))
	{
		int extraLen = (append ? vsnprintf(NULL, 0, append, args) : 0) + absPathLen;
		if(extraLen < 0)  // vsnprintf can return < 0 on error
			rp_SetLastError(ORP_ERR_BAD_ARG, NULL);
		else
		{
			unsigned int fullLength = extraLen + 1;
			*buf = rp_malloc(fullLength);

			if(*buf)
			{
				if(absPath)
					rp_GetSpecialDir(RP1210_HOME_DIR, *buf, absPathLen);

				if(!ORP_IS_ERR(rpGetLastError()))
				{						
					if(append)
					{
						if(absPathLen > 1)
							(*buf)[absPathLen - 1] = rp_PathSeparator();

						va_end(args);
						va_start(args, append);
						vsnprintf((*buf) + absPathLen, fullLength - absPathLen, append, args);
					}
					else
						(*buf)[absPathLen] = 0;
				}
			}
		}
	}

	va_end(args);
	return rpGetLastError();
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
static inline void rp_InitApiImpls(S_RP1210ApiImpls *impls, S_RP1210ApiImpl **impl, unsigned short numImpls, S_RP1210ImplLoadErr **loadErrors, unsigned short numLoadErrors)
{
	impls->Impls = impl;
	impls->NumImpls = numImpls;
	impls->LoadErrors = loadErrors;
	impls->NumLoadErrors = numLoadErrors;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rp_ReadRP1210Ini(char **implsStr)
{
	rp_SetLastError(ORP_ERR_NO_ERROR, NULL);

	char *rp1210IniPath = NULL;
	ORP_ERR r = rp_GetRp1210IniPath(&rp1210IniPath, "%s%s", RP1210_HOME_SUBDIR, RP1210_ININAME);

	if(!ORP_IS_ERR(r))
	{
		ORP_HANDLE rp1210Ini = rp_IniOpen(rp1210IniPath, NULL);
		if(rp1210Ini)
		{
			size_t len = 0;
			if((r = rp_IniReadKeyLength(rp1210Ini, "RP1210Support", "APIImplementations", &len)) == ORP_ERR_NO_ERROR)
			{
				*implsStr = rp_malloc(len + 1);
				if(*implsStr)
				{
					if((r = rp_IniReadKey(rp1210Ini, "RP1210Support", "APIImplementations", *implsStr, len + 1)) != ORP_ERR_NO_ERROR)
						rp_free(*implsStr);
				}
				else
					r = rp_SetLastError(ORP_ERR_MEM_ALLOC, NULL);
			}

			rpFreeHandle(rp1210Ini);
		}
		else
			r = rpGetLastError();

		rp_free(rp1210IniPath);
	}
	else
		rp_SetLastError(r, " Failed to find %s", RP1210_ININAME);

	return r;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
unsigned short rp_GetRP1210ImplMax(const char *rp1210ImplStr, const char *delim)
{
	unsigned short maxImpls = 0;
	size_t iniImplsLen = strlen(rp1210ImplStr);

	if(iniImplsLen > 0) // else: empty string, no impl, assumes INI configured to trim white space
	{
		maxImpls++; // at least one potential impl

		if(strchr(rp1210ImplStr, delim[0]) != NULL)
		{
			for(unsigned int i = 0; i < iniImplsLen; i++)
				if(rp1210ImplStr[i] == delim[0] && maxImpls < USHRT_MAX)
					maxImpls++;
		}
	}

	return maxImpls;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
char *rp_ReadRP1210IniKey(ORP_HANDLE ini, const char *section, const char *key)
{
	char *v = NULL;
	size_t keyLength = 0;

	if(rp_IniReadKeyLength(ini, section, key, &keyLength) == ORP_ERR_NO_ERROR)
	{
		v = rp_malloc(keyLength + 1);
		if(v)
		{
			if(rp_IniReadKey(ini, section, key, v, keyLength + 1) != ORP_ERR_NO_ERROR)
			{
				rp_free(v);
				v = NULL;
			}
		}
	}

	return v;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_DestroyVendorInformation(S_RP1210VendorInformation *vi)
{
	DEL_INI_FIELD(vi, Name);
	DEL_INI_FIELD(vi, Address1);
	DEL_INI_FIELD(vi, Address2);
	DEL_INI_FIELD(vi, City);
	DEL_INI_FIELD(vi, State);
	DEL_INI_FIELD(vi, Country);
	DEL_INI_FIELD(vi, Postal);
	DEL_INI_FIELD(vi, Telephone);
	DEL_INI_FIELD(vi, Fax);
	DEL_INI_FIELD(vi, VendorURL);
	DEL_INI_FIELD(vi, MessageString);
	DEL_INI_FIELD(vi, ErrorString);
	DEL_INI_FIELD(vi, TimestampWeight);
	DEL_INI_FIELD(vi, Devices);
	DEL_INI_FIELD(vi, Protocols);

	#if RP1210_VERSION >= RP1210_VERSION_B
		DEL_INI_FIELD(vi, AutoDetectCapable);
		DEL_INI_FIELD(vi, Version);
		DEL_INI_FIELD(vi, RP1210);
		DEL_INI_FIELD(vi, DebugLevel);
		DEL_INI_FIELD(vi, DebugFile);
		DEL_INI_FIELD(vi, DebugMode);
		DEL_INI_FIELD(vi, DebugFileSize);
		DEL_INI_FIELD(vi, NumberOfRTSCTSSessions);
	#endif
	#if RP1210_VERSION >= RP1210_VERSION_C
		DEL_INI_FIELD(vi, CANFormatsSupported);
		DEL_INI_FIELD(vi, J1939FormatsSupported);
		DEL_INI_FIELD(vi, J1939Addresses);
		DEL_INI_FIELD(vi, CANAutoBaud);
		DEL_INI_FIELD(vi, J1708FormatsSupported);
		DEL_INI_FIELD(vi, ISO15765FormatsSupported);
	#endif
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_DestroyDeviceInformation(S_RP1210DeviceInformation *di)
{
	DEL_INI_FIELD(di, DeviceID);
	DEL_INI_FIELD(di, DeviceDescription);
	DEL_INI_FIELD(di, DeviceName);
	DEL_INI_FIELD(di, DeviceParams);
	DEL_INI_FIELD(di, MultiCANChannels);
	DEL_INI_FIELD(di, MultiJ1939Channels);
	DEL_INI_FIELD(di, MultiISO15765Channels);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_DestroyProtocolInformation(S_RP1210ProtocolInformation *pi)
{
	DEL_INI_FIELD(pi, ProtocolDescription);
	DEL_INI_FIELD(pi, ProtocolSpeed);
	DEL_INI_FIELD(pi, ProtocolString);
	DEL_INI_FIELD(pi, ProtocolParams);
	DEL_INI_FIELD(pi, Devices);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_DestroyProtocolDeviceMap(S_ProtocolDeviceMap *dm)
{
	if(dm->DeviceIds)
		rp_free(dm->DeviceIds);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_DestroyApiImpl(S_RP1210ApiImpl *impl)
{
	rp_DestroyVendorInformation(&impl->VendorInformation);

	for(unsigned int i = 0; i < impl->NumDevices; i++)
	{
		rp_DestroyDeviceInformation(impl->Devices[i]);
		rp_free(impl->Devices[i]);
	}

	for(unsigned int i = 0; i < impl->NumProtocols; i++)
	{
		rp_DestroyProtocolInformation(impl->Protocols[i]);
		rp_free(impl->Protocols[i]);

		rp_DestroyProtocolDeviceMap(impl->ProtocolDeviceMap[i]);
		rp_free(impl->ProtocolDeviceMap[i]);
	}

	if(impl->Name)
		rp_free(impl->Name);
	if(impl->DriverPath)
		rp_free(impl->DriverPath);
	if(impl->ProtocolDeviceMap)
		rp_free(impl->ProtocolDeviceMap);
	if(impl->Devices)
		rp_free(impl->Devices);
	if(impl->Protocols)
		rp_free(impl->Protocols);

	rp_free(impl);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_DestroyLoaderError(S_RP1210ImplLoadErr *e)
{
	if(e->Description)
		rp_free(e->Description);
	if(e->ImplName)
		rp_free(e->ImplName);

	rp_free(e);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_ReadVendorInformation(ORP_HANDLE ini, S_RP1210VendorInformation *vi)
{
	memset(vi, 0, sizeof(S_RP1210VendorInformation));
	
	READ_INI_VI_FIELD(ini, vi, Name);
	READ_INI_VI_FIELD(ini, vi, Address1);
	READ_INI_VI_FIELD(ini, vi, City);
	READ_INI_VI_FIELD(ini, vi, Country);
	READ_INI_VI_FIELD(ini, vi, Postal);
	READ_INI_VI_FIELD(ini, vi, Telephone);
	READ_INI_VI_FIELD(ini, vi, Fax);
	READ_INI_VI_FIELD(ini, vi, VendorURL);
	READ_INI_VI_FIELD(ini, vi, MessageString);
	READ_INI_VI_FIELD(ini, vi, ErrorString);
	READ_INI_VI_FIELD(ini, vi, TimestampWeight);
	READ_INI_VI_FIELD(ini, vi, Devices);
	READ_INI_VI_FIELD(ini, vi, Protocols);

	#if RP1210_VERSION >= RP1210_VERSION_B
		READ_INI_VI_FIELD(ini, vi, AutoDetectCapable);
		READ_INI_VI_FIELD(ini, vi, Version);
		READ_INI_VI_FIELD(ini, vi, RP1210);
		READ_INI_VI_FIELD(ini, vi, DebugLevel);
		READ_INI_VI_FIELD(ini, vi, DebugFile);
		READ_INI_VI_FIELD(ini, vi, DebugMode);
		READ_INI_VI_FIELD(ini, vi, DebugFileSize);
		READ_INI_VI_FIELD(ini, vi, NumberOfRTSCTSSessions);
	#endif
	#if RP1210_VERSION >= RP1210_VERSION_C
		READ_INI_VI_FIELD(ini, vi, CANFormatsSupported);
		READ_INI_VI_FIELD(ini, vi, J1939FormatsSupported);
		READ_INI_VI_FIELD(ini, vi, J1939Addresses);
		READ_INI_VI_FIELD(ini, vi, CANAutoBaud);
		READ_INI_VI_FIELD(ini, vi, J1708FormatsSupported);
		READ_INI_VI_FIELD(ini, vi, ISO15765FormatsSupported);
	#endif
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_EnumIniSections(ORP_HANDLE hIni, S_RP1210ApiImpl *impl, const char *sections, const char *sectionDelim, const char *sectionPrefix, EnumIniSectionCallback callback)
{
	char fullSectionName[MAX_RP1210_SECTION_NAME];
	unsigned short sectionIndex = 0;

	char *tokContext = NULL;
	char *tokPtr = rp_malloc(strlen(sections) + 1);
	char *tok = tokPtr;
	if(tok)
	{
		strcpy(tok, sections);
		tok = rp_strtok(tok, sectionDelim, &tokContext);
	}

	if(tok)
	{
		do
		{
			long devId = rp_strtol(tok);
			if(rpGetLastError() != ORP_ERR_NO_ERROR)
				continue;
			else
			{
				if(devId >= 0)
				{
					if(snprintf(fullSectionName, MAX_RP1210_SECTION_NAME, "%s%ld", sectionPrefix, devId) > 0)
					{
						if(rp_IniHasSection(hIni, fullSectionName) && callback)
							callback(hIni, impl, sectionIndex++, fullSectionName);
					}
					else
						continue;
				}
				else
					continue;
			}


		} while((tok = rp_strtok(NULL, sectionDelim, &tokContext)));
	}

	rp_free(tokPtr);

	if(rpGetLastError() == ORP_ERR_INI_KEYNOTFOUND)
		rp_ClearLastError();
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_EnumValidDeviceCallback(ORP_HANDLE hIni, S_RP1210ApiImpl *impl, unsigned int sectionIndex, const char *iniSectionName)
{
	impl->NumDevices++;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_EnumCreateDeviceCallback(ORP_HANDLE hIni, S_RP1210ApiImpl *impl, unsigned int sectionIndex, const char *iniSectionName)
{
	S_RP1210DeviceInformation *devInfo = rp_malloc(sizeof(S_RP1210DeviceInformation));
	
	if(devInfo)
	{
		devInfo->DeviceDescription = rp_ReadRP1210IniKey(hIni, iniSectionName, "DeviceDescription");
		devInfo->DeviceID = rp_ReadRP1210IniKey(hIni, iniSectionName, "DeviceId");
		devInfo->DeviceName = rp_ReadRP1210IniKey(hIni, iniSectionName, "DeviceName");
		devInfo->DeviceParams = rp_ReadRP1210IniKey(hIni, iniSectionName, "DeviceParams");

		devInfo->MultiCANChannels = rp_ReadRP1210IniKey(hIni, iniSectionName, "MultiCANChannels");
		devInfo->MultiISO15765Channels = rp_ReadRP1210IniKey(hIni, iniSectionName, "MultiISO15765Channels");
		devInfo->MultiJ1939Channels = rp_ReadRP1210IniKey(hIni, iniSectionName, "MultiJ1939Channels");

		impl->Devices[sectionIndex] = devInfo;
	}
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_EnumValidProtocolCallback(ORP_HANDLE hIni, S_RP1210ApiImpl *impl, unsigned int sectionIndex, const char *iniSectionName)
{
	impl->NumProtocols++;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_EnumCreateProtocolCallback(ORP_HANDLE hIni, S_RP1210ApiImpl *impl, unsigned int sectionIndex, const char *iniSectionName)
{
	if(sectionIndex < impl->NumProtocols)
	{
		S_RP1210ProtocolInformation *protoInfo = rp_malloc(sizeof(S_RP1210ProtocolInformation));

		if(protoInfo)
		{
			protoInfo->ProtocolDescription = rp_ReadRP1210IniKey(hIni, iniSectionName, "ProtocolDescription");
			protoInfo->ProtocolSpeed = rp_ReadRP1210IniKey(hIni, iniSectionName, "ProtocolSpeed");
			protoInfo->ProtocolString = rp_ReadRP1210IniKey(hIni, iniSectionName, "ProtocolString");
			protoInfo->ProtocolParams = rp_ReadRP1210IniKey(hIni, iniSectionName, "ProtocolParams");
			protoInfo->Devices = rp_ReadRP1210IniKey(hIni, iniSectionName, "Devices");

			impl->Protocols[sectionIndex] = protoInfo;

			impl->ProtocolDeviceMap[sectionIndex] = rp_malloc(sizeof(S_ProtocolDeviceMap));
			if(impl->ProtocolDeviceMap[sectionIndex])
			{
				impl->ProtocolDeviceMap[sectionIndex]->ProtocolId = sectionIndex;
				impl->ProtocolDeviceMap[sectionIndex]->Protocol = protoInfo;
				impl->ProtocolDeviceMap[sectionIndex]->NumDevices = 0;
				impl->ProtocolDeviceMap[sectionIndex]->DeviceIds = NULL;

				if(protoInfo->Devices)
				{
					const char *devDelim = ",";
					unsigned short devCount = 0;
					impl->ProtocolDeviceMap[sectionIndex]->DeviceIds = rp_malloc(impl->NumDevices * sizeof(unsigned int));

					if(!impl->ProtocolDeviceMap[sectionIndex]->DeviceIds)
						rp_SetLastError(ORP_ERR_MEM_ALLOC, NULL);
					else
					{
						char *tokContext = NULL;
						char *tokPtr = rp_malloc(strlen(protoInfo->Devices) + 1);
						char *tok = tokPtr;

						if(tok)
						{
							strcpy(tok, protoInfo->Devices);
							tok = rp_strtok(tok, devDelim, &tokContext);
						}
						else
							rp_SetLastError(ORP_ERR_MEM_ALLOC, "");

						if(tok)
						{
							do
							{
								long devId = rp_strtol(tok);
								if(rpGetLastError() != ORP_ERR_NO_ERROR)
									continue;
								else
								{
									if(devId >= 0 && devId <= UINT32_MAX && devCount < impl->NumDevices)
										impl->ProtocolDeviceMap[sectionIndex]->DeviceIds[devCount++] = (int)devId;
									else
										continue;
								}

							} while((tok = rp_strtok(NULL, devDelim, &tokContext)));
						}

						rp_free(tokPtr);
					}

					impl->ProtocolDeviceMap[sectionIndex]->NumDevices = devCount;
				}
			}
		}
	}
	else
		rp_SetLastError(ORP_ERR_BAD_RANGE, NULL);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_ReadDevices(ORP_HANDLE hIni, S_RP1210ApiImpl *impl)
{
	const char *devDelim = ",";
	char *devNamePrefix = "DeviceInformation";

	rp_EnumIniSections(hIni, impl, impl->VendorInformation.Devices, devDelim, devNamePrefix, rp_EnumValidDeviceCallback);

	if(rpGetLastError() == ORP_ERR_NO_ERROR)
	{
		impl->Devices = rp_malloc(impl->NumDevices * sizeof(S_RP1210DeviceInformation *));

		if(impl->Devices)
		{
			memset(impl->Devices, 0, impl->NumDevices * sizeof(S_RP1210DeviceInformation *));
			rp_EnumIniSections(hIni, impl, impl->VendorInformation.Devices, devDelim, devNamePrefix, rp_EnumCreateDeviceCallback);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_ReadProtocols(ORP_HANDLE hIni, S_RP1210ApiImpl *impl)
{
	const char *devDelim = ",";
	char *devNamePrefix = "ProtocolInformation";

	rp_EnumIniSections(hIni, impl, impl->VendorInformation.Protocols, devDelim, devNamePrefix, rp_EnumValidProtocolCallback);

	if(rpGetLastError() == ORP_ERR_NO_ERROR)
	{
		impl->ProtocolDeviceMap = rp_malloc(impl->NumProtocols * sizeof(S_ProtocolDeviceMap *));

		if(impl->ProtocolDeviceMap)
		{
			memset(impl->ProtocolDeviceMap, 0, impl->NumProtocols * sizeof(S_ProtocolDeviceMap *));
			impl->Protocols = rp_malloc(impl->NumProtocols * sizeof(S_RP1210ProtocolInformation *));

			if(impl->Protocols)
			{
				memset(impl->Protocols, 0, impl->NumProtocols * sizeof(S_RP1210ProtocolInformation *));
				rp_EnumIniSections(hIni, impl, impl->VendorInformation.Protocols, devDelim, devNamePrefix, rp_EnumCreateProtocolCallback);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_RP1210ApiImpl *rp_ReadRP1210ImplIni(const char *implName)
{
	S_RP1210ApiImpl *impl = NULL;
	char *implPath = NULL;
	ORP_ERR r = rp_GetRp1210IniPath(&implPath, "%s%s%s", RP1210_HOME_SUBDIR, implName, ".ini");

	if(!ORP_IS_ERR(r))
	{
		ORP_HANDLE hIni = rp_IniOpen(implPath, NULL);
		if(hIni)
		{
			impl = rp_mallocZ(sizeof(S_RP1210ApiImpl));
			if(impl)
			{
				impl->Name = rp_mallocZ(strlen(implName) + 1);
				if(impl->Name)
				{
					strcpy(impl->Name, implName);
					rp_GetRp1210DriverPath(true, &impl->DriverPath, "%s%s%s.%s", RP1210_HOME_SUBDIR, RP1210_DRIVER_SUBDIR, implName, RP1210_DRIVER_EXT);
				}
			}

			if(rpGetLastError() != ORP_ERR_NO_ERROR)
			{
				rp_DestroyApiImpl(impl);
				impl = NULL;
			}
			else if(impl)
			{
				rp_ReadVendorInformation(hIni, &impl->VendorInformation);
				rp_ReadDevices(hIni, impl);
				rp_ReadProtocols(hIni, impl);
			}

			rpFreeHandle(hIni);
		}

		rp_free(implPath);
	}

	return impl;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_RP1210ApiImpl *rp_CreateRP1210Impl(const char *implName, S_RP1210ImplLoadErr **loadError)
{
	S_RP1210ApiImpl *impl = rp_ReadRP1210ImplIni(implName);

	if(!impl && loadError)
	{
		// save error generated in rp_ReadRP1210ImplIni
		// if unable to save it, retain last error in rpGetLastError from rp_ReadRP1210ImplIni
		// this means errors below will be swallowed (only failures below are malloc fails anyway)
		S_RP1210ImplLoadErr *le = rp_malloc(sizeof(S_RP1210ImplLoadErr));
		if(le)
		{
			memset(le, 0, sizeof(S_RP1210ImplLoadErr));
			const char *e = rpGetLastErrorDesc();

			le->Description = rp_malloc(strlen(e) + 1);
			if(le->Description)
			{
				le->ImplName = rp_malloc(strlen(implName) + 1);
				if(le->ImplName)
				{
					le->Error = rpGetLastError();

					strcpy(le->Description, e);
					strcpy(le->ImplName, implName);

					rp_SetLastError(ORP_ERR_NO_ERROR, NULL);
				}
			}

			if(rpGetLastError() != ORP_ERR_NO_ERROR)
				rp_DestroyLoaderError(le);
			else
				*loadError = le;
		}
	}

	return impl;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rp_FreeApiImpls(ORP_HANDLE hImpls)
{
	S_RP1210ApiImpls *impls = rp_HandleToTarget(hImpls);

	if(impls->Impls)
		for(int i = 0; i < impls->NumImpls; i++)
			rp_DestroyApiImpl(impls->Impls[i]);

	if(impls->LoadErrors)
		for(int i = 0; i < impls->NumLoadErrors; i++)
			rp_DestroyLoaderError(impls->LoadErrors[i]);

	rp_free(impls->LoadErrors);
	rp_free(impls->Impls);

	memset(impls, 0, sizeof(S_RP1210ApiImpls));
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_HANDLE rpGetApiImpls(void)
{
	rp_ClearLastError();

	S_RP1210ApiImpls impls;
	memset(&impls, 0, sizeof(impls));

	char *rp1210IniImpls = NULL;
	ORP_ERR r = rp_ReadRP1210Ini(&rp1210IniImpls);

	if(!ORP_IS_ERR(r))
	{
		const char delim[] = ",";
		unsigned short numImpls = 0;
		unsigned short numLoadErrs = 0;
		unsigned short maxImpls = rp_GetRP1210ImplMax(rp1210IniImpls, delim);

		if(maxImpls > 0)
		{
			S_RP1210ApiImpl **localImpls = rp_malloc(maxImpls * sizeof(S_RP1210ApiImpl *));
			S_RP1210ImplLoadErr **loadErrors = rp_malloc(maxImpls * sizeof(S_RP1210ImplLoadErr *));
			if(localImpls && loadErrors)
			{
				char *tok = rp1210IniImpls;
				char *tokContext = NULL;

				tok = rp_strtok(tok, delim, &tokContext);
				while(tok != NULL)
				{
					assert(numImpls < maxImpls && numLoadErrs < maxImpls); // shouldn't happen, but still....

					S_RP1210ApiImpl *impl = rp_CreateRP1210Impl(tok, &loadErrors[numLoadErrs]);
					if(impl)
						localImpls[numImpls++] = impl;
					else if(loadErrors[numLoadErrs])
						numLoadErrs++;

					tok = rp_strtok(NULL, delim, &tokContext);
				}
			}
			else
				r = rp_SetLastError(ORP_ERR_MEM_ALLOC, NULL);

			if(!ORP_IS_ERR(r))
				rp_InitApiImpls(&impls, localImpls, numImpls, loadErrors, numLoadErrs);
			else
			{
				if(localImpls)
					for(int i = 0; i < numImpls; i++)
						rp_DestroyApiImpl(localImpls[i]);

				if(loadErrors)
					for(int i = 0; i < numImpls; i++)
						rp_DestroyLoaderError(loadErrors[i]);

				rp_free(localImpls);
				rp_free(loadErrors);
			}
		}
		else
			rp_InitApiImpls(&impls, NULL, 0, NULL, 0);

		rp_free(rp1210IniImpls);
	}

	return r == ORP_ERR_NO_ERROR ? rp_CopyToHandle(&impls, sizeof(impls), rp_FreeApiImpls) : NULL;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
unsigned int rpGetNumApiImpls(ORP_HANDLE hImpls)
{
	assert(hImpls != NULL);

	rp_ClearLastError();
	return ((S_RP1210ApiImpls *)rp_HandleToTarget(hImpls))->NumImpls;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
unsigned int rpGetNumApiImplLoadErrors(ORP_HANDLE hImpls)
{
	assert(hImpls != NULL);

	rp_ClearLastError();
	return ((S_RP1210ApiImpls *)rp_HandleToTarget(hImpls))->NumLoadErrors;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_RP1210ImplLoadErr *rpGetApiImplLoadError(ORP_HANDLE hImpls, unsigned int index)
{
	assert(hImpls != NULL);

	rp_ClearLastError();

	S_RP1210ImplLoadErr *le = NULL;
	S_RP1210ApiImpls *impls = rp_HandleToTarget(hImpls);

	if(index < impls->NumLoadErrors)
		le = impls->LoadErrors[index];
	else
		rp_SetLastError(ORP_ERR_BAD_ARG, NULL);

	return le;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_HANDLE rpGetApiImpl(ORP_HANDLE hImpls, unsigned int index)
{
	assert(hImpls != NULL);

	rp_ClearLastError();

	ORP_HANDLE hImpl = NULL;
	S_RP1210ApiImpls *impls = rp_HandleToTarget(hImpls);

	if(index < impls->NumImpls)
		hImpl = rp_CreateHandle(impls->Impls[index], NULL);
	else
		rp_SetLastError(ORP_ERR_BAD_ARG, NULL);

	return hImpl;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_HANDLE rpGetApiImplByName(ORP_HANDLE hImpls, const char *name)
{
	assert(hImpls != NULL);
	assert(name != NULL);

	rp_ClearLastError();

	ORP_HANDLE hImpl = NULL;
	S_RP1210ApiImpls *impls = rp_HandleToTarget(hImpls);

	for(int i = 0; i < impls->NumImpls; i++)
	{
		if(strcmp(name, impls->Impls[i]->Name) == 0)
		{
			hImpl = rp_CreateHandle(impls->Impls[i], NULL);
			break;
		}
	}

	return hImpl;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
const char *rpGetApiImplName(ORP_HANDLE hImpl)
{
	assert(hImpl != NULL);

	rp_ClearLastError();
	return ((S_RP1210ApiImpl *)rp_HandleToTarget(hImpl))->Name;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
const char *rpGetDriverPath(ORP_HANDLE hImpl)
{
	assert(hImpl != NULL);

	rp_ClearLastError();
	return ((S_RP1210ApiImpl *)rp_HandleToTarget(hImpl))->DriverPath;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_RP1210VendorInformation *rpGetVendorInfo(ORP_HANDLE hImpl)
{
	assert(hImpl != NULL);

	rp_ClearLastError();
	return &((S_RP1210ApiImpl *)rp_HandleToTarget(hImpl))->VendorInformation;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
unsigned int rpGetNumDevices(ORP_HANDLE hImpl)
{
	assert(hImpl != NULL);

	rp_ClearLastError();
	return ((S_RP1210ApiImpl *)rp_HandleToTarget(hImpl))->NumDevices;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_RP1210DeviceInformation *rpGetDeviceInfo(ORP_HANDLE hImpl, unsigned int index)
{
	assert(hImpl != NULL);

	rp_ClearLastError();
	S_RP1210DeviceInformation *devInfo = NULL;
	S_RP1210ApiImpl *impl = rp_HandleToTarget(hImpl);

	if(index < impl->NumDevices)
		devInfo = impl->Devices[index];
	else
		rp_SetLastError(ORP_ERR_BAD_ARG, NULL);

	return devInfo;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_RP1210DeviceInformation *rpGetDeviceInfoByName(ORP_HANDLE hImpl, const char *name, unsigned char deviceIndex)
{
	assert(hImpl != NULL);
	assert(name != NULL);

	rp_ClearLastError();
	S_RP1210DeviceInformation *devInfo = NULL;
	S_RP1210ApiImpl *impl = rp_HandleToTarget(hImpl);

	unsigned char devCount = 0;
	for(unsigned int i = 0; i < impl->NumDevices; i++)
	{
		if(strcmp(impl->Devices[i]->DeviceName, name) == 0)
		{
			if(devCount == deviceIndex)
			{
				devInfo = impl->Devices[i];
				break;
			}
			else
				devCount++;
		}
	}

	return devInfo;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_RP1210DeviceInformation *rpGetDeviceInfoById(ORP_HANDLE hImpl, unsigned int deviceId)
{
	assert(hImpl != NULL);

	rp_ClearLastError();
	S_RP1210DeviceInformation *devInfo = NULL;
	S_RP1210ApiImpl *impl = rp_HandleToTarget(hImpl);

	for(unsigned int i = 0; i < impl->NumDevices; i++)
	{
		unsigned int devId = rp_strtol(impl->Devices[i]->DeviceID);
		if(rpGetLastError() == ORP_ERR_NO_ERROR && devId == deviceId)
		{
			devInfo = impl->Devices[i];
			break;
		}
	}

	return devInfo;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
int rpGetDeviceId(S_RP1210DeviceInformation *deviceInfo)
{
	assert(deviceInfo != NULL);
	ORP_ERR r = ORP_ERR_NO_ERROR;

	if(deviceInfo->DeviceID == NULL)
		return ORP_ERR_BAD_ARG;
	else
	{
		long devId = rp_strtol(deviceInfo->DeviceID);
		r = rpGetLastError();

		return r == ORP_ERR_NO_ERROR ? devId : r;
	}
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rpGetDevicesByProtocol(ORP_HANDLE hImpl, const char *protocol, S_RP1210DeviceInformation **devices, unsigned int *numDevices)
{
	assert(hImpl != NULL);
	assert(protocol != NULL);

	ORP_ERR r = rp_ClearLastError();
	S_RP1210ApiImpl *impl = rp_HandleToTarget(hImpl);

	unsigned int actualNumDevices = 0;
	unsigned int protocolId = 0;
	bool foundProto = false;

	for(unsigned int i = 0; i < impl->NumProtocols; i++)
	{
		if(strcmp(impl->ProtocolDeviceMap[i]->Protocol->ProtocolString, protocol) == 0)
		{
			protocolId = impl->ProtocolDeviceMap[i]->ProtocolId;
			foundProto = true;
			break;
		}
	}

	if(foundProto)
	{
		for(unsigned int i = 0; i < impl->ProtocolDeviceMap[protocolId]->NumDevices; i++)
		{
			S_RP1210DeviceInformation *dev = rpGetDeviceInfoById(hImpl, impl->ProtocolDeviceMap[protocolId]->DeviceIds[i]);
			if(dev)
			{
				if(devices && actualNumDevices < *numDevices)
					devices[actualNumDevices++] = dev;
				else if(!devices)
					actualNumDevices++;
			}
		}
	}

	if(numDevices)
		*numDevices = actualNumDevices;

	return r;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
unsigned int rpGetNumProtocols(ORP_HANDLE hImpl)
{
	assert(hImpl != NULL);

	rp_ClearLastError();
	return ((S_RP1210ApiImpl *)rp_HandleToTarget(hImpl))->NumProtocols;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_RP1210ProtocolInformation *rpGetProtocolInfo(ORP_HANDLE hImpl, unsigned int index)
{
	assert(hImpl != NULL);

	rp_ClearLastError();
	S_RP1210ProtocolInformation *protoInfo = NULL;
	S_RP1210ApiImpl *impl = rp_HandleToTarget(hImpl);

	if(index < impl->NumProtocols)
		protoInfo = impl->Protocols[index];
	else
		rp_SetLastError(ORP_ERR_BAD_ARG, NULL);

	return protoInfo;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_RP1210ProtocolInformation *rpGetProtocolInfoByName(ORP_HANDLE hImpl, const char *name)
{
	assert(hImpl != NULL);
	assert(name != NULL);

	rp_ClearLastError();
	S_RP1210ProtocolInformation *protoInfo = NULL;
	S_RP1210ApiImpl *impl = rp_HandleToTarget(hImpl);

	for(unsigned int i = 0; i < impl->NumProtocols; i++)
	{
		if(strcmp(impl->Protocols[i]->ProtocolString, name) == 0)
		{
			protoInfo = impl->Protocols[i];
			break;
		}
	}

	return protoInfo;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
S_RP1210ProtocolInformation *rpGetProtocolInfoById(ORP_HANDLE hImpl, unsigned int id)
{
	assert(hImpl != NULL);

	rp_ClearLastError();
	S_RP1210ProtocolInformation *protoInfo = NULL;
	S_RP1210ApiImpl *impl = rp_HandleToTarget(hImpl);

	for(unsigned int i = 0; i < impl->NumProtocols; i++)
	{
		if(impl->ProtocolDeviceMap[i]->ProtocolId == id)
		{
			protoInfo = impl->ProtocolDeviceMap[i]->Protocol;
			break;
		}
	}

	return protoInfo;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
ORP_ERR rpGetProtocolInfoByDevice(ORP_HANDLE hImpl, S_RP1210DeviceInformation *deviceInfo, S_RP1210ProtocolInformation **protocols, unsigned int *numProtocols)
{
	assert(hImpl != NULL);
	assert(deviceInfo != NULL);

	ORP_ERR r = rp_ClearLastError();

	S_RP1210ApiImpl *impl = rp_HandleToTarget(hImpl);
	unsigned int actualNumProtocols = 0;

	unsigned int devId = rp_strtol(deviceInfo->DeviceID);
	if(rpGetLastError() == ORP_ERR_NO_ERROR)
	{
		for(unsigned int i = 0; i < impl->NumProtocols; i++)
		{
			for(unsigned int j = 0; j < impl->ProtocolDeviceMap[i]->DeviceIds[j]; j++)
			{
				if(impl->ProtocolDeviceMap[i]->DeviceIds[j] == devId)
				{
					if(protocols && actualNumProtocols < *numProtocols)
						protocols[actualNumProtocols++] = impl->ProtocolDeviceMap[i]->Protocol;
					else if(!protocols)
						actualNumProtocols++;
					break;
				}
			}
		}

		if(numProtocols)
			*numProtocols = actualNumProtocols;
	}
	else
		r = rp_SetLastError(ORP_ERR_BAD_ARG, NULL);

	return r;
}
