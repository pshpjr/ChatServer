#pragma once
#include <array>
#include <string>
#include <cstddef>
#include <WinSock2.h>
#include <Windows.h>

class Executable
{
    inline static std::array ioTypeToStr = {
        std::wstring(L"BASE"),
        std::wstring(L"SEND"),
        std::wstring(L"RECV"),
        std::wstring(L"POST_RECV"),
        std::wstring(L"RELEASE"),
        std::wstring(L"GROUP"),
        std::wstring(L"CUSTOM")
    };

    class OverlappedIO
    {
    public:
        OverlappedIO() {
            Clear();
        }

        void Clear() {
            memset(&_overlapped, 0, sizeof(_overlapped));
        }

        LPOVERLAPPED Get() {
            return &_overlapped;
        }

    private:
        OVERLAPPED _overlapped;
    };

public:
    virtual void Execute(ULONG_PTR arg, DWORD transferred, void* iocp) = 0;
    virtual ~Executable() = default;

     enum class IoType
     {
         Base = 0,
         Send,
         Recv,
         Postrecv,
         Release,
         Group,
         Custom
     };

    Executable(IoType type) : _type{type}
    {
    }

    void Clear()
    {
        _overlapped.Clear();
    }

    LPOVERLAPPED GetOverlapped() {
        return _overlapped.Get();
    }

    static Executable* GetExecutable(OVERLAPPED* overlap)
    {
        return (Executable*)((char*)overlap - offsetof(Executable, _overlapped));
    }

    static const std::wstring& GetIoType(const Executable& executable)
    {
        return ioTypeToStr[static_cast<char>(executable._type)];
    }

private:
    IoType _type{IoType::Base};
    OverlappedIO _overlapped{};
};

