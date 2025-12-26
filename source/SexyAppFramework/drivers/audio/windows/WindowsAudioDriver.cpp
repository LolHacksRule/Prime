#include "WindowsAudioDriver.h"
#include "DSoundManager.h"
#include "../bass/BassMusicInterface.h"
//#include "../fmod/FModMusicInterface.h"
//#include "../fmod/FModSoundManager.h"

using namespace Sexy;

IAudioDriver* IAudioDriver::CreateAudioDriver(SexyAppBase* app) //10-12
{
	return new WindowsAudioDriver(app);
}

WindowsAudioDriver::WindowsAudioDriver(SexyAppBase* app) //15-19
{
	mApp = app;
}

WindowsAudioDriver::~WindowsAudioDriver() //22-24
{
}

bool WindowsAudioDriver::InitAudioDriver() //27-48
{
	//Big documentation prob here
	return true;
}

SoundManager* WindowsAudioDriver::CreateSoundManager() //52-56
{
	return new DSoundManager(mApp->mInvisHWnd, false);
}

MusicInterface* WindowsAudioDriver::CreateMusicInterface() //59-64
{
#ifdef USE_FMOD
	//return new FModMusicInterface(mApp->mInvisHWnd);
#else
	return new BassMusicInterface(mApp->mInvisHWnd); //Or fmod?
#endif
}