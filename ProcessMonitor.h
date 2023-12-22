#pragma once
#include <Pdh.h>
#include <format>
#pragma comment(lib,"Pdh.lib")
#include "Types.h"


class ProcessMonitor
{
public:
	ProcessMonitor(String ProcessName)
	{
		auto openRet = PdhOpenQuery(NULL, NULL, &_swQuery);

		PdhAddCounter(_swQuery, std::format(L"\\Process({:s})\\% Processor Time", ProcessName).c_str(), NULL, &pCpuTotal);
		PdhAddCounter(_swQuery, std::format(L"\\Process({:s})\\% Privileged Time", ProcessName).c_str(), NULL, &pCpuKernel);
		PdhAddCounter(_swQuery, std::format(L"\\Process({:s})\\% User Time", ProcessName).c_str(), NULL, &pCpuUser);
		PdhAddCounter(_swQuery, std::format(L"\\Process({:s})\\Page Faults/sec", ProcessName).c_str(), NULL, &pPageFault);
		PdhAddCounter(_swQuery, std::format(L"\\Process({:s})\\Private Bytes", ProcessName).c_str(), NULL, &pUseMemory);

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

	double _totalProcessorTime = 0;
	double _kernelProcessorTime = 0;
	double _userProcessorTime = 0;

	long _pageFault = 0;
	double _useMemoryMB = 0;

	PDH_HQUERY pCpuTotal;
	PDH_HQUERY pCpuKernel;
	PDH_HQUERY pCpuUser;

	PDH_HQUERY pPageFault;
	PDH_HQUERY pUseMemory;

};

