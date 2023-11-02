#include "stdafx.h"
#include "CMap.h"
#include "CSerializeBuffer.h"
#include "Player.h"
#include "IOCP.h"
CMap GMap;


char searchTable[9][2] = { { -1, -1},{-1,0},{-1,1},{0,-1},{0,0},{0,1},{1,-1},{1,0},{1,1} };

void CMap::AddPlayer(Player& newPlayer)
{
	_map[newPlayer._curY][newPlayer._curX].push_back(&newPlayer);
}

void CMap::MovePlayer(Player& player, short nextX, short nextY)
{
	DeletePlayerFromSector(player);


	player._curX = nextX;
	player._curY = nextY;

	_map[player._curY][player._curX].push_back(&player);

}

void CMap::DeletePlayerFromSector(Player& delPlayer)
{
	for (auto it = _map[delPlayer._curY][delPlayer._curX].begin(); it != _map[delPlayer._curY][delPlayer._curX].end(); ++it)
	{
		if (*it == &delPlayer)
		{
			_map[delPlayer._curY][delPlayer._curX].erase(it);
			break;
		}
	}

	int a = 10;
}

void CMap::Broadcast(Player& player, CSerializeBuffer& buffer)
{
	auto sectorX = player._curX;
	auto sectorY = player._curY;
	int broadCount = 0;
	for (auto searchOffset : searchTable)
	{
		int16 currentSectorX = sectorX + searchOffset[1];
		int16 currentSectorY = sectorY + searchOffset[0];

		if (currentSectorX < 0 || currentSectorX >= 50 || currentSectorY < 0 || currentSectorY >= 50)
			continue;


		for (auto p : _map[currentSectorY][currentSectorX])
		{

			_owner->SendPacket(p->_sessionId, &buffer);
			int a = 0;
		}
	}
}



