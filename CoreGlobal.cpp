#include "stdafx.h"
#include "CoreGlobal.h"

#include "CLogger.h"
#include "SettingParser.h"
#include "CrashDump.h"
//#include "Profiler.h"

CrashDump* GCrashDump = nullptr;
CLogger* GLogger = nullptr;
SettingParser* GSettingParser = nullptr;

class CoreGlobal
{
	//순서 고려하면서 생성/ 소멸시킬 것
public:
	CoreGlobal()
	{
		GCrashDump = new CrashDump();
		GLogger = new CLogger();
		GSettingParser = new SettingParser();

	}
	~CoreGlobal()
	{

		delete GSettingParser;
		delete GLogger;
		delete GCrashDump;
	}
} GCoreGlobal;