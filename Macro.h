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


/*-----------------
	Lock
 -----------------*/

#define USE_MANY_LOCKS(count)	shared_mutex _locks[count];
#define USE_LOCK				USE_MANY_LOCKS(1)
#define	READ_LOCK_IDX(idx)		ReadLockGuard readLockGuard_##idx(_locks[idx]);
#define READ_LOCK				READ_LOCK_IDX(0)
#define	WRITE_LOCK_IDX(idx)		WriteLockGuard writeLockGuard_##idx(_locks[idx]);
#define WRITE_LOCK				WRITE_LOCK_IDX(0)