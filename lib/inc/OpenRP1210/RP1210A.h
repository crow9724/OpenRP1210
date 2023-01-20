//------------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//------------------------------------------------------------------------------
#ifndef OPENRP1210_RP1210A_H__
#define OPENRP1210_RP1210A_H__

//--------------------------------------------------------------
// RP1210A RP1210_SendCommand Defines
//--------------------------------------------------------------

typedef unsigned char U8;

#define RP1210_Reset_Device                           0
#define RP1210_Set_All_Filters_States_to_Pass         3
#define RP1210_Set_Message_Filtering_For_J1939        4
#define RP1210_Set_Message_Filtering_For_CAN          5
#define RP1210_Set_Message_Filtering_For_J1708        7
#define RP1210_Generic_Driver_Command                14
#define RP1210_Set_J1708_Mode                        15
#define RP1210_Echo_Transmitted_Messages             16
#define RP1210_Set_All_Filters_States_to_Discard     17
#define RP1210_Set_Message_Receive                   18
#define RP1210_Protect_J1939_Address                 19

#define BLOCKING_IO              1 // For Blocking calls to send/read.
#define NON_BLOCKING_IO          0 // For Non-Blocking calls to send/read.
#define BLOCK_INFINITE           0 // For Non-Blocking calls to send/read.
#define BLOCK_UNTIL_DONE         0
#define RETURN_BEFORE_COMPLETION 2
#define CONVERTED_MODE           1 // RP1210Mode="Converted"
#define RAW_MODE                 0 // RP1210Mode="Raw"

#define ECHO_OFF 0x00 // EchoMode
#define ECHO_ON  0x01 // EchoMode

#define RECEIVE_ON  0x01 // Set Message Receive
#define RECEIVE_OFF 0x00 // Set Message Receive

#define FILTER_PGN         0x00000001 // Setting of J1939 filters
#define FILTER_SOURCE      0x00000004 // Setting of J1939 filters
#define FILTER_DESTINATION 0x00000008 // Setting of J1939 filters
#define FILTER_INCLUSIVE         0x00 // FilterMode
#define FILTER_EXCLUSIVE         0x01 // FilterMode

#define SILENT_J1939_CLAIM        0x00 // Claim J1939 Address
#define PASS_J1939_CLAIM_MESSAGES 0x01 // Claim J1939 Address

#define STANDARD_CAN                   0x00 // Filters
#define EXTENDED_CAN                   0x01 // Filters

#ifndef TRUE
	#define TRUE  0x01
	#define FALSE 0x00
#endif

//--------------------------------------------------------------
// RP1210C Return Definitions
//--------------------------------------------------------------
#define NO_ERRORS                                  0
#define ERR_DLL_NOT_INITIALIZED                  128
#define ERR_INVALID_CLIENT_ID                    129
#define ERR_CLIENT_ALREADY_CONNECTED             130
#define ERR_CLIENT_AREA_FULL                     131
#define ERR_FREE_MEMORY                          132
#define ERR_NOT_ENOUGH_MEMORY                    133
#define ERR_INVALID_DEVICE                       134
#define ERR_DEVICE_IN_USE                        135
#define ERR_INVALID_PROTOCOL                     136
#define ERR_TX_QUEUE_FULL                        137
#define ERR_TX_QUEUE_CORRUPT                     138
#define ERR_RX_QUEUE_FULL                        139
#define ERR_RX_QUEUE_CORRUPT                     140
#define ERR_MESSAGE_TOO_LONG                     141
#define ERR_HARDWARE_NOT_RESPONDING              142
#define ERR_COMMAND_NOT_SUPPORTED                143
#define ERR_INVALID_COMMAND                      144
#define ERR_TXMESSAGE_STATUS                     145
#define ERR_ADDRESS_CLAIM_FAILED                 146
#define ERR_CANNOT_SET_PRIORITY                  147
#define ERR_CLIENT_DISCONNECTED                  148
#define ERR_CONNECT_NOT_ALLOWED                  149
#define ERR_CHANGE_MODE_FAILED                   150
#define ERR_BUS_OFF                              151
#define ERR_COULD_NOT_TX_ADDRESS_CLAIMED         152
#define ERR_ADDRESS_LOST                         153
#define ERR_CODE_NOT_FOUND                       154
#define ERR_BLOCK_NOT_ALLOWED                    155
#define ERR_MULTIPLE_CLIENTS_CONNECTED           156
#define ERR_ADDRESS_NEVER_CLAIMED                157
#define ERR_WINDOW_HANDLE_REQUIRED               158
#define ERR_MESSAGE_NOT_SENT                     159
#define ERR_MAX_NOTIFY_EXCEEDED                  160
#define ERR_MAX_FILTERS_EXCEEDED                 161
#define ERR_HARDWARE_STATUS_CHANGE               162

#define GET_CONFIG     0x01
#define SET_CONFIG     0x02
#define FIVE_BAUD_INIT 0x04
#define FAST_INIT      0x05

// ISO9141/KWP2000 Ioctl values reserved 0x03, 0x06-0xFFFF
// Ioctl values that are vendor specific 0x10000-0xFFFFFFFF
typedef struct
{
	unsigned long Parameter; /*name of parameter*/
	unsigned long Value;     /*value of the parameter*/
}SCONFIG;

typedef struct
{
	unsigned long NumOfParams; /*number of SCONFIG elements*/
	SCONFIG *pConfig;          /*array of SCONFIG elements*/
}SCONFIG_LIST;

typedef struct
{
	unsigned long NumOfBytes; /*number of bytes in the array*/
	unsigned char *pBytes;    /*array of bytes*/
}SBYTE_ARRAY;

//--------------------------------------------------------------
// TMC RP1210C Defined Function Prototypes
//--------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
	short DLLEXPORT WINAPI RP1210_ClientConnect(
		HWND hwndClient,
		short nDeviceId,
		const char *fpchProtocol,
		long lxBufferSize,
		long lRcvBufferSize,
		short nIsAppPacketizingIncomingMsgs);

	short DLLEXPORT WINAPI RP1210_ClientDisconnect(short nClientID);

	short DLLEXPORT WINAPI RP1210_SendMessage(
		short nClientID,
		char *fpchClientMessage,
		short nMessageSize,
		short nNotifyStatusOnTx,
		short nBlockOnSend);

	short DLLEXPORT WINAPI RP1210_ReadMessage(
		short nClientID,
		char *fpchAPIMessage,
		short nBufferSize,
		short nBlockOnRead);

	short DLLEXPORT WINAPI RP1210_SendCommand(
		short nCommandNumber,
		short nClientID,
		char *fpchClientCommand,
		short nMessageSize);

	void DLLEXPORT WINAPI RP1210_ReadVersion(
		char *fpchDLLMajorVersion,
		char *fpchDLLMinorVersion,
		char *fpchAPIMajorVersion,
		char *fpchAPIMinorVersion);

	short DLLEXPORT WINAPI RP1210_GetHardwareStatus(
		short nClientID,
		char *fpchClientInfo,
		short nInfoSize,
		short nBlockOnRequest);

	short DLLEXPORT WINAPI RP1210_GetErrorMsg(
		short ErrorCode,
		char *fpchMessage);

#ifdef __cplusplus
}
#endif
#endif