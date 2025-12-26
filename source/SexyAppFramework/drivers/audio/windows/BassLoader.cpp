#include "BassLoader.h"
#include <stdlib.h>
#include "../../../Common.h"

using namespace Sexy;

BASS_INSTANCE* Sexy::gBass = NULL;
static long gBassLoadCount = 0;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void Sexy::CheckBassFunction(unsigned int theFunc, const char *theName) //Inside Sexy namespace now | 16-24
{
	if (theFunc==0)
	{
		char aBuf[1024];
		sprintf(aBuf,"%s function not found in bass.dll",theName);
		MessageBoxA(NULL,aBuf,"Error",MB_OK | MB_ICONERROR);
		exit(0);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
BASS_INSTANCE::BASS_INSTANCE(const char *dllName) //TODO | 29-148
{
    mModule = LoadLibrary(dllName);
	if (!mModule)
		return;

#define GETPROC(_x) CheckBassFunction(*((unsigned int *)&_x) = (unsigned int)GetProcAddress(mModule, #_x),#_x)    

    GETPROC(BASS_GetVersion);

    if (BASS_GetVersion() >> 16 != BASSVERSION) //We're doing this for the DLL version since its a short.
    {
        MessageBoxA(NULL, "Incompatible version of bass.dll", "Error", MB_OK | MB_ICONERROR);
        exit(0);
    }

    GETPROC(BASS_SetConfig);
    GETPROC(BASS_GetConfig);
    GETPROC(BASS_SetConfigPtr);
    GETPROC(BASS_GetConfigPtr);
    GETPROC(BASS_ErrorGetCode);
    GETPROC(BASS_GetDeviceInfo);
    GETPROC(BASS_Init);
    GETPROC(BASS_SetDevice);
    GETPROC(BASS_GetDevice);
    GETPROC(BASS_Free);
    GETPROC(BASS_GetInfo);
    GETPROC(BASS_Update);
    GETPROC(BASS_GetCPU);
    GETPROC(BASS_Start);
    GETPROC(BASS_Stop);
    GETPROC(BASS_Pause);
    GETPROC(BASS_SetVolume);
    GETPROC(BASS_GetVolume);
    GETPROC(BASS_PluginLoad);
    GETPROC(BASS_PluginFree);
    GETPROC(BASS_PluginGetInfo);
    GETPROC(BASS_Set3DFactors);
    GETPROC(BASS_Get3DFactors);
    GETPROC(BASS_Set3DPosition);
    GETPROC(BASS_Get3DPosition);
    GETPROC(BASS_Apply3D);
    GETPROC(BASS_MusicLoad);
    GETPROC(BASS_MusicFree);
    GETPROC(BASS_SampleLoad);
    GETPROC(BASS_SampleCreate);
    GETPROC(BASS_SampleFree);
    GETPROC(BASS_SampleSetData);
    GETPROC(BASS_SampleGetData);
    GETPROC(BASS_SampleGetInfo);
    GETPROC(BASS_SampleSetInfo);
    GETPROC(BASS_SampleGetChannel);
    GETPROC(BASS_SampleGetChannels);
    GETPROC(BASS_SampleStop);
    GETPROC(BASS_StreamCreate);
    GETPROC(BASS_StreamCreateFile);
    GETPROC(BASS_StreamCreateURL);
    GETPROC(BASS_StreamCreateFileUser);
    GETPROC(BASS_StreamFree);
    GETPROC(BASS_StreamGetFilePosition);
    GETPROC(BASS_StreamPutData);
    GETPROC(BASS_StreamPutFileData);
    GETPROC(BASS_RecordGetDeviceInfo);
    GETPROC(BASS_RecordInit);
    GETPROC(BASS_RecordSetDevice);
    GETPROC(BASS_RecordGetDevice);
    GETPROC(BASS_RecordFree);
    GETPROC(BASS_RecordGetInfo);
    GETPROC(BASS_RecordGetInputName);
    GETPROC(BASS_RecordSetInput);
    GETPROC(BASS_RecordGetInput);
    GETPROC(BASS_RecordStart);
    GETPROC(BASS_ChannelBytes2Seconds);
    GETPROC(BASS_ChannelSeconds2Bytes);
    GETPROC(BASS_ChannelGetDevice);
    GETPROC(BASS_ChannelSetDevice);
    GETPROC(BASS_ChannelIsActive);
    GETPROC(BASS_ChannelGetInfo);
    GETPROC(BASS_ChannelGetTags);
    GETPROC(BASS_ChannelFlags);
    GETPROC(BASS_ChannelUpdate);
    GETPROC(BASS_ChannelLock);
    GETPROC(BASS_ChannelPlay);
    GETPROC(BASS_ChannelStop);
    GETPROC(BASS_ChannelPause);
    GETPROC(BASS_ChannelSetAttribute);
    GETPROC(BASS_ChannelGetAttribute);
    GETPROC(BASS_ChannelSlideAttribute);
    GETPROC(BASS_ChannelIsSliding);
    GETPROC(BASS_ChannelSet3DAttributes);
    GETPROC(BASS_ChannelGet3DAttributes);
    GETPROC(BASS_ChannelSet3DPosition);
    GETPROC(BASS_ChannelGet3DPosition);
    GETPROC(BASS_ChannelGetLength);
    GETPROC(BASS_ChannelSetPosition);
    GETPROC(BASS_ChannelGetPosition);
    GETPROC(BASS_ChannelGetLevel);
    GETPROC(BASS_ChannelGetData);
    GETPROC(BASS_ChannelSetSync);
    GETPROC(BASS_ChannelRemoveSync);
    GETPROC(BASS_ChannelSetDSP);
    GETPROC(BASS_ChannelRemoveDSP);
    GETPROC(BASS_ChannelSetLink);
    GETPROC(BASS_ChannelRemoveLink);
    GETPROC(BASS_ChannelSetFX);
    GETPROC(BASS_ChannelRemoveFX);
    GETPROC(BASS_FXSetParameters);
    GETPROC(BASS_FXGetParameters);
    GETPROC(BASS_FXReset);

#undef GETPROC
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
BASS_INSTANCE::~BASS_INSTANCE() //153-156
{
    if (mModule)
        FreeLibrary(mModule);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Sexy::LoadBassDLL() //162-172
{
	InterlockedIncrement(&gBassLoadCount);
	if (gBass!=NULL)
		return;

	gBass = new BASS_INSTANCE(GetFullPath(".\\bass.dll").c_str());
	if (gBass->mModule==NULL)
	{
		MessageBoxA(NULL,"Can't find bass.dll." ,"Error",MB_OK | MB_ICONERROR);
		exit(0);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Sexy::FreeBassDLL() //177-186
{
	if (gBass!=NULL)
	{
		if (InterlockedDecrement(&gBassLoadCount) <= 0)
		{
			delete gBass;
			gBass = NULL;
		}
	}
}


