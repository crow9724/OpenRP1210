//------------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//------------------------------------------------------------------------------
#include "OpenRP1210/OpenRP1210.h"
#include <string>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

enum class CmdAction
{
    Action_ReadCAN,
    Action_ListDevices,
    Action_None
};

struct CommandLineArgs
{
    CmdAction Action;
    unsigned int ActionTime;
    std::string Vendor;
    unsigned int DeviceId;
    unsigned int Baud;

    CommandLineArgs()
    {
        Action = CmdAction::Action_None;
        ActionTime = 2000;
        Vendor = "";
        DeviceId = 0;
        Baud = 250;
    }
};

bool StrToInt(std::string str, unsigned int *i)
{
    bool r = true;

    try
    {
        *i = std::stoi(str);
    }
    catch (std::exception &)
    {
        r = false;
    }

    return r;
}

std::string GetCmdArg(char **argv, int index)
{
    std::string s = argv[index];
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    return s;
}

char *GetRP1210ErrorMsg(short err, char *buf, int length)
{
    memset(buf, 0, length);
    RP1210_GetErrorMsg(err, buf);

    return buf;
}

bool ParseCmdLine(int argc, char **argv, CommandLineArgs *cmdArgs)
{  
    bool err = false;
    int i = 1; // ignore program name
    while (i < argc && !err)
    {
        std::string arg = GetCmdArg(argv, i);
        if (arg[0] == '-')
        {
            if (i + 1 < argc)
            {
                std::string arg2 = GetCmdArg(argv, i + 1);
                if (arg2[0] == '-')
                {
                    std::cout << "Missing value for command line argument: " << arg << std::endl;
                    err = true;
                }
                else
                {
                    i++;
                    if(arg == "-action")
                    {
                        if(arg2 == "readcan")
                            cmdArgs->Action = CmdAction::Action_ReadCAN;
                        else if(arg2 == "listdev")
                            cmdArgs->Action = CmdAction::Action_ListDevices;
                        else
                        {
                            std::cout << "Unknown action: " << arg2 << std::endl;
                            err = true;
                        }
                    }
                    else if(arg == "-t")
                    {
                        if(!StrToInt(arg2, &cmdArgs->ActionTime))
                        {
                            std::cout << "Invalid time: " << arg2 << std::endl;
                            err = true;
                        }
                    }
                    else if(arg == "-baud")
                    {
                        if(!StrToInt(arg2, &cmdArgs->Baud))
                        {
                            std::cout << "Invalid baud rate: " << arg2 << std::endl;
                            err = true;
                        }
                    }
                    else if(arg == "-vendor")
                        cmdArgs->Vendor = argv[i];
                    else if(arg == "-device")
                    {
                        if (!StrToInt(arg2, &cmdArgs->DeviceId))
                        {
                            std::cout << "Invalid device ID: " << arg2 << std::endl;
                            err = true;
                        }
                    }
                    else
                    {
                        std::cout << "Unknown command line argument: " << arg << std::endl;
                        err = true;
                    }
                }
            }
            else
            {
                std::cout << "Missing value for command line argument: " << arg << std::endl;
                err = true;
            }
        }
        else
        {
            std::cout << "Unknown command line argument: " << arg << std::endl;
            err = true;
        }

        i++;
    }

    if (!err)
    {
        bool valid = false;

        if (cmdArgs->Action == CmdAction::Action_None)
            std::cout << "Action not specified." << std::endl;
        else
            valid = true;

        return valid;
    }
    else
        return false;
}

void PrintCAN(char *buf, unsigned int bufLength, unsigned int canId, unsigned char dlc, unsigned char *data, unsigned int dataLength)
{
    unsigned int written = snprintf(buf, bufLength, " %08X [%d] ", canId, dlc);
    for(unsigned int i = 0; i < dataLength; i++)
        if(written < bufLength)
            written += snprintf(buf + written, bufLength - written, "%02X ", data[i]);
}

bool LoadContext(const std::string &vendorName, unsigned int deviceId)
{
    bool r = false;

    ORP_HANDLE hImpls = rpGetApiImpls();
    if (hImpls)
    {
        int loadErrCount = rpGetNumApiImplLoadErrors(hImpls);
        for (int i = 0; i < loadErrCount; i++)
        {
            S_RP1210ImplLoadErr *le = rpGetApiImplLoadError(hImpls, i);
            std::cout << "****Failed to load Implementation: " << le->ImplName << ". Error = " << le->Error << " Description = " << le->Description << "****" << std::endl;
        }
        
        ORP_HANDLE hImpl = rpGetApiImplByName(hImpls, vendorName.c_str());
        if (hImpl)
        {
            S_RP1210DeviceInformation *device = rpGetDeviceInfoById(hImpl, deviceId);
            if (device)
            {
                rpLoadContext(hImpl);
                if(rpGetLastError() != ORP_ERR_NO_ERROR)
                    std::cout << "Error loading RP1210 context: " << rpGetLastErrorDesc() << std::endl;
                else
                    r = true;
            }
            else
                std::cout << "Device not found. Vendor Name: " << vendorName << " Device ID: " << deviceId << std::endl;

            rpFreeHandle(hImpl);
        }
        else
            std::cout << "Failed to find API implementation. Vendor Name: " << vendorName << std::endl;

        rpFreeHandle(hImpls);
    }
    else
        std::cout << "Failed to find API implementations. Error: " << rpGetLastErrorDesc() << std::endl;

    return r;
}

void ReadCAN(const std::string &vendorName, unsigned int deviceId, unsigned int baud, unsigned int time_ms)
{
    char errMsg[80];

    if (LoadContext(vendorName, deviceId))
    {
        char protocolStr[20];
        snprintf(protocolStr, 20, "CAN:Baud=%d", baud);

        short clientId = RP1210_ClientConnect(0, deviceId, protocolStr, 0, 0, 0);
        if (clientId >= 0 && clientId <= 127)
        {
            RP1210_SendCommand(RP1210_Set_All_Filters_States_to_Pass, clientId, nullptr, 0);

            std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

            while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count() < time_ms)
            {
                enum { MAX_DATA = 30 };
                char data[MAX_DATA];
                memset(data, 0, MAX_DATA);

                short r = RP1210_ReadMessage(clientId, data, MAX_DATA, NON_BLOCKING_IO);
                if(r < 0)
                    std::cout << "RP1210_ReadMessage failed: " << GetRP1210ErrorMsg(clientId, errMsg, 80);
                else
                {
                    if (r > 0)
                    {
                        unsigned int canId = 0x00;
                        unsigned int dataOffset = 0;

                        if (data[4] == STANDARD_CAN)
                        {
                            canId = (data[5] << 8) | data[6];
                            dataOffset = 7;
                        }
                        else if (data[4] == EXTENDED_CAN)
                        {
                            canId = ((unsigned char)(data[5]) << 24) | ((unsigned char)(data[6]) << 16) | ((unsigned char)(data[7]) << 8) | (unsigned char)data[8];
                            dataOffset = 9;
                        }

                        unsigned char dlc = r - dataOffset;

                        char msgStr[100];
                        PrintCAN(msgStr, 100, canId, dlc, ((unsigned char *)data) + dataOffset, dlc);
                        std::cout << msgStr << std::endl;
                    }
                }
            }

            RP1210_ClientDisconnect(clientId);
        }
        else
            std::cout << "RP1210_ClientConnect failed: " << GetRP1210ErrorMsg(clientId, errMsg, 80);

        rpClearContext();
    }
}

void ListDevices(void)
{
    ORP_HANDLE hImpls = rpGetApiImpls();
    if(hImpls)
    {
        int apiCount = rpGetNumApiImpls(hImpls);

        for(int i = 0; i < apiCount; i++)
        {
            ORP_HANDLE hImpl = rpGetApiImpl(hImpls, i);
            if(hImpl)
            {
                S_RP1210VendorInformation *vi = rpGetVendorInfo(hImpl);
                std::cout << "***" << rpGetApiImplName(hImpl) << ", " << vi->Name << "***" << std::endl;

                unsigned int numDevices = rpGetNumDevices(hImpl);

                for(unsigned int j = 0; j < numDevices; j++)
                {
                    S_RP1210DeviceInformation *devInfo = rpGetDeviceInfo(hImpl, j);
                    std::cout << "   Device: " << devInfo->DeviceName << "  Description: " << devInfo->DeviceDescription << std::endl;
                }

                std::cout << std::endl;
                rpFreeHandle(hImpl);
            }
            else
                std::cout << "Failed to get API implementation. Error: " << rpGetLastErrorDesc() << std::endl;
        }

        int loadErrCount = rpGetNumApiImplLoadErrors(hImpls);
        for (int i = 0; i < loadErrCount; i++)
        {
            S_RP1210ImplLoadErr *le = rpGetApiImplLoadError(hImpls, i);
            std::cout << "****Failed to load Implementation: " << le->ImplName << ". Error = " << le->Error << " Description = " << le->Description << "****" << std::endl;
        }

        rpFreeHandle(hImpls);
    }
    else
        std::cout << "Failed to find API implementations. Error: " << rpGetLastErrorDesc() << std::endl;
}

int main(int argc, char **argv)
{ 
    // examples:
    //  Read CAN frames at 500K from OpenRP1210 Driver for 2 seconds and dump to stdout:   .\DemoApp.exe -action readCAN -t 2000 -vendor OpenRP1210 -device 1 -baud 500
    //  List all devices for all RP1210 drivers installed on the system:                   .\DemoApp.exe -action listDev

    CommandLineArgs cmdArgs;

    if (ParseCmdLine(argc, argv, &cmdArgs))
    {
        std::cout << "RP1210 Library Version = " << RP1210_VERSION_STRING << std::endl << std::endl;

        if (cmdArgs.Action == CmdAction::Action_ReadCAN)
            ReadCAN(cmdArgs.Vendor, cmdArgs.DeviceId, cmdArgs.Baud, cmdArgs.ActionTime);
        else if (cmdArgs.Action == CmdAction::Action_ListDevices)
            ListDevices();
    }

    #if defined(_WIN32) && defined(_DEBUG)
        _CrtDumpMemoryLeaks();   
    #endif
}
