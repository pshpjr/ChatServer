#include "stdafx.h"
#include "SettingParser.h"

#include <Windows.h>
#include <string.h>

#include <iostream>
#include <fstream>


#define WORD_START _buffer[_bufferIndex]
#define WORD_END _buffer[wordEnd]


void SettingParser::Init(LPCWSTR location )
{
	loadSetting(location);
	parse();
}

void SettingParser::loadSetting(LPCTSTR location)
{
	std::wifstream rawText { location,std::ios::binary | std::ios::ate };

	if ( !rawText.is_open() )
	{
		throw EFileOpen();
	}

	size_t fileSize = rawText.tellg();

	rawText.seekg(0);
	_buffer = ( LPWCH ) malloc(fileSize * 2 + 2);
	bufferSize = fileSize + 1;

	rawText.read(_buffer, fileSize);
	auto count = rawText.gcount();
	//파일 읽고 문자열 끝에 null 추가
	if ( count != fileSize )
	{
		throw EFileLoad();
	}

	_buffer[fileSize] = L'\0';
}

//텍스트 파일은 키 : 값 쌍으로 이루어짐
void SettingParser::parse()
{
	WCHAR key[MAX_WORD_SIZE];
	WCHAR value[MAX_WORD_SIZE];
	WCHAR op[MAX_WORD_SIZE];

	while ( true )
	{

		if ( getTok(key) == false )
			break;

		//그룹 종료 예외 처리
		if ( key[0] == L'}' )
		{
			continue;
		}

		if ( getTok(op) == false )
			break;

		// ':' 위치에 다른 거 있으면 에러
		if ( op[0] != L':' )
		{
			throw EUnexpectOp();
		}

		if ( getTok(value) == false )
			break;

		//그룹 시작 예외 처리
		if ( value[0] == L'{' )
		{
			_groupIndex++;
			_groupsName[_groupIndex] = key;
			continue;
		}
		_settingsContainer[_groupIndex].insert({ key,value });
	}
}

bool SettingParser::getTok(LPWCH word)
{
	//의미 있는 문자까지 이동. 널문자를 만나도 탈출
	while ( _bufferIndex < bufferSize )
	{
		if ( WORD_START == L'﻿' || WORD_START == L'\n' || WORD_START == L' ' || WORD_START == 0x09 || WORD_START == 0x0a || WORD_START == 0x0d )
		{
			_bufferIndex++;
		}
		else
			break;
	}

	//널문자 만났는지 확인
	if ( WORD_START == L'\0' || _bufferIndex == bufferSize )
		return false;

	size_t wordEnd = _bufferIndex + 1;

	//의미 있는 문자가 한 줄 주석이면 다음 줄 시작까지 이동
	if ( WORD_START == L'/' && WORD_END == L'/' )
	{
		while ( wordEnd < bufferSize )
		{
			if ( WORD_END == '\n' )
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
	if ( WORD_START == L'/' && WORD_END == L'*' )
	{
		while ( wordEnd < bufferSize )
		{
			if ( WORD_END == L'*' && _buffer[wordEnd + 1] == L'/' )
			{
				wordEnd += 2;
				break;
			}
			else
				wordEnd++;
		}
		_bufferIndex = wordEnd;
		return getTok(word);
	}

	if ( WORD_START == L'"' )
	{
		//시작하는 " 문자열에 안 들어가게 하기 위한 처리
		_bufferIndex++;
		while ( wordEnd < bufferSize )
		{
			if ( WORD_END == L'"' )
			{
				wordEnd++;
				break;
			}
			else
				wordEnd++;
		}

		if ( _bufferIndex == bufferSize )
		{
			return false;
		}

		//끝나는 " 문자열에 안 들어가게 
		size_t wordSize = wordEnd - _bufferIndex - 1;

		wcsncpy_s(word, MAX_WORD_SIZE, &WORD_START, wordSize);
		_bufferIndex = wordEnd;
		return true;
	}


	//하나짜리 토큰이면 바로 리턴
	if ( WORD_START == L':' || WORD_START == L'{' || WORD_START == L'}' )
	{
		size_t wordSize = wordEnd - _bufferIndex;
		wcsncpy_s(word, MAX_WORD_SIZE, &WORD_START, wordSize);
		word[wordSize] = '\0';

		_bufferIndex = wordEnd;

		return true;
	}


	//긴 토큰 받는 곳
	while ( wordEnd < bufferSize )
	{
		if ( WORD_END == L'\n' || WORD_END == L' ' || WORD_END == 0x09 ||
			WORD_END == 0x0a || WORD_END == 0x0d || WORD_END == TEXT('\0') ||
			WORD_END == L':' || WORD_END == L'{' || WORD_END == L'}' )
		{
			break;
		}
		else
		{
			wordEnd++;
		}

	}
	if ( _bufferIndex == bufferSize )
	{
		return false;
	}

	size_t wordSize = wordEnd - _bufferIndex;
	wcsncpy_s(word, MAX_WORD_SIZE, &WORD_START, wordSize);
	word[wordSize] = '\0';

	_bufferIndex = wordEnd;

	return true;
}

String SettingParser::getValue(const String name)
{
	size_t groupEnd = name.find(L'.');

	auto nameStart = name.begin();
	String Group(nameStart, nameStart + groupEnd);
	String Name(nameStart + groupEnd + 1, name.end());

	for ( int i = 0; i < MAXGROUPSIZE; i++ )
	{
		if ( _groupsName[i].compare(Group) == 0 )
		{
			return _settingsContainer[i].find(Name)->second;
		}
	}
	throw EInvalidGroup();
}


//void SettingParser::GetValue(const String name, OUT String& value)
//{
//	auto result = getValue(name);
//
//	value = result->second;
//	return;
//}



//value 배열에 값을 복사한다. 이 때 value 배열의 크기는 SettingParser::MAX_WORD_SIZE를 사용한다. 
//void SettingParser::GetValue(const String name, LPWSTR value)
//{
//	auto result = getValue(name);
//
//	wcscpy_s(value, MAX_WORD_SIZE, result->second.c_str());
//}

