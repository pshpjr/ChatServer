#pragma once
#define _WINSOCKAPI_
#include <Windows.h>

class SpinLock
{
public:
	void Lock()
	{
		while (InterlockedExchange64(&flag,1) != 0)
		{
		}
	}

	void UnLock()
	{
		InterlockedExchange64(&flag, 0);
		//flag = 0;
	}

private:
	long long flag = 0;
};
