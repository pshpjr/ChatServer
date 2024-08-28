﻿#include "SendExecutable.h"
#include "IOCP.h"
#include "Session.h"
#include "stdafx.h"

void SendExecutable::Execute(const ULONG_PTR key, DWORD transferred, void* iocp)
{
    UNREFERENCED_PARAMETER(transferred);
    UNREFERENCED_PARAMETER(iocp);
    const auto session = reinterpret_cast<Session*>(key);
    session->RealSend();
}
