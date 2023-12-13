#pragma once
#define dfPACKET_CODE		0x77

#pragma pack(1)
struct LANHeader
{
	uint16 len;
};
#pragma pack( )

#pragma pack(1)
struct NetHeader
{
	unsigned char code;
	unsigned short len;
	unsigned char randomKey;
	unsigned char checkSum;
}; 
#pragma pack( )