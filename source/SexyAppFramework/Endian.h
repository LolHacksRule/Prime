#ifndef __ENDIAN_H__
#define __ENDIAN_H__

#include "Common.h"

UINT16 EndianUINT16(UINT16 l); //Mobile
UINT32 EndianUINT32(UINT32 l); //Mobile (Unused in EAMT Android PVZ Free)
DWORD EndianDWORD(DWORD l);
WORD EndianWORD(WORD l);
int EndianInt(int l);
short EndianShort(short l);
float EndianFloat(float l);
double EndianDouble(double src); //Unused on Win
FILETIME EndianFileTime(FILETIME ft); //Unused

#endif