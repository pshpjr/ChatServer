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

	template <typename T>
	void GetValue(String name, T& value)
	{
		auto result = getValue(name);
		
		if constexpr ( is_integral_v<T>)
		{
			value = stoi(result);
		}
		else if constexpr ( is_floating_point_v<T>)
		{
			value = stod(result);
		}
		else if constexpr ( is_same_v<T, String> )
		{
			value = result;
		}
		else if constexpr ( is_same_v<T, std::string> )
		{
			std::string str(result.length(), 0);
			std::transform(result.begin(), result.end(), str.begin(), 
						   [](wchar_t c)
						   {
							   return ( char ) c;
						   });
			value = move(str);
		}
		else if constexpr ( is_same_v<T, LPCWSTR> )
		{
			wcscpy_s(value, MAX_WORD_SIZE, result.c_str());
		}
		else
		{
			static_assert( std::is_integral<T>::value || std::is_floating_point<T>::value, "Unsupported type" );
		}

	}
	//void GetValue(const String name, OUT LPWSTR value);
	//void GetValue(const String name, OUT String& value);
	//void GetValue(const String name, OUT std::string& value);

	~SettingParser()
	{
		free(_buffer);
	}

private:
	String getValue(const String name);
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
	LPWSTR _buffer = nullptr;

	size_t _bufferIndex = 0;
	size_t bufferSize = 0;

	int32 _groupIndex = -1;
	String _groupsName[MAXGROUPSIZE];
	
	HashMap<String, String> _settingsContainer[MAXGROUPSIZE];
};

