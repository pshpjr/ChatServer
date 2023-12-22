#include "stdafx.h"
#include "HardwareMonitor.h"
#include <Pdh.h>

void HardwareMonitor::Update()
{
	PdhCollectQueryData(_hwQuery);
	PDH_FMT_COUNTERVALUE counterVal;

	PdhGetFormattedCounterValue(pCpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
	_totalProcessorTime = counterVal.doubleValue;

	PdhGetFormattedCounterValue(pCpuKernel, PDH_FMT_DOUBLE, NULL, &counterVal);
	_kernelProcessorTime = counterVal.doubleValue;

	PdhGetFormattedCounterValue(pCpuUser, PDH_FMT_DOUBLE, NULL, &counterVal);
	_userProcessorTime = counterVal.doubleValue;

	PdhGetFormattedCounterValue(pInterrupt, PDH_FMT_LONG, NULL, &counterVal);
	_interrupt = counterVal.longValue;

	PdhGetFormattedCounterValue(pInterruptPercent, PDH_FMT_DOUBLE, NULL, &counterVal);
	_interruptPercent = counterVal.doubleValue;

	PdhGetFormattedCounterValue(pContextSwitch, PDH_FMT_LONG, NULL, &counterVal);
	_contextSwitch = counterVal.longValue;

	PdhGetFormattedCounterValue(pNonPaged, PDH_FMT_DOUBLE, NULL, &counterVal);
	_nonPagedMb = counterVal.doubleValue/1000'000.;

	PdhGetFormattedCounterValue(pAvailableBytes, PDH_FMT_DOUBLE, NULL, &counterVal);
	_availableMb = counterVal.doubleValue / 1000'000.;

	PdhGetFormattedCounterValue(pHardFault, PDH_FMT_LONG, NULL, &counterVal);
	_hardFault = counterVal.longValue;

	PdhGetFormattedCounterValue(pSendBytes, PDH_FMT_DOUBLE, NULL, &counterVal);
	_sendKBytes = counterVal.doubleValue/1000.;

	PdhGetFormattedCounterValue(pRecvBytes, PDH_FMT_DOUBLE, NULL, &counterVal);
	_recvKBytes = counterVal.doubleValue/1000.;

}