#include "SettingParser.h"
#include "CLogger.h"
#include <Windows.h>
#include <string.h>

#include "CoreGlobal.h"
#include <iostream>
#include <fstream>

#define WORD_START _buffer[_bufferIndex]
#define WORD_END _buffer[wordEnd]

bool SettingParser::init(LPCWSTR location)
{
	const bool isSettingLoaded = loadSetting(location);

	if (isSettingLoaded == false)
		return false;

	parse();

	return true;
}

bool SettingParser::getValue(LPCTSTR name, OUT String& value)
{
	int groupEnd = 0;

	for (int i = 0; name[i] != L'\0'; i++) {
		if (name[i] == '.')
		{
			groupEnd = i;
		}
	}

	TCHAR Group[WORD_SIZE];
	TCHAR Name[WORD_SIZE];

	wcsncpy_s(Group, name, groupEnd);
	wcscpy_s(Name, &name[groupEnd + 1]);


	for (int i = 0; i < MAXGROUPSIZE; i++)
	{
		if (wcscmp(_groupsName[i], Group) == 0)
		{
			auto result = _settingsContainer[i].find(Name);
			if (result == _settingsContainer[i].end())
				return false;
			value = result->second;
			return true;
		}
	}
	return false;
}

bool SettingParser::getValue(LPCWSTR name, int32& value)
{
	int groupEnd = 0;

	for (int i = 0; name[i] != L'\0'; i++) {
		if (name[i] == '.')
		{
			groupEnd = i;
		}
	}

	TCHAR Group[WORD_SIZE];
	TCHAR Name[WORD_SIZE];

	wcsncpy_s(Group, name, groupEnd);
	wcscpy_s(Name, &name[groupEnd + 1]);


	for (int i = 0; i < MAXGROUPSIZE; i++)
	{
		if (wcscmp(_groupsName[i], Group) == 0)
		{
			auto result = _settingsContainer[i].find(Name);
			if (result == _settingsContainer[i].end())
				return false;
			value = stoi(result->second.c_str());
			return true;
		}
	}
	return false;
}

//value 배열에 값을 복사한다. 이 때 value 배열의 크기는 SettingParser::WORD_SIZE를 사용한다. 
bool SettingParser::getValue(LPCWSTR name, LPTSTR value)
{
	int groupEnd = 0;
	
	for(int i = 0; name[i] != L'\0'; i++){
		if(name[i] == '.')
		{
			groupEnd = i;
		}
	}

	TCHAR Group[WORD_SIZE];
	TCHAR Name[WORD_SIZE];

	wcsncpy_s(Group, name, groupEnd);
	wcscpy_s(Name, &name[groupEnd + 1]);


	for(int i =0; i<MAXGROUPSIZE;i++)
	{
		if(wcscmp(_groupsName[i], Group) == 0)
		{
			auto result = _settingsContainer[i].find(Name);
			if (result == _settingsContainer[i].end())
				return false;
			wcscpy_s(value, WORD_SIZE, result->second.c_str());
			return true;
		}
	}
	return false;
}

bool SettingParser::loadSetting(LPCWSTR location)
{
	std::wifstream rawText{ location,std::ios::binary | std::ios::ate };

	if (!rawText.is_open())
	{
		GLogger->write(L"Parser",LogLevel::Error, L" SettingParser::loadSetting, fopen ");
		return false;
	}
	long fileSize = rawText.tellg();

	rawText.seekg(0);
	_buffer = (LPWCH)malloc(fileSize+2);

	memset(_buffer, 0, fileSize);

	rawText.read(_buffer, fileSize);
	auto count = rawText.gcount();
	//파일 읽고 문자열 끝에 null 추가
	if(count != fileSize)
	{
		GLogger->write(L"Parser", LogLevel::Error, L"SettingParser::loadSetting, fread ");
		return false;
	}

	_buffer[fileSize] = L'\0';

	return true;
}

//텍스트 파일은 키 : 값 쌍으로 이루어짐
bool SettingParser::parse()
{
	WCHAR key[WORD_SIZE];
	WCHAR value[WORD_SIZE];
	WCHAR op[WORD_SIZE];

	while(true)
	{

		if (getTok(key) == false)
			break;

		//그룹 종료 예외 처리
		if(key[0] == '}')
		{
			continue;
		}
		
		if (getTok(op) == false)
			break;

		// ':' 위치에 다른 거 있으면 에러
		if(op[0] != ':')
		{
			GLogger->write(L"Parser", LogLevel::Error, L"parseError: Invalid Operand. Incorrect file syntax may occur this problem.");
			return false;
		}

		if (getTok(value) == false)
			break;

		//그룹 시작 예외 처리
		if(value[0] == '{')
		{
			_groupIndex++;
			wcscpy_s(_groupsName[_groupIndex], WORD_SIZE, key);
			continue;
		}
		_settingsContainer[_groupIndex].insert({key,value});
	}

	return true;
}

bool SettingParser::getTok(LPWCH word)
{
	//의미 있는 문자까지 이동. 널문자를 만나도 탈출
	while (true)
	{
		if (WORD_START == L'﻿' || WORD_START == L'\n' || WORD_START == L' '  || WORD_START == 0x09 || WORD_START == 0x0a || WORD_START == 0x0d)
		{
			_bufferIndex++;
		}
		else
			break;
	}

	//널문자 만났는지 확인
	if (WORD_START == L'\0')
		return false;

	size_t wordEnd = _bufferIndex+1;

	//의미 있는 문자가 한 줄 주석이면 다음 줄 시작까지 이동
	if (WORD_START == L'/' && WORD_END == L'/')
	{
		while (true)
		{
			if (WORD_END == '\n')
			{
				wordEnd++;
				break;
			}
			else
				wordEnd++;
		}
		_bufferIndex = wordEnd;
		return getTok(word);
	}

	//여러 줄 주석이면 */ 만날 때 까지 이동
	if (WORD_START == L'/' && WORD_END == L'*')
	{
		while (true)
		{
			if (WORD_END == L'*' && _buffer[wordEnd+1] == L'/')
			{
				wordEnd+=2;
				break;
			}
			else
				wordEnd++;
		}
		_bufferIndex = wordEnd;
		return getTok(word);
	}

	if (WORD_START == L'"')
	{
		//시작하는 " 문자열에 안 들어가게 하기 위한 처리
		_bufferIndex++;
		while (true)
		{
			if (WORD_END == L'"')
			{
				wordEnd++;
				break;
			}
			else
				wordEnd++;
		}

		//끝나는 " 문자열에 안 들어가게 
		size_t wordSize = wordEnd - _bufferIndex-1;

		wcsncpy_s(word, WORD_SIZE, &WORD_START, wordSize);
		_bufferIndex = wordEnd;
		return true;
	}


	//하나짜리 토큰이면 바로 리턴
	if(WORD_START == L':' || WORD_START == L'{' || WORD_START == L'}' )
	{
		int32 wordSize = wordEnd - _bufferIndex;
		wcsncpy_s(word, WORD_SIZE, &WORD_START, wordSize);
		word[wordSize] = '\0';

		_bufferIndex = wordEnd;

		return true;
	}


	//긴 토큰 받는 곳
	while (true)
	{
		if (WORD_END == L'\n' || WORD_END == L' ' || WORD_END == 0x09 ||
			WORD_END == 0x0a || WORD_END == 0x0d || WORD_END == TEXT ('\0') ||
			WORD_END == L':' || WORD_END == L'{' || WORD_END == L'}')
		{
			break;
		}
		else 
		{
			wordEnd++;
		}

	}


	size_t wordSize = wordEnd - _bufferIndex;
	wcsncpy_s(word,WORD_SIZE, &WORD_START, wordSize);
	word[wordSize] = '\0';

	_bufferIndex = wordEnd;

	return true;
}
