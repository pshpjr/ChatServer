#pragma once
#include <exception>

class DBErr : public std::exception
{
public:
	DBErr(const char* errStr) : _errString(errStr)
	{

	}

	std::string& getErrStr() { return _errString; }

private:
	std::string _errString;
};