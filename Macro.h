#pragma once

/*--------------------
		DEBUG
---------------------*/
#define OUT
#ifdef PSH_DEBUG


#endif


/*---------------
	  Crash
---------------*/

#define CRASH(cause)								\
{																			\
	DebugBreak();												\
}

#define ASSERT_CRASH(expr,cause)			\
do{																	\
	if (!(expr))												\
	{																\
		CRASH(cause);		\
		__analysis_assume(expr);			\
	}																\
}while(false)


/*--------------------
		Profiler
---------------------*/
#ifdef USEPROFILE
#define PRO_BEGIN(TagName) do { \
	ProfileItem t__COUNTER__(L#TagName); \
} while (false)
#else

#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#endif
#include <stdio.h>


/*--------------------
		Util Function
---------------------*/
static void to_str(int num, OUT char* dest, int bufferSize)
{
	sprintf_s(dest, bufferSize, "%d", num);
}

#define MEMBER_SIZE(type, member) sizeof(((type *)0)->member)

#define MIN(a,b) (((a) < (b)) ? (a) : (b))