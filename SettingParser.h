#pragma once
#include "Types.h"
#include "Container.h"
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

	bool init(LPCWSTR location = L"serverSetting.txt");

	bool getValue(LPCTSTR name, OUT int32& value);
	bool getValue(LPCTSTR name, OUT LPTSTR value);
	bool getValue(LPCTSTR name, OUT String& value);

	~SettingParser()
	{
		free(_buffer);
	}


private:
	bool loadSetting(LPCTSTR location);
	bool parse();
	bool getTok(OUT LPTSTR word);

public:
	enum 
	{
		 WORD_SIZE = 40,
		 MAXERRLEN = 100,
		 MAXGROUPSIZE = 10,
	};

private:
	FILE* _settingStream =nullptr;

	LPWSTR _buffer = nullptr;

	size_t _bufferIndex = 0;

	int32 _groupIndex = -1;
	WCHAR _groupsName[MAXGROUPSIZE][WORD_SIZE] = { {0}, };
	
	HashMap<String, String> _settingsContainer[MAXGROUPSIZE];
};

