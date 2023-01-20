//------------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//------------------------------------------------------------------------------
#include "OpenRP1210/OpenRP1210.h"
#include "OpenRP1210/Common.h"
#include "OpenRP1210/platform/Platform.h"
#include <string.h>
#include <assert.h>

typedef struct S_ContextInfo_t
{
    ORP_HANDLE hLibrary;
}S_ContextInfo;

const char *ERR_NO_CONTEXT = "No RP1210 Context. ";
struct S_RP1210Context *g_rp1210Context = NULL;

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void *rp_LoadRP1210Sym(ORP_HANDLE hLib, const char *symbolName, const char *altSymbolName)
{
    void *symbol = rp_GetLibSymbol(hLib, symbolName);
    if(!symbol && altSymbolName)
        symbol = rp_GetLibSymbol(hLib, altSymbolName);

    return symbol;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
struct S_RP1210Context *rpGetContext(ORP_HANDLE hImpl)
{
    assert(hImpl != NULL);
    rp_ClearLastError();

    struct S_RP1210Context *context = NULL;
    const char *libPath = rpGetDriverPath(hImpl);

    ORP_HANDLE hLib = rp_OpenLib(libPath);
    if(hLib)
    {
        context = rp_mallocZ(sizeof(struct S_RP1210Context));
        if(context)
        {
            S_ContextInfo contextInfo;
            memset(&contextInfo, 0, sizeof(S_ContextInfo));

            contextInfo.hLibrary = hLib;

            context->RP1210_ClientConnect       = rp_LoadRP1210Sym(hLib, RP1210_EXPORT_CLIENTCONNECT,     RP1210_EXPORT_CLIENTCONNECT_ALT);
            context->RP1210_ClientDisconnect    = rp_LoadRP1210Sym(hLib, RP1210_EXPORT_CLIENTDISCONNECT,  RP1210_EXPORT_CLIENTDISCONNECT_ALT);
            context->RP1210_SendMessage         = rp_LoadRP1210Sym(hLib, RP1210_EXPORT_SENDMESSAGE,       RP1210_EXPORT_SENDMESSAGE_ALT);
            context->RP1210_ReadMessage         = rp_LoadRP1210Sym(hLib, RP1210_EXPORT_READMESSAGE,       RP1210_EXPORT_READMESSAGE_ALT);
            context->RP1210_SendCommand         = rp_LoadRP1210Sym(hLib, RP1210_EXPORT_SENDCOMMAND,       RP1210_EXPORT_SENDCOMMAND_ALT);
            context->RP1210_ReadVersion         = rp_LoadRP1210Sym(hLib, RP1210_EXPORT_READVERSION,       RP1210_EXPORT_READVERSION_ALT);
            context->RP1210_GetHardwareStatus   = rp_LoadRP1210Sym(hLib, RP1210_EXPORT_GETHARDWARESTATUS, RP1210_EXPORT_GETHARDWARESTATUS_ALT);
            context->RP1210_GetErrorMsg         = rp_LoadRP1210Sym(hLib, RP1210_EXPORT_GETERRORMSG,       RP1210_EXPORT_GETERRORMSG_ALT);

            #if RP1210_VERSION >= RP1210_VERSION_B
                context->RP1210_GetLastErrorMsg     = rp_LoadRP1210Sym(hLib, RP1210_EXPORT_GETLASTERRORMSG,     RP1210_EXPORT_GETLASTERRORMSG_ALT);
                context->RP1210_ReadDetailedVersion = rp_LoadRP1210Sym(hLib, RP1210_EXPORT_READDETAILEDVERSION, RP1210_EXPORT_READDETAILEDVERSION_ALT);
            #endif

            context->Context = rp_CopyToHandle(&contextInfo, sizeof(S_ContextInfo), NULL);
        }
    }

    return context;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rpLoadContext(ORP_HANDLE hImpl)
{
    assert(hImpl != NULL);

    rpReleaseContext(g_rp1210Context);
    g_rp1210Context = rpGetContext(hImpl);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rpClearContext(void)
{
    rpReleaseContext(g_rp1210Context);
    g_rp1210Context = NULL;
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void rpReleaseContext(struct S_RP1210Context *context)
{
    if(context)
    {
        S_ContextInfo *contextInfo = rp_HandleToTarget(context->Context);

        rpFreeHandle(contextInfo->hLibrary);
        rpFreeHandle(context->Context);
        rp_free(context);
    }
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
short WINAPI RP1210_ClientConnect(HWND hwndClient,
                                  short nDeviceId,
	                              const char *fpchProtocol,
                                  long lSendBuffer,
                                  long lReceiveBuffer,
                                  short nIsAppPacketizingIncomingMsgs)
{
    if(!g_rp1210Context)
        return rp_SetLastError(ORP_ERR_GENERAL, ERR_NO_CONTEXT);
    else
        return g_rp1210Context->RP1210_ClientConnect(hwndClient, nDeviceId, fpchProtocol, lSendBuffer, lReceiveBuffer, nIsAppPacketizingIncomingMsgs);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
short WINAPI RP1210_ClientDisconnect(short nClientID)
{
    if(!g_rp1210Context)
        return rp_SetLastError(ORP_ERR_GENERAL, ERR_NO_CONTEXT);
    else
        return g_rp1210Context->RP1210_ClientDisconnect(nClientID);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
short WINAPI RP1210_SendMessage(short nClientID,
                                char *fpchClientMessage,
                                short nMessageSize,
                                short nNotifyStatusOnTx,
                                short nBlockOnSend)
{
    if(!g_rp1210Context)
        return rp_SetLastError(ORP_ERR_GENERAL, ERR_NO_CONTEXT);
    else
        return g_rp1210Context->RP1210_SendMessage(nClientID, fpchClientMessage, nMessageSize, nNotifyStatusOnTx, nBlockOnSend);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
short WINAPI RP1210_ReadMessage(short nClientID,
                         char *fpchAPIMessage,
                         short nBufferSize,
                         short nBlockOnRead)
{
    if(!g_rp1210Context)
        return rp_SetLastError(ORP_ERR_GENERAL, ERR_NO_CONTEXT);
    else
        return g_rp1210Context->RP1210_ReadMessage(nClientID, fpchAPIMessage, nBufferSize, nBlockOnRead);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
short WINAPI RP1210_SendCommand(short nCommandNumber,
                                short nClientID,
                                char *fpchClientCommand,
                                short nMessageSize)
{
    if(!g_rp1210Context)
        return rp_SetLastError(ORP_ERR_GENERAL, ERR_NO_CONTEXT);
    else
        return g_rp1210Context->RP1210_SendCommand(nCommandNumber, nClientID, fpchClientCommand, nMessageSize);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
void WINAPI RP1210_ReadVersion(char *fpchDLLMajorVersion,
                               char *fpchDLLMinorVersion,
                               char *fpchAPIMajorVersion,
                               char *fpchAPIMinorVersion)
{
    if(!g_rp1210Context)
        rp_SetLastError(ORP_ERR_GENERAL, ERR_NO_CONTEXT);
    else
        g_rp1210Context->RP1210_ReadVersion(fpchDLLMajorVersion, fpchDLLMinorVersion, fpchAPIMajorVersion, fpchAPIMinorVersion);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
short WINAPI RP1210_ReadDetailedVersion(short nClientID,
                                        char *fpchAPIVersionInfo,
                                        char *fpchDLLVersionInfo,
                                        char *fpchFWVersionInfo)
{
    if(!g_rp1210Context)
        return rp_SetLastError(ORP_ERR_GENERAL, ERR_NO_CONTEXT);
    else
        return g_rp1210Context->RP1210_ReadDetailedVersion(nClientID, fpchAPIVersionInfo, fpchDLLVersionInfo, fpchFWVersionInfo);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
short WINAPI RP1210_GetHardwareStatus(short nClientID,
                                      char *fpchClientInfo,
                                      short nInfoSize,
                                      short nBlockOnRequest)
{
    if(!g_rp1210Context)
        return rp_SetLastError(ORP_ERR_GENERAL, ERR_NO_CONTEXT);
    else
        return g_rp1210Context->RP1210_GetHardwareStatus(nClientID, fpchClientInfo, nInfoSize, nBlockOnRequest);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
short WINAPI RP1210_GetErrorMsg(short err_code, char *fpchMessage)
{
    if(!g_rp1210Context)
        return rp_SetLastError(ORP_ERR_GENERAL, ERR_NO_CONTEXT);
    else
        return g_rp1210Context->RP1210_GetErrorMsg(err_code, fpchMessage);
}

/////////////////////////////////////////////////////////////////////////////////
///
///
/////////////////////////////////////////////////////////////////////////////////
short WINAPI RP1210_GetLastErrorMsg(short err_code, int *SubErrorCode, char *fpchMessage, short nClientID)
{
    if(!g_rp1210Context)
        return rp_SetLastError(ORP_ERR_GENERAL, ERR_NO_CONTEXT);
    else
        return g_rp1210Context->RP1210_GetLastErrorMsg(err_code, SubErrorCode, fpchMessage, nClientID);
}
