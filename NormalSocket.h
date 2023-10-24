#pragma once
#include <WinSock2.h>



class NormalSocket
{
public:
	NormalSocket(SOCKET socket, SOCKADDR_IN addr) : _socket(socket), _sockAddr(addr)
	{

	}


protected:
	
	SOCKADDR_IN _sockAddr;
	SOCKET _socket;

};
