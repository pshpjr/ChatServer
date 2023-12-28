#pragma once

class MemoryUsage
{

public:
	void Update();

	size_t GetPageFaultCount() const;
	size_t GetPeakPMemSize() const;
	size_t GetPMemSize() const;
	size_t GetPeakVirtualMemSize() const;
	size_t GetVirtualMemSize() const;
	size_t GetPeakPagedPoolUsage() const;
	size_t GetPeakNonPagedPoolUsage() const;
	size_t GetPagedPoolUsage() const;
	size_t GetNonPagedPoolUsage() const;

private:
	size_t _pageFaultCount = 0;
	size_t _peakPMemSize = 0;
	size_t _pMemSize = 0;
	size_t _peakVmemSize = 0;
	size_t _vMemsize = 0;

	size_t _peakPagedPoolUsage = 0;
	size_t _peakNonPagedPoolUsage = 0;
	size_t _pagedPoolUsage = 0;
	size_t _nonPagedPoolUsage = 0;

	HANDLE _hProcess = INVALID_HANDLE_VALUE;
};

