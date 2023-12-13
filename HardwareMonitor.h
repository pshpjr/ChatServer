#pragma once
#include <Pdh.h>

#pragma comment(lib,"Pdh.lib")

class HardwareMonitor
{
public:
	HardwareMonitor()
	{
		PdhOpenQuery(NULL, NULL, &_hwQuery);
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
		PdhAddCounter(_hwQuery, L"\\Network Interface(*)\\Bytes Recv/sec", NULL, &pRecvBytes);

	}

	void Update();

	double TotalProcessorTime() const { return _totalProcessorTime; }
	double KernelProcessorTime() const { return _kernelProcessorTime; }
	double UserProcessorTime() const { return _userProcessorTime; }
	long Interrupt() const { return _interrupt; }
	double InterruptPercent() const { return _interruptPercent; }
	long ContextSwitch() const { return _contextSwitch; }

	double NonPagedMb() const { return _nonPagedMb; }
	double AvailableMb() const { return _availableMb; }

	long HardFault() const { return _hardFault; }
	double SendBytes() const { return _sendBytes; }
	double RecvBytes() const { return _recvBytes; }

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

	double _sendBytes = 0;
	double _recvBytes = 0;




private:
	PDH_HQUERY _hwQuery;

	PDH_HCOUNTER pCpuTotal;
	PDH_HCOUNTER pCpuKernel;
	PDH_HCOUNTER pCpuUser;

	PDH_HCOUNTER pInterrupt;
	PDH_HCOUNTER pInterruptPercent;
	PDH_HCOUNTER pContextSwitch;
	PDH_HCOUNTER pPageFault;
	PDH_HCOUNTER pNonPaged;
	PDH_HCOUNTER pAvailableBytes;
	PDH_HCOUNTER pHardFault;


	PDH_HCOUNTER pSendBytes;
	PDH_HCOUNTER pRecvBytes;
};

