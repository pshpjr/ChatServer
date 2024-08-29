﻿#pragma once
#include <BuildOption.h>
#include <Types.h>
#include SOCKET_HEADER

class Address;

class Socket : public SOCKET_CLASS
{
    friend class Session;

public:
    /**
     * \brief 빈 소켓 만들기
     */
    Socket();

    ~Socket() = default;

    Socket(SOCKET socket, SOCKADDR_IN address);

    bool IsSameSubnet(const String& comp, char subnetMaskBits) const;
    static bool IsSameSubnet(const IN_ADDR& a, const IN_ADDR& b, char subnetMaskBits);

    bool Init();
    bool Bind(const String& ip, uint16 port);
    bool Listen(int backlog) const;
    Socket Accept() const;
    WSAResult<bool> Connect(const String& ip, uint16 port);
    void CancelIO() const;
    void Close();
    static IN_ADDR Ip2Address(const WCHAR* ip);

    /// <summary>
    /// 
    /// </summary>
    /// <returns>이상 없으면 0, 아니면 에러코드</returns>
    bool IsValid() const;
    int Send(LPWSABUF buf, DWORD bufCount, DWORD flag, LPWSAOVERLAPPED lpOverlapped) const;
    int Recv(LPWSABUF buf, DWORD bufCount, LPDWORD flag, LPWSAOVERLAPPED lpOverlapped) const;

    static int LastError();
    void SetLinger(bool on) const;
    void SetNoDelay(bool on) const;
    void SetSendBuffer(int size) const;

    SOCKADDR_IN GetSockAddress() const;
    String GetIp() const;
    uint16 GetPort() const;

private:
    SOCKET _beforeSocket = INVALID_SOCKET;
};
