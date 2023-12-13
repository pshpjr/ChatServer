#pragma once



#define USE_NORMAL_SOCKET
//#define USE_RUDP_SOCKET

#if defined( USE_NORMAL_SOCKET)

#define SOCKET_CLASS NormalSocket
#define SOCKET_HEADER "NormalSocket.h"
#define IOCP_CLASS NormalIOCP
#define IOCP_HEADER "NormalIOCP.h"

#elif defined(USE_RUDP_SOCKET)
#define SOCKET_CLASS RUDPSocket
#define IOCP_CLASS RUDPIOCP

#else
#error("You must define one of USE_NORMAL_SOCKET or USE_RUDP_SOCKET")
#endif

