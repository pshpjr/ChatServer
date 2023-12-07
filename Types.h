#pragma once
#include <string>



using BYTE = unsigned char;
using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned  __int64;

using String = std::wstring;

//소켓 관련
using SessionID = uint64;
using SocketID = uint64;
#include <WinSock2.h>
using SockAddr_in = SOCKADDR_IN;
using Port = uint16;