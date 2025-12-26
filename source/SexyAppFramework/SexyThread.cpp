#include "SexyThread.h"

using namespace Sexy;

void Sexy::LaunchThread(SexyThreadProc proc, void* param) //9-11
{
	_beginthread(proc, 0, param);
}

SexyThreadId Sexy::GetCurrentRunningThread() //14-16
{
	return GetCurrentThreadId();
}