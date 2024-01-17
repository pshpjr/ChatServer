#pragma once
#include <string>

#include "Result.h"


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
using StringView = std::wstring_view;
//소켓 관련

class SessionID
{
public:
	union 
	{
		struct
		{
			short tmp1;
			short tmp2;
			short tmp3;
			unsigned short index;
		};
		unsigned long long id;
	};

	friend bool operator==(const SessionID lhs, const SessionID rhs)
	{
		return lhs.id == rhs.id;
	}
};


using SocketID = uint64;
using GroupID = long;
#include <WinSock2.h>
using SockAddr_in = SOCKADDR_IN;
using Port = uint16;

template <typename T>
using WSAResult = Result<T,int>;