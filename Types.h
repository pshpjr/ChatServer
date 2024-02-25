#pragma once
#include <string>
#define _WINSOCKAPI_
#include "Windows.h"
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

	friend std::hash<SessionID>;
	friend std::equal_to<SessionID>;
};


using GroupID = volatile long;

inline static long g_GroupID = 0;
static GroupID NextID() { return GroupID(InterlockedIncrement(&g_GroupID)); }
static consteval GroupID InvalidGroupID() { return GroupID{ -1 }; }


//class GroupID
//{
//public:
//	explicit constexpr GroupID(long id) :_id (id){}
//
//	
//	
//	operator long() const { return _id; }
//
//	friend std::hash<GroupID>;
//	friend std::equal_to<GroupID>;
//private:
//
//};

using SocketID = uint64;

namespace std 
{
	template <> class hash<GroupID>
	{
	public:
		size_t operator()(const GroupID& id) const
		{
			return std::hash<long>()( long(id) );
		}
	};

	template <> class hash<SessionID>
	{
	public:
		size_t operator()(const SessionID& id) const
		{
			return std::hash<unsigned long long>()( id.id );
		}
	};
	template<> class equal_to<SessionID>
	{
	public:
		bool operator()(const SessionID& lhs, const SessionID& rhs) const
		{
			return lhs.id == rhs.id;
		}
	};
}


#include <WinSock2.h>
using SockAddr_in = SOCKADDR_IN;
using Port = uint16;

template <typename T>
using WSAResult = Result<T,int>;