#include "HardwareMonitor.h"

HardwareMonitor::HardwareMonitor()
{
    PdhOpenQuery(nullptr, NULL, &_hwQuery);
    PdhAddCounter(_hwQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &pCpuTotal);
    PdhAddCounter(_hwQuery, L"\\Processor(_Total)\\% Privileged Time", NULL, &pCpuKernel);
    PdhAddCounter(_hwQuery, L"\\Processor(_Total)\\% User Time", NULL, &pCpuUser);
    PdhAddCounter(_hwQuery, L"\\Processor(*)\\Interrupts/sec", NULL, &pInterrupt);
    PdhAddCounter(_hwQuery, L"\\Processor(*)\\% Interrupt Time", NULL, &pInterruptPercent);
    PdhAddCounter(_hwQuery, L"\\System\\Context Switches/sec", NULL, &pContextSwitch);


    PdhAddCounter(_hwQuery, L"\\Memory\\Pool Nonpaged Bytes", NULL, &pNonPaged);
    PdhAddCounter(_hwQuery, L"\\Memory\\Available Bytes", NULL, &pAvailableBytes);
    PdhAddCounter(_hwQuery, L"\\Memory\\Pages/sec", NULL, &pHardFault);

    PdhAddCounter(_hwQuery, L"\\Network Interface(*)\\Bytes Sent/sec", NULL, &pSendBytes);
    PdhAddCounter(_hwQuery, L"\\Network Interface(*)\\Bytes Received/sec", NULL, &pRecvBytes);
}

double HardwareMonitor::TotalProcessorTime() const
{
    return _totalProcessorTime;
}

double HardwareMonitor::KernelProcessorTime() const
{
    return _kernelProcessorTime;
}

double HardwareMonitor::UserProcessorTime() const
{
    return _userProcessorTime;
}

long HardwareMonitor::Interrupt() const
{
    return _interrupt;
}

double HardwareMonitor::InterruptPercent() const
{
    return _interruptPercent;
}

long HardwareMonitor::ContextSwitch() const
{
    return _contextSwitch;
}

double HardwareMonitor::NonPagedMb() const
{
    return _nonPagedMb;
}

double HardwareMonitor::AvailableMb() const
{
    return _availableMb;
}

long HardwareMonitor::HardFault() const
{
    return _hardFault;
}

double HardwareMonitor::SendKBytes() const
{
    return _sendKBytes;
}

double HardwareMonitor::RecvKBytes() const
{
    return _recvKBytes;
}

void HardwareMonitor::Update()
{
    PdhCollectQueryData(_hwQuery);
    PDH_FMT_COUNTERVALUE counterVal;

    if (PdhGetFormattedCounterValue(pCpuTotal, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _totalProcessorTime = counterVal.doubleValue;
    }

    if (PdhGetFormattedCounterValue(pCpuKernel, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _kernelProcessorTime = counterVal.doubleValue;
    }


    if (PdhGetFormattedCounterValue(pCpuUser, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _userProcessorTime = counterVal.doubleValue;
    }


    if (PdhGetFormattedCounterValue(pInterrupt, PDH_FMT_LONG, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _interrupt = counterVal.longValue;
    }

    if (PdhGetFormattedCounterValue(pInterruptPercent, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _interruptPercent = counterVal.doubleValue;
    }

    if (PdhGetFormattedCounterValue(pContextSwitch, PDH_FMT_LONG, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _contextSwitch = counterVal.longValue;
    }

    if (PdhGetFormattedCounterValue(pNonPaged, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _nonPagedMb = counterVal.doubleValue / 1000'000.;
    }

    if (PdhGetFormattedCounterValue(pAvailableBytes, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _availableMb = counterVal.doubleValue / 1000'000.;
    }

    if (PdhGetFormattedCounterValue(pHardFault, PDH_FMT_LONG, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _hardFault = counterVal.longValue;
    }

    if (PdhGetFormattedCounterValue(pSendBytes, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        _sendKBytes = counterVal.doubleValue / 1000.;
    }

    if (const auto result = PdhGetFormattedCounterValue(pRecvBytes, PDH_FMT_DOUBLE, nullptr, &counterVal);
        result == ERROR_SUCCESS)
    {
        _recvKBytes = counterVal.doubleValue / 1000.;
    }
}
