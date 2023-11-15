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
		Util Function
---------------------*/
static void to_str(int num, OUT char* dest, int bufferSize)
{
	sprintf_s(dest, bufferSize, "%d", num);
}

#define MEMBER_SIZE(type, member) sizeof(((type *)0)->member)

#define MIN(a,b) (((a) < (b)) ? (a) : (b))