#pragma once

#include <Pdh.h>
#include <string>

class ProcessMonitor
{
public:

    ProcessMonitor(std::wstring ProcessName);

    void Update();

    double TotalProcessTime() const;

    double KernelProcessTime() const;

    double UserProcessTime() const;

    long PageFault() const;

    double UseMemoryMB() const;

private:
    void AddCounter(const std::wstring& counterPath, PDH_HCOUNTER& counter);


    int _iNumberOfProcessors = 0;
    double _maxProcessorValue = 0;

    double _totalProcessorTime = 0;
    double _kernelProcessorTime = 0;
    double _userProcessorTime = 0;

    long _pageFault = 0;
    double _useMemoryMB = 0;

    PDH_HQUERY _hQuery{};
    //PDH_HQUERY pCpuTotal;
    PDH_HQUERY _pCpuKernel{};
    PDH_HQUERY _pCpuUser{};

    PDH_HQUERY _pPageFault{};
    PDH_HQUERY _pUseMemory{};
    PDH_HQUERY _pCpuTotal{};
    std::wstring _processName;

};