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
	SettingParser() : _groupsName(MAXGROUPSIZE),_settingsContainer(MAXGROUPSIZE) { }
	enum class ErrType
	{
		Success,
		FileOpenErr,
		FileReadErr,
		OpErr,
	};

	void Init(LPCWSTR location = L"serverSetting.txt");

	template <typename T>
	void GetValue(const String name, T& value)
	{
		auto result = GetValueImpl(name);
		
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
						   [](const wchar_t c)
						   {
							   return static_cast<char>(c);
						   });
			value = move(str);
		}
		else if constexpr ( is_same_v<T, LPCWSTR> )
		{
			wcscpy_s(value, MAX_WORD_SIZE, result.c_str());
		}
		else
		{
			static_assert( std::is_integral_v<T> || std::is_floating_point_v<T>, "Unsupported type" );
		}

	}
	//void GetValue(const String name, OUT LPWSTR value);
	//void GetValue(const String name, OUT String& value);
	//void GetValue(const String name, OUT std::string& value);

	~SettingParser() = default;

private:
	String GetValueImpl(const String& name);
	void LoadSetting(LPCTSTR location);
	void Parse();
	bool GetTok(OUT LPTSTR word);


private:
	static constexpr int MAX_WORD_SIZE = 100;
	static constexpr int MAXERRLEN = 100;
	static constexpr int MAXGROUPSIZE = 10;
	
	LPWSTR _buffer = nullptr;

	size_t _bufferIndex = 0;
	size_t bufferSize = 0;

	int32 _groupIndex = -1;
	vector<String> _groupsName;
	
	vector<HashMap<String, String>> _settingsContainer;
};

