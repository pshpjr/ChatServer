#include "ProcessMonitor.h"

#include <format>


ProcessMonitor::ProcessMonitor(String ProcessName)
{
    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
    _iNumberOfProcessors = SystemInfo.dwNumberOfProcessors;
    // _maxProcessorValue = _iNumberOfProcessors * 100;

    PdhOpenQuery(nullptr, NULL, &_swQuery);

    PdhAddCounterW(_swQuery, std::format(L"\\Process({:s})\\% Processor Time", ProcessName).c_str(), NULL, &_pCpuTotal);
    PdhAddCounterW(_swQuery, std::format(L"\\Process({:s})\\% Privileged Time", ProcessName).c_str(), NULL
                   , &_pCpuKernel);
    PdhAddCounterW(_swQuery, std::format(L"\\Process({:s})\\% User Time", ProcessName).c_str(), NULL, &_pCpuUser);
    PdhAddCounterW(_swQuery, std::format(L"\\Process({:s})\\Page Faults/sec", ProcessName).c_str(), NULL
                   , &_pPageFault);
    PdhAddCounterW(_swQuery, std::format(L"\\Process({:s})\\Private Bytes", ProcessName).c_str(), NULL
                   , &_pUseMemory);
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
    PdhCollectQueryData(_swQuery);


    PDH_FMT_COUNTERVALUE counterVal;
    double tot = 0;
    if (PdhGetFormattedCounterValue(_pCpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal) == ERROR_SUCCESS)
    {
        //tot = counterVal.doubleValue;
        _totalProcessorTime = counterVal.doubleValue;
    }
    _kernelProcessorTime = 0;
    _userProcessorTime = 0;
    _totalProcessorTime = 0;
    _pageFault = 0;
    _useMemoryMB = 0;
    if (PdhGetFormattedCounterValue(_pCpuKernel, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _kernelProcessorTime = counterVal.doubleValue;
    }

    if (PdhGetFormattedCounterValue(_pCpuUser, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _userProcessorTime = counterVal.doubleValue;
    }

    //_totalProcessorTime = _kernelProcessorTime + _userProcessorTime;


    if (PdhGetFormattedCounterValue(_pPageFault, PDH_FMT_LONG, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _pageFault = counterVal.longValue;
    }

    if (PdhGetFormattedCounterValue(_pUseMemory, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _useMemoryMB = counterVal.doubleValue / 1000'000;
    }
}
