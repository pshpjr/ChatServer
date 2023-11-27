#pragma once
#include "Types.h"
#include "Container.h"
#include "SettingParserException.h"
/*
	파일 형식은

	그룹 :
	{
	키 : 값
	키 : 값
	}
	값은 띄어쓰기 단위로 구별한다. 
	값은 {}로 감싸면 안 된다
	 
 */
class SettingParser
{
public:

	enum class ErrType
	{
		Succ,
		FileOpenErr,
		FileReadErr,
		OpErr,
	};

	void Init(LPCWSTR location = L"serverSetting.txt");

	bool GetValue(LPCWSTR name, OUT int32& value);
	bool GetValue(LPCWSTR name, OUT LPWSTR value);
	bool GetValue(LPCWSTR name, OUT String& value);
	bool GetValue(LPCWSTR name, OUT std::string& value);

	~SettingParser()
	{
		free(_buffer);
	}


private:
	void loadSetting(LPCTSTR location);
	void parse();
	bool getTok(OUT LPTSTR word);

public:
	enum 
	{
		 MAX_WORD_SIZE = 100,
		 MAXERRLEN = 100,
		 MAXGROUPSIZE = 10,
	};

private:
	FILE* _settingStream =nullptr;

	LPWSTR _buffer = nullptr;

	size_t _bufferIndex = 0;
	size_t bufferSize = 0;

	int32 _groupIndex = -1;
	WCHAR _groupsName[MAXGROUPSIZE][MAX_WORD_SIZE] = { {0}, };
	
	HashMap<String, String> _settingsContainer[MAXGROUPSIZE];
};

