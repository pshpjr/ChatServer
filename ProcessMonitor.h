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
		PdhOpenQuery(NULL, NULL, &_swQuery);

		PdhAddCounter(_swQuery, std::format(L"\\Process({:s})\\% Processor Time", ProcessName).c_str(), NULL, &pCpuTotal);
		PdhAddCounter(_swQuery, std::format(L"\\Process({:s})\\% Privileged Time", ProcessName).c_str(), NULL, &pCpuKernel);
		PdhAddCounter(_swQuery, std::format(L"\\Process({:s})\\% User Time", ProcessName).c_str(), NULL, &pCpuUser);
		PdhAddCounter(_swQuery, std::format(L"\\Process({:s})\\% Processor Time", ProcessName).c_str(), NULL, &pPageFault);
		PdhAddCounter(_swQuery, std::format(L"\\Process({:s})\\% Processor Time", ProcessName).c_str(), NULL, &pUseMemory);


	}
	void Update();


private:
	PDH_HQUERY _swQuery;

	double _totalProcessorTime;
	double _kernelProcessorTime;
	double _userProcessorTime;

	long _pageFault;
	double _useMemoryMB;

	PDH_HQUERY pCpuTotal;
	PDH_HQUERY pCpuKernel;
	PDH_HQUERY pCpuUser;

	PDH_HQUERY pPageFault;
	PDH_HQUERY pUseMemory;

};

