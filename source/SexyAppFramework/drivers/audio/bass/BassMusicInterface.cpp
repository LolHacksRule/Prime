#include "BassMusicInterface.h"
#include "../windows/BassLoader.h"
#include "../../../PakLib\PakInterface.h"

using namespace Sexy;

#define BASS2_MUSIC_RAMP			BASS_MUSIC_RAMP	// normal ramping

#define BASS_CONFIG_BUFFER			0

//No longer in BASS_INSTANCE

BOOL Sexy::BASS_MusicPlay(HMUSIC handle) //14-16
{
	return BASS_ChannelPlay(handle, true);
}

BOOL Sexy::BASS_MusicPlayEx(HMUSIC handle, DWORD pos, int flags, BOOL reset, QWORD theStartPos) //TODO | 19-48
{
	QWORD anOffset = 0;
	DWORD aMode;
	if (theStartPos)
	{
		if (theStartPos & 0x80000000)
		{
			anOffset = theStartPos;
			aMode = 0;
		}
		else
		{
			anOffset = theStartPos & 0x7FFFFFFF;
			aMode = 1;
		}
	}
	BASS_ChannelStop(handle);
	BASS_ChannelFlags(handle, flags, -1);
	if (!BASS_ChannelSetPosition(handle, anOffset, aMode))
		BASS_ChannelSetPosition(handle, 0, aMode);
	return BASS_ChannelPlay(handle, false/*reset*/);
}

BOOL Sexy::BASS_ChannelResume(HMUSIC handle) //51-53
{
	return BASS_ChannelPlay(handle, false);
}

BOOL Sexy::BASS_StreamPlay(HSTREAM handle, BOOL flush, DWORD flags) //Changed | 56-59	
{
	BASS_ChannelFlags(handle, flags, -1);
	return BASS_ChannelPlay(handle, flush);
}

DWORD Sexy::BASS_MusicGetOrders(HMUSIC handle) //63-64
{
	return BASS_ChannelGetLength(handle, BASS_POS_MUSIC_ORDER);
}

DWORD Sexy::BASS_MusicGetOrderPosition(HMUSIC handle) //67-69
{
	return BASS_ChannelGetPosition(handle, BASS_POS_MUSIC_ORDER);
}

BOOL Sexy::BASS_MusicSetAmplify(HMUSIC handle, DWORD value) //TODO changed | 72-76
{
	BASS_ChannelSetAttribute(handle, BASS_ATTRIB_MUSIC_AMPLIFY, value);
	return true;
}


DWORD Sexy::BASS_MusicGetAmplify(HMUSIC handle) //79-83
{
	float aFloat;
	BASS_ChannelGetAttribute(handle, BASS_ATTRIB_MUSIC_AMPLIFY, &aFloat);
	return aFloat;
}

BOOL Sexy::BASS_MusicSetBPM(HMUSIC handle, DWORD value) //86-89
{
	BASS_ChannelSetAttribute(handle, BASS_ATTRIB_MUSIC_BPM, value);
	return true;
}

DWORD Sexy::BASS_MusicGetBPM(HMUSIC handle) //92-96
{
	float aFloat;
	BASS_ChannelGetAttribute(handle, BASS_ATTRIB_MUSIC_BPM, &aFloat);
	return aFloat;
}

BOOL Sexy::BASS_MusicSetPanSep(HMUSIC handle, DWORD value) //99-102
{
	BASS_ChannelSetAttribute(handle, BASS_ATTRIB_MUSIC_PANSEP, value);
	return true;
}

DWORD Sexy::BASS_MusicGetPanSep(HMUSIC handle) //105-109
{
	float aFloat;
	BASS_ChannelGetAttribute(handle, BASS_ATTRIB_MUSIC_PANSEP, &aFloat);
	return aFloat;
}

BOOL Sexy::BASS_MusicSetPScaler(HMUSIC handle, DWORD value) //112-115
{
	BASS_ChannelSetAttribute(handle, BASS_ATTRIB_MUSIC_PSCALER, value);
	return true;
}

DWORD Sexy::BASS_MusicGetPScaler(HMUSIC handle) //118-122
{
	float aFloat;
	BASS_ChannelGetAttribute(handle, BASS_ATTRIB_MUSIC_PSCALER, &aFloat);
	return aFloat;
}

BOOL Sexy::BASS_MusicSetSpeed(HMUSIC handle, DWORD value) //125-128
{
	BASS_ChannelSetAttribute(handle, BASS_ATTRIB_MUSIC_SPEED, value);
	return true;
}

DWORD Sexy::BASS_MusicGetSpeed(HMUSIC handle) //131-135
{
	float aFloat;
	BASS_ChannelGetAttribute(handle, BASS_ATTRIB_MUSIC_SPEED, &aFloat);
	return aFloat;
}

BOOL Sexy::BASS_MusicSetGlobalVolume(HMUSIC handle, DWORD value) //138-141
{
	BASS_ChannelSetAttribute(handle, BASS_ATTRIB_MUSIC_VOL_GLOBAL, value);
	return true;
}

DWORD Sexy::BASS_MusicGetGlobalVolume(HMUSIC handle) //144-148
{
	float aFloat;
	BASS_ChannelGetAttribute(handle, BASS_ATTRIB_MUSIC_VOL_GLOBAL, &aFloat);
	return aFloat;
}

BOOL Sexy::BASS_MusicSetChannelVolumeFloat(HMUSIC handle, DWORD channel, DWORD value) //? | 151-159
{
	BASS_ChannelSetAttribute(handle, channel + BASS_ATTRIB_MUSIC_VOL_CHAN, value);
	return true;
}

FLOAT Sexy::BASS_MusicGetChannelVolumeFloat(HMUSIC handle, DWORD channel) //? | 162-167
{
	float aFloat;
	return BASS_ChannelGetAttribute(handle, channel + BASS_ATTRIB_MUSIC_VOL_CHAN, &aFloat) ? aFloat : 1.0;
}

BOOL Sexy::BASS_MusicSetChannelVolumeInt(HMUSIC handle, DWORD channel, int value) //170-172
{
	return BASS_MusicSetChannelVolumeFloat(handle, channel, value / 100.0);
}

int Sexy::BASS_MusicGetChannelVolumeInt(HMUSIC handle, DWORD channel) //? | 175-180
{
	float aFloat = BASS_MusicGetChannelVolumeFloat(handle, channel);
	if (aFloat >= 0.0)
		return (aFloat * 100.0);
	else
		return -1;
}

BOOL Sexy::BASS_MusicSetInstrumentVolumeFloat(HMUSIC handle, DWORD inst, float value) //183-186
{
	BASS_ChannelSetAttribute(handle, inst + BASS_ATTRIB_MUSIC_VOL_INST, value);
	return true;
}

FLOAT Sexy::BASS_MusicGetInstrumentVolumeFloat(HMUSIC handle, DWORD inst) //? | 189-193
{
	float aFloat;
	BASS_ChannelGetAttribute(handle, inst + BASS_ATTRIB_MUSIC_VOL_INST, &aFloat);
	return aFloat;
}

BOOL Sexy::BASS_MusicSetInstrumentVolumeInt(HMUSIC handle, DWORD inst, int value) //196-198
{
	return BASS_MusicSetInstrumentVolumeFloat(handle, inst, value / 100.0);
}

int Sexy::BASS_MusicGetInstrumentVolumeInt(HMUSIC handle, DWORD inst) //201-203
{
	return BASS_MusicGetInstrumentVolumeFloat(handle, inst) * 100.0;
}

BOOL Sexy::BASS_ChannelSetAttributes(HMUSIC handle, int freq, int volume, int pan) //206-215
{
	int result = 1;
	if (freq >= 0)
		result = BASS_ChannelSetAttribute(handle, BASS_ATTRIB_FREQ, freq) & 1;
	if (volume >= 0)
		result &= BASS_ChannelSetAttribute(handle, BASS_ATTRIB_VOL, volume);
	if (pan >= -100)
		result & BASS_ChannelSetAttribute(handle, BASS_ATTRIB_PAN, pan) & 1;
	return result;
}

int Sexy::BASS_ChannelGetAttributes(HMUSIC handle, DWORD* freq, DWORD* volume, int* pan) //? | 218-237
{
	int result = 1;
	float aFloat;
	if (freq)
	{
		result = BASS_ChannelGetAttribute(handle, BASS_ATTRIB_FREQ, &aFloat) & 1;
		*freq = aFloat;
	}
	if (volume)
	{
		result = BASS_ChannelGetAttribute(handle, BASS_ATTRIB_VOL, &aFloat) & 1;
		*volume = aFloat * 100.0;
	}
	if (pan)
	{
		result = BASS_ChannelGetAttribute(handle, BASS_ATTRIB_PAN, &aFloat) & 1;
		*pan = aFloat * 100.0;
	}
	return result;
}

int Sexy::BASS_ChannelSetPosition(HMUSIC handle, QWORD pos) //? | 240-250
{
	if (MAKEMUSICPOS(pos, 0) == 0)
		BASS_ChannelSetPosition(handle, pos, 0);
	else
		BASS_ChannelSetPosition(handle, pos & -1, 1);

	return true;
}

QWORD Sexy::BASS_ChannelGetPosition(HMUSIC handle) //253-255
{
	return BASS_ChannelGetPosition(handle, 0);
}

QWORD Sexy::BASS_ChannelGetLength(HMUSIC handle) //258-260
{
	return BASS_ChannelGetLength(handle, BASS_POS_BYTE);
}

BassMusicInfo::BassMusicInfo() //265-272
{	
	mVolume = 0.0;
	mVolumeAdd = 0.0;
	mVolumeCap = 1.0;
	mStopOnFade = false;
	mHMusic = NULL;
	mHStream = NULL;
}

BassMusicInterface::BassMusicInterface(HWND theHWnd) //275-327
{
	LoadBassDLL();

	MIXERCONTROLDETAILS mcd;
	MIXERCONTROLDETAILS_UNSIGNED mxcd_u;
	MIXERLINECONTROLS mxlc;
	MIXERCONTROL mlct;
	MIXERLINE mixerLine;
	HMIXEROBJ phmx;
	MIXERCAPS pmxcaps;	

	mixerOpen((HMIXER*) &phmx, 0, 0, 0, MIXER_OBJECTF_MIXER);
	mixerGetDevCaps(0, &pmxcaps, sizeof(pmxcaps));

	mxlc.cbStruct = sizeof(mxlc);	
	mxlc.cbmxctrl = sizeof(mlct);
	mxlc.pamxctrl = &mlct;
	mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	mixerLine.cbStruct = sizeof(mixerLine);
	mixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;
	mixerGetLineInfo(phmx, &mixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE);
	mxlc.dwLineID = mixerLine.dwLineID;
	mixerGetLineControls(phmx, &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);	

	mcd.cbStruct = sizeof(mcd);
	mcd.dwControlID = mlct.dwControlID;
	mcd.cChannels = 1;
	mcd.cMultipleItems = 0;
	mcd.cbDetails = sizeof(mxcd_u);
	mcd.paDetails = &mxcd_u;
	
	mixerGetControlDetails(phmx, &mcd, 0L);

	//return mxcd_u.dwValue;	

	BASS_Init(-1, 44100, 0, theHWnd, 0); //Only 2.x.x is supported on PC
	
	BASS_SetConfig(BASS_CONFIG_BUFFER, 2000);

	mixerSetControlDetails(phmx, &mcd, 0L);

	BASS_Start();

	mixerClose((HMIXER) phmx);

	mMaxMusicVolume = 40;

	mMusicLoadFlags = 131588; //? BASS_UNICODE | BASS_CONFIG_GVOL_STREAM | BASS_ACTIVE_PAUSED | BASS_MUSIC_LOOP
}

BassMusicInterface::~BassMusicInterface() //330-337
{
	BASS_Stop();
	BASS_Free();

	FreeBassDLL();
}

bool BassMusicInterface::LoadMusic(int theSongId, const std::string& theFileName) //Correct? | 340-383
{
	HMUSIC aHMusic = NULL;
	HSTREAM aStream = NULL;
	
	std::string anExt;
	int aDotPos = theFileName.find_last_of('.');
	if (aDotPos!=std::string::npos)
		anExt = StringToLower(theFileName.substr(aDotPos+1));

	if (anExt=="wav" || anExt=="ogg" || anExt=="mp3")
		aStream = BASS_StreamCreateFile(FALSE, (void*) theFileName.c_str(), 0, 0, 0);
	else
	{
		PFILE* aFP = p_fopen(theFileName.c_str(), "rb");
		if (aFP == NULL)
			return false;

		p_fseek(aFP, 0, SEEK_END);
		int aSize = p_ftell(aFP);
		p_fseek(aFP, 0, SEEK_SET);

		uchar* aData = new uchar[aSize];
		p_fread(aData, 1, aSize, aFP);
		p_fclose(aFP);

		aHMusic = BASS_MusicLoad(TRUE, (void*) theFileName.c_str(), 0, 0, BASS_MUSIC_LOOP);

		delete aData;
	}

	if (aHMusic==NULL && aStream==NULL)
		return false;
	
	BassMusicInfo aMusicInfo;	
	aMusicInfo.mHMusic = aHMusic;
	aMusicInfo.mHStream = aStream;
	mMusicMap.insert(BassMusicMap::value_type(theSongId, aMusicInfo));

	return true;	
}

void BassMusicInterface::PlayMusic(int theSongId, int theOffset, bool noLoop, uint64 theStartPos) //386-411
{
	BassMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		BassMusicInfo* aMusicInfo = &anItr->second;
		aMusicInfo->mVolume = aMusicInfo->mVolumeCap;
		aMusicInfo->mVolumeAdd = 0.0;
		aMusicInfo->mStopOnFade = noLoop;
		BASS_ChannelSetAttributes(aMusicInfo->GetHandle(), -1, (int) (aMusicInfo->mVolume*100), -101);			   

		BASS_ChannelStop(aMusicInfo->GetHandle());
		if (aMusicInfo->mHMusic)
		{
			BASS_MusicPlayEx(aMusicInfo->mHMusic, theOffset, BASS_MUSIC_POSRESETEX | BASS2_MUSIC_RAMP | (noLoop ? 0 : BASS_MUSIC_LOOP), TRUE, theStartPos);
		}
		else
		{
			BOOL flush = theOffset == -1 ? FALSE : TRUE;
			BASS_StreamPlay(aMusicInfo->mHStream, flush, noLoop ? 0 : BASS_MUSIC_LOOP);
			if (theOffset > 0)
				BASS_ChannelSetPosition(aMusicInfo->mHStream, theOffset, 0);
		}
	}
}

void BassMusicInterface::StopMusic(int theSongId) //414-422
{
	BassMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		BassMusicInfo* aMusicInfo = &anItr->second;
		aMusicInfo->mVolume = 0.0;
		BASS_ChannelStop(aMusicInfo->GetHandle());
	}
}

void BassMusicInterface::StopAllMusic() //425-434
{
	BassMusicMap::iterator anItr = mMusicMap.begin();
	while (anItr != mMusicMap.end())
	{
		BassMusicInfo* aMusicInfo = &anItr->second;
		aMusicInfo->mVolume = 0.0;
		BASS_ChannelStop(aMusicInfo->GetHandle());
		++anItr;
	}
}

void BassMusicInterface::UnloadMusic(int theSongId) //437-455
{
	StopMusic(theSongId);
	
	BassMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		BassMusicInfo* aMusicInfo = &anItr->second;
		if (aMusicInfo->mHStream)
			BASS_StreamFree(aMusicInfo->mHStream);
		else if (aMusicInfo->mHMusic)
			BASS_MusicFree(aMusicInfo->mHMusic);

		mMusicMap.erase(anItr);
	}
}

void BassMusicInterface::UnloadAllMusic() //458-473
{
	StopAllMusic();
	for (BassMusicMap::iterator anItr = mMusicMap.begin(); anItr != mMusicMap.end(); ++anItr)
	{
		BassMusicInfo* aMusicInfo = &anItr->second;
		if (aMusicInfo->mHStream)
			BASS_StreamFree(aMusicInfo->mHStream);
		else if (aMusicInfo->mHMusic)
			BASS_MusicFree(aMusicInfo->mHMusic);
	}
	mMusicMap.clear();
}

void BassMusicInterface::PauseMusic(int theSongId) //476-483
{
	BassMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		BassMusicInfo* aMusicInfo = &anItr->second;
		BASS_ChannelPause(aMusicInfo->GetHandle());
	}
}

void BassMusicInterface::PauseAllMusic() //486-493
{
	for (BassMusicMap::iterator anItr = mMusicMap.begin(); anItr != mMusicMap.end(); ++anItr)
	{
		BassMusicInfo* aMusicInfo = &anItr->second;
		if (BASS_ChannelIsActive(aMusicInfo->GetHandle()) == BASS_ACTIVE_PLAYING)
			BASS_ChannelPause(aMusicInfo->GetHandle());
	}
}

void BassMusicInterface::ResumeAllMusic() //496-504
{
	for (BassMusicMap::iterator anItr = mMusicMap.begin(); anItr != mMusicMap.end(); ++anItr)
	{
		BassMusicInfo* aMusicInfo = &anItr->second;

		if (BASS_ChannelIsActive(aMusicInfo->GetHandle()) == BASS_ACTIVE_PAUSED)
			BASS_ChannelResume(aMusicInfo->GetHandle());
	}
}

void BassMusicInterface::ResumeMusic(int theSongId) //507-514
{
	BassMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		BassMusicInfo* aMusicInfo = &anItr->second;
		BASS_ChannelResume(aMusicInfo->GetHandle());
	}
}

void BassMusicInterface::FadeIn(int theSongId, int theOffset, double theSpeed, bool noLoop) //517-550
{
	BassMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{
		BassMusicInfo* aMusicInfo = &anItr->second;
				
		aMusicInfo->mVolumeAdd = theSpeed;
		aMusicInfo->mStopOnFade = noLoop;

		BASS_ChannelStop(aMusicInfo->GetHandle());
		BASS_ChannelSetAttributes(aMusicInfo->GetHandle(), -1, (int) (aMusicInfo->mVolume*100), -101);
		if (aMusicInfo->mHMusic)
		{
			if (theOffset == -1)
				BASS_MusicPlay(aMusicInfo->mHMusic);
			else
			{
				BASS_MusicPlayEx(aMusicInfo->mHMusic, theOffset, BASS2_MUSIC_RAMP | (noLoop ? 0 : BASS_MUSIC_LOOP), TRUE);
			}
		}
		else
		{
			BOOL flush = theOffset == -1 ? FALSE : TRUE;
			BASS_StreamPlay(aMusicInfo->mHStream, flush, noLoop ? 0 : BASS_MUSIC_LOOP);
			if (theOffset > 0)
				BASS_ChannelSetPosition(aMusicInfo->mHStream, theOffset);
		}

	}
}

void BassMusicInterface::FadeOut(int theSongId, bool stopSong, double theSpeed) //553-568
{
	BassMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{		
		BassMusicInfo* aMusicInfo = &anItr->second;
		
		if (aMusicInfo->mVolume != 0.0)
		{
			aMusicInfo->mVolumeAdd = -theSpeed;			
		}

		aMusicInfo->mStopOnFade = stopSong;
	}
}

void BassMusicInterface::FadeOutAll(bool stopSong, double theSpeed) //571-582
{
	BassMusicMap::iterator anItr = mMusicMap.begin();
	while (anItr != mMusicMap.end())
	{
		BassMusicInfo* aMusicInfo = &anItr->second;
				
		aMusicInfo->mVolumeAdd = -theSpeed;
		aMusicInfo->mStopOnFade = stopSong;

		++anItr;
	}
}

void BassMusicInterface::SetVolume(double theVolume) //585-588
{
	BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC, (int) (theVolume * 100));
	BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, (int) (theVolume * 100));
}

void BassMusicInterface::SetSongVolume(int theSongId, double theVolume) //591-600
{
	BassMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{		
		BassMusicInfo* aMusicInfo = &anItr->second;

		aMusicInfo->mVolume = theVolume;
		BASS_ChannelSetAttributes(aMusicInfo->GetHandle(), -1, (int) (aMusicInfo->mVolume*100), -101);
	}
}

void BassMusicInterface::SetSongMaxVolume(int theSongId, double theMaxVolume) //603-613
{
	BassMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{		
		BassMusicInfo* aMusicInfo = &anItr->second;

		aMusicInfo->mVolumeCap = theMaxVolume;
		aMusicInfo->mVolume = min(aMusicInfo->mVolume, theMaxVolume);
		BASS_ChannelSetAttributes(aMusicInfo->GetHandle(), -1, (int) (aMusicInfo->mVolume*100), -101);
	}
}

bool BassMusicInterface::IsPlaying(int theSongId) //616-625
{
	BassMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{		
		BassMusicInfo* aMusicInfo = &anItr->second;
		return BASS_ChannelIsActive(aMusicInfo->GetHandle()) == BASS_ACTIVE_PLAYING;
	}

	return false;
}

void BassMusicInterface::SetMusicAmplify(int theSongId, double theAmp) //628-635
{
	BassMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{		
		BassMusicInfo* aMusicInfo = &anItr->second;		
		BASS_MusicSetAmplify(aMusicInfo->GetHandle(), (int) (theAmp * 100));
	}
}

void BassMusicInterface::Update() //638-667
{
	BassMusicMap::iterator anItr = mMusicMap.begin();
	while (anItr != mMusicMap.end())
	{
		BassMusicInfo* aMusicInfo = &anItr->second;

		if (aMusicInfo->mVolumeAdd != 0.0)
		{
			aMusicInfo->mVolume += aMusicInfo->mVolumeAdd;
			
			if (aMusicInfo->mVolume > aMusicInfo->mVolumeCap)
			{
				aMusicInfo->mVolume = aMusicInfo->mVolumeCap;
				aMusicInfo->mVolumeAdd = 0.0;
			}
			else if (aMusicInfo->mVolume < 0.0)
			{
				aMusicInfo->mVolume = 0.0;
				aMusicInfo->mVolumeAdd = 0.0;

				if (aMusicInfo->mStopOnFade)
					BASS_ChannelStop(aMusicInfo->GetHandle());
			}

			BASS_ChannelSetAttributes(aMusicInfo->GetHandle(), -1, (int) (aMusicInfo->mVolume*100), -101);
		}

		++anItr;
	}	
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// MODs are broken up into several orders or patterns. This returns the current order a song is on.
int BassMusicInterface::GetMusicOrder(int theSongId) //674-682
{
	BassMusicMap::iterator anItr = mMusicMap.find(theSongId);
	if (anItr != mMusicMap.end())
	{		
		BassMusicInfo* aMusicInfo = &anItr->second;
		int aPosition = BASS_MusicGetOrderPosition(aMusicInfo->GetHandle());
		return aPosition;
	}
	return -1;
}