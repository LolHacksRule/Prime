#include "MusicInterface.h"
#include "SexyAppBase.h"

using namespace Sexy;

MusicInterface::MusicInterface() //7-8
{
}

MusicInterface::~MusicInterface() //11-12
{
}

bool MusicInterface::LoadMusic(int theSongId, const std::string& theFileName) //15-17
{
	return false;
}

void MusicInterface::PlayMusic(int theSongId, int theOffset, bool noLoop, uint64 theStartPos) //20-21
{
}

void MusicInterface::StopMusic(int theSongId) //24-25
{
}

void MusicInterface::PauseMusic(int theSongId) //27-29
{
}

void MusicInterface::ResumeMusic(int theSongId) //31-33
{
}

void MusicInterface::StopAllMusic() //36-37
{
}

void MusicInterface::UnloadMusic(int theSongId) //40-41
{
}

void MusicInterface::UnloadAllMusic() //44-45
{
}

void MusicInterface::PauseAllMusic() //48-49
{
}

void MusicInterface::ResumeAllMusic() //52-54
{
}

void MusicInterface::FadeIn(int theSongId, int theOffset, double theSpeed, bool noLoop) //56-57
{
}

void MusicInterface::FadeOut(int theSongId, bool stopSong, double theSpeed) //60-61
{
}

void MusicInterface::FadeOutAll(bool stopSong, double theSpeed) //64-65
{
}

void MusicInterface::SetSongVolume(int theSongId, double theVolume) //68-69
{
}

void MusicInterface::SetSongMaxVolume(int theSongId, double theMaxVolume) //72-73
{
}

bool MusicInterface::IsPlaying(int theSongId) //76-78
{
	return false;
}

void MusicInterface::SetVolume(double theVolume) //81-82
{
}

void MusicInterface::SetMusicAmplify(int theSongId, double theAmp) //85-86
{
}

void MusicInterface::Update() //89-90
{
}
