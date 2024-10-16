//
// Created by Park on 24. 8. 16.
//

#ifndef SESSIONDATA_H
#define SESSIONDATA_H

#include "Container.h"
#include "Utility.h"

//세션 관련

#pragma warning (disable:4201)
class SessionID
{
public:
    union
    {
        struct
        {
            unsigned short tmp1;
            unsigned short tmp2;
            unsigned short tmp3;
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

consteval SessionID InvalidSessionID()
{
    return {0xffff, 0xffff, 0xffff, 0xffff};
}

struct timeoutInfo
{
    enum class IOtype
    {
        recv
        , send
    };

    IOtype type;
    long long waitTime = 0;
    int timeoutTime = 0;
};

namespace std
{
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

template <typename data>
using SessionMap = HashMap<SessionID, data>;

using SessionSet = HashSet<SessionID>;

#endif //SESSIONDATA_H
