#pragma once

class EFileOpen : public std::exception
{

public:
	char const* what() const override
	{
		return "file open fail";
	}

};
class EFileLoad : public std::exception
{

public:
	char const* what() const override
	{
		return "file load fail";
	}

};

class EUnexpectOp : public std::exception
{

public:
	char const* what() const override
	{
		return "operation is invalid";
	}
};
class EInvalidGroup : public std::exception
{

public:
	char const* what() const override
	{
		return "group name is invalid";
	}

};