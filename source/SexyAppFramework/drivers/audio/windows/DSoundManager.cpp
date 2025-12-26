#include "DSoundManager.h"
#include <io.h>
#include <fcntl.h>
#include "..\..\..\Debug.h"
#include "DSoundInstance.h"
//#include "..\..\..\FModLoader.h"
#include <math.h>
#include "..\..\..\PakLib\PakInterface.h"
#include "../../../SexyCache.h"



using namespace Sexy;

#define USE_OGG_LIB


#ifdef USE_OGG_LIB
#include "..\..\..\ogg\ivorbiscodec.h"
#include "..\..\..\ogg\ivorbisfile.h"
#endif

#define SOUND_FLAGS (DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME |  DSBCAPS_STATIC | DSBCAPS_LOCSOFTWARE | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLFREQUENCY)
DSoundManager::DSoundManager(HWND theHWnd, bool haveFMod) //25-108
{
	mHaveFMod = haveFMod;
	mLastReleaseTick = 0;
	mPrimaryBuffer = NULL;

	int i;

	for (i = 0; i < MAX_SOURCE_SOUNDS; i++)
	{
		mSourceSounds[i] = NULL;
		mBaseVolumes[i] = 1;
		mBasePans[i] = 0;
	}

	for (i = 0; i < MAX_CHANNELS; i++)
		mPlayingSounds[i] = NULL;

	mDirectSound = NULL;

	for (i = 0; i < MAX_CHANNELS/2; i++) //?
		mMasterVolume[i] = 1.0;

	if (theHWnd != NULL)
	{
		extern HMODULE gDSoundDLL;
		typedef HRESULT (WINAPI *DirectSoundCreateFunc)(LPCGUID lpcGuid, LPDIRECTSOUND * ppDS, LPUNKNOWN  pUnkOuter);
		DirectSoundCreateFunc aDirectSoundCreateFunc = (DirectSoundCreateFunc)GetProcAddress(gDSoundDLL,"DirectSoundCreate8");

		if (aDirectSoundCreateFunc != NULL && aDirectSoundCreateFunc(NULL, &mDirectSound, NULL) == DS_OK)
		{
			//FSOUND_SetOutput(FSOUND_OUTPUT_WINMM); //FMOD is not in Bejeweled 3 on Win but IS on Xbox 360
			/*
			if (mHaveFMod)
            {
                LoadFModDLL();
 
                gFMod->FSOUND_SetHWND(theHWnd);
                gFMod->FSOUND_SetBufferSize(200); // #LUC
                gFMod->FSOUND_Init(44100, 64, FSOUND_INIT_GLOBALFOCUS);  
            }
			*/
			HRESULT aResult = mDirectSound->SetCooperativeLevel(theHWnd,DSSCL_PRIORITY);
			if (SUCCEEDED(aResult))
			{
				// Set primary buffer to 16-bit 44.1Khz
				WAVEFORMATEX aWaveFormat;
				DSBUFFERDESC aBufferDesc;

				// Set up wave format structure.
				int aBitCount = 16;
				int aChannelCount = 2;
				int aSampleRate = 44100;

				// Set up wave format structure.
				memset(&aWaveFormat, 0, sizeof(WAVEFORMATEX));
				aWaveFormat.cbSize = sizeof(WAVEFORMATEX);
				aWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
				aWaveFormat.nChannels = aChannelCount;
				aWaveFormat.nSamplesPerSec = aSampleRate;
				aWaveFormat.nBlockAlign = aChannelCount*aBitCount/8;
				aWaveFormat.nAvgBytesPerSec = 
				aWaveFormat.nSamplesPerSec * aWaveFormat.nBlockAlign;
				aWaveFormat.wBitsPerSample = aBitCount;

				// Set up DSBUFFERDESC structure.
				memset(&aBufferDesc, 0, sizeof(DSBUFFERDESC)); // Zero it out.
				aBufferDesc.dwSize = sizeof(DSBUFFERDESC1);
				aBufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;//| DSBCAPS_CTRL3D; // Need default controls (pan, volume, frequency).
				aBufferDesc.dwBufferBytes = 0;
				aBufferDesc.lpwfxFormat =NULL;//(LPWAVEFORMATEX)&aWaveFormat;

				HRESULT aResult = mDirectSound->CreateSoundBuffer(&aBufferDesc, &mPrimaryBuffer, NULL);
				if (aResult == DS_OK)
				{
					aResult = mPrimaryBuffer->SetFormat(&aWaveFormat);
				}
			}
			else
			{
				aResult = mDirectSound->SetCooperativeLevel(theHWnd,DSSCL_NORMAL);
			}
		}
	}	
}

DSoundManager::~DSoundManager() //111-130
{
	ReleaseChannels();
	ReleaseSounds();

	if (mPrimaryBuffer)
		mPrimaryBuffer->Release();

	if (mDirectSound != NULL)
	{
		/*if (mHaveFMod)
			gFMod->FSOUND_Close();*/

		mDirectSound->Release();

		/*if (mHaveFMod)
			FreeFModDLL();*/
	}


}

int	DSoundManager::FindFreeChannel() //133-158
{
	int aTry = 0;
	DWORD aTick = GetTickCount();
	if (aTick-mLastReleaseTick > 1000 || aTry == 1)
	{
		ReleaseFreeChannels();
		mLastReleaseTick = aTick;
	}

	for (int i = 0; i < MAX_CHANNELS; i++)
	{		
		if (mPlayingSounds[i] == NULL)
			return i;
		
		if (mPlayingSounds[i]->IsReleased())
		{
			delete mPlayingSounds[i];
			mPlayingSounds[i] = NULL;
			return i;
		}
	}
	++aTry;

	if (aTry >= 2)
		return -1;
}

bool DSoundManager::Initialized() //161-170
{
/*
	if (mDirectSound!=NULL)
	{
		mDirectSound->SetCooperativeLevel(theHWnd,DSSCL_NORMAL);
	}
*/

	return (mDirectSound != NULL);
}

int DSoundManager::VolumeToDB(double theVolume) //173-179
{
	int aVol = (int) ((log10(1 + theVolume*9) - 1.0) * 2333);
	if (aVol < -2331) //Why idk
		aVol = -10000;

	return aVol;
}

void DSoundManager::SetVolume(double theVolume) //182-184
{
	SetVolume(0);
}

void DSoundManager::SetVolume(int theVolIdx, double theVolume) //187-193
{
	mMasterVolume[theVolIdx] = theVolume;

	for (int i = 0; i < MAX_CHANNELS; i++)
		if (mPlayingSounds[i] != NULL)
			mPlayingSounds[i]->RehupVolume();
}

bool DSoundManager::LoadWAVSound(unsigned int theSfxID, const std::string& theFilename) //196-336 (UNMATCHED)
{		
	int aDataSize;

	PFILE* fp;

	fp = p_fopen(theFilename.c_str(), "rb");

	if (fp <= 0)
		return false;	

	char aChunkType[5];	
	aChunkType[4] = '\0';
	ulong aChunkSize;

	p_fread(aChunkType, 1, 4, fp);	
	if (!strcmp(aChunkType, "RIFF") == 0)
		return false;
	p_fread(&aChunkSize, 4, 1, fp);

	p_fread(aChunkType, 1, 4, fp);	
	if (!strcmp(aChunkType, "WAVE") == 0)
		return false;

	ushort aBitCount = 16;
	ushort aChannelCount = 1;
	ulong aSampleRate = 22050;
	uchar anXor = 0;

	while (!p_feof(fp))
	{
		p_fread(aChunkType, 1, 4, fp);		
		if (p_fread(&aChunkSize, 4, 1, fp) == 0)
			return false;

		int aCurPos = p_ftell(fp);

		if (strcmp(aChunkType, "fmt ") == 0)
		{
			ushort aFormatTag;
			ulong aBytesPerSec;
			ushort aBlockAlign;			

			p_fread(&aFormatTag, 2, 1, fp);
			p_fread(&aChannelCount, 2, 1, fp);
			p_fread(&aSampleRate, 4, 1, fp);
			p_fread(&aBytesPerSec, 4, 1, fp);
			p_fread(&aBlockAlign, 2, 1, fp);
			p_fread(&aBitCount, 2, 1, fp);

			if (aFormatTag != 1)
				return false;
		}
		else if (strcmp(aChunkType, "dep ") == 0)
		{
			char aStr[256];
			ushort aStrLen;

			p_fread(&aStrLen, 2, 1, fp);
			if (aStrLen > 255)
				aStrLen = 255;
			p_fread(aStr, 1, aStrLen, fp);
			aStr[aStrLen] = '\0';

			FILETIME aSavedFileTime;
			p_fread(&aSavedFileTime, sizeof(FILETIME), 1, fp);

			FILETIME anActualFileTime;
			memset(&anActualFileTime, 0, sizeof(FILETIME));
			GetTheFileTime(aStr, &anActualFileTime);

			if ((aSavedFileTime.dwHighDateTime != anActualFileTime.dwHighDateTime) ||
				(aSavedFileTime.dwLowDateTime  != anActualFileTime.dwLowDateTime ))
				return false;				
		}
		else if (strcmp(aChunkType, "xor ") == 0)
		{			
			p_fread(&anXor, 1, 1, fp);			
		}
		else if (strcmp(aChunkType, "data") == 0)
		{
			aDataSize = aChunkSize;

			mSourceDataSizes[theSfxID] = aChunkSize;

			PCMWAVEFORMAT aWaveFormat;
			DSBUFFERDESC aBufferDesc;    			

			// Set up wave format structure.
			memset(&aWaveFormat, 0, sizeof(PCMWAVEFORMAT));
			aWaveFormat.wf.wFormatTag = WAVE_FORMAT_PCM;
			aWaveFormat.wf.nChannels = aChannelCount;
			aWaveFormat.wf.nSamplesPerSec = aSampleRate;
			aWaveFormat.wf.nBlockAlign = aChannelCount*aBitCount/8;
			aWaveFormat.wf.nAvgBytesPerSec = 
				aWaveFormat.wf.nSamplesPerSec * aWaveFormat.wf.nBlockAlign;
			aWaveFormat.wBitsPerSample = aBitCount;
			// Set up DSBUFFERDESC structure.
			memset(&aBufferDesc, 0, sizeof(DSBUFFERDESC)); // Zero it out.
			aBufferDesc.dwSize = sizeof(DSBUFFERDESC);
			//aBufferDesc.dwFlags = DSBCAPS_CTRL3D; 
			aBufferDesc.dwFlags = SOUND_FLAGS; //DSBCAPS_CTRLDEFAULT;

			//aBufferDesc.dwFlags = 0;

			aBufferDesc.dwBufferBytes = aDataSize;                                                             
			aBufferDesc.lpwfxFormat = (LPWAVEFORMATEX)&aWaveFormat;

			if (mDirectSound->CreateSoundBuffer(&aBufferDesc, &mSourceSounds[theSfxID], NULL) != DS_OK)
			{				
				p_fclose(fp);
				return false;
			}


			void* lpvPtr;
			DWORD dwBytes;
			if (mSourceSounds[theSfxID]->Lock(0, aDataSize, &lpvPtr, &dwBytes, NULL, NULL, 0) != DS_OK)
			{
				p_fclose(fp);
				return false;
			}

			int aReadSize = p_fread(lpvPtr, 1, aDataSize, fp);
			p_fclose(fp);

			for (int i = 0; i < aDataSize; i++)
				((uchar*) lpvPtr)[i] ^= anXor;

			if (mSourceSounds[theSfxID]->Unlock(lpvPtr, dwBytes, NULL, NULL) != DS_OK)
				return false;

			if (aReadSize != aDataSize)
				return false;

			return true;
		}

		p_fseek(fp, aCurPos+aChunkSize, SEEK_SET);
	}
	return false;
}

// Load FMod sound can handle oggs and mp3s and whatever else fmod can decode
bool DSoundManager::LoadFModSound(unsigned int theSfxID, const std::string& theFilename) //Was this changed | 340-430
{
	if (!mHaveFMod)
		return false;

	DBG_ASSERT(false); //344

	//Commented/defined stuff?
}

#ifdef USE_OGG_LIB

static int p_fseek64_wrap(PFILE *f,ogg_int64_t off,int whence){ //434-437
	if(f==NULL)return(-1);
	return p_fseek(f,(long)off,whence);
}

int ov_pak_open(PFILE *f,OggVorbis_File *vf,char *initial,long ibytes){ //439-448
	ov_callbacks callbacks = {
		(size_t (*)(void *, size_t, size_t, void *))  p_fread,
		(int (*)(void *, ogg_int64_t, int))             p_fseek64_wrap,
		(int (*)(void *))                             p_fclose,
		(long (*)(void *))                            p_ftell
	};

	return ov_open_callbacks((void *)f, vf, initial, ibytes, callbacks);
}

bool DSoundManager::LoadOGGSound(unsigned int theSfxID, const std::string& theFilename) //451-523
{
	OggVorbis_File vf;
	int current_section;

	PFILE *aFile = p_fopen(theFilename.c_str(),"rb");
	if (aFile==NULL)
		return false;

	if(ov_pak_open(aFile, &vf, NULL, 0) < 0) 
	{
		p_fclose(aFile);
		return false;
	}
  
	vorbis_info *anInfo = ov_info(&vf,-1);
	
	PCMWAVEFORMAT aWaveFormat;
	DSBUFFERDESC aBufferDesc;    			

	// Set up wave format structure.
	memset(&aWaveFormat, 0, sizeof(PCMWAVEFORMAT));
	aWaveFormat.wf.wFormatTag = WAVE_FORMAT_PCM;
	aWaveFormat.wf.nChannels = anInfo->channels;
	aWaveFormat.wf.nSamplesPerSec = anInfo->rate;
	aWaveFormat.wBitsPerSample = 16;
	aWaveFormat.wf.nBlockAlign = aWaveFormat.wf.nChannels*aWaveFormat.wBitsPerSample/8;
	aWaveFormat.wf.nAvgBytesPerSec = aWaveFormat.wf.nSamplesPerSec * aWaveFormat.wf.nBlockAlign;	

	int aLenBytes = (int) (ov_pcm_total(&vf,-1) * aWaveFormat.wf.nBlockAlign);	
	memset(&aBufferDesc, 0, sizeof(DSBUFFERDESC)); // Zero it out.

	mSourceDataSizes[theSfxID] = aLenBytes;
	
	//FUNK
	aBufferDesc.dwSize = sizeof(DSBUFFERDESC);
	aBufferDesc.dwFlags = SOUND_FLAGS;
	aBufferDesc.dwBufferBytes = aLenBytes;
	aBufferDesc.lpwfxFormat =(LPWAVEFORMATEX)&aWaveFormat;	

	if (mDirectSound->CreateSoundBuffer(&aBufferDesc, &mSourceSounds[theSfxID], NULL) != DS_OK)
	{
		ov_clear(&vf);
		return false;
	}

	char* aBuf;
	DWORD dwBytes;
	if (mSourceSounds[theSfxID]->Lock(0, aLenBytes, (LPVOID*)&aBuf, &dwBytes, NULL, NULL, 0) != DS_OK)
	{
		ov_clear(&vf);
		return false;
	}

	char *aPtr = aBuf;
	int aNumBytes = dwBytes;
	while(aNumBytes > 0)
	{		
		long ret=ov_read(&vf,aPtr,aNumBytes,&current_section);
		if (ret == 0)
			break;
		else if (ret < 0) 
			break;
		else 
		{
			aPtr += ret;
			aNumBytes -= ret;
		}
	}

	mSourceSounds[theSfxID]->Unlock(aBuf, dwBytes, NULL, 0);
	ov_clear(&vf);
	return aNumBytes==0;  
}
#else
bool DSoundManager::LoadOGGSound(unsigned int theSfxID, const std::string& theFilename)
{
	return false;
}
#endif


bool DSoundManager::LoadAUSound(unsigned int theSfxID, const std::string& theFilename) //Correct? | 533-689
{
	PFILE* fp;

	fp = p_fopen(theFilename.c_str(), "rb");	

	if (fp <= 0)
		return false;	

	char aHeaderId[5];	
	aHeaderId[4] = '\0';	
	p_fread(aHeaderId, 1, 4, fp);	
	if (!strcmp(aHeaderId, ".snd") == 0)
		return false;

	ulong aHeaderSize;	
	p_fread(&aHeaderSize, 4, 1, fp);
	aHeaderSize = LONG_BIGE_TO_NATIVE(aHeaderSize);

	ulong aDataSize;
	p_fread(&aDataSize, 4, 1, fp);
	aDataSize = LONG_BIGE_TO_NATIVE(aDataSize);

	ulong anEncoding;
	p_fread(&anEncoding, 4, 1, fp);
	anEncoding = LONG_BIGE_TO_NATIVE(anEncoding);

	ulong aSampleRate;
	p_fread(&aSampleRate, 4, 1, fp);
	aSampleRate = LONG_BIGE_TO_NATIVE(aSampleRate);

	ulong aChannelCount;
	p_fread(&aChannelCount, 4, 1, fp);
	aChannelCount = LONG_BIGE_TO_NATIVE(aChannelCount);

	p_fseek(fp, aHeaderSize, SEEK_SET);	

	bool ulaw = false;

	ulong aSrcBitCount = 8;
	ulong aBitCount = 16;			
	switch (anEncoding)
	{
	case 1:
		aSrcBitCount = 8;
		aBitCount = 16;
		ulaw = true;
		break;
	case 2:
		aSrcBitCount = 8;
		aBitCount = 8;
		break;
	
	/*
	Support these formats?
	
	case 3:
		aBitCount = 16;
		break;
	case 4:
		aBitCount = 24;
		break;
	case 5:
		aBitCount = 32;
		break;*/

	default:
		return false;		
	}
	

	ulong aDestSize = aDataSize * aBitCount/aSrcBitCount;
	mSourceDataSizes[theSfxID] = aDestSize;

	PCMWAVEFORMAT aWaveFormat;
	DSBUFFERDESC aBufferDesc;    			

	// Set up wave format structure.
	memset(&aWaveFormat, 0, sizeof(PCMWAVEFORMAT));
	aWaveFormat.wf.wFormatTag = WAVE_FORMAT_PCM;
	aWaveFormat.wf.nChannels = (WORD) aChannelCount;
	aWaveFormat.wf.nSamplesPerSec = aSampleRate;
	aWaveFormat.wf.nBlockAlign = (WORD) (aChannelCount*aBitCount/8);
	aWaveFormat.wf.nAvgBytesPerSec = 
		aWaveFormat.wf.nSamplesPerSec * aWaveFormat.wf.nBlockAlign;
	aWaveFormat.wBitsPerSample = (WORD) aBitCount;
	// Set up DSBUFFERDESC structure.
	memset(&aBufferDesc, 0, sizeof(DSBUFFERDESC)); // Zero it out.
	aBufferDesc.dwSize = sizeof(DSBUFFERDESC);
	//aBufferDesc.dwFlags = DSBCAPS_CTRL3D; 
	aBufferDesc.dwFlags = SOUND_FLAGS;
	aBufferDesc.dwBufferBytes = aDestSize;
	aBufferDesc.lpwfxFormat = (LPWAVEFORMATEX)&aWaveFormat;

	if (mDirectSound->CreateSoundBuffer(&aBufferDesc, &mSourceSounds[theSfxID], NULL) != DS_OK)
	{
		p_fclose(fp);
		return false;
	}		

	void* lpvPtr;
	DWORD dwBytes;
	if (mSourceSounds[theSfxID]->Lock(0, aDestSize, &lpvPtr, &dwBytes, NULL, NULL, 0) != DS_OK)
	{
		p_fclose(fp);
		return false;
	}

	uchar* aSrcBuffer = new uchar[aDataSize];
	
	int aReadSize = p_fread(aSrcBuffer, 1, aDataSize, fp);
	p_fclose(fp);

	if (ulaw)
	{
		short* aDestBuffer = (short*) lpvPtr;

		for (ulong i = 0; i < aDataSize; i++)
		{
			int ch = aSrcBuffer[i];

			int sign = (ch < 128) ? -1 : 1;
			ch = ch | 0x80;
			if (ch > 239)
				ch = ((0xF0 | 15) - ch) * 2;
			else if (ch > 223)
				ch = (((0xE0 | 15) - ch) * 4) + 32;
			else if (ch > 207)
				ch = (((0xD0 | 15) - ch) * 8) + 96;
			else if (ch > 191)
				ch = (((0xC0 | 15) - ch) * 16) + 224;
			else if (ch > 175)
				ch = (((0xB0 | 15) - ch) * 32) + 480;
			else if (ch > 159)
				ch = (((0xA0 | 15) - ch) * 64) + 992;
			else if (ch > 143)
				ch = (((0x90 | 15) - ch) * 128) + 2016;
			else if (ch > 128)
				ch = (((0x80 | 15) - ch) * 256) + 4064;
			else
				ch = 0xff;			

			aDestBuffer[i] = sign * ch * 4;
		}		
	}
	else
		memcpy(lpvPtr, aSrcBuffer, aDataSize);	

	delete [] aSrcBuffer;		

	if (mSourceSounds[theSfxID]->Unlock(lpvPtr, dwBytes, NULL, NULL) != DS_OK)
		return false;

	if (aReadSize != aDataSize)
		return false;
	
	return true;
}

bool DSoundManager::LoadSound(unsigned int theSfxID, const std::string& theFilename) //Correct? | 692-759
{
	if ((theSfxID < 0) || (theSfxID >= MAX_SOURCE_SOUNDS))
		return false;

	ReleaseSound(theSfxID);

	if (!mDirectSound)
		return true; // sounds just	won't play, but this is not treated as a failure condition

	mSourceFileNames[theSfxID] = theFilename;

	std::string aFilename = theFilename;
	std::string aCachedName;

	if (ReadFromSexyCache(theSfxID, aFilename))
		return true;

	if ((aFilename.length() > 2) && (aFilename[0] != '\\') && (aFilename[0] != '/') &&
		(aFilename[1] != ':'))
	{
		// Not an absolute path
		aCachedName = GetAppDataFolder() + "cached\\" + aFilename + ".wav";
		if (LoadWAVSound(theSfxID, aCachedName))
		{
			WriteToSexyCache(theSfxID, aFilename);
			return true;
		}
		MkDir(GetFileDir(aCachedName));
	}		

	if (LoadWAVSound(theSfxID, aFilename + ".wav"))
		return true;

	if (mHaveFMod)
	{
		if (LoadFModSound(theSfxID, aFilename + ".mp3"))
		{
			WriteToSexyCache(theSfxID, aFilename);
			WriteWAV(theSfxID, aCachedName, aFilename + ".mp3");
			return true;
		}
#ifndef USE_OGG_LIB
		if (LoadFModSound(theSfxID, aFilename + ".ogg"))
		{		
			WriteToSexyCache(theSfxID, aFilename);
			WriteWAV(theSfxID, aCachedName, aFilename + ".ogg");
			return true;
		}
#endif
	}
	
#ifdef USE_OGG_LIB
	if (LoadOGGSound(theSfxID, aFilename + ".ogg"))
	{		
		WriteToSexyCache(theSfxID, aFilename);
		WriteWAV(theSfxID, aCachedName, aFilename + ".ogg");
		return true;
	}
#endif

	if (LoadAUSound(theSfxID, aFilename + ".au"))
	{
		WriteToSexyCache(theSfxID, aFilename);
		WriteWAV(theSfxID, aCachedName, aFilename + ".au");
		return true;
	}

	return false;
}

int DSoundManager::LoadSound(const std::string& theFilename) //762-780
{
	int i;
	for (i = 0; i < MAX_SOURCE_SOUNDS; i++)
		if (mSourceFileNames[i] == theFilename)
			return i;

	for (i = MAX_SOURCE_SOUNDS-1; i >= 0; i--)
	{		
		if (mSourceSounds[i] == NULL)
		{
			if (!LoadSound(i, theFilename))
				return -1;
			else
				return i;
		}
	}	

	return -1;
}

void DSoundManager::ReleaseSound(unsigned int theSfxID) //783-790
{
	if (mSourceSounds[theSfxID] != NULL)
	{
		mSourceSounds[theSfxID]->Release();
		mSourceSounds[theSfxID] = NULL;
		mSourceFileNames[theSfxID] = "";
	}
}

int DSoundManager::GetFreeSoundId() //793-801
{
	for (int i=0; i<MAX_SOURCE_SOUNDS; i++)
	{
		if (mSourceSounds[i]==NULL)
			return i;
	}

	return -1;
}

int DSoundManager::GetNumSounds() //804-813
{
	int aCount = 0;
	for (int i=0; i<MAX_SOURCE_SOUNDS; i++)
	{
		if (mSourceSounds[i]!=NULL)
			aCount++;
	}

	return aCount;
}

bool DSoundManager::SetBaseVolume(unsigned int theSfxID, double theBaseVolume) //816-822
{
	if ((theSfxID < 0) || (theSfxID >= MAX_SOURCE_SOUNDS))
		return false;

	mBaseVolumes[theSfxID] = theBaseVolume;
	return true;
}

bool DSoundManager::SetBasePan(unsigned int theSfxID, int theBasePan) //825-831
{
	if ((theSfxID < 0) || (theSfxID >= MAX_SOURCE_SOUNDS))
		return false;

	mBasePans[theSfxID] = theBasePan;
	return true;
}

bool DSoundManager::GetTheFileTime(const std::string& theDepFile, FILETIME* theFileTime) //834-843
{	
	memset(theFileTime, 0, sizeof(FILETIME));
	HANDLE aDepFileHandle = CreateFile(theDepFile.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (aDepFileHandle == INVALID_HANDLE_VALUE)
		return false;
	
	GetFileTime(aDepFileHandle, NULL, NULL, theFileTime);	
	CloseHandle(aDepFileHandle);
	return true;
}

bool DSoundManager::WriteWAV(unsigned int theSfxID, const std::string& theFilename, const std::string& theDepFile) //846-933
{
	if ((theFilename.length() == 0) || (theSfxID < 0) || (theSfxID >= MAX_SOURCE_SOUNDS))
		return false;

	ulong aDataSize = mSourceDataSizes[theSfxID];

	void* lpvPtr;
	DWORD dwBytes;
	if (mSourceSounds[theSfxID]->Lock(0, aDataSize, &lpvPtr, &dwBytes, NULL, NULL, 0) != DS_OK)
		return false;

	FILE* fp;
	fp = fopen(theFilename.c_str(), "wb");

	if (fp <= 0)
	{
		mSourceSounds[theSfxID]->Unlock(lpvPtr, dwBytes, NULL, NULL);
		return false;
	}	

	char aChunkType[5];	
	aChunkType[4] = '\0';
	ulong aChunkSize = 4 + 8 + 16 + 8 + aDataSize;

	fwrite("RIFF", 1, 4, fp);	
	fwrite(&aChunkSize, 4, 1, fp);
	fwrite("WAVE", 1, 4, fp);

	ulong aBufferSize;
	mSourceSounds[theSfxID]->GetFormat(NULL, 0, &aBufferSize);

	WAVEFORMATEX* aWaveFormat = (WAVEFORMATEX*) new char[aBufferSize];
	memset(aWaveFormat, 0, sizeof(WAVEFORMATEX));	
	mSourceSounds[theSfxID]->GetFormat(aWaveFormat, aBufferSize, NULL);

	ushort aFormatTag = 1;
	ushort aChannelCount = aWaveFormat->nChannels;
	ulong aSampleRate = aWaveFormat->nSamplesPerSec;	
	ushort aBitCount = aWaveFormat->wBitsPerSample;
	ushort aBlockAlign = (aBitCount * aChannelCount) / 8;
	ulong aBytesPerSec = aSampleRate * aBlockAlign;
	
	delete aWaveFormat;

	aChunkSize = 16;
	fwrite("fmt ", 1, 4, fp);
	fwrite(&aChunkSize, 1, 4, fp);
	fwrite(&aFormatTag, 2, 1, fp);
	fwrite(&aChannelCount, 2, 1, fp);
	fwrite(&aSampleRate, 4, 1, fp);
	fwrite(&aBytesPerSec, 4, 1, fp);
	fwrite(&aBlockAlign, 2, 1, fp);
	fwrite(&aBitCount, 2, 1, fp);

	FILETIME aFileTime;
	memset(&aFileTime, 0, sizeof(FILETIME));
	GetTheFileTime(theDepFile, &aFileTime);

	ushort aStrLen = theDepFile.length();
	aChunkSize = 2 + aStrLen + sizeof(FILETIME);
	fwrite("dep ", 1, 4, fp);
	fwrite(&aChunkSize,4, 1, fp);
	fwrite(&aStrLen, 2, 1, fp);
	fwrite(theDepFile.c_str(), 1, aStrLen, fp);
	fwrite(&aFileTime, sizeof(FILETIME), 1, fp);
	
	aChunkSize = 1;
	uchar anXor = 0xF7;
	fwrite("xor ", 1, 4, fp);
	fwrite(&aChunkSize, 4, 1, fp); 
	fwrite(&anXor, 1, 1, fp);	

	for (DWORD i = 0; i < dwBytes; i++)
		((uchar*) lpvPtr)[i] ^= anXor;

	fwrite("data", 1, 4, fp);
	fwrite(&aDataSize, 4, 1, fp);
	fwrite(lpvPtr, 1, aDataSize, fp);
	fclose(fp);

	for (DWORD i = 0; i < dwBytes; i++)
		((uchar*) lpvPtr)[i] ^= anXor;

	if (mSourceSounds[theSfxID]->Unlock(lpvPtr, dwBytes, NULL, NULL) != DS_OK)
		return false;

	return true;
}

bool DSoundManager::WriteToSexyCache(unsigned int theSfxID, const std::string& theSrcFile) //936-1001
{
	if (!gSexyCache.Connected())
		return false;

	if (!theSrcFile.length() || theSfxID >= MAX_SOURCE_SOUNDS)
		return false;

	ulong aDataSize = mSourceDataSizes[theSfxID];
	int aSize = aDataSize + 18;
	void* thePtr = gSexyCache.AllocSetData(GetFullPath(theSrcFile), "Sound", aSize);

	if (!thePtr)
		return false;

	void* p = thePtr;
	void* lpvPtr;
	ulong dwBytes;

	if (mSourceSounds[theSfxID]->Lock(0, aDataSize, &lpvPtr, &dwBytes, NULL, NULL, 0) != DS_OK)
		return false;

	ulong aBufferSize;
	WAVEFORMATEX* aWaveFormat = (WAVEFORMATEX*) new char[aBufferSize];
	ZeroMemory(aWaveFormat, sizeof WAVEFORMATEX);
	mSourceSounds[theSfxID]->GetFormat(aWaveFormat, aBufferSize, 0);
	ushort aFormatTag = 1;
	ushort aChannelCount = aWaveFormat->nChannels;
	ulong aSampleRate = aWaveFormat->nSamplesPerSec;
	ushort aBitCount = aWaveFormat->wBitsPerSample;
	ushort aBlockAlign = (aBitCount * aChannelCount) / 8;
	ulong aBytesPerSec = aSampleRate * aBlockAlign;
	delete aWaveFormat;
	SMemW(&p, &aDataSize, 4);
	SMemW(&p, &aChannelCount, 2);
	SMemW(&p, &aSampleRate, 4);
	SMemW(&p, &aBitCount, 2);
	SMemW(&p, &aBlockAlign, 2);
	SMemW(&p, &aBytesPerSec, 4);
	SMemW(&p, lpvPtr, dwBytes);
	int aWriteSize = (char*)p - thePtr;
	DBG_ASSERTE(aWriteSize == aSize); //990
	gSexyCache.SetData(thePtr);
	gSexyCache.FreeSetData(thePtr);

	if (mSourceSounds[theSfxID]->Unlock(lpvPtr, dwBytes, NULL, NULL) != DS_OK)
		return false;

	gSexyCache.SetFileDeps(GetFullPath(theSrcFile), "Sound", GetFullPath(theSrcFile) + ".*");
	return true;
}

bool DSoundManager::CheckSexyCache(const std::string& theSrcFile) //1004-1006
{
	return gSexyCache.CheckData(GetFullPath(theSrcFile), "*");
}

bool DSoundManager::SetCacheUpToDate(const std::string& theSrcFile) //1009-1011
{
	return gSexyCache.SetUpToDate(GetFullPath(theSrcFile), "*");
}

bool DSoundManager::ReadFromSexyCache(unsigned int theSfxID, const std::string& theSrcFile) //1014-1084
{
	void* thePtr;
	int theSize;

	if (!gSexyCache.GetData(GetFullPath(theSrcFile), "Sound", &thePtr, &theSize))
		return false;

	void* p = thePtr;
	ulong aDataSize;
	ushort aChannelCount;
	ulong aSampleRate;
	ushort aBitCount;
	ushort aBlockAlign;
	ulong aBytesPerSec;
	SMemR(&p, &aDataSize, 4);
	SMemR(&p, &aChannelCount, 2);
	SMemR(&p, &aSampleRate, 4);
	SMemR(&p, &aBitCount, 2);
	SMemR(&p, &aBlockAlign, 2);
	SMemR(&p, &aBytesPerSec, 4);
	mSourceDataSizes[theSfxID] = aDataSize;
	PCMWAVEFORMAT aWaveFormat;
	DSBUFFERDESC aBufferDesc;
	ZeroMemory(&aWaveFormat, sizeof(PCMWAVEFORMAT));
	aWaveFormat.wf.wFormatTag = WAVE_FORMAT_PCM;
	aWaveFormat.wf.nChannels = aChannelCount;
	aWaveFormat.wf.nSamplesPerSec = aSampleRate;
	aWaveFormat.wf.nBlockAlign = aChannelCount * aBitCount / 8;
	aWaveFormat.wf.nAvgBytesPerSec = aSampleRate * aWaveFormat.wf.nBlockAlign;
	aWaveFormat.wBitsPerSample = aBitCount;
	ZeroMemory(&aBufferDesc, sizeof(DSBUFFERDESC));
	aBufferDesc.dwSize = sizeof(DSBUFFERDESC);
	aBufferDesc.dwFlags = SOUND_FLAGS;
	aBufferDesc.dwBufferBytes = aDataSize;
	aBufferDesc.lpwfxFormat = (LPWAVEFORMATEX)&aWaveFormat;
	void* lpvPtr;
	DWORD dwBytes;
	if (mDirectSound->CreateSoundBuffer(&aBufferDesc, &mSourceSounds[theSfxID], NULL) || mSourceSounds[theSfxID]->Lock(0, aDataSize, &lpvPtr, &dwBytes, NULL, NULL, 0))
	{
		gSexyCache.FreeGetData(thePtr);
		return false;
	}
	else
	{
		SMemR(&p, lpvPtr, aDataSize);
		gSexyCache.FreeGetData(thePtr);
		return mSourceSounds[theSfxID]->Unlock(lpvPtr, dwBytes, NULL, NULL) != DS_OK;
	}
}

SoundInstance* DSoundManager::GetSoundInstance(unsigned int theSfxID) //1089-1111
{
	if (theSfxID > MAX_SOURCE_SOUNDS)
		return NULL;

	int aFreeChannel = FindFreeChannel();
	if (aFreeChannel < 0)
		return NULL;

	if (mDirectSound==NULL)
	{
		mPlayingSounds[aFreeChannel] = new DSoundInstance(this, NULL);
	}
	else
	{
		if (mSourceSounds[theSfxID] == NULL)
			return NULL;

		mPlayingSounds[aFreeChannel] = new DSoundInstance(this, mSourceSounds[theSfxID]);
	}

	mPlayingSounds[aFreeChannel]->SetBasePan(mBasePans[theSfxID]);
	mPlayingSounds[aFreeChannel]->SetBaseVolume(mBaseVolumes[theSfxID]);

	return mPlayingSounds[aFreeChannel];
}

void DSoundManager::ReleaseSounds() //1114-1122
{
	for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
		if (mSourceSounds[i] != NULL)
		{
			mSourceSounds[i]->Release();
			mSourceSounds[i] = NULL;
			mSourceFileNames[i].clear();
		}
}

void DSoundManager::ReleaseChannels() //1125-1132
{
	for (int i = 0; i < MAX_CHANNELS; i++)
		if (mPlayingSounds[i] != NULL)
		{
			delete mPlayingSounds[i];
			mPlayingSounds[i] = NULL;
		}
}

void DSoundManager::ReleaseFreeChannels() //Changed? | 1135-1142
{
	for (int i = 0; i < MAX_CHANNELS; i++)
		if (mPlayingSounds[i] != NULL && mPlayingSounds[i]->IsReleased())
		{
			delete mPlayingSounds[i];
			mPlayingSounds[i] = NULL;
		}
}

void DSoundManager::StopAllSounds() //1145-1153
{
	for (int i = 0; i < MAX_CHANNELS; i++)
		if (mPlayingSounds[i] != NULL)
		{
			bool isAutoRelease = mPlayingSounds[i]->mAutoRelease;
			mPlayingSounds[i]->Stop();
			mPlayingSounds[i]->mAutoRelease = isAutoRelease;
		}
}


double DSoundManager::GetMasterVolume() //1157-1191
{
	MIXERCONTROLDETAILS mcd;
	MIXERCONTROLDETAILS_UNSIGNED mxcd_u;
	MIXERLINECONTROLS mxlc;
	MIXERCONTROL mlct;
	MIXERLINE mixerLine;
	HMIXER hmx;
	MIXERCAPS pmxcaps;	

	mixerOpen((HMIXER*) &hmx, 0, 0, 0, MIXER_OBJECTF_MIXER);
	mixerGetDevCaps(0, &pmxcaps, sizeof(pmxcaps));

	mxlc.cbStruct = sizeof(mxlc);	
	mxlc.cbmxctrl = sizeof(mlct);
	mxlc.pamxctrl = &mlct;
	mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	mixerLine.cbStruct = sizeof(mixerLine);
	mixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;
	mixerGetLineInfo((HMIXEROBJ) hmx, &mixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE);
	mxlc.dwLineID = mixerLine.dwLineID;
	mixerGetLineControls((HMIXEROBJ) hmx, &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);	

	mcd.cbStruct = sizeof(mcd);
	mcd.dwControlID = mlct.dwControlID;
	mcd.cChannels = 1;
	mcd.cMultipleItems = 0;
	mcd.cbDetails = sizeof(mxcd_u);
	mcd.paDetails = &mxcd_u;
		
	mixerGetControlDetails((HMIXEROBJ) hmx, &mcd, 0L);	

	mixerClose(hmx);

	return mxcd_u.dwValue / (double) 0xFFFF;
}

void DSoundManager::SetMasterVolume(double theVolume) //1194-1227
{
	MIXERCONTROLDETAILS mcd;
	MIXERCONTROLDETAILS_UNSIGNED mxcd_u;
	MIXERLINECONTROLS mxlc;
	MIXERCONTROL mlct;
	MIXERLINE mixerLine;
	HMIXER hmx;
	MIXERCAPS pmxcaps;	

	mixerOpen((HMIXER*) &hmx, 0, 0, 0, MIXER_OBJECTF_MIXER);
	mixerGetDevCaps(0, &pmxcaps, sizeof(pmxcaps));

	mxlc.cbStruct = sizeof(mxlc);	
	mxlc.cbmxctrl = sizeof(mlct);
	mxlc.pamxctrl = &mlct;
	mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	mixerLine.cbStruct = sizeof(mixerLine);
	mixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;
	mixerGetLineInfo((HMIXEROBJ) hmx, &mixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE);
	mxlc.dwLineID = mixerLine.dwLineID;
	mixerGetLineControls((HMIXEROBJ) hmx, &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);	

	mcd.cbStruct = sizeof(mcd);
	mcd.dwControlID = mlct.dwControlID;
	mcd.cChannels = 1;
	mcd.cMultipleItems = 0;
	mcd.cbDetails = sizeof(mxcd_u);
	mcd.paDetails = &mxcd_u;
	
	mxcd_u.dwValue = (int) (0xFFFF * theVolume);
	mixerSetControlDetails((HMIXEROBJ) hmx, &mcd, 0L);

	mixerClose(hmx);
}

void DSoundManager::Flush() //1230-1231
{
}

void DSoundManager::SetCooperativeWindow(HWND theHWnd, bool isWindowed) //1234-1241
{
	if (mDirectSound != NULL)
		mDirectSound->SetCooperativeLevel(theHWnd,DSSCL_NORMAL);
/*
	if (isWindowed==true) mDirectSound->SetCooperativeLevel(theHWnd,DSSCL_NORMAL);
	else mDirectSound->SetCooperativeLevel(theHWnd,DSSCL_EXCLUSIVE);
	*/
}

void DSoundManager::Update() //1244-1245
{
}

#undef SOUND_FLAGS
