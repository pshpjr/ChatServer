 // IOCPTest2.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "stdafx.h"

#include <chrono>

#include "Server.h"
#include <chrono>
#include "CommonProtocol.h"
#include "Player.h"
#include "CMap.h"
struct connection {
	SessionID id;
	chrono::system_clock::time_point echo;
};

list<connection> loginWait;

HashMap<SessionID,Player*> gplayers;

void HandlePacket(CSerializeBuffer& buffer,SessionID id, Server& server);
int main()
{
	std::string hexString = "77 4e 00 31 bf 4a 92 eb 7b 23 94 cf 31 1b ea 8e 29 0e e8 9d 19 51 c6 99 3b ef 8c 62 53 95 b4 ea 0f 85 fc 05 c5 e9 7a 63 b3 5a af 5c 18 a3 f3 35 f8 b3 5f 6d 6d ad 4a 2d 97 15 9d dd cb 9d d9 27 58 b1 0e 3d 9f f3 3c 1a 8f f6 06 42 81 08 f4 f3 8d 0a 0c";

	// Split the string by space to get individual hex values
	std::vector<std::string> hexValues;
	size_t pos = 0;
	while ((pos = hexString.find(" ")) != std::string::npos) {
		hexValues.push_back(hexString.substr(0, pos));
		hexString.erase(0, pos + 1);
	}
	hexValues.push_back(hexString);

	// Convert hex values to a char array
	std::vector<unsigned char> byteArray;
	for (const std::string& hexValue : hexValues) {
		byteArray.push_back(static_cast<unsigned char>(std::stoi(hexValue, nullptr, 16)));
	}

	auto size = byteArray.size();
	// Copy the values to a char array
	char charArray[100];
	for (size_t i = 0; i < byteArray.size(); ++i) {
		charArray[i] = static_cast<char>(byteArray[i]);
	}

	CSerializeBuffer b2;
	memcpy_s(b2.GetHead(), 100, charArray, byteArray.size());
	b2.MoveWritePos(byteArray.size() - 5);
	b2.Decode(0x32);

	timeBeginPeriod(1);
	srand(GetCurrentThreadId());
	Server& server = *new Server;

	server.Init(L"0.0.0.0", 12001,20,1, 0x32);
	GMap.SetOwner(&server);

	while (true) 
	{
		server._packetQueue.Swap();

		ContentJob* job;

		while (true) 
		{
			job = server._packetQueue.Dequeue();
			if (job == nullptr)
				break;

			CSerializeBuffer& buffer = *job->_buffer;



			switch (job->_type)
			{
			case ContentJob::ePacketType::Connect:
				loginWait.push_back({ job->_id,chrono::system_clock::now() });
				break;

			case ContentJob::ePacketType::Disconnect:
				
				break;
			case ContentJob::ePacketType::TimeoutCheck:
				break;
			case ContentJob::ePacketType::Packet:
				
				HandlePacket(buffer,job->_id, server);
				break;
			default:
				DebugBreak();
			}

			job->Free();


		}

		Sleep(20);
	}

}

void HandlePacket(CSerializeBuffer& buffer, SessionID id, Server& server)
{
	WORD type;
	buffer >> type;
	switch (type)
	{

	case en_PACKET_CS_CHAT_REQ_LOGIN: 
	{
		int64 AccountNo;
		WCHAR ID[20];
		WCHAR Nickname[20];
		char SessionKey[64];

		buffer >> AccountNo;
		buffer.GetSTR(ID, 20);
		buffer.GetSTR(Nickname, 20);

		for (int i = 0; i < 64; i++) {
			buffer>>SessionKey[i];
		}

		//정상적이면 불가능한데 중복 로그인 정도 막는 걸로 
		bool find = false;
		for (auto it = loginWait.begin(); it != loginWait.end(); ++it) {
			if (it->id == id) {
				loginWait.erase(it);
				find = true;
				break;
			}
		}
		if (find)
		{
			auto newPlayer = new Player();
			newPlayer->lastEcho = chrono::system_clock::now();
			newPlayer->_sessionId = id;
			auto Inedx = id >> 47;
			newPlayer->AccountNo = AccountNo;
			newPlayer->_curX = -1;
			newPlayer->_curY = -1;
			wcscpy_s(newPlayer->ID,20, ID);
			wcscpy_s(newPlayer->Nickname,20, Nickname);
			memcpy_s(newPlayer->SessionKey, 64, SessionKey, 64);

			gplayers.insert({ newPlayer->_sessionId,newPlayer });

			auto& resBuffer = *CSerializeBuffer::Alloc();
			resBuffer << en_PACKET_CS_CHAT_RES_LOGIN << (BYTE)find << newPlayer->AccountNo;

			server.SendPacket(id, &resBuffer);
		}

	}
		break;

	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
	{
		auto player = gplayers.find(id);

		if (player == gplayers.end())
		{
			break;
		}
		auto& targetPlayer = *player->second;

		int64 accountNo;
		WORD sectorX;
		WORD sectorY;

		buffer >> accountNo >> sectorX >> sectorY;

		if (targetPlayer._curX == -1)
		{
			targetPlayer._curX = sectorX;
			targetPlayer._curY = sectorY;
			GMap.AddPlayer(targetPlayer);
		}
		else
		{
			GMap.MovePlayer(targetPlayer, sectorX, sectorY);
		}
		auto& resBuffer = *CSerializeBuffer::Alloc();

		resBuffer << en_PACKET_CS_CHAT_RES_SECTOR_MOVE << targetPlayer.AccountNo << targetPlayer._curX << targetPlayer._curY;

		server.SendPacket(id, &resBuffer);
	}
	break;
	case en_PACKET_CS_CHAT_REQ_MESSAGE: 
	{
		auto player = gplayers.find(id);

		if (player == gplayers.end())
		{
			break;
		}
		auto& targetPlayer = *player->second;

		int64 accountNo;
		String msg;

		buffer >> accountNo >> msg;

		auto& resBuffer = *CSerializeBuffer::Alloc();
		resBuffer << en_PACKET_CS_CHAT_RES_MESSAGE << targetPlayer.AccountNo;
		resBuffer.SetSTR(targetPlayer.ID, 20);
		resBuffer.SetSTR(targetPlayer.Nickname,20);
		resBuffer<< msg;
		GMap.Broadcast(targetPlayer, resBuffer);
	}
		break;

	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		break;

	default:
		break;
	}
}
