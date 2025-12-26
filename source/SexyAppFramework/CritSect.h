#ifndef _H_CritSect
#define _H_CritSect

#include "Common.h"

#ifndef WIN32
#include <pthread.h>
#endif

class CritSync;

namespace Sexy
{
	class CritSect //Not on iOS (2012)
	{
private:
	CRITICAL_SECTION mCriticalSection;
	friend class AutoCrit;

public:
	CritSect(void) //Moved to here why idk | 19-22
	{
		InitializeCriticalSection(&mCriticalSection);
	}
	~CritSect(void) //25-27
	{
		DeleteCriticalSection(&mCriticalSection);
	}
	void Unlock() //Changed from Leave in Transmension
	{
		LeaveCriticalSection(&mCriticalSection);
	}
	void Lock() //Changed from Enter in Transmension
	{
		EnterCriticalSection(&mCriticalSection);
	}
#if !defined(WIN32) && !defined(_IPHONEOS)
	bool TryLock();
#endif
};

	class Condition //Idk where this goes
	{
	private:
		HANDLE mEvent;
	public:
		Condition() //38-41
		{
			mEvent = CreateEvent(0, 0, 0, 0);
		}
		~Condition() //44-46
		{
			CloseHandle(mEvent);
		}
		void Notify() //49-51
		{
			SetEvent(mEvent);
		}
		void Wait(uint timeout) //54-56
		{
			WaitForSingleObject(mEvent, timeout);
		}
		void Reset();
	};

}

#endif // _H_CritSect
