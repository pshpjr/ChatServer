#pragma once

/*--------------------
		DEBUG
---------------------*/
#define OUT
#define PSH_DEBUG

#ifdef PSH_DEBUG
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
#else

#define ASSERT_CRASH(expr,cause)			\



#endif


/*---------------
	  Crash
---------------*/







/*--------------------
		Util Function
---------------------*/
static void to_str(const int num, OUT char* dest, const int bufferSize)
{
	sprintf_s(dest, bufferSize, "%d", num);
}

#define MEMBER_SIZE(type, member) sizeof(((type *)0)->member)

#define MIN(a,b) (((a) < (b)) ? (a) : (b))