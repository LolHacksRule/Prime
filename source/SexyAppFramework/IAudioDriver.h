#ifndef __IAUDIODRIVER_H__
#define __IAUDIODRIVER_H__

#include "SexyAppBase.h"

namespace Sexy
{
	class IAudioDriver
	{
	public:
		static IAudioDriver* CreateAudioDriver(SexyAppBase* app);
		virtual bool InitAudioDriver() = 0;
		virtual SoundManager* CreateSoundManager() = 0;
		virtual MusicInterface* CreateMusicInterface() = 0;
		virtual ~IAudioDriver() = 0; //16
	};
}

#endif //__IAUDIODRIVER_H__