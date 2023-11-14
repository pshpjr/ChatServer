#include "stdafx.h"
#include "SettingParser.h"

#include <Windows.h>
#include <string.h>

#include <iostream>
#include <fstream>

#define WORD_START _buffer[_bufferIndex]
#define WORD_END _buffer[wordEnd]




SettingParser::ErrType SettingParser::init(LPCWSTR location)
{
	const auto loadResult = loadSetting(location);

	if (loadResult != ErrType::Succ)
		return loadResult;
	
	return parse();
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

SettingParser::ErrType SettingParser::loadSetting(LPCWSTR location)
{
	std::wifstream rawText{ location,std::ios::binary | std::ios::ate };

	if (!rawText.is_open())
	{
		return ErrType::FileOpenErr;
	}
	long fileSize = rawText.tellg();

	rawText.seekg(0);
	_buffer = (LPWCH)malloc(fileSize+2);
	bufferSize = fileSize + 2;

	memset(_buffer, 0, fileSize);

	rawText.read(_buffer, fileSize);
	auto count = rawText.gcount();
	//파일 읽고 문자열 끝에 null 추가
	if(count != fileSize)
	{
		return ErrType::FileReadErr;
	}

	_buffer[fileSize] = L'\0';

	return ErrType::Succ;
}

//텍스트 파일은 키 : 값 쌍으로 이루어짐
SettingParser::ErrType SettingParser::parse()
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
			return ErrType::OpErr;
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

	return ErrType::Succ;
}

bool SettingParser::getTok(LPWCH word)
{
	//의미 있는 문자까지 이동. 널문자를 만나도 탈출
	while (_bufferIndex < bufferSize)
	{
		if (WORD_START == L'﻿' || WORD_START == L'\n' || WORD_START == L' '  || WORD_START == 0x09 || WORD_START == 0x0a || WORD_START == 0x0d)
		{
			_bufferIndex++;
		}
		else
			break;
	}

	//널문자 만났는지 확인
	if (WORD_START == L'\0' || _bufferIndex == bufferSize)
		return false;

	size_t wordEnd = _bufferIndex+1;

	//의미 있는 문자가 한 줄 주석이면 다음 줄 시작까지 이동
	if (WORD_START == L'/' && WORD_END == L'/')
	{
		while (wordEnd < bufferSize)
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
		while (wordEnd < bufferSize)
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
		while (wordEnd < bufferSize)
		{
			if (WORD_END == L'"')
			{
				wordEnd++;
				break;
			}
			else
				wordEnd++;
		}

		if (_bufferIndex == bufferSize) {
			return false;
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
	while (wordEnd < bufferSize)
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
	if (_bufferIndex == bufferSize) {
		return false;
	}

	size_t wordSize = wordEnd - _bufferIndex;
	wcsncpy_s(word,WORD_SIZE, &WORD_START, wordSize);
	word[wordSize] = '\0';

	_bufferIndex = wordEnd;

	return true;
}
