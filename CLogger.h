#pragma once
#include <clocale>
#define _WINSOCKAPI_
#include <Windows.h>
//TODO: 멀티스레드에서 안전하게 동작하도록 수정.


enum class LogLevel
{
	Debug,
	Invalid,
	Error,
	System,

};

class CLogger
{
public:
	CLogger(LPCWCH fileName = L"");

	void Write(LPCWSTR type, LogLevel level, LPCWSTR format, ...);

private:
	WCHAR _dic[4][8] = { L"Debug",L"Invalid",L"Error",L"System"};

	int _logLevel = 0;
	
};

