#ifndef __POLYDIVIDE_H__
#define __POLYDIVIDE_H__

#include "Point.h"

namespace Sexy
{
	bool DividePoly(FPoint* v, int n, FPoint* theTris[][3], int theMaxTris, int* theNumTris);
}

#endif //__POLYDIVIDE_H__