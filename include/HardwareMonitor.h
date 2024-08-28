#pragma once
#include <Pdh.h>
#pragma comment(lib,"Pdh.lib")

class HardwareMonitor
{
public:
    HardwareMonitor();

    void Update();

    double TotalProcessorTime() const;

    double KernelProcessorTime() const;

    double UserProcessorTime() const;

    long Interrupt() const;

    double InterruptPercent() const;

    long ContextSwitch() const;

    double NonPagedMb() const;

    double AvailableMb() const;

    long HardFault() const;

    double SendKBytes() const;

    double RecvKBytes() const;

    //Monitor items
private:
    double _totalProcessorTime = 0;
    double _kernelProcessorTime = 0;
    double _userProcessorTime = 0;
    long _interrupt = 0;
    double _interruptPercent = 0;
    long _contextSwitch = 0;

    double _nonPagedMb = 0;
    double _availableMb = 0;
    long _hardFault = 0;

    double _sendKBytes = 0;
    double _recvKBytes = 0;

    PDH_HQUERY _hwQuery;

    PDH_HCOUNTER pCpuTotal = nullptr;
    PDH_HCOUNTER pCpuKernel = nullptr;
    PDH_HCOUNTER pCpuUser = nullptr;

    PDH_HCOUNTER pInterrupt = nullptr;
    PDH_HCOUNTER pInterruptPercent = nullptr;
    PDH_HCOUNTER pContextSwitch = nullptr;
    PDH_HCOUNTER pPageFault = nullptr;
    PDH_HCOUNTER pNonPaged = nullptr;
    PDH_HCOUNTER pAvailableBytes = nullptr;
    PDH_HCOUNTER pHardFault = nullptr;


    PDH_HCOUNTER pSendBytes;
    PDH_HCOUNTER pRecvBytes;
};
