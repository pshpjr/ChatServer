#pragma once
#include <exception>
#include <chrono>
class DBErr : public std::exception
{
public:
	DBErr(const char* errStr, const int errNo, const chrono::milliseconds dur) : _errString(errStr), _errNo(errNo),_dur(dur)
	{

	}

	std::string& getErrStr() { return _errString; }

	int getErrCode() const { return _errNo; }
private:
	std::string _errString;
	int _errNo = 0;
	chrono::milliseconds _dur;
};