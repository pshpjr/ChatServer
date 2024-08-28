#pragma once
#include <functional>
#include <string>
#include <memory>
#include "MyWindows.h"



using BYTE = unsigned char;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;
template <typename T> using  PoolPtr = std::unique_ptr<T,std::function<void(T*)>>;


using String = std::wstring;
using StringView = std::wstring_view;
//소켓 관련
#pragma warning (disable:4201)
class SessionID
{
public:
    union
    {
        struct
        {
            short tmp1;
            short tmp2;
            short tmp3;
            unsigned short index;
        };

        unsigned long long id;
    };

    friend bool operator==(const SessionID lhs, const SessionID rhs)
    {
        return lhs.id == rhs.id;
    }

    friend std::hash<SessionID>;
    friend std::equal_to<SessionID>;
};
#pragma warning (default:4201)

// using GroupID = volatile long;
//
// inline static long g_GroupID = 0;
// static GroupID NextID() { return GroupID(InterlockedIncrement(&g_GroupID)); }
// static consteval GroupID InvalidGroupID() { return GroupID{ -1 }; }


class GroupID
{
public:
    explicit constexpr GroupID(const int id) : _id(id)
    {
    }
    static GroupID NextID()
    {
        return GroupID(InterlockedIncrement(&g_GroupID));
    }

    static consteval GroupID InvalidGroupID()
    {
        return GroupID{-1};
    }

    explicit operator long() const
    {
        return _id;
    }

    friend bool operator==(const GroupID& lhs, const GroupID& rhs)
    {
        return lhs._id == rhs._id;
    }

    friend bool operator!=(const GroupID& lhs, const GroupID& rhs)
    {
        return !(lhs == rhs);
    }

    friend std::hash<GroupID>;
    friend std::equal_to<GroupID>;

private:
    long _id;
    inline static long g_GroupID = 0;
};

using SocketID = uint64;

namespace std
{
    template <>
    struct hash<GroupID>
    {
    public:
        size_t operator()(const GroupID& id) const noexcept
        {
            return std::hash<long>()(id._id);
        }
    };

    template <>
    struct hash<SessionID>
    {
    public:
        size_t operator()(const SessionID& id) const noexcept
        {
            return std::hash<unsigned long long>()(id.id);
        }
    };

    template <>
    struct equal_to<SessionID>
    {
    public:
        bool operator()(const SessionID& lhs, const SessionID& rhs) const
        {
            return lhs.id == rhs.id;
        }
    };
}


#include <WinSock2.h>
using SockAddr_in = SOCKADDR_IN;
using Port = uint16;

#include <variant>

#pragma once

template <typename T,typename E>
class Result
{
public:
    using valueType = T;
    using errorType = E;  // Assuming error codes are represented by integers

    // Enum to track the current state of the Result
    enum class State {
        HasValue,
        HasError
    };

    Result(valueType value)
        : state_(State::HasValue), variant_(std::move(value)) {
    }

    Result(errorType error)
        : state_(State::HasError), variant_(error) {
    }

    bool HasValue() const {
        return state_ == State::HasValue;
    }

    valueType Value() {
        if (state_ == State::HasValue) {
            return std::get<valueType>(variant_);
        } else {
            throw std::exception("Error: Result does not contain a value");
        }
    }


    bool HasError() const {
        return state_ == State::HasError;
    }

    errorType Error() {
        if (state_ == State::HasError) {
            return std::get<errorType>(variant_);
        } else {
            throw std::exception("Error: Result does not contain an error");
        }
    }

private:
    State state_;
    std::variant<valueType, errorType> variant_;
};

template <typename T>
using WSAResult = Result<T, int>;
