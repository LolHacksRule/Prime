#ifndef __AUTOCRIT_INCLUDED__
#define __AUTOCRIT_INCLUDED__

#include "Common.h"
#include "CritSect.h"

namespace Sexy
{

class AutoCrit //Not on iOS (2012)
{
private:
	CritSect&			mCritSec;
public:
	AutoCrit(CritSect& theCritSect) : 
		mCritSec(theCritSect) //16-18
	{ 
		mCritSec.Lock();
	}
	~AutoCrit() //21-23
	{ 
		mCritSec.Unlock();
	}
};

}

#endif //__AUTOCRIT_INCLUDED__