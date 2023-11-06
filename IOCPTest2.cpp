 // IOCPTest2.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "stdafx.h"

#include <chrono>

#include "Server.h"
#include <chrono>
#include "CommonProtocol.h"
#include "Player.h"
#include "CMap.h"



void printLog(Server& server);
void HandlePacket(CSerializeBuffer& buffer,SessionID id, Server& server);

void LogFunc(Server& server) {
	while (!server.isEnd()) {
		printLog(server);
		Sleep(1000);
	}
}
const int debugSize = 1000;
struct debugJob {
	char input[debugSize];
	int debugIndex = 0;
};





int main()
{
	timeBeginPeriod(1);
	srand(GetCurrentThreadId());
	Server& server = *new Server;

	server.Init(L"0.0.0.0", 11777,20,4, 0x32);

	auto logThread = thread(LogFunc, ref(server));

	while (!server.isEnd())
	{
		server._packetQueue.Swap();

		ContentJob* job = nullptr;

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
			{
				bool find = false;
				for (auto it = loginWait.begin(); it != loginWait.end();++it) {
					if (it->id == job->_id) 
					{
						loginWait.erase(it);
						find = true;
						break;
					}
				}

				if (find)
					break;

				auto result = gplayers.find(job->_id);
				if (result == gplayers.end()) {
					DebugBreak();
				}
					
				auto& player = *result->second;
				player.getDisconnect = true;
				gplayers.erase(result);

				if(player._curX != -1)
					GMap.DeletePlayerFromSector(player);

				delete &player;
				break; 
			}

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
	}
	if (logThread.joinable())
		logThread.join();

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
		WCHAR ID[20] = { 0, };
		WCHAR Nickname[20] = { 0, };
		char SessionKey[64] = { 0, };

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
			resBuffer.Release();
		}
		else {
			server.DisconnectSession(id);
		}
	}
		break;

	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
	{
		auto player = gplayers.find(id);

		if (player == gplayers.end())
		{
			DebugBreak();
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
		resBuffer.Release();
	}
	break;
	case en_PACKET_CS_CHAT_REQ_MESSAGE: 
	{
		auto player = gplayers.find(id);

		if (player == gplayers.end())
		{
			server.DisconnectSession(id);
			DebugBreak();
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
		resBuffer.Release();
	}
		break;

	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		break;

	default:
		break;
	}
}

void userInput(Server& server) {
	static bool keyWait = false;
	printf("keyWait : %s, toggle : i, stop : s\n", keyWait ? "True" : "False");
	if (keyWait) 
	{
		if (GetAsyncKeyState('I')) {
			keyWait = false;
		}
		if (GetAsyncKeyState('S')) {
			server.Stop();
		}
		if (GetAsyncKeyState('D')) {
			DebugBreak();
		}
	}
	else
	{
		if (GetAsyncKeyState('I')) {
			keyWait = true;
		}
	}

}


void printLog(Server& server) {
	userInput(server);
	printf("==================================\n");
	printf("+              LIBS              +\n");
	printf("==================================\n");
	printf("   Sessions : %u                \n", server.GetSessions());
	printf(" PacketPool : %lld, PoolEmpty : %u\n", server.GetPacketPoolSize(), server.GetPacketPoolEmptyCount());
	printf("  AcceptTot : %lld, AcceptTps : %lld\n", server.GetAcceptCount(), server.GetAcceptTps());
	printf("  RecvTPS   : %lld, SendTPS   : %lld\n", server.GetRecvTps(), server.GetSendTps());


	auto loginSize = loginWait.size();
	auto players = gplayers.size();

	printf("==================================\n");
	printf("+            Contents            +\n");
	printf("==================================\n");
	printf("   login    : %lld, Players : %lld\n", loginSize, players);
	printf(" connect    :%lld,QueueSize : %lld\n", loginSize + players,server._packetQueue.Size());
	printf("    Queue   : %lld\n", server._packetQueue.Size());


	printf("==================================\n");
}