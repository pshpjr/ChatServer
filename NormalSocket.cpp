#include "stdafx.h"
#include "NormalSocket.h"
#include "Socket.h"


Socket::Socket() : NormalSocket(INVALID_SOCKET, { 0 })
{
}

Socket::Socket(SOCKET socket, SOCKADDR_IN addr) : NormalSocket(socket, addr)
{
}

bool Socket::isSameSubnet(const String& comp, char subnetMaskBits)
{
	IN_ADDR compAddr;
	InetPtonW(AF_INET, comp.c_str(), &compAddr);

	return isSameSubnet(compAddr, _sockAddr.sin_addr, subnetMaskBits);
}

bool Socket::isSameSubnet(const IN_ADDR& a, const IN_ADDR& b, char subnetMaskBits)
{
	ASSERT_CRASH(0 <= subnetMaskBits && subnetMaskBits <= 32, "subnetSizeError");
	unsigned long subnetMask = ( 0xFFFFFFFFull >> ( 32 - subnetMaskBits ) ) & 0xFFFFFFFF;

	unsigned long compMasked = a.S_un.S_addr & subnetMask;
	unsigned long tarMasked = b.S_un.S_addr & subnetMask;


	return compMasked == tarMasked;
}

void Socket::CancleIO()
{

	auto result = CancelIoEx(( HANDLE ) _socket, nullptr);
	if ( result == 0 )
	{
		auto error = GetLastError();

		//소켓 닫혀서 gqcs에러 받은 상황에서 send 실패하면 io 자체가 없음. 에러 아님. 
		if ( error == ERROR_NOT_FOUND )
		{
		}
		else
		{
			DebugBreak();
			printf("error : %d\n", error);
		}

	}


}

void Socket::Close()
{
	_beforeSocket = _socket;
	_socket = INVALID_SOCKET;
	closesocket(_beforeSocket);
}

int Socket::Send(LPWSABUF buf, DWORD bufCount, DWORD flag, LPWSAOVERLAPPED lpOverlapped)
{
	return WSASend(_socket, buf, bufCount, nullptr, flag, lpOverlapped, nullptr);
}

int Socket::Recv(LPWSABUF buf, DWORD bufCount, LPDWORD flag, LPWSAOVERLAPPED lpOverlapped)
{

	return WSARecv(_socket, buf, bufCount, nullptr, flag, lpOverlapped, nullptr);

}


int Socket::lastError() const
{
	return WSAGetLastError();
}

void Socket::setLinger(bool on)
{
	linger l = { on,0 };
	setsockopt(_socket, SOL_SOCKET, SO_LINGER, ( char* ) &l, sizeof(l));
}

void Socket::setNoDelay(bool on)
{
	setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, ( char* ) &on, sizeof(on));
}


SOCKADDR_IN Socket::GetSockAddr() const
{
	return _sockAddr;
}

String Socket::GetIP() const
{
	WCHAR str[47];
	InetNtopW(AF_INET, &_sockAddr.sin_addr, str, 47);

	return String(str);
}

uint16 Socket::GetPort() const
{
	uint16 result;
	WSANtohs(_socket, _sockAddr.sin_port, &result);
	return result;
}

bool Socket::Init()
{
	_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( _socket == INVALID_SOCKET )
	{
		return false;
	}
	return true;
}
bool Socket::isValid() const
{
	return _socket != INVALID_SOCKET;
}


bool Socket::Bind(String ip, uint16 port)
{
	memset(&_sockAddr, 0, sizeof(_sockAddr));
	_sockAddr.sin_family = AF_INET;
	_sockAddr.sin_addr = ip2Address(ip.c_str());
	_sockAddr.sin_port = htons(port);

	return SOCKET_ERROR != bind(_socket, ( SOCKADDR* ) &_sockAddr, sizeof(_sockAddr));
}

bool Socket::Listen(int backlog)
{
	return SOCKET_ERROR != listen(_socket, backlog);
}

Socket Socket::Accept()
{
	SOCKADDR_IN clientAddr = {};
	int len = sizeof(SOCKADDR);

	SOCKET clientSocket = accept(_socket, ( SOCKADDR* ) &clientAddr, &len);


	return { clientSocket, clientAddr };
}

WSAResult<bool> Socket::Connect(String ip, uint16 port)
{
	memset(&_sockAddr, 0, sizeof(_sockAddr));
	_sockAddr.sin_family = AF_INET;
	_sockAddr.sin_addr = ip2Address(ip.c_str());
	_sockAddr.sin_port = htons(port);

	int result = 0;
	if ( SOCKET_ERROR == connect(_socket, ( SOCKADDR* ) &_sockAddr, sizeof(_sockAddr)))
	{
		result = WSAGetLastError();
		return result;
	}
	  
	return true;
}


IN_ADDR Socket::ip2Address(const WCHAR* ip)
{
	IN_ADDR address = { 0 };
	InetPtonW(AF_INET, ip, &address);
	return address;
}