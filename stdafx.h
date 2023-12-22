#pragma once

#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>



#include "Container.h"
#include "Profiler.h"
#include "Types.h"
#include "Macro.h"
//#include "BuildOption.h"
//#define BUILD_WITH_EASY_PROFILER
//#define USING_EASY_PROFILER
#include "externalHeader/easy/profiler.h"
#pragma comment (lib,"easy_profiler.lib")