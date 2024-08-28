#pragma once

#include "Types.h"
#include <Pdh.h>
#pragma comment(lib,"Pdh.lib")




class ProcessMonitor
{
public:
    ProcessMonitor(String ProcessName);

    void Update();

    double TotalProcessTime() const;

    double KernelProcessTime() const;

    double UserProcessTime() const;

    long PageFault() const;

    double UseMemoryMB() const;

private:
    PDH_HQUERY _swQuery{};


    int _iNumberOfProcessors = 0;
    double _maxProcessorValue = 0;

    double _totalProcessorTime = 0;
    double _kernelProcessorTime = 0;
    double _userProcessorTime = 0;

    long _pageFault = 0;
    double _useMemoryMB = 0;

    //PDH_HQUERY pCpuTotal;
    PDH_HQUERY _pCpuKernel{};
    PDH_HQUERY _pCpuUser{};

    PDH_HQUERY _pPageFault{};
    PDH_HQUERY _pUseMemory{};
};
