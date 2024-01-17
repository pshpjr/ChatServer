#pragma once
#include <format>
#include <Pdh.h>
#pragma comment(lib,"Pdh.lib")

#include "Types.h"


class ProcessMonitor
{
public:
	ProcessMonitor(String ProcessName)
	{
		SYSTEM_INFO SystemInfo;
		GetSystemInfo(&SystemInfo);
		_iNumberOfProcessors = SystemInfo.dwNumberOfProcessors;
		_maxProcessorValue = _iNumberOfProcessors * 100;

		PdhOpenQuery(nullptr, NULL, &_swQuery);

		//PdhAddCounter(_swQuery, std::format(L"\\Process({:s})\\% Processor Time", ProcessName).c_str(), NULL, &pCpuTotal);
		PdhAddCounter(_swQuery, std::format(L"\\Process({:s})\\% Privileged Time", ProcessName).c_str(), NULL, &_pCpuKernel);
		PdhAddCounter(_swQuery, std::format(L"\\Process({:s})\\% User Time", ProcessName).c_str(), NULL, &_pCpuUser);
		PdhAddCounter(_swQuery, std::format(L"\\Process({:s})\\Page Faults/sec", ProcessName).c_str(), NULL, &_pPageFault);
		PdhAddCounter(_swQuery, std::format(L"\\Process({:s})\\Private Bytes", ProcessName).c_str(), NULL, &_pUseMemory);
	}

	void Update();
	
	double TotalProcessorTime() const
	{
		return _totalProcessorTime;
	}

	double KernelProcessorTime() const 
	{
		return _kernelProcessorTime;
	}

	double UserProcessorTime() const 
	{
		return _userProcessorTime;
	}

	long PageFault() const
	{
		return _pageFault;
	}
    
	double UseMemoryMB() const
	{
		return _useMemoryMB;
	}

private:
	PDH_HQUERY _swQuery;


	int _iNumberOfProcessors = 0;
	double _maxProcessorValue = 0;

	double _totalProcessorTime = 0;
	double _kernelProcessorTime = 0;
	double _userProcessorTime = 0;

	long _pageFault = 0;
	double _useMemoryMB = 0;

	//PDH_HQUERY pCpuTotal;
	PDH_HQUERY _pCpuKernel;
	PDH_HQUERY _pCpuUser;

	PDH_HQUERY _pPageFault;
	PDH_HQUERY _pUseMemory;

};

