//
// Created by Park on 24. 8. 16.
//
#include "Types.h"
#ifndef SESSIONDATA_H
#define SESSIONDATA_H


consteval SessionID InvalidSessionID()
{
    return {-1, 0};
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

#endif //SESSIONDATA_H
