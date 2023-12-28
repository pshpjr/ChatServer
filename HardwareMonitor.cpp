#include "stdafx.h"
#include "HardwareMonitor.h"
#include <Pdh.h>

void HardwareMonitor::Update()
{
	PdhCollectQueryData(_hwQuery);
	PDH_FMT_COUNTERVALUE counterVal;

	if ( PdhGetFormattedCounterValue(pCpuTotal, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS )
	{
		_totalProcessorTime = counterVal.doubleValue;
	}

	if ( PdhGetFormattedCounterValue(pCpuKernel, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS )
	{
		_kernelProcessorTime = counterVal.doubleValue;
	}


	if ( PdhGetFormattedCounterValue(pCpuUser, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS )
	{
		_userProcessorTime = counterVal.doubleValue;
	}


	if( PdhGetFormattedCounterValue(pInterrupt, PDH_FMT_LONG, nullptr, &counterVal) == ERROR_SUCCESS  )
	{
		_interrupt = counterVal.longValue;
	}

	if( PdhGetFormattedCounterValue(pInterruptPercent, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS )
	{
		_interruptPercent = counterVal.doubleValue;
	}

	if( PdhGetFormattedCounterValue(pContextSwitch, PDH_FMT_LONG, nullptr, &counterVal) == ERROR_SUCCESS )
	{
		_contextSwitch = counterVal.longValue;
	}

	if( PdhGetFormattedCounterValue(pNonPaged, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS )
	{
		_nonPagedMb = counterVal.doubleValue/1000'000.;
	}

	if( PdhGetFormattedCounterValue(pAvailableBytes, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS )
	{
		_availableMb = counterVal.doubleValue / 1000'000.;
	}

	if( PdhGetFormattedCounterValue(pHardFault, PDH_FMT_LONG, nullptr, &counterVal) == ERROR_SUCCESS )
	{
		_hardFault = counterVal.longValue;
	}

	if( PdhGetFormattedCounterValue(pSendBytes, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS )
	{
		_sendKBytes = counterVal.doubleValue/1000.;
	}

	if(const auto result = PdhGetFormattedCounterValue(pRecvBytes, PDH_FMT_DOUBLE, nullptr, &counterVal); result == ERROR_SUCCESS )
	{
		_recvKBytes = counterVal.doubleValue/1000.;
	}
}
