#pragma once
#include "MyWindows.h"


class NormalSocket
{
public:
    NormalSocket(const SOCKET socket, const SOCKADDR_IN addr) : _sockAddr(addr)
                                                              , _socket(socket)
    {
    }

protected:
    SOCKADDR_IN _sockAddr;
    SOCKET _socket;
};
