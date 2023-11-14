
#include "Profiler.h"

#include <algorithm>
#include <chrono>
#include <Windows.h>
#include <fstream>
#include <ctime>
#include <string>
#include <thread>
#include <vector>

ProfileManager GProfiler;
/*
 * 	TCHAR errBuffer[MAXERRLEN];

	//텍스트 모드로 열 경우 ftell의 값과 파일 크기가 다를 수 있다. 
	const auto openRet = _tfopen_s(&_settingStream, location, TEXT("rb"));
	if (openRet != 0 || _settingStream == nullptr)
	{
		_stprintf_s(errBuffer, TEXT("errNo : %d | SettingParser::loadSetting, fopen "), openRet);
		GLogger->write(errBuffer);
		return false;
	}
 */
void Profiler::ProfileDataOutText(LPWSTR szFileName)
{
	FILE* fout;

	const auto openRet = _wfopen_s(&fout, szFileName, L"wb");
	if(openRet != 0 || fout == nullptr)
	{
		WCHAR errBuffer[100];
		WCHAR buffer[100];
		_wcserror_s(buffer,openRet);
		swprintf_s(errBuffer, TEXT("errNo : %d | SettingParser::loadSetting, fopen %s"), openRet, buffer );
		return;
	}
	fwprintf_s(fout, L"\t\tName\t\t|\t\t\tAvg\t\t\t|\t\tCall\t\t|\t\tMin\t\t|\t\tMax\t\t|\t\t\n");
	
	for (auto& sample : Profile_Samples)
	{
		if (sample.lFlag == false)
			break;

		auto tot = sample.iTotalTime;
		auto cnt = sample.iCall;
		if (sample.iCall > 2) 
		{
			tot -= sample.iMax;
			tot -= sample.iMin;
			cnt -= 2;
		}


		auto avg = tot / ((double)cnt); // 평균 틱에 1000을 곱해 ms 단위로 변경

		fwprintf_s(fout, L"\t%10s\t\t\t\t%10.6fus\t\t\t\t%10lld\t\t\t%10lld\t\t\t%10lld\t\t\n", sample.szName, avg.count(), sample.iCall, sample.iMin.count() , sample.iMax.count());
	}

	fclose(fout);
}


int Profiler::getItemNumber(LPCWSTR name)
{

	for (int i = 0; i < MAXITEM; i++)
	{
		if (Profile_Samples[i].lFlag == false)
		{
			Profile_Samples[i].lFlag = true;
			memcpy_s(Profile_Samples[i].szName, MAXNAME, name, MAXNAME);
			_size++;
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
void Profiler::applyProfile(const int& item_number, std::chrono::microseconds time)
{
	Profile_Samples[item_number].iCall++;
	Profile_Samples[item_number].iTotalTime += time;

	if (time > Profile_Samples[item_number].iMax)
	{
		Profile_Samples[item_number].iMax = time;
	}
	if (time < Profile_Samples[item_number].iMin)
	{
		Profile_Samples[item_number].iMin = time;
	}
}
#undef ITEM




ProfileItem::ProfileItem(LPCWSTR name)
{
	_itemNumber = GProfiler.Get().getItemNumber(name);
	_start = std::chrono::system_clock::now();
}

ProfileItem::~ProfileItem()
{
	_end = std::chrono::system_clock::now();
	GProfiler.Get().applyProfile(_itemNumber, std::chrono::duration_cast<std::chrono::microseconds>(_end - _start));
}

bool cmp(PROFILE_SAMPLE& lhs, PROFILE_SAMPLE& rhs)
{
	return wcscmp(lhs.szName, rhs.szName) == -1;
}


void ProfileManager::ProfileDataOutText(LPWSTR szFileName)
{
	AcquireSRWLockShared(&_profileListLock);

	std::vector<PROFILE_SAMPLE> samples;
	for (auto profiler : _profilerList)
	{
		for (int i = 0; i < profiler->_size; i++)
		{
			samples.push_back(profiler->Profile_Samples[i]);
		}
	}
	if (samples.empty())
	{
		return;
	}



	FILE* fout;

	const auto openRet = _wfopen_s(&fout, szFileName, L"wb");
	if (openRet != 0 || fout == nullptr)
	{
		WCHAR errBuffer[100];
		WCHAR buffer[100];
		_wcserror_s(buffer, openRet);
		swprintf_s(errBuffer, TEXT("errNo : %d | SettingParser::loadSetting, fopen %s"), openRet, buffer);
		return;
	}
	//if(!optionalText.empty())
	//{
	//	fwprintf_s(fout, L"%s\n",optionalText.c_str());
	//}

	//fwprintf_s(fout, L"\t\tName\t\t|\t\t\tAvg\t\t\t|\t\tCall\t\t|\t\tMin\t\t|\t\tMax\t\t|\t\n");


	std::sort(samples.begin(), samples.end(), cmp);
	samples.emplace_back();

	const WCHAR* name = samples.begin()->szName;
	std::chrono::microseconds tot = std::chrono::microseconds(0);
	long long cnt = 0;

	std::chrono::microseconds min = std::chrono::microseconds(987654321);
	std::chrono::microseconds max = std::chrono::microseconds(-1);
	for (auto& sample : samples)
	{
		if(wcscmp(name,sample.szName) !=0)
		{

			auto avg = tot / ((double)cnt);

			//fwprintf_s(fout, L"\t%14s\t\t%12.6fus\t\t\t%12lld\t\t%12lld\t\t%12lld\t\n", name, avg.count(), cnt, min.count(), max.count());
			fwprintf_s(fout, L"%s %.6fus\n", name, avg.count());

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


	fclose(fout);


	ReleaseSRWLockShared(&_profileListLock);
}

void ProfileManager::SetOptional(std::wstring optional)
{
	optionalText = optional;
}

