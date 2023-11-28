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

	bool isSameSubnet(const String& comp, char subnetMaskBits);
	static bool isSameSubnet(const IN_ADDR& a, const IN_ADDR& b, char subnetMaskBits);

	bool Init();
	bool Bind(String ip, uint16 port);
	bool Listen(int backlog);
	Socket Accept();
	bool Connect(String ip, uint16 port);
	void CancleIO();
	void Close();
	static IN_ADDR ip2Address(const WCHAR* ip);

	/// <summary>
	/// 
	/// </summary>
	/// <returns>이상 없으면 0, 아니면 에러코드</returns>
	bool isValid() const;
	int Send(LPWSABUF buf, DWORD bufCount, DWORD flag,  LPWSAOVERLAPPED lpOverlapped);
	int Recv(LPWSABUF buf, DWORD bufCount, LPDWORD flag, LPWSAOVERLAPPED lpOverlapped);

	int lastError() const;
	void setLinger(bool on);
	void setNoDelay(bool on);

	SOCKADDR_IN GetSockAddr() const;
	String GetIP() const;
	uint16 GetPort() const;


private:
	SOCKET _beforeSocket = -1;
};

