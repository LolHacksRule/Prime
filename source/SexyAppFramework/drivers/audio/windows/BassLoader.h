#ifndef __BASSLOADER_H__
#define __BASSLOADER_H__

#define WIN32_LEAN_AND_MEAN //Do we need? Mac uses Bass iirc
#include <windows.h>
#include <stdio.h>

#include "../bass/bass.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
namespace Sexy
{

struct BASS_INSTANCE
{
	BASS_INSTANCE(const char *dllName);
	virtual ~BASS_INSTANCE();
    HMODULE         mModule;
};

extern BASS_INSTANCE *gBass;


void CheckBassFunction(unsigned int theFunc, const char* theName);
void LoadBassDLL(); // exits on failure
void FreeBassDLL();

} // namespace Sexy

#endif