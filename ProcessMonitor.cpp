#include "stdafx.h"
#include "ProcessMonitor.h"

void ProcessMonitor::Update()
{
	auto result = PdhCollectQueryData(_swQuery);

	
	PDH_FMT_COUNTERVALUE counterVal;

	if ( PdhGetFormattedCounterValue(pCpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal) == ERROR_SUCCESS )
		_totalProcessorTime = counterVal.doubleValue;

	if( PdhGetFormattedCounterValue(pCpuKernel, PDH_FMT_DOUBLE, NULL, &counterVal) == ERROR_SUCCESS )
		_kernelProcessorTime = counterVal.doubleValue;

	if( PdhGetFormattedCounterValue(pCpuUser, PDH_FMT_DOUBLE, NULL, &counterVal) == ERROR_SUCCESS )
		_userProcessorTime = counterVal.doubleValue;

	if( PdhGetFormattedCounterValue(pPageFault, PDH_FMT_LONG, NULL, &counterVal) == ERROR_SUCCESS )
		_pageFault = counterVal.doubleValue;

	if( PdhGetFormattedCounterValue(pUseMemory, PDH_FMT_DOUBLE, NULL, &counterVal) == ERROR_SUCCESS )
		_useMemoryMB = counterVal.doubleValue/1000'000;
}