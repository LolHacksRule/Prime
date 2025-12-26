#pragma warning( disable : 4786 )

#include "CritSect.h"

#if defined(_WIN32_) || defined(_MAC)
#include <windows.h>
#endif

using namespace Sexy;

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

#if !defined(WIN32) && !defined (_IPHONEOS) //Only on non-Win (or new code[?]) and not iOS, does this also go in the header idk
bool CritSect::TryLock()
{
}
#endif