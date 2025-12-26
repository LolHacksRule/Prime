#ifndef __DSOUNDINSTANCE_H__
#define __DSOUNDINSTANCE_H__

#include "../../../SoundInstance.h"
#include "dsoundversion.h"

namespace Sexy
{

class DSoundManager;

class DSoundInstance : public SoundInstance
{
	friend class DSoundManager;

protected:
	DSoundManager*			mSoundManagerP;
	LPDIRECTSOUNDBUFFER		mSourceSoundBuffer;
	LPDIRECTSOUNDBUFFER		mSoundBuffer;
	bool					mAutoRelease;
	bool					mHasPlayed;
	bool					mReleased;

	int						mBasePan;
	double					mBaseVolume;

	double					mBaseRate;
	int						mMasterVolumeIdx;

	int						mPan;
	double					mVolume;	
	
	double					mRate;

	DWORD					mDefaultFrequency;

protected:
	void					RehupVolume();
	void					RehupPan();
	void					RehupRate();

public:
	DSoundInstance(DSoundManager* theSoundManager, LPDIRECTSOUNDBUFFER theSourceSound);
	virtual ~DSoundInstance();	
	virtual void			Release();
	
	virtual void			SetBaseVolume(double theBaseVolume);
	virtual void			SetBasePan(int theBasePan);
	virtual void			SetBaseRate(double theBaseRate);

	virtual void			SetVolume(double theVolume); 
	
	virtual void			SetMasterVolumeIdx(int theVolumeIdx); 
	
	virtual void			SetPan(int thePosition); //-hundredth db to +hundredth db = left to right	
	virtual void			AdjustPitch(double theNumSteps);

	virtual bool			Play(bool looping, bool autoRelease);
	virtual void			Stop();
	virtual bool			IsPlaying();
	virtual bool			IsReleased();
	virtual double			GetVolume();

};

}

#endif //__DSOUNDINSTANCE_H__