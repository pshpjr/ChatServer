#include "stdafx.h"
#include "ProcessMonitor.h"

void ProcessMonitor::Update()
{
	PdhCollectQueryData(_swQuery);
	PDH_FMT_COUNTERVALUE counterVal;

	PdhGetFormattedCounterValue(pCpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
	_totalProcessorTime = counterVal.doubleValue;

	PdhGetFormattedCounterValue(pCpuKernel, PDH_FMT_DOUBLE, NULL, &counterVal);
	_kernelProcessorTime = counterVal.doubleValue;

	PdhGetFormattedCounterValue(pCpuUser, PDH_FMT_DOUBLE, NULL, &counterVal);
	_userProcessorTime = counterVal.doubleValue;

	PdhGetFormattedCounterValue(pPageFault, PDH_FMT_LONG, NULL, &counterVal);
	_pageFault = counterVal.doubleValue;

	PdhGetFormattedCounterValue(pUseMemory, PDH_FMT_DOUBLE, NULL, &counterVal);
	_useMemoryMB = counterVal.doubleValue/1000'000;
}