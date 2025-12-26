#ifndef __BASSMUSICINTERFACE_H__
#define __BASSMUSICINTERFACE_H__

#include "..\..\..\MusicInterface.h"
#include "bass.h"

namespace Sexy
{

class SexyAppBase;

BOOL BASS_MusicPlay(HMUSIC handle);
BOOL BASS_MusicPlayEx(HMUSIC handle, DWORD pos, int flags, BOOL reset, QWORD theStartPos);
BOOL BASS_ChannelResume(HMUSIC handle);
BOOL BASS_StreamPlay(HSTREAM handle, BOOL flush, DWORD flags);
BOOL BASS_MusicSetAmplify(HMUSIC handle, DWORD amp);
DWORD BASS_MusicGetAmplify(HMUSIC handle);
BOOL BASS_MusicSetBPM(HMUSIC handle, DWORD value);
DWORD BASS_MusicGetBPM(HMUSIC handle);
BOOL BASS_MusicSetPanSep(HMUSIC handle, DWORD value);
DWORD BASS_MusicGetPanSep(HMUSIC handle);
BOOL BASS_MusicSetPScaler(HMUSIC handle, DWORD value);
DWORD BASS_MusicGetPScaler(HMUSIC handle);
BOOL BASS_MusicSetSpeed(HMUSIC handle, DWORD value);
DWORD BASS_MusicGetSpeed(HMUSIC handle);
BOOL BASS_MusicSetGlobalVolume(HMUSIC handle, DWORD value);
DWORD BASS_MusicGetGlobalVolume(HMUSIC handle);
BOOL BASS_MusicSetChannelVolumeFloat(HMUSIC handle, DWORD channel, DWORD value);
FLOAT BASS_MusicGetChannelVolumeFloat(HMUSIC handle, DWORD channel);
BOOL BASS_MusicSetChannelVolumeInt(HMUSIC handle, DWORD channel, int value);
int BASS_MusicGetChannelVolumeInt(HMUSIC handle, DWORD channel);
BOOL BASS_MusicSetInstrumentVolumeFloat(HMUSIC handle, DWORD inst, float value);
FLOAT BASS_MusicGetInstrumentVolumeFloat(HMUSIC handle, DWORD inst);
BOOL BASS_MusicSetInstrumentVolumeInt(HMUSIC handle, DWORD inst, int value);
int BASS_MusicGetInstrumentVolumeInt(HMUSIC handle, DWORD inst);
BOOL BASS_ChannelSetAttributes(HMUSIC handle, int freq, int volume, int pan);
int BASS_ChannelGetAttributes(HMUSIC handle, DWORD* freq, DWORD* volume, int* pan);
int BASS_ChannelSetPosition(HMUSIC handle, QWORD pos);
QWORD BASS_ChannelGetPosition(HMUSIC handle);
QWORD BASS_ChannelGetLength(HMUSIC handle);
DWORD BASS_MusicGetOrders(HMUSIC handle);
DWORD BASS_MusicGetOrderPosition(HMUSIC handle);

class BassMusicInfo
{
public:
	HMUSIC					mHMusic;
	HSTREAM					mHStream;
	double					mVolume;
	double					mVolumeAdd;
	double					mVolumeCap;
	bool					mStopOnFade;

public:
	BassMusicInfo();

	DWORD GetHandle() { return mHMusic?mHMusic:mHStream; } //96
};

typedef std::map<int, BassMusicInfo> BassMusicMap;

class BassMusicInterface : public MusicInterface
{
public:	
	BassMusicMap			mMusicMap;
	int						mMaxMusicVolume;
	int						mMusicLoadFlags;

public:
	BassMusicInterface(HWND theHWnd);
	virtual ~BassMusicInterface();
	
	virtual bool			LoadMusic(int theSongId, const std::string& theFileName);
	virtual void			PlayMusic(int theSongId, int theOffset = 0, bool noLoop = false, uint64 theStartPos);
	virtual void			StopMusic(int theSongId);
	virtual void			StopAllMusic();		
	virtual void			UnloadMusic(int theSongId);
	virtual void			UnloadAllMusic();
	virtual void			PauseAllMusic();
	virtual void			ResumeAllMusic();
	virtual void			PauseMusic(int theSongId);
	virtual void			ResumeMusic(int theSongId);	
	virtual void			FadeIn(int theSongId, int theOffset = -1, double theSpeed = 0.002, bool noLoop = false);
	virtual void			FadeOut(int theSongId, bool stopSong = true, double theSpeed = 0.004);
	virtual void			FadeOutAll(bool stopSong = true, double theSpeed = 0.004);
	virtual void			SetSongVolume(int theSongId, double theVolume);
	virtual void			SetSongMaxVolume(int theSongId, double theMaxVolume);
	virtual bool			IsPlaying(int theSongId);

	virtual void			SetVolume(double theVolume);
	virtual void			SetMusicAmplify(int theSongId, double theAmp); // default is 0.50
	virtual void			Update();

	// functions for dealing with MODs
	int						GetMusicOrder(int theSongId);
};

}

DWORD(WINAPI* BASS_GetVersion)();

BOOL(WINAPI* BASS_Init)(int device, DWORD freq, DWORD flags, HWND win, const GUID* dsguid);
void(WINAPI* BASS_Free)();
BOOL(WINAPI* BASS_Stop)();
BOOL(WINAPI* BASS_Start)();
void(WINAPI* BASS_SetGlobalVolumes)(int musvol, int samvol, int strvol);
BOOL(WINAPI* BASS_SetVolume)(DWORD volume);
BOOL(WINAPI* BASS_GetVolume)();
BOOL(WINAPI* BASS_GetInfo)(BASS_INFO* info);

DWORD(WINAPI* BASS_SetConfig)(DWORD option, DWORD value);
DWORD(WINAPI* BASS_GetConfig)(DWORD option);

BOOL(WINAPI* BASS_ChannelStop)(DWORD handle);
BOOL(WINAPI* BASS_ChannelPlay)(DWORD handle, BOOL restart);
BOOL(WINAPI* BASS_ChannelPause)(DWORD handle);
BOOL(WINAPI* BASS_ChannelSetAttributes)(DWORD handle, int freq, int volume, int pan);
BOOL(WINAPI* BASS_ChannelGetAttributes)(DWORD handle, DWORD* freq, DWORD* volume, int* pan);
BOOL(WINAPI* BASS_ChannelSetPosition)(DWORD handle, QWORD pos, DWORD mode);
QWORD(WINAPI* BASS_ChannelGetPosition)(DWORD handle, DWORD mode);
BOOL(WINAPI* BASS_ChannelSetFlags)(DWORD handle, DWORD flags);
DWORD(WINAPI* BASS_ChannelIsActive)(DWORD handle);
BOOL(WINAPI* BASS_ChannelSlideAttributes)(DWORD handle, int freq, int volume, int pan, DWORD time);
DWORD(WINAPI* BASS_ChannelIsSliding)(DWORD handle);
DWORD(WINAPI* BASS_ChannelGetLevel)(DWORD handle);
HFX(WINAPI* BASS_ChannelSetFX)(DWORD handle, DWORD theType, int priority);
BOOL(WINAPI* BASS_ChannelRemoveFX)(DWORD handle, HFX fx);
QWORD(WINAPI* BASS_ChannelGetLength)(DWORD handle, DWORD mode);
DWORD(WINAPI* BASS_ChannelGetData)(DWORD handle, void* buffer, DWORD length);
BOOL(WINAPI* BASS_ChannelPreBuf)(DWORD handle, DWORD length);
HSYNC(WINAPI* BASS_ChannelSetSync)(DWORD handle, DWORD theType, QWORD theParam, SYNCPROC* proc, DWORD user);
BOOL(WINAPI* BASS_ChannelRemoveSync)(DWORD handle, HSYNC sync);

HMUSIC(WINAPI* BASS_MusicLoad)(BOOL mem, void* file, DWORD offset, DWORD length, DWORD flags);
HMUSIC(WINAPI* BASS_MusicLoad2)(BOOL mem, void* file, DWORD offset, DWORD length, DWORD flags, DWORD freq);
void(WINAPI* BASS_MusicFree)(HMUSIC handle);

HSTREAM(WINAPI* BASS_StreamCreateFile)(BOOL mem, void* file, DWORD offset, DWORD length, DWORD flags);
void(WINAPI* BASS_StreamFree)(HSTREAM handle);

BOOL(WINAPI* BASS_FXSetParameters)(HFX handle, void* par);
BOOL(WINAPI* BASS_FXGetParameters)(HFX handle, void* par);

DWORD(WINAPI* BASS_MusicGetAttribute)(HMUSIC handle, DWORD attrib);
void(WINAPI* BASS_MusicSetAttribute)(HMUSIC handle, DWORD attrib, DWORD value);

DWORD(WINAPI* BASS_MusicGetOrders)(HMUSIC handle);
DWORD(WINAPI* BASS_MusicGetOrderPosition)(HMUSIC handle);

HPLUGIN(WINAPI* BASS_PluginLoad)(char* file, DWORD flags);

HSAMPLE(WINAPI* BASS_SampleLoad)(BOOL mem, void* file, DWORD offset, DWORD length, DWORD max, DWORD flags);
BOOL(WINAPI* BASS_SampleFree)(HSAMPLE handle);
BOOL(WINAPI* BASS_SampleSetInfo)(HSAMPLE handle, BASS_SAMPLE* info);
BOOL(WINAPI* BASS_SampleGetInfo)(HSAMPLE handle, BASS_SAMPLE* info);
HCHANNEL(WINAPI* BASS_SampleGetChannel)(HSAMPLE handle, BOOL onlynew);
BOOL(WINAPI* BASS_SampleStop)(HSAMPLE handle);

int (WINAPI* BASS_ErrorGetCode)();

#endif //__BASSMUSICINTERFACE_H__
