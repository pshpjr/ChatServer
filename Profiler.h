﻿#pragma once

#include <chrono>
#include <list>
#include <Windows.h>
#include <thread>
class ProfileManager;
class ProfileItem;

static const int MAXNAME = 64;


struct PROFILE_SAMPLE {
	PROFILE_SAMPLE() :iTotalTime(0), iMin(987654321), iMax(-1) {  }
	WCHAR			szName[MAXNAME] = L"";
	long			lFlag = false;				// 프로파일의 사용 여부. (배열시에만)
		// 프로파일 샘플 이름.

	std::chrono::microseconds			iTotalTime;			// 전체 사용시간 카운터 Time.	(출력시 호출회수로 나누어 평균 구함)
	std::chrono::microseconds			iMin;			
	std::chrono::microseconds			iMax;			

	__int64			iCall = 0;				// 누적 호출 횟수.
};

class Profiler
{
	friend ProfileManager;
	friend ProfileItem;
public:
	void ProfileDataOutText(LPWSTR szFileName);

	void ProfileReset()
	{
		for (int i = 0; i < MAXITEM; ++i)
		{
			new (&Profile_Samples[i]) PROFILE_SAMPLE;
		}
	}
private:
	Profiler() {  };

	static const int MAXITEM = 64;

	int getItemNumber(LPCWSTR name);
	void applyProfile(const int& item_number, std::chrono::microseconds time);

	PROFILE_SAMPLE Profile_Samples[MAXITEM];
	int _size = 0;
};

class ProfileItem
{
public:
	ProfileItem(LPCWSTR name);
	~ProfileItem();

private:
	int _itemNumber = 0;
	std::chrono::system_clock::time_point  _start;
	std::chrono::system_clock::time_point  _end;
};

class ProfileManager
{
public:
	ProfileManager() : TLSNum(TlsAlloc()) { InitializeSRWLock(&_profileListLock); }
	~ProfileManager()
	{
		time_t timer = time(NULL);
		tm t;
		localtime_s(&t, &timer);
		WCHAR buffer[100];
		wsprintfW(buffer,
			L"profile_%d-%d-%d_%2d%2d_%x.txt",
			t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, std::this_thread::get_id());
		ProfileDataOutText(buffer);
		TlsFree(TLSNum);
	}
	Profiler& Get()
	{
		auto ret = (Profiler*)TlsGetValue(TLSNum);

		if (ret == nullptr)
		{
			AcquireSRWLockExclusive(&_profileListLock);

			ret = new Profiler;
			_profilerList.push_back(ret);
			TlsSetValue(TLSNum, ret);
			ReleaseSRWLockExclusive(&_profileListLock);
		}

		return *ret;
	}

	void ProfileDataOutText(LPWSTR szFileName);

	void ProfileReset(void)
	{
		AcquireSRWLockShared(&_profileListLock);
		for (auto& pro : _profilerList)
		{
			pro->ProfileReset();
		}
		ReleaseSRWLockShared(&_profileListLock);
	}
	void SetOptional(std::wstring optional);
private:
	std::list<Profiler*> _profilerList;
	SRWLOCK _profileListLock;
	std::wstring optionalText;
	const DWORD TLSNum;
};
extern ProfileManager GProfiler;

#define PROFILE

#ifdef PROFILE
#define PRO_BEGIN(TagName) ProfileItem t__COUNTER__(L#TagName);
#else

#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#endif