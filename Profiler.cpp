#include "stdafx.h"
#include "Profiler.h"
#include <algorithm>
#include <chrono>
#include <winnt.h>
#include <fstream>
#include <ctime>
#include <string>
#include <thread>
#include <vector>

void Profiler::ProfileDataOutText(const LPWSTR szFileName)
{
	FILE* fOut;

	if(const auto openRet = _wfopen_s(&fOut, szFileName, L"wb"); openRet != 0 || fOut == nullptr)
	{
		WCHAR errBuffer[100];
		WCHAR buffer[100];
		_wcserror_s(buffer,openRet);
		swprintf_s(errBuffer, TEXT("errNo : %d | SettingParser::loadSetting, fopen %s"), openRet, buffer );
		return;
	}
	fwprintf_s(fOut, L"\t%10s\t\t\t\t%16s\t\t\t\t%10s\t\t\t%10s\t\t\t%10s\t\t\n",L"Name",L"Avg",L"Call",L"Min",L"Max");
	
	for (auto& sample : Profile_Samples)
	{
		if (sample.lFlag == false)
		{
			break;
		}

		auto tot = sample.iTotalTime;
		auto cnt = sample.iCall;
		if (sample.iCall > 2) 
		{
			tot -= sample.iMax;
			tot -= sample.iMin;
			cnt -= 2;
		}


		auto avg = tot / static_cast<double>(cnt); // 평균 틱에 1000을 곱해 ms 단위로 변경

		fwprintf_s(fOut, L"\t%10s\t\t\t\t%10.6fus\t\t\t\t%10lld\t\t\t%10lld\t\t\t%10lld\t\t\n", sample.szName
		           , avg.count(), sample.iCall, sample.iMin.count(), sample.iMax.count());
	}

	fclose(fOut);
}


int Profiler::GetItemNumber(const LPCWSTR name)
{

	for (int i = 0; i < MAX_ITEM; i++)
	{
		if (Profile_Samples[i].lFlag == false)
		{
			Profile_Samples[i].lFlag = true;
			Profile_Samples[i].iCall = 0;
			Profile_Samples[i].iTotalTime = 0us;
			Profile_Samples[i].iMin = 987654321us;
			Profile_Samples[i].iMax = -1us;


			memcpy_s(Profile_Samples[i].szName, MAX_NAME, name, MAX_NAME);
			return i;
		}

		if(wcscmp(Profile_Samples[i].szName,name)  == 0)
		{
			return i;
		}
	}
	return -1;
}

#define ITEM Profile_Samples[item_number]
void Profiler::ApplyProfile(const int& itemNumber, const std::chrono::microseconds time)
{
	Profile_Samples[itemNumber].iCall++;
	Profile_Samples[itemNumber].iTotalTime += time;

	if (time > Profile_Samples[itemNumber].iMax)
	{
		Profile_Samples[itemNumber].iMax = time;
	}
	if (time < Profile_Samples[itemNumber].iMin)
	{
		Profile_Samples[itemNumber].iMin = time;
	}
}
#undef ITEM




ProfileItem::ProfileItem(const LPCWSTR name)
{
	memcpy_s(_name, MAX_NAME, name, MAX_NAME);
	_start = std::chrono::steady_clock::now();
}

ProfileItem::~ProfileItem()
{
	_end = std::chrono::steady_clock::now();
	ProfileManager::Get().GetLocalProfiler().ApplyProfile(ProfileManager::Get().GetLocalProfiler().GetItemNumber(_name),
														  std::chrono::duration_cast<std::chrono::microseconds>(_end - _start));
}

bool cmp(const PROFILE_SAMPLE& lhs, const PROFILE_SAMPLE& rhs)
{
	return wcscmp(lhs.szName, rhs.szName) == -1;
}


std::wstring pad_string(const std::wstring& str, const size_t length) {
	if ( str.length() >= length ) {
		return str;
	}
	return str + std::wstring(length - str.length(), L' ');
}

int call_profile_manager_destructor()
{
	ProfileManager::Get().~ProfileManager();
	return 0;
}

void ProfileManager::ProfileDataOutText(const LPWSTR szFileName)
{
	AcquireSRWLockShared(&_profileListLock);

	std::vector<PROFILE_SAMPLE> samples;
	for (const auto profiler : _profilerList)
	{
		for (int i = 0; i < MAX_ITEM; i++)
		{
			if ( profiler->Profile_Samples[i].lFlag == false )
			{
				continue;
			}

			samples.push_back(profiler->Profile_Samples[i]);
		}
	}
	if (samples.empty())
	{
		return;
	}
	ReleaseSRWLockShared(&_profileListLock);


	FILE* fOut;

	if (const auto openRet = _wfopen_s(&fOut, szFileName, L"wb"); openRet != 0 || fOut == nullptr)
	{
		WCHAR errBuffer[100];
		WCHAR buffer[100];
		_wcserror_s(buffer, openRet);
		swprintf_s(errBuffer, TEXT("errNo : %d | SettingParser::loadSetting, fopen %s"), openRet, buffer);
		return;
	}
	if(!optionalText.empty())
	{
		fwprintf_s(fOut, L"%s\n",optionalText.c_str());
	}

	fwprintf_s(fOut, L"%35s\t|\tAvg\t|\tCall\t|\tMin\t|\tMax\t|\n",  L"Name");

	ranges::sort(samples, cmp);
	samples.emplace_back();

	const WCHAR* name = samples.begin()->szName;
	auto tot = std::chrono::microseconds(0);
	long long cnt = 0;

	auto min = std::chrono::microseconds(987654321);
	auto max = std::chrono::microseconds(-1);
	for (auto& sample : samples)
	{
		if(wcscmp(name,sample.szName) !=0)
		{
			auto avg = tot / static_cast<double>(cnt);
			fwprintf_s(fOut, L"%35s\t%20.6fus\t%20lld\t%20lld\t%20lld\n", name, avg.count(), cnt, min.count(), max.count());
//			fwprintf_s(fout, L"%s %.6fus\n", name, avg.count());

			name = sample.szName;
			cnt = 0;
			tot = std::chrono::microseconds(0);
			min = std::chrono::microseconds(987654321);
			max = std::chrono::microseconds(-1);
		}

		if (sample.iCall > 2)
		{
			tot -= sample.iMax;
			tot -= sample.iMin;
			cnt -= 2;
		}
		if(sample.iMax > max)
		{
			max = sample.iMax;
		}
		if (sample.iMin < min)
		{
			min = sample.iMin;
		}
		tot += sample.iTotalTime;
		cnt += sample.iCall;
	}


	fclose(fOut);



}

void ProfileManager::SetOptional(const std::wstring& optional)
{
	optionalText = optional;
}

