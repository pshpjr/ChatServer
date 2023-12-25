#pragma once
#include <exception>

class DBErr : public std::exception
{
public:
	DBErr(const char* errStr, int errNo, chrono::milliseconds dur) : _errString(errStr), _errNo(errNo),_dur(dur)
	{

	}

	std::string& getErrStr() { return _errString; }

	int getErrCode() const { return _errNo; }
private:
	std::string _errString;
	int _errNo = 0;
	chrono::milliseconds _dur;
};