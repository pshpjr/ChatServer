#pragma once
#include "BuildOption.h"

class Address;

class Socket : public SOCKET_CLASS
{
	friend class Session; 
public:
	/**
	 * \brief 빈 소켓 만들기
	 */
	Socket();

	Socket(SOCKET socket, SOCKADDR_IN addr);


	bool Init();
	bool Bind(String ip, uint16 port);
	bool Listen(int backlog);
	Socket Accept();
	bool Connect(String ip, uint16 port);
	void Close();
	bool isValid() const;

	int Send(LPWSABUF buf, DWORD bufCount, DWORD flag,  LPWSAOVERLAPPED lpOverlapped);
	int Recv(LPWSABUF buf, DWORD bufCount, LPDWORD flag, LPWSAOVERLAPPED lpOverlapped);

	int lastError() const;
	void setLinger(bool on);
	void setNoDelay(bool on);

	SOCKADDR_IN GetSockAddr() const;
	int _beforeSocket=-1;
private:
	static IN_ADDR ip2Address(const WCHAR* ip);
};

