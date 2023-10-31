#pragma once
#include "Protocol.h"
class Player;

class CMap
{
public:

	void AddPlayer(Player& newPlayer);
	void MovePlayer(Player& player, short nextX, short nextY);
	void DeletePlayerFromSector(Player& delPlayer);
	void Broadcast(Player& player,CSerializeBuffer& buffer);
	void SetOwner(IOCP* iocp) { _owner = iocp; }

	list<Player*> _map[50][50];
	IOCP* _owner;
};

extern CMap GMap;