#ifndef __SEXYTHREAD_H__
#define __SEXYTHREAD_H__

#include <process.h>
#include "Common.h"

typedef DWORD SexyThreadId;
typedef _beginthread_proc_type SexyThreadProc;

namespace Sexy //Only designed for Windows atm. C++ only.
{
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	void				LaunchThread(SexyThreadProc proc, void* param, SexyThreadPriority priority);
	void				PThreadStubProc();
#else
	void				LaunchThread(SexyThreadProc proc, void* param);
#endif
	SexyThreadId		GetCurrentRunningThread();
};

#endif