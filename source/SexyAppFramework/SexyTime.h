#ifndef __SEXYTIME_H__
#define __SEXYTIME_H__

#include <time.h>

namespace Sexy
{
	time_t SexyTime() //55-57
	{
		return timeGetTime();
	}
}

#endif