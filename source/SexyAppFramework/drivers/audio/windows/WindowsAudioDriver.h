#ifndef __WINDOWSAUDIODRIVER_H__
#define __WINDOWSAUDIODRIVER_H__

#include "../../../IAudioDriver.h"

namespace Sexy
{
    class WindowsAudioDriver : public IAudioDriver
    {
    public:
        WindowsAudioDriver(SexyAppBase* app);
        ~WindowsAudioDriver();
        bool InitAudioDriver();
        SoundManager* CreateSoundManager();
        MusicInterface* CreateMusicInterface();
    private:
        SexyAppBase* mApp;
    };
}
#endif //__WINDOWSAUDIODRIVER_H__