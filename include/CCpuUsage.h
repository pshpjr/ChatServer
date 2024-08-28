﻿#pragma once
#define _WINSOCKAPI_
#include <Windows.h>

/////////////////////////////////////////////////////////////////////////////
// CCpuUsage CPUTime(); // CPUTime(hProcess)
//
// while ( 1 )
// {
// CPUTIme.UpdateCpuTime();
// wprintf(L"Processor:%f / Process:%f \n", CPUTime.ProcessorTotal(), CPUTime.ProcessTotal());
// wprintf(L"ProcessorKernel:%f / ProcessKernel:%f \n", CPUTime.ProcessorKernel(), CPUTime.ProcessKernel());
// wprintf(L"ProcessorUser:%f / ProcessUser:%f \n", CPUTime.ProcessorUser(), CPUTime.ProcessUser());
// Sleep(1000);
// }
/////////////////////////////////////////////////////////////////////////////
class CCpuUsage
{
public:
    //----------------------------------------------------------------------
    // 생성자, 확인대상 프로세스 핸들. 미입력시 자기 자신.
    //----------------------------------------------------------------------
    CCpuUsage(HANDLE hProcess = INVALID_HANDLE_VALUE);
    void UpdateCpuTime(void);

    float ProcessorTotal(void) const
    {
        return _fProcessorTotal;
    }

    float ProcessorUser(void) const
    {
        return _fProcessorUser;
    }

    float ProcessorKernel(void) const
    {
        return _fProcessorKernel;
    }

    float ProcessTotal(void) const
    {
        return _fProcessTotal;
    }

    float ProcessUser(void) const
    {
        return _fProcessUser;
    }

    float ProcessKernel(void) const
    {
        return _fProcessKernel;
    }

private:
    HANDLE _hProcess;
    int _iNumberOfProcessors;
    float _fProcessorTotal;
    float _fProcessorUser;
    float _fProcessorKernel;
    float _fProcessTotal;
    float _fProcessUser;
    float _fProcessKernel;
    ULARGE_INTEGER _ftProcessor_LastKernel;
    ULARGE_INTEGER _ftProcessor_LastUser;
    ULARGE_INTEGER _ftProcessor_LastIdle;
    ULARGE_INTEGER _ftProcess_LastKernel;
    ULARGE_INTEGER _ftProcess_LastUser;
    ULARGE_INTEGER _ftProcess_LastTime;
};
