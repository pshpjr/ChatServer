
#include "ProcessMonitor.h"

#include <format>
#include <iostream>

void ProcessMonitor::AddCounter(const std::wstring& counterPath, PDH_HCOUNTER& counter) {
    PDH_STATUS status = PdhAddCounterW(_hQuery, counterPath.c_str(), 0, &counter);
    if (status != ERROR_SUCCESS) {
        std::wcout << L"PdhAddCounter 실패: " << counterPath << L" (" << status << L")" << std::endl;
    }
}

ProcessMonitor::ProcessMonitor(std::wstring ProcessName):_processName(ProcessName)
{
    PDH_STATUS status = PdhOpenQuery(nullptr, 0, &_hQuery);
    if (status != ERROR_SUCCESS) {
        wprintf(L"PdhOpenQuery 실패: %X\n", status);
        return;
    }

    // 카운터 추가
    AddCounter(L"\\Process(" + ProcessName + L")\\% Processor Time", _pCpuTotal);
    AddCounter(L"\\Process(" + ProcessName + L")\\% Privileged Time", _pCpuKernel);
    AddCounter(L"\\Process(" + ProcessName + L")\\% User Time", _pCpuUser);
    AddCounter(L"\\Process(" + ProcessName + L")\\Page Faults/sec", _pPageFault);
    AddCounter(L"\\Process(" + ProcessName + L")\\Private Bytes", _pUseMemory);

}

double ProcessMonitor::TotalProcessTime() const
{
    return _totalProcessorTime;
}

double ProcessMonitor::KernelProcessTime() const
{
    return _kernelProcessorTime;
}

double ProcessMonitor::UserProcessTime() const
{
    return _userProcessorTime;
}

long ProcessMonitor::PageFault() const
{
    return _pageFault;
}

double ProcessMonitor::UseMemoryMB() const
{
    return _useMemoryMB;
}

void ProcessMonitor::Update()
{
    PdhCollectQueryData(_hQuery);

    _totalProcessorTime = 0;
    _kernelProcessorTime = 0;
    _userProcessorTime = 0;
    _pageFault = 0;
    _useMemoryMB = 0;

    if (PDH_FMT_COUNTERVALUE counterVal; PdhGetFormattedCounterValue(_pCpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal) == ERROR_SUCCESS)
    {
        _totalProcessorTime = counterVal.doubleValue;
    }

    if (PDH_FMT_COUNTERVALUE counterVal; PdhGetFormattedCounterValue(_pCpuKernel, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _kernelProcessorTime = counterVal.doubleValue;
    }

    if (PDH_FMT_COUNTERVALUE counterVal; PdhGetFormattedCounterValue(_pCpuUser, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _userProcessorTime = counterVal.doubleValue;
    }

    if (PDH_FMT_COUNTERVALUE counterVal; PdhGetFormattedCounterValue(_pPageFault, PDH_FMT_LONG, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _pageFault = counterVal.longValue;
    }

    if (PDH_FMT_COUNTERVALUE counterVal; PdhGetFormattedCounterValue(_pUseMemory, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _useMemoryMB = counterVal.doubleValue / 1000'000;
    }
}