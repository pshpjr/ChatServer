#include "stdafx.h"
#include "ProcessMonitor.h"

void ProcessMonitor::Update()
{
	PdhCollectQueryData(_swQuery);

	
	PDH_FMT_COUNTERVALUE counterVal;
	//double tot = 0;
	//if ( PdhGetFormattedCounterValue(pCpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal) == ERROR_SUCCESS )
	//{
	//	tot = counterVal.doubleValue;
	//	_totalProcessorTime = counterVal.doubleValue / _maxProcessorValue * 100;
	//}
	_kernelProcessorTime = 0;
	_userProcessorTime = 0;
	_totalProcessorTime = 0;
	_pageFault = 0;
	_useMemoryMB = 0;
	if ( PdhGetFormattedCounterValue(_pCpuKernel, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS )
	{
		_kernelProcessorTime = counterVal.doubleValue / _maxProcessorValue * 100;
	}

	if ( PdhGetFormattedCounterValue(_pCpuUser, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS )
	{
		_userProcessorTime = counterVal.doubleValue / _maxProcessorValue * 100;
	}

	_totalProcessorTime = _kernelProcessorTime + _userProcessorTime;


	if( PdhGetFormattedCounterValue(_pPageFault, PDH_FMT_LONG, nullptr, &counterVal) == ERROR_SUCCESS )
	{
		_pageFault = counterVal.doubleValue;
	}

	if( PdhGetFormattedCounterValue(_pUseMemory, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS )
	{
		_useMemoryMB = counterVal.doubleValue/1000'000;
	}
}
