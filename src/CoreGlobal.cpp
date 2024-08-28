#include "CoreGlobal.h"
#include "stdafx.h"

#include "CLogger.h"
#include "CrashDump.h"
#include "SettingParser.h"
//#include "Profiler.h"

CrashDump* gCrashDump = nullptr;
CLogger* gLogger = nullptr;
SettingParser* gSettingParser = nullptr;

class CoreGlobal
{
    //순서 고려하면서 생성/ 소멸시킬 것
public:
    CoreGlobal()
    {
        gCrashDump = new CrashDump();
        gLogger = new CLogger();
        gSettingParser = new SettingParser();
    }

    ~CoreGlobal()
    {
        delete gSettingParser;
        delete gLogger;
        delete gCrashDump;
    }
} gCoreGlobal;
