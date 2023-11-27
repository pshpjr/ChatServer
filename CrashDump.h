#pragma once

#include <psapi.h>
#include <handleapi.h>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")

class CrashDump
{
public:
	CrashDump()
	{
		_invalid_parameter_handler oldHandler, newHandler;
		newHandler = InvalidParamHandler;

		oldHandler = _set_invalid_parameter_handler(newHandler);

		_CrtSetReportMode(_CRT_WARN, 0);
		_CrtSetReportMode(_CRT_ASSERT, 0);
		_CrtSetReportMode(_CRT_ERROR, 0);

		_CrtSetReportHook(_custom_Report_hook);

		_set_purecall_handler(PurecallHandler);

		SetHandler();
	}

	static void Crash()
	{
		int* p = nullptr;
		p = 0;
	}

	static LONG WINAPI ExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer)
	{
		size_t iWorkingMemory = 0;
		SYSTEMTIME st;

		long DumpCount = InterlockedIncrement(&_dumpCount);

		HANDLE hProcess = GetCurrentProcess();
		if (hProcess == NULL)
			return 0;

		PROCESS_MEMORY_COUNTERS pmc;

		if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
		{
			iWorkingMemory = pmc.WorkingSetSize;
		}
		CloseHandle(hProcess);

		WCHAR szFileName[MAX_PATH] = { 0, };
		GetLocalTime(&st);
		wsprintf(szFileName, L"DUMP_%04d%02d%02d_%02d%02d%02d_%d_%dMB.dmp",
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, DumpCount, iWorkingMemory);

		wprintf(L"\n\n\t\t!!!!!! CRASH DUMP !!!!!  %04d%02d%02d_%02d%02d%02d\n\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		wprintf(L"Save Dump File : %s\n", szFileName);

		HANDLE hFile = CreateFile(
			szFileName,
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_EXCEPTION_INFORMATION eInfo {};
			eInfo.ThreadId = GetCurrentThreadId();
			eInfo.ExceptionPointers = pExceptionPointer;
			eInfo.ClientPointers = TRUE;

			MiniDumpWriteDump(GetCurrentProcess(),
				GetCurrentProcessId(),
				hFile,
				MiniDumpWithFullMemory,
				&eInfo,
				NULL,
 NULL);
			CloseHandle(hFile);

			wprintf(L"Dump File Saved\n\n\n");
		}
		return EXCEPTION_EXECUTE_HANDLER;
	}

	static void SetHandler()
	{
		SetUnhandledExceptionFilter(ExceptionFilter);
	}

	static void InvalidParamHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
	{
		Crash();
	}

	static int _custom_Report_hook(int reportType, char* message, int* returnValue)
	{
		Crash();
		return true;
	}

	static void PurecallHandler()
	{
		Crash();
	}

	static long _dumpCount;
};