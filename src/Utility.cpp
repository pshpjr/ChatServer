//
// Created by Park on 24. 8. 22.
//

#include "Utility.h"

#include <codecvt>
#include <locale>

#include "concurrentqueue.h"
#include "dpp/exception.h"

std::string psh::util::WToS(const std::wstring &wstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t> > converter;
    return converter.to_bytes(wstr);
}

// 디버거에게 스레드 이름을 전달하는 예외 처리
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO {
    DWORD dwType; // 반드시 0x1000이어야 합니다.
    LPCSTR szName; // 스레드 이름
    DWORD dwThreadID; // 스레드 ID (-1로 설정하면 호출한 스레드)
    DWORD dwFlags; // 예약 필드, 0으로 설정
} THREADNAME_INFO;
#pragma pack(pop)

void psh::util::set_thread_name(const char *threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = GetCurrentThreadId();
    info.dwFlags = 0;

    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *) &info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}
