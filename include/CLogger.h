#pragma once
#include <clocale>
#include "MyWindows.h"
#include "Types.h"
//TODO: 멀티스레드에서 안전하게 동작하도록 수정.


class CLogger
{
public:
    enum class LogLevel
    {
        Debug
        , Invalid
        , Error
        , System
    };

    CLogger(LPCWSTR fileName = L"", LogLevel level = LogLevel::Invalid);

    void Write(LPCWSTR type, LogLevel level, LPCWSTR format, ...);

private:
    WCHAR _dic[4][8] = {L"Debug", L"Invalid", L"Error", L"System"};

    LogLevel _logLevel;
    String _extraFileName;
};
