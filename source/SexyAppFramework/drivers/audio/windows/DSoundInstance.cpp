#include "DSoundInstance.h"
#include "DSoundManager.h"
#include "../../../Debug.h"

using namespace Sexy;


DSoundInstance::DSoundInstance(DSoundManager* theSoundManager, LPDIRECTSOUNDBUFFER theSourceSound) //9-50
{
	mSoundManagerP = theSoundManager;
	mReleased = false;
	mAutoRelease = false;
	mHasPlayed = false;
	mSourceSoundBuffer = theSourceSound;
	mSoundBuffer = NULL;
	mMasterVolumeIdx = 0;

	mBaseVolume = 1.0;
	mBasePan = 0;
	mBaseRate = 1.0;

	mVolume = 1.0;
	mPan = 0;
	mRate = 1.0;

	mDefaultFrequency = 44100;

	HRESULT hr;

	if (mSourceSoundBuffer != NULL)
	{
		hr=mSoundManagerP->mDirectSound->DuplicateSoundBuffer(mSourceSoundBuffer, &mSoundBuffer);
		if (hr!=DS_OK)
		{
			switch (hr)
			{
			case DSERR_ALLOCATED: MessageBoxA(0,"DSERR_ALLOCATED","Hey",MB_OK);break;
			case DSERR_INVALIDCALL: MessageBoxA(0,"DSERR_INVALIDCALL","Hey",MB_OK);break;
			case DSERR_INVALIDPARAM: MessageBoxA(0,"DSERR_INVALIDPARAM","Hey",MB_OK);break;
			case DSERR_OUTOFMEMORY: MessageBoxA(0,"DSERR_OUTOFMEMORY","Hey",MB_OK);break;
			case DSERR_UNINITIALIZED: MessageBoxA(0,"DSERR_UNINITIALIZED","Hey",MB_OK);break;
			}
			exit(0);
		}

		mSoundBuffer->GetFrequency(&mDefaultFrequency);
	}

	RehupVolume();
}

DSoundInstance::~DSoundInstance() //53-56
{
	if (mSoundBuffer != NULL)
		mSoundBuffer->Release();
}

void DSoundInstance::RehupVolume() //59-69
{
	if (mSoundBuffer == NULL)
		return;

	int db = -10000;
	if (mSoundManagerP->mMasterVolume[mMasterVolumeIdx] > 0.01499999966472387) //?
		db = mSoundManagerP->VolumeToDB(mVolume) + mSoundManagerP->VolumeToDB(mBaseVolume) + mSoundManagerP->VolumeToDB(mSoundManagerP->mMasterVolume[mMasterVolumeIdx]);
	if (db < -DSBFREQUENCY_MAX)
		db = -DSBFREQUENCY_MAX;
	mSoundBuffer->SetVolume(db);
}

void DSoundInstance::RehupPan() //72-75
{
	if (mSoundBuffer != NULL)
		mSoundBuffer->SetPan(mBasePan + mPan);
}

void DSoundInstance::RehupRate() //78-89
{
	if (mSoundBuffer != NULL)
	{
		double aNewFrequency = mDefaultFrequency * mBaseRate * mRate;
		if (aNewFrequency < DSBFREQUENCY_MIN)
			aNewFrequency = DSBFREQUENCY_MIN;
		if (aNewFrequency > DSBFREQUENCY_MAX)
			aNewFrequency = DSBFREQUENCY_MAX;

		mSoundBuffer->SetFrequency(aNewFrequency);
	}
}

void DSoundInstance::Release() //92-95
{
	Stop();
	mReleased = true;			
}

void DSoundInstance::SetVolume(double theVolume) // 0 = max | 98-101
{
	mVolume = theVolume;
	RehupVolume();	
}

void DSoundInstance::SetMasterVolumeIdx(int theVolumeIdx) //104-107
{
	mMasterVolumeIdx = theVolumeIdx;
	RehupVolume();
}

void DSoundInstance::SetPan(int thePosition) //-db to =db = left to right | 110-113
{
	mPan = thePosition;
	RehupPan();	
}

void DSoundInstance::SetBaseVolume(double theBaseVolume) //116-119
{
	mBaseVolume = theBaseVolume;
	RehupVolume();
}

void DSoundInstance::SetBasePan(int theBasePan) //122-125
{
	mBasePan = theBasePan;
	RehupPan();
}

void DSoundInstance::SetBaseRate(double theBaseRate) //128-131
{
	mBaseRate = theBaseRate;
	RehupRate();
}

bool DSoundInstance::Play(bool looping, bool autoRelease) //134-159
{
	Stop();

	mHasPlayed = true;	
	mAutoRelease = autoRelease;	

	if (mSoundBuffer == NULL)
	{
		return false;
	}

	if (looping)
	{
		if (mSoundBuffer->Play(0, 0, DSBPLAY_LOOPING) != DS_OK)
			return false;
	}
	else
	{
		if (mSoundBuffer->Play(0, 0, 0) != DS_OK)
		{
			return false;
		}
	}

	return true;
}

void DSoundInstance::Stop() //162-169
{
	if (mSoundBuffer != NULL)
	{
		mSoundBuffer->Stop();
		mSoundBuffer->SetCurrentPosition(0);
		mAutoRelease = false;
	}
}

#include "DirectXErrorString.h"
void DSoundInstance::AdjustPitch(double theNumSteps) //173-181
{
	mRate = pow(1.0594630943592952645618252949463, theNumSteps);


	DBG_ASSERT(mRate > 0.05); //177
	DBG_ASSERT(mRate < 20.0); //178

	RehupRate();
}

bool DSoundInstance::IsPlaying() //184-197
{
	if (!mHasPlayed)
		return false;

	if (mSoundBuffer == NULL)
		return false;

	DWORD aStatus;
	if (mSoundBuffer->GetStatus(&aStatus) == DS_OK)
		// Has the sound stopped?
		return ((aStatus & DSBSTATUS_PLAYING) != 0);
	else
		return false;
}

bool DSoundInstance::IsReleased() //200-205
{
	if ((!mReleased) && (mAutoRelease) && (mHasPlayed) && (!IsPlaying()))	
		Release();	

	return mReleased;
}

double DSoundInstance::GetVolume() //208-210
{
	return mVolume; 
}

