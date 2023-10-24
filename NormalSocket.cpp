#include "stdafx.h"
#include "NormalSocket.h"

#include "Socket.h"

Socket::Socket() : NormalSocket(INVALID_SOCKET, {0})
{
}

Socket::Socket(SOCKET socket, SOCKADDR_IN addr) : NormalSocket(socket, addr)
{
}

void Socket::Close()
{
	//auto result = CancelIoEx((HANDLE)_socket, nullptr);
	//if(result == 0)
	//{
	//	auto error = GetLastError();

	//	if(error == ERROR_NOT_FOUND)
	//	{
	//		printf("error : %d\n", error);
	//	}
	//	else
	//	{
	//		DebugBreak();
	//		printf("error : %d\n", error);
	//	}

	//}
	_beforeSocket = _socket;
	closesocket(_socket);
	_socket = INVALID_SOCKET;
}

int Socket::Send(LPWSABUF buf, DWORD bufCount, DWORD flag, LPWSAOVERLAPPED lpOverlapped)
{
	DWORD recvBytes = 0;
	 WSASend(_socket, buf, bufCount, nullptr, flag, lpOverlapped, nullptr);
	 return recvBytes;
}

int Socket::Recv(LPWSABUF buf, DWORD bufCount, LPDWORD flag, LPWSAOVERLAPPED lpOverlapped)
{
	DWORD recvBytes = 0;
	WSARecv(_socket, buf, bufCount, &recvBytes, flag, lpOverlapped, nullptr);
	return recvBytes;
}


int Socket::lastError() const
{
	return WSAGetLastError();
}

void Socket::setLinger( bool on)
{
	linger l = {on,0};
	setsockopt(_socket, SOL_SOCKET, SO_LINGER, (char*)&l, sizeof(l));
}

void Socket::setNoDelay( bool on)
{
	setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on));
}


SOCKADDR_IN Socket::GetSockAddr() const
{
	return _sockAddr;
}

bool Socket::Init()
{
	_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(_socket == INVALID_SOCKET)
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

	return SOCKET_ERROR != bind(_socket, (SOCKADDR*)&_sockAddr, sizeof(_sockAddr));
}

bool Socket::Listen(int backlog)
{
	return SOCKET_ERROR != listen(_socket, backlog);
}

Socket Socket::Accept()
{
	SOCKADDR_IN clientAddr = {};
	int len = sizeof(SOCKADDR);

	SOCKET clientSocket = accept(_socket, (SOCKADDR*)&clientAddr, &len);

	return { clientSocket, clientAddr };
}

bool Socket::Connect(String ip, uint16 port)
{
	memset(&_sockAddr, 0, sizeof(_sockAddr));
	_sockAddr.sin_family = AF_INET;
	_sockAddr.sin_addr = ip2Address(ip.c_str());
	_sockAddr.sin_port = htons(port);

	return SOCKET_ERROR != connect(_socket, (SOCKADDR*)&_sockAddr, sizeof(_sockAddr));

}


IN_ADDR Socket::ip2Address(const WCHAR* ip)
{
	IN_ADDR address = {0};
	InetPtonW(AF_INET, ip, &address);
	return address;
}