#pragma once
#include "Session.h"


class PlayerSession;
class Room;
class CSerializeBuffer;

class Player
{
public:
	Player(){}

	Player(short x, short y, SessionID id) : _curX(x),_curY(y),_sessionId(id)
	{
		lastEcho = chrono::system_clock::now();
	}


	short _curX = 0;
	short _curY = 0;
	int64 AccountNo;
	WCHAR ID[20];
	WCHAR Nickname[20];
	char SessionKey[64];

	SessionID _sessionId;

	chrono::time_point<chrono::system_clock> lastEcho;
};


