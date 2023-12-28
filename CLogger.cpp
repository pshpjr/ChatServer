#include "stdafx.h"
#include "CLogger.h"
#include "strsafe.h"

//TODO: fileName에 로그 남기기.
CLogger::CLogger(LPCWCH fileName)
{
	setlocale(LC_ALL, "");
}

void CLogger::Write(const LPCWSTR type, LogLevel level, LPCWSTR format, ...)
{
	if (_logLevel > static_cast<int>(level))
	{
		return;
	}

	const time_t timer = time(nullptr);
	tm t;
	localtime_s(&t, &timer);

	WCHAR fileName[40];

	//파일 이름 실패시 로그 남기기
	if(const auto filenameResult = StringCchPrintfW(fileName, sizeof(fileName), L"%d-%d-%d_%s.txt",
	                                                t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, type); filenameResult != S_OK)
	{
		Write(L"SYSTEM", LogLevel::Error, L"LOGGER : 파일 이름 버퍼보다 이름이 큽니다.\n");
	}


	FILE* stream = _wfsopen(fileName, L"a+", _SH_DENYNO );
	if(stream ==nullptr)
	{
		printf("Cannot Open File!\n");
		return;
	}

	WCHAR logHeader[128];

	if (const auto headResult = StringCchPrintfW(logHeader, 256,
	                                             L"[%s] [%d-%d-%d %d:%d:%d / %s]\n",
	                                             type,
	                                             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
	                                             t.tm_hour, t.tm_min, t.tm_sec,
	                                             _dic[static_cast<int>(level)]); headResult != S_OK)
	{
		Write(L"SYSTEM", LogLevel::Error, L"LOGGER : 헤더 버퍼보다 헤더가 큽니다.\n");
	}

	WCHAR logBuffer[2048];
	va_list va;
	va_start(va, format);

	if(const auto logWriteResult = StringCchVPrintfW(logBuffer, 2048, format,va);
		logWriteResult != S_OK)
	{
		Write(L"SYSTEM", LogLevel::Error, L"로그 버퍼보다 로그가 큽니다.\n");
	}

	fwprintf(stream, L"%s %s\n", logHeader, logBuffer);

	fclose(stream);
}
