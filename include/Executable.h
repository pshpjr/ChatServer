#pragma once
#include "Container.h"


namespace executable
{
    enum ExecutableTransfer
    {
        BASE = 1
        , POST
        , RECV
        , SEND
        , RELEASE
        , WAIT
        , GROUP
    };
}

class Executable
{
public:
    enum ioType
    {
        BASE = 1
        , SEND
        , RECV
        , POSTRECV
        , RELEASE
        , GROUP
        , CUSTOM
    };

    static const String GetIoType(Executable& executable)
    {
        return ioTypeToStr[executable._type];
    }


    Executable()
        : _type(CUSTOM)
        , _overlapped{0}
    {
    }

    virtual void Execute(ULONG_PTR arg, DWORD transferred, void* iocp) = 0;
    virtual ~Executable() = default;

    static Executable* GetExecutable(OVERLAPPED* overlap)
    {
        return (Executable*)((char*)overlap - offsetof(Executable, _overlapped));
    }

    void Clear()
    {
        memset(&_overlapped, 0, sizeof(_overlapped));
    }

    LPOVERLAPPED GetOverlapped()
    {
        return &_overlapped;
    }

    //Executable(const Executable& other) = delete;
    //Executable(Executable&& other) noexcept = delete;
    //Executable& operator=(const Executable& other) = delete;
    //Executable& operator=(Executable&& other) noexcept = delete;
    ioType _type;
    OVERLAPPED _overlapped;

private:
    inline static const Array<String, 8> ioTypeToStr = {
        L"empty"
        , L"BASE"
        , L"SEND"
        , L"RECV"
        , L"POST_RECV"
        , L"RELEASE"
        , L"GROUP"
        , L"CUSTOM"
    };
};
