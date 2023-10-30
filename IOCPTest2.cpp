// IOCPTest2.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "stdafx.h"

#include <chrono>

#include "Server.h"
#include <chrono>


int main()
{
	timeBeginPeriod(1);

	Server& server = *new Server;

	server.Init(L"0.0.0.0", 6000,20,2,0);

	while (true) 
	{
		server._packetQueue.Swap();

		ContentJob* job;

		while (true) 
		{
			job = server._packetQueue.Dequeue();

			if (job == nullptr) 
			{
				break;
			}


			switch (job->_type)
			{
			case ContentJob::ePacketType::Connect:
				break;
			case ContentJob::ePacketType::Disconnect:
				break;
			case ContentJob::ePacketType::TimeoutCheck:
				break;
			case ContentJob::ePacketType::Packet:
			{
				int64 data;
				auto& sendBuffer = *CSerializeBuffer::Alloc();
				*job->_buffer >> data;

				sendBuffer << data;

				server.SendPacket(job->_id, &sendBuffer);
				break;
			}
			default:
				DebugBreak();
			}

			job->Free();


		}

		Sleep(20);
	}

}


// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.
