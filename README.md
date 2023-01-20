# OpenRP1210
This is a C library for communicating with hardware that implements the RP1210 vehicle communication specification.
The library can be built and ran on both Windows and Linux.

The following communication protocols are supported by RP1210 and can be used within the library:
| Protocol     | Description                             |
| -------------| ----------------------------------------| 
| CAN          | CAN Network Protocol                    | 
| J1939        | SAE J1939 Protocol                      |
| Iso15765/UDS | ISO15765 Protocol                       |
| KWP2000      | Keyword 2000 Protocol / ISO9141         |
| J1708        | SAE J1708 Protocol                      |
| J2284        | SAE J2284 Network Protocol              |
| PLC          | Power Line Carrier Protocol             |
| J1850_104k   | SAE J1850 Vehicle Protocol (10.4k Baud) |
| J1850_416k   | SAE J1850 Vehicle Protocol (41.6k Baud) |
| ISO9141      | ISO9141 Protocol                        |

For more information regarding RP1210 see the official specification: [RP1210 Standards](https://www.atabusinesssolutions.com/Shopping/Product/viewproduct/2675472/TMC-Individual-RP)

## Basic Usage
### Connecting to a Device
The following code assumes you have an RP1210 driver installed called "OpenRP1210". This should be listed in the RP1210.ini file. It also assumes the driver defines a device with device id 1.
Error handling is not implemented for brevity.
```c
// connect to a 500K CAN device using driver "OpenRP1210" with deviceId 1.
ORP_HANDLE hImpls = rpGetApiImpls();
ORP_HANDLE hImpl = rpGetApiImplByName(hImpls, "OpenRP1210");
rpLoadContext(hImpl);

short clientId = RP1210_ClientConnect(0, 1, "CAN:Baud=500", 0, 0, 0); //devId: 1
```
### Reading a Message
Assuming a valid clientId as shown above,
```c
RP1210_SendCommand(RP1210_Set_All_Filters_States_to_Pass, clientId, NULL, 0); // clear all filters

unsigned char data[17]; // [TimeStamp 4 bytes, Message Type 1 byte, CAN ID 2/4 bytes, Data 0-8 bytes]
memset(data, 0, 17);

short r = RP1210_ReadMessage(clientId, data, 17, NON_BLOCKING_IO);
if(r > 0)
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

	unsigned char dlc = r - dataOffset; // number of bytes of data
	
	// canId = the CAN identifier
	// data[dataOffset, dataOffset + dlc - 1] = CAN frame data 
}
```
More complete examples can be found in the demo application. See [DemoApp](demo/src/DemoApp.cpp).

## Compiling OpenRP1210
On Windows there are Visual Studio 2019/2022 [project files](project/) (x86 & x64) that will build the OpenRP1210 DLL and import lib.

On Linux there's a [Makefile](project/linux/). Simply run "make", and it will produce a shared library.
Alternatively, there's a [Visual Studio Code](project/vscode) workspace that can be used to build.

The demo application can be built with "make DemoApp".

## Demo Application Usage
The demo application currently supports:
##### Listing all devices for all RP1210 drivers installed on the system
```
.\DemoApp.exe -action listDev
```

##### Reading CAN frames
Read CAN frames at 500K from OpenRP1210 Driver for 2 seconds and dump to stdout.
```
.\DemoApp.exe -action readCAN -t 2000 -vendor OpenRP1210 -device 1 -baud 500
```

## Documenation
Documentation can be built using Doxygen with the [Doxyfile](doc/Doxyfile).

## License
This project is licensed under the [Mozilla Public License 2.0](https://choosealicense.com/licenses/mpl-2.0/).

