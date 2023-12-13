#include "stdafx.h"
#include "MemoryUsage.h"
#include <Psapi.h>
#include <process.h>

#pragma comment (lib, "Psapi.lib")

void MemoryUsage::Update()
{
	PROCESS_MEMORY_COUNTERS pmc;
	if ( GetProcessMemoryInfo(_hProcess, &pmc, sizeof(pmc)) )
	{
		_pageFaultCount = pmc.PageFaultCount;

		_peakPagedPoolUsage = pmc.QuotaPeakPagedPoolUsage;
		_peakNonPagedPoolUsage = pmc.QuotaPeakNonPagedPoolUsage;
		_pagedPoolUsage = pmc.QuotaPagedPoolUsage;
		_nonPagedPoolUsage = pmc.QuotaNonPagedPoolUsage;

		_peakPMemSize = pmc.PeakWorkingSetSize;
		_peakVmemSize = pmc.PeakPagefileUsage;

		_pMemSize = pmc.WorkingSetSize;
		_vMemsize = pmc.PagefileUsage;
	}
	else
	{

	}
}

size_t MemoryUsage::GetPageFaultCount() const
{
	return _pageFaultCount;
}

size_t MemoryUsage::GetPeakPMemSize() const
{
	return _peakPMemSize;
}

size_t MemoryUsage::GetPMemSize() const
{
	return _pMemSize;
}

size_t MemoryUsage::GetPeakVmemSize() const
{
	return _peakVmemSize;
}

size_t MemoryUsage::GetVMemsize() const
{
	return _vMemsize;
}

size_t MemoryUsage::GetPeakPagedPoolUsage() const
{
	return _peakPagedPoolUsage;
}

size_t MemoryUsage::GetPeakNonPagedPoolUsage() const
{
	return _peakNonPagedPoolUsage;
}

size_t MemoryUsage::GetPagedPoolUsage() const
{
	return _pagedPoolUsage;
}

size_t MemoryUsage::GetNonPagedPoolUsage() const
{
	return _nonPagedPoolUsage;
}
