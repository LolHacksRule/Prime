#include "Endian.h"

//No clue why these have a lot of hidden code, maybe defs for platform endians

#ifdef _SEXYDECOMP_USE_LATEST_CODE
UINT16 EndianUINT16(UINT16 l)
{
	return l;
}

UINT32 EndianUINT32(UINT32 l)
{
	return l;
}
#endif

DWORD EndianDWORD(DWORD l) //13-26
{
	return l;
}

WORD EndianWORD(WORD l) //29-40
{
	return l;
}

int EndianInt(int l) //44-57
{
	return l;
}

short EndianShort(short l) //60-71
{
	return l;
}

float EndianFloat(float l) //74-88
{
	return l;
}

double EndianDouble(double src) //91-109
{
	return src;
}

FILETIME EndianFileTime(FILETIME ft) //112-131
{
	return ft;
}
