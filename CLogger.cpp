#include "stdafx.h"
#include "CLogger.h"
#include "strsafe.h"

CLogger::CLogger(LPCWCH fileName)
{
	setlocale(LC_ALL, "");
}

void CLogger::write(LPCWSTR type, LogLevel level, LPCWSTR format, ...)
{
	if (_logLevel > (int)level)
	{
		return;
	}

	time_t timer = time(NULL);
	tm t;
	localtime_s(&t, &timer);

	WCHAR fileName[40];

	auto filenameResult = StringCchPrintfW(fileName, sizeof(fileName), L"%d-%d-%d_%s.txt",
		t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, type);	
	//파일 이름 실패시 로그 남기기
	if(filenameResult != S_OK)
	{
		write(L"SYSTEM", LogLevel::Error, L"LOGGER : 파일 이름 버퍼보다 이름이 큽니다.\n");
	}


	FILE* stream;
	auto openRet = _wfopen_s(&stream, fileName, L"a+");
	if(openRet != 0)
	{
		printf("Cannot Open File!\n");
		return;
	}

	WCHAR logHeader[128];

	auto headResult = StringCchPrintfW(logHeader, 256,
		L"[%s] [%d-%d-%d %d:%d:%d / %s]\n",
		type,
		t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
		t.tm_hour, t.tm_min, t.tm_sec,
		_dic[(int)level]);

	if (headResult != S_OK)
	{
		write(L"SYSTEM", LogLevel::Error, L"LOGGER : 헤더 버퍼보다 헤더가 큽니다.\n");
	}

	WCHAR logBuffer[256];
	va_list va;
	va_start(va, format);
	
	auto logWriteResult = StringCchVPrintfW(logBuffer, 256, format,va);
	if(logWriteResult != S_OK)
	{
		write(L"SYSTEM", LogLevel::Error, L"로그 버퍼보다 로그가 큽니다.\n");
	}

	fwprintf(stream, L"%s %s\n", logHeader, logBuffer);

	fclose(stream);
}
