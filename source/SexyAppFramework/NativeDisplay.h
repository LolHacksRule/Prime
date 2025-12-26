#ifndef __NATIVEDISPLAY_H__
#define __NATIVEDISPLAY_H__
#include "Common.h"

namespace Sexy
{

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class NativeDisplay
{
public:
	int						mRGBBits;
	ulong					mRedMask;
	ulong					mGreenMask;
	ulong					mBlueMask;
	int						mRedBits;
	int						mGreenBits;
	int						mBlueBits;
	int						mRedShift;
	int						mGreenShift;
	int						mBlueShift;
	int*					mRedAddTable;
	int*					mGreenAddTable;
	int*					mBlueAddTable;
	ulong					mRedConvTable[256];
	ulong					mGreenConvTable[256];
	ulong					mBlueConvTable[256];

public:
	NativeDisplay();
};

};


#endif