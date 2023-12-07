#pragma once
#define dfPACKET_CODE		0x77

struct LANHeader
{
	uint16 len;
};

struct NetHeader
{
	char code;
	unsigned short len;
	char randomKey;
	unsigned char checkSum;
};