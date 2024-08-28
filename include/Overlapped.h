#pragma once

class Overlapped
{
public:
    enum class Type
    {
        Recv
        , Send
        ,
    };

    OVERLAPPED overlapped = {0,};
    Type type;
};
