// IOCPTest2.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "stdafx.h"

#include <chrono>

#include "Server.h"
#pragma comment(lib, "winmm.lib")
#include<thread>
int main()
{
	timeBeginPeriod(1);

	Server iocp;

	iocp.Init(L"0.0.0.0",11777,20,4);

	int gameLoop = 0;
	int MaxLoopMs = 0;
	auto targetTime = std::chrono::system_clock::now()+chrono::milliseconds(20);
	auto secLoop = targetTime + std::chrono::seconds(2);
	bool getCommand = false;

	while (true)
	{

		iocp.Update();

		auto now = std::chrono::system_clock::now();

		if (now >= secLoop)
		{
			secLoop += std::chrono::seconds(1);
			printf("-------------------------\n");
			printf("InputState : %d\n", getCommand);
			printf("game : %d Max : %d\n", gameLoop,MaxLoopMs);
			printf("AcceptTps : %lld SendTps : %lld RecvTps : %lld \n", iocp.GetAcceptTps(),iocp.GetSendTps(),iocp.GetRecvTps());
			printf("Player: %d\n", iocp.GetPlayerCount());
			printf("-------------------------\n");
			gameLoop = 0;
			MaxLoopMs = 9999;
		}
		if(GetAsyncKeyState(VK_UP))
		{
			getCommand = true;
		}
		if (GetAsyncKeyState(VK_DOWN))
		{
			getCommand = true;
		}

		if(getCommand)
		{
			if (GetAsyncKeyState('E'))
			{
				iocp.Stop();
				break;
			}
		}

		gameLoop++;
		targetTime += chrono::milliseconds(20);
		auto sleepTime =std::chrono::duration_cast<std::chrono::milliseconds>(targetTime - now);

		MaxLoopMs = min(MaxLoopMs, sleepTime.count());

		if (sleepTime.count()<0)
			sleepTime = std::chrono::milliseconds(0);

		Sleep(sleepTime.count());
	}


}

enum iotype {
	push,
	pop
};
struct debugData
{
	iotype type;
	unsigned long long oldTop;
	unsigned long long newTop;
	int data;
	long long Count;
	std::thread::id tId;
};

// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.
