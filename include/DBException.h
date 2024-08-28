#pragma once
#include <chrono>
#include <exception>

class DBErr : public std::exception
{
public:
    DBErr(const char* errStr, const unsigned int errNo, const std::chrono::milliseconds dur)
        : _errString(errStr)
        , _errNo(errNo)
        , _dur(dur)
    {
    }

    std::string& getErrStr()
    {
        return _errString;
    }

    [[nodiscard]] unsigned int getErrCode() const
    {
        return _errNo;
    }

private:
    std::string _errString;
    unsigned int _errNo = 0;
    std::chrono::milliseconds _dur;
};
