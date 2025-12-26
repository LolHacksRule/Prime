//#define SEXY_TRACING_ENABLED
//#define SEXY_PERF_ENABLED
//#define SEXY_MEMTRACE

#include "SexyAppBase.h"
#ifdef _WIN32
#include "SEHCatcher.h"
#endif
#include "WidgetManager.h"
#include "Widget.h"
#include "Debug.h"
#include "DeviceImage.h"
#include "MemoryImage.h"
#include "HTTPTransfer.h"
#include "Dialog.h"
#include "ImageLib\ImageLib.h"
#include "SoundManager.h"
#include "SoundInstance.h"
#include "Rect.h"
#include "MusicInterface.h"
#include "PropertiesParser.h"
#include "PerfTimer.h"
#include "MTRand.h"
#include <time.h>
#include <math.h>
#include "SysFont.h"
#include "ResourceManager.h"
#include "ResStreamsManager.h"
#include "AutoCrit.h"
#include "Debug.h" //PopCap includes this twice, no idea why.
#include "PakLib/PakInterface.h"
#include <string>

#include "memmgr.h"

#include "SexyCache.h"
#include "SexyThread.h"
#include "SafeStrFntRcd.h"

#include "IAppDriver.h"
#include "IAudioDriver.h"
#include "IFileDriver.h"
#include "IGamepadDriver.h"
#include "IGraphicsDriver.h"
#include "IResStreamsDriver.h"

using namespace Sexy;

const int DEMO_FILE_ID = 0x42BEEF78; //PopCap, why are these still here, even XNA and PVZF (Android) has it, at least BejClassic doesn't.
const int DEMO_VERSION = 2;

SexyAppBase* Sexy::gSexyAppBase = NULL;

#ifdef _WIN32
SEHCatcher Sexy::gSEHCatcher; //Line 64, only on PC/Mac
#endif

//////////////////////////////////////////////////////////////////////////
// Decompilation by LolHacksRule, original engine belongs to PopCap. Support PopCap by remembering to buy the things they made.
//////////////////////////////////////////////////////////////////////////

#ifdef _SEXYDECOMP_USE_LATEST_CODE //Mobile
void SexyAppBase::OnFullVersionChange()
{
}
#endif

void SexyAppBase::InitFileDriver() //81-84
{
	if (!gFileDriver)
		gFileDriver = gFileDriver->CreateFileDriver();
}

//////////////////////////////////////////////////////////////////////////

SexyAppBase::SexyAppBase() //89-131
{
	mResStreamsManager = NULL;
	mGamepadLocked = -1;
	mMainThreadId = GetCurrentRunningThread();
	mMaxUpdateBacklog = 200;
	mPauseWhenMoving = true;
	mGraphicsDriver = NULL;
	InitFileDriver();
	mFileDriver = gFileDriver;
	mAppDriver = mAppDriver->CreateAppDriver(this);
	mAudioDriver = mAudioDriver->CreateAudioDriver(this);
	mGamepadDriver = mGamepadDriver->CreateGamepadDriver(); //Null on mobile, Win and OSX, XInput on Win (Pre Bej3 Steam), PS3GamepadDriver on PS3
	mResStreamsDriver = mResStreamsDriver->CreateResStreamsDriver(); //Not on Bej3 Win/Steam.

#if defined(_SEXYDECOMP_USE_LATEST_CODE) || defined(BEJ3_OSX_STEAM)
	mProfileDriver = mProfileDriver->CreateProfileDriver(); //FilesystemProfileDriver, PS3ProfileDriver on PS3
	mSaveGameDriver = mSaveGameDriver->CreateSaveGameDriver(); //FilesystemSaveGameDriver, PS3SaveGameDriver on PS3
#ifndef BEJ3_OSX_STEAM
	mHttpDriver = mHttpDriver->CreateHttpDriver(this); //iOS: CoreFoundation, EAMT (Android): EAMT, Null on Console
#endif
	mLeaderboardDriver = mLeaderboardDriver->CreateLeaderboardDriver(); //Null on iOS and EAMT (Android), not on Bej3Steam
#if defined(_CONSOLE) || defined(PRIME_MOBILE)
	mAchievementDriver = mAchievementDriver->CreateAchievementDriver(); //Null on iOS and EAMT (Android).
#endif
#if defined(IPHONEOS) && !defined(PVZ_IOS)
	mDiagDriver = mDiagDriver->CreateDiagDriver(); //Not on PVZF/CHN (iOS)?
#endif
#endif

	gSexyAppBase = this;
	mWidgetManager = new WidgetManager(this);
	mResourceManager = new ResourceManager(this);
	mAppDriver->InitAppDriver();
	mFileDriver->InitFileDriver(this);
	mAudioDriver->InitAudioDriver();
	mGamepadDriver->InitGamepadDriver(this);
	if (mResStreamsDriver)
		mResStreamsDriver->InitWithApp(this); //C++ only
	mResStreamsManager = new ResStreamsManager(this);
	mStrFntRcd = new SafeStrFntRcd(); //Windows only?
}

SexyAppBase::~SexyAppBase() //Right? | 134-168
{
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	CleanupSysFont();
#endif
	DialogMap::iterator aDialogItr = mDialogMap.begin();
	while (aDialogItr != mDialogMap.end())
	{
		Widget* aWidget = aDialogItr->second;
		if (aWidget->mParent != NULL)
			mWidgetManager->RemoveWidget(aDialogItr->second);
		/*if (aDialogItr->second != NULL)
			ButtonDownTick(1);*/
		delete aDialogItr->second;
		++aDialogItr;
	}
	mDialogMap.clear();
	mDialogList.clear();
	delete mWidgetManager;
	delete mResourceManager;

	//FPS image is deleted in *AppDriver

	SharedImageMap::iterator aSharedImageItr = mSharedImageMap.begin();

	while (aSharedImageItr != mSharedImageMap.end())
	{
		SharedImage* aSharedImage = &aSharedImageItr->second;
		DBG_ASSERTE(aSharedImage->mRefCount == 0); //157 | 204 in BejLiveWin8
		delete aSharedImage->mImage;
		mSharedImageMap.erase(aSharedImageItr++);
	}
	delete mSharedRTPool;
	mSharedRTPool = NULL;
	mAppDriver->Shutdown();
	delete mAppDriver;
	gSexyAppBase = NULL;
}

void SexyAppBase::ClearUpdateBacklog(bool relaxForASecond) //171-173
{
	mAppDriver->ClearUpdateBacklog(relaxForASecond);
}

bool SexyAppBase::IsScreenSaver() //176-178
{
	return mIsScreenSaver;
}

bool SexyAppBase::AppCanRestore() //181-183
{
	return !mIsDisabled;
}

Dialog* SexyAppBase::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) //187-190
{	
	Dialog* aDialog = new Dialog(NULL, NULL, theDialogId, isModal, theDialogHeader,	theDialogLines, theDialogFooter, theButtonMode);		
	return aDialog;
}

Dialog* SexyAppBase::DoDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) //193-201
{
	KillDialog(theDialogId);

	Dialog* aDialog = NewDialog(theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);		

	AddDialog(theDialogId, aDialog);

	return aDialog;
}


Dialog*	SexyAppBase::GetDialog(int theDialogId) //205-212
{
	DialogMap::iterator anItr = mDialogMap.find(theDialogId);

	if (anItr != mDialogMap.end())	
		return anItr->second;

	return NULL;
}

bool SexyAppBase::KillDialog(int theDialogId, bool removeWidget, bool deleteWidget) //215-249
{
	DialogMap::iterator anItr = mDialogMap.find(theDialogId);

	if (anItr != mDialogMap.end())
	{
		Dialog* aDialog = anItr->second;

		// set the result to something else so DoMainLoop knows that the dialog is gone 
		// in case nobody else sets mResult		
		if (aDialog->mResult == -1) 
			aDialog->mResult = 0;
		
		DialogList::iterator aListItr = std::find(mDialogList.begin(),mDialogList.end(),aDialog);
		if (aListItr != mDialogList.end())
			mDialogList.erase(aListItr);
		
		mDialogMap.erase(anItr);

		if (removeWidget || deleteWidget)
		mWidgetManager->RemoveWidget(aDialog);

		if (aDialog->IsModal())
		{			
			ModalClose();
			mWidgetManager->RemoveBaseModal(aDialog);
		}				

		if (deleteWidget)
		SafeDeleteWidget(aDialog);
		
		return true;
	}

	return false;
}

bool SexyAppBase::KillDialog(int theDialogId) //252-254
{
	return KillDialog(theDialogId,true,true);
}

bool SexyAppBase::KillDialog(Dialog* theDialog) //257-259
{
	return KillDialog(theDialog->mId);
}

int SexyAppBase::GetDialogCount() //262-264
{
	return mDialogMap.size();
}

void SexyAppBase::AddDialog(int theDialogId, Dialog* theDialog) //267-286
{
	KillDialog(theDialogId);

	if (theDialog->mWidth == 0)
	{
		// Set the dialog position ourselves
		int aWidth = mWidth/2;
		theDialog->Resize((mWidth - aWidth)/2, mHeight / 5, aWidth, theDialog->GetPreferredHeight(aWidth));
	}

	mDialogMap.insert(DialogMap::value_type(theDialogId, theDialog));
	mDialogList.push_back(theDialog);

	mWidgetManager->AddWidget(theDialog);
	if (theDialog->IsModal())
	{
		mWidgetManager->AddBaseModal(theDialog);
		ModalOpen();
	}
}

void SexyAppBase::AddDialog(Dialog* theDialog) //289-291
{
	AddDialog(theDialog->mId, theDialog);
}

void SexyAppBase::ModalOpen() //294-295
{
}

void SexyAppBase::ModalClose() //298-299
{
}

void SexyAppBase::DialogButtonPress(int theDialogId, int theButtonId) //302-307
{	
	if (theButtonId == Dialog::ID_YES)
		ButtonPress(2000 + theDialogId);
	else if (theButtonId == Dialog::ID_NO)
		ButtonPress(3000 + theDialogId);	
}

void SexyAppBase::DialogButtonDepress(int theDialogId, int theButtonId) //310-315
{
	if (theButtonId == Dialog::ID_YES)
		ButtonDepress(2000 + theDialogId);
	else if (theButtonId == Dialog::ID_NO)
		ButtonDepress(3000 + theDialogId);
}

void SexyAppBase::GotFocus() //318-319
{
}

void SexyAppBase::LostFocus() //322-323
{
}

void SexyAppBase::URLOpenFailed(const std::string& theURL) //326-328
{
	mIsOpeningURL = false;
}

void SexyAppBase::URLOpenSucceeded(const std::string& theURL) //331-336
{
	mIsOpeningURL = false;

	if (mShutdownOnURLOpen)
		Shutdown();	
}

bool SexyAppBase::OpenURL(const std::string& theURL, bool shutdownOnOpen) //339-341
{
	mAppDriver->OpenURL(theURL, shutdownOnOpen);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SexyAppBase::SetCursorImage(int theCursorNum, Image* theImage) //346-348
{
	mAppDriver->SetCursorImage(theCursorNum, theImage);
}

double SexyAppBase::GetLoadingThreadProgress() //353-355
{
	mAppDriver->GetLoadingThreadProgress();
}

//////////////////////////////////////////////////////////////////////////
bool SexyAppBase::RegistryWriteString(const std::string& theValueName, const std::string& theString) //359-361
{
	return mAppDriver->ConfigWriteString(theValueName, theString);
}

bool SexyAppBase::RegistryWriteString(const std::string& theValueName, const std::wstring& theString) //364-366
{
	return mAppDriver->ConfigWriteString(theValueName, theString);
}

bool SexyAppBase::RegistryWriteInteger(const std::string& theValueName, int theValue) //369-371
{
	return mAppDriver->ConfigWriteInteger(theValueName, theValue);
}

bool SexyAppBase::RegistryWriteBoolean(const std::string& theValueName, bool theValue) //374-376
{
	return mAppDriver->ConfigWriteBoolean(theValueName, theValue);
}

bool SexyAppBase::RegistryWriteData(const std::string& theValueName, const uchar* theValue, ulong theLength) //379-381
{
	return mAppDriver->ConfigWriteData(theValueName, theValue, theLength);
}

void SexyAppBase::WriteToRegistry() //Why are most of these ints, I can understand the numeric values but shouldn't the simple flags be true or false | 381-396
{	
	RegistryWriteInteger("MusicVolume", (int) (mMusicVolume * 100));
	RegistryWriteInteger("SfxVolume", (int) (mSfxVolume * 100));
	RegistryWriteInteger("Muted", (mMuteCount - mAutoMuteCount > 0) ? 1 : 0);
	RegistryWriteInteger("ScreenMode", mIsWindowed ? 0 : 1);
	RegistryWriteInteger("PreferredX", mPreferredX);
	RegistryWriteInteger("PreferredY", mPreferredY);
	RegistryWriteInteger("CustomCursors", mCustomCursorsEnabled ? 1 : 0);		
	RegistryWriteInteger("InProgress", 0);
	RegistryWriteBoolean("WaitForVSync", mWaitForVSync);	
}
//////////////////////////////////////////////////////////////////////////// On iOS there's no Prime exe that has these
bool SexyAppBase::RegistryEraseKey(const SexyString& _theKeyName) //399-402
{
	return mAppDriver->ConfigEraseKey(_theKeyName);
}

void SexyAppBase::RegistryEraseValue(const SexyString& _theValueName) //405-407
{
	mAppDriver->ConfigEraseValue(_theValueName);
}

bool SexyAppBase::RegistryGetSubKeys(const std::string& theKeyName, StringVector* theSubKeys) //410-412
{
	return mAppDriver->ConfigGetSubKeys(theKeyName, theSubKeys);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool SexyAppBase::RegistryReadString(const std::string& theKey, std::string* theString) //417-419
{
	return mAppDriver->ConfigReadString(theKey, theString);
}

bool SexyAppBase::RegistryReadString(const std::string& theKey, std::wstring* theString) //422-424
{
	return mAppDriver->ConfigReadString(theKey, theString);
}

bool SexyAppBase::RegistryReadInteger(const std::string& theKey, int* theValue) //427-429
{
	return mAppDriver->ConfigReadInteger(theKey, theValue);
}

bool SexyAppBase::RegistryReadBoolean(const std::string& theKey, bool* theValue) //432-434
{
	return mAppDriver->ConfigReadBoolean(theKey, theValue);
}
//On iOS there's no Prime exe that has this //////////////////////////////////////////////////////////////////////////
bool SexyAppBase::RegistryReadData(const std::string& theKey, uchar* theValue, ulong* theLength) //437-439
{		
	return mAppDriver->ConfigReadData(theKey, theValue, theLength);
}

void SexyAppBase::ReadFromRegistry() //442-477
{
	mReadFromRegistry = true;
	mRegKey = SexyStringToString(GetString("RegistryKey", StringToSexyString(mRegKey)));

	if (mRegKey.length() == 0)
		return;				

	int anInt;
	if (RegistryReadInteger("MusicVolume", &anInt))
		mMusicVolume = anInt / 100.0;
	
	if (RegistryReadInteger("SfxVolume", &anInt))
		mSfxVolume = anInt / 100.0;

	if (RegistryReadInteger("Muted", &anInt))
		mMuteCount = anInt;

	if (RegistryReadInteger("ScreenMode", &anInt))
		mIsWindowed = anInt == 0 && !mForceFullscreen;

	RegistryReadInteger("PreferredX", &mPreferredX);
	RegistryReadInteger("PreferredY", &mPreferredY);

	RegistryReadInteger("PreferredWidth", &mPreferredWidth);
	RegistryReadInteger("PreferredHeight", &mPreferredHeight);

	if (RegistryReadInteger("CustomCursors", &anInt))
		EnableCustomCursors(anInt != 0);	
			
	RegistryReadBoolean("WaitForVSync", &mWaitForVSync);	

	if (RegistryReadInteger("InProgress", &anInt))
		mLastShutdownWasGraceful = anInt == 0;
		
	if (!IsScreenSaver())
		RegistryWriteInteger("InProgress", 1);	
	//mAppDriver->ReadFromConfig(); in XNA
}

bool SexyAppBase::WriteBytesToFile(const std::string& theFileName, const void *theData, unsigned long theDataLen) //480-482
{
	return mAppDriver->WriteBytesToFile(theFileName, theData, theDataLen);
}

bool SexyAppBase::WriteBufferToFile(const std::string& theFileName, const Buffer* theBuffer) //485-487
{
	return WriteBytesToFile(theFileName,theBuffer->GetDataPtr(),theBuffer->GetDataLen());
}

bool SexyAppBase::ReadBufferFromFile(const std::string& theFileName, Buffer* theBuffer, bool dontWriteToDemo) //TODO! Why is the demo val there | 491-584
{
	/*
	void *, thePtr
	struct PFILE *, aFP
	int, theSize
	int, aFileSize
	unsigned char *, aData
	unsigned long, theGroupId
	std::string, anRSBFileName
	unsigned long, theBufferSize
	unsigned char *, theBufferPtr
	*/

	void* thePtr;
	int theSize;

	if (mSexyCacheBuffers) //C++ only
		gSexyCache.GetData(GetFullPath(theFileName), "Buffer", &thePtr, &theSize);
	//?

	if (thePtr)
	{
		theBuffer->Clear();
		theBuffer->SetData((uchar*)thePtr + 1, theSize - 1);
		gSexyCache.FreeGetData(thePtr);
		return true;
	}
	else
		return false;

	//else
	{
		if (mResStreamsManager && mResStreamsManager->IsInitialized())
		{
			std::string anRSBFileName = theFileName;
			ulong theGroupId = mResStreamsManager->GetGroupForFile(anRSBFileName);
			if (theGroupId != -1 && mResStreamsManager->IsGroupLoaded(theGroupId) || mResStreamsManager->ForceLoadGroup(theGroupId))
			{
				DBG_ASSERTE(mResStreamsManager->IsGroupLoaded(theGroupId)); //519 | 592 in BejLiveWin8
				uchar* theBufferPtr = NULL;
				ulong theBufferSize;
				if (mResStreamsManager->GetResidentFileBuffer(theGroupId, anRSBFileName, &theBufferPtr, &theBufferSize))
				{
					theBuffer->Clear();
					theBuffer->SetData(theBufferPtr, theBufferSize);
					return true;
				}
			}
		}
		PFILE* aFP = p_fopen(theFileName.c_str(), "rb");
		if (aFP != NULL)
		{
			p_fseek(aFP, 0, SEEK_END);
			int aFileSize = p_ftell(aFP);
			p_fseek(aFP, 0, SEEK_SET);
			uchar* aData = new uchar[aFileSize];
			p_fread(aData, 1, aFileSize, aFP);
			p_fclose(aFP);
			theBuffer->Clear();
			theBuffer->SetData(aData, aFileSize);
			if (mWriteToSexyCache && mSexyCacheBuffers)
			{
				thePtr = gSexyCache.AllocSetData(GetFullPath(theFileName), "Buffer", aFileSize + 1);
				if (thePtr)
				{
					thePtr = (void*)1; //?
					memcpy(&thePtr + 1, aData, aFileSize);
					gSexyCache.SetData(thePtr);
					gSexyCache.FreeSetData(thePtr);
					gSexyCache.SetFileDeps(GetFullPath(theFileName), "Buffer", GetFullPath(theFileName));
				}
			}
			delete[] aData;
			return true;
		}
		else
		{
			if (mWriteToSexyCache && mSexyCacheBuffers)
			{
				thePtr = gSexyCache.AllocSetData(GetFullPath(theFileName), "Buffer", 1);
				if (thePtr)
				{
					thePtr = 0;
					gSexyCache.SetData(thePtr);
					gSexyCache.FreeGetData(thePtr);
					gSexyCache.SetFileDeps(GetFullPath(theFileName), "Buffer", GetFullPath(theFileName));
				}
			}
			return false;
		}
	}
}

bool SexyAppBase::FileExists(const std::string& theFileName) //587-589
{
	return mFileDriver->FileExists(theFileName, false);
}

bool SexyAppBase::EraseFile(const std::string& theFileName) //592-594
{
	return mFileDriver->DeleteFile(theFileName);
}

//////////////////////////////////////////////////////////////////////////
void SexyAppBase::ShutdownHook() //598-599
{
}

void SexyAppBase::Shutdown() //602-612
{
#ifdef _DEBUG //Debug only
	if (mStrFntRcd)
	{
		mStrFntRcd->WriteRecordToFile();
		delete mStrFntRcd;
		mStrFntRcd = NULL;
	}
#endif
	mAppDriver->Shutdown();
}

//////////////////////////////////////////////////////////////////////////
void SexyAppBase::DoExit(int theCode) //616-618
{
	mAppDriver->DoExit(theCode);
}

void SexyAppBase::UpdateFrames() //Correct? | 621-646
{
	mUpdateCount++;	
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	mGamepadDriver->Updaate();
#endif
	if (!mMinimized)
	{
		PerfTimer aTimer;
		if (mShowFPS && mShowFPSMode <= FPS_ShowFPSWithHistory)
			aTimer.Start();
		if (mWidgetManager->UpdateFrame())
			++mFPSDirtyCount;
		if (mShowFPS && mShowFPSMode <= FPS_ShowFPSWithHistory)
		{
			mVFPSUpdateCount++;
			mVFPSUpdateTimes += aTimer.GetDuration();
		}
	}
	if (mResStreamsManager)
		mResStreamsManager->Update();
	mGamepadDriver->Update();
	mSoundManager->Update();
	mMusicInterface->Update();	
	CleanSharedImages();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SexyAppBase::BeginPopup() //651-653
{
	mAppDriver->BeginPopup();
}

void SexyAppBase::EndPopup() //656-658
{
	mAppDriver->EndPopup();
}

int SexyAppBase::MsgBox(const std::string& theText, const std::string& theTitle, int theFlags) //661-663
{
	mAppDriver->MsgBox(theText, theTitle, theFlags);
}

int SexyAppBase::MsgBox(const std::wstring& theText, const std::wstring& theTitle, int theFlags) //666-668
{
	mAppDriver->MsgBox(theText, theTitle, theFlags);
}

void SexyAppBase::Popup(const std::string& theString) //671-673
{
	mAppDriver->Popup(theString);
}

void SexyAppBase::Popup(const std::wstring& theString) //676-678
{
	mAppDriver->Popup(theString);
}

void SexyAppBase::SafeDeleteWidget(Widget* theWidget) //681-686
{
	WidgetSafeDeleteInfo aWidgetSafeDeleteInfo;
	aWidgetSafeDeleteInfo.mUpdateAppDepth = mUpdateAppDepth;
	aWidgetSafeDeleteInfo.mWidget = theWidget;
	mSafeDeleteList.push_back(aWidgetSafeDeleteInfo);
}

//Where are these used
int aNumBigMoveMessages = 0;
int aNumSmallMoveMessages = 0;
int aNumTimerMessages = 0;

bool SexyAppBase::KeyDown(int theKey) //701-703
{
	return mAppDriver->KeyDown(theKey);
}

bool SexyAppBase::DebugKeyDown(int theKey) //706-708
{
	return mAppDriver->DebugKeyDown(theKey);
}

bool SexyAppBase::DebugKeyDownAsync(int theKey, bool ctrlDown, bool altDown) //711-713
{
	return false;
}

#if defined(SEXYDECOMP_LATEST_CODE) || defined(BEJ3_STEAM) //Maybe on Win you can define stuff to call it

void SexyAppBase::ShowKeyboard()
{
	mAppDriver->ShowKeyboard();
}

void SexyAppBase::HideKeyboard()
{
	mAppDriver->HideKeyboard();
}

#endif

void SexyAppBase::CloseRequestAsync() //716-717
{
}

//////////////////////////////////////////////////////////////////////////
void SexyAppBase::Done3dTesting() //MOVED TO *AppDriver | 721-722
{
}

// return file name that you want to upload
std::string	SexyAppBase::NotifyCrashHook() //726-728
{
	return "";
}

void SexyAppBase::DeleteNativeImageData() //Dummied in XNA | 731-741
{
	AutoCrit anAutoCrit(mImageSetCritSect);
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;		
		aMemoryImage->DeleteNativeData();			
		++anItr;
	}
}

void SexyAppBase::DeleteExtraImageData() //Dummied in XNA | 744-760
{
	AutoCrit anAutoCrit(mImageSetCritSect);
	DeviceImage* theScreenImage = mGraphicsDriver->GetScreenImage();
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;
		if (aMemoryImage != theScreenImage)
			aMemoryImage->DeleteExtraBuffers();
		++anItr;
	}
}

void SexyAppBase::ReInitImages() //763-773
{
	AutoCrit anAutoCrit(mImageSetCritSect);
	MemoryImageSet::iterator anItr = mMemoryImageSet.begin();
	while (anItr != mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;				
		aMemoryImage->ReInit();
		++anItr;
	}
}


void SexyAppBase::LoadingThreadProc() //777-778
{
}

void SexyAppBase::LoadingThreadCompleted() //781-782
{
}

//////////////////////////////////////////////////////////////////////////
void SexyAppBase::StartLoadingThread() //786-788
{	
	mAppDriver->StartLoadingThread();
}

//////////////////////////////////////////////////////////////////////////
void SexyAppBase::SwitchScreenMode(bool wantWindowed, bool is3d, bool force) //793-795
{
	mAppDriver->SwitchScreenMode(wantWindowed, is3d, force);
}

void SexyAppBase::SwitchScreenMode(bool wantWindowed) //798-800
{
	SwitchScreenMode(wantWindowed, Is3DAccelerated());
}

void SexyAppBase::SwitchScreenMode() //803-805
{
	SwitchScreenMode(mIsWindowed, Is3DAccelerated(), true);
}

//////////////////////////////////////////////////////////////////////////
void SexyAppBase::ProcessSafeDeleteList() //809-824
{
	MTAutoDisallowRand aDisallowRand;

	WidgetSafeDeleteList::iterator anItr = mSafeDeleteList.begin();
	while (anItr != mSafeDeleteList.end())
	{
		WidgetSafeDeleteInfo* aWidgetSafeDeleteInfo = &(*anItr);
		if (mUpdateAppDepth <= aWidgetSafeDeleteInfo->mUpdateAppDepth)
		{
			delete aWidgetSafeDeleteInfo->mWidget;
			anItr = mSafeDeleteList.erase(anItr);
		}
		else
			++anItr;
	}	
}

bool SexyAppBase::UpdateAppStep(bool* updated) //1021-1023
{
	return mAppDriver->UpdateAppStep(updated);
}

bool SexyAppBase::UpdateApp() //1026-1035
{
	bool updated;
	for (;;)
	{
		if (!UpdateAppStep(&updated))
			return false;
		if (updated)
			return true;
	}
}

std::string SexyAppBase::GetGameSEHInfo() //GetSEHWebParams moved to AppDriver | 1038-1040
{
	return mAppDriver->GetGameSEHInfo();
}

//////////////////////////////////////////////////////////////////////////
void SexyAppBase::PreTerminate() //1044-1045
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SexyAppBase::Start() //1048-1050
{
	mAppDriver->Start();
}

bool SexyAppBase::CheckSignature(const Buffer& theBuffer, const std::string& theFileName) //1053-1056
{
	mAppDriver->CheckSignature(theBuffer, theFileName);
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE //Get locales dir
bool SexyAppBase::LoadProperties(const std::string& theFileName, bool required, bool checkSig, bool needsLocaleCorrection)
#else
bool SexyAppBase::LoadProperties(const std::string& theFileName, bool required, bool checkSig) //1059-1090
#endif
{
	Buffer aBuffer;
	if (!ReadBufferFromFile(theFileName, &aBuffer))
	{
#ifdef _SEXYDECOMP_USE_LATEST_CODE   
		bool unk = false;
		if (needsLocaleCorrection && mResourceManager)
		{
			aBuffer.Clear();
			unk = ReadBufferFromFile(mResourceManager->GetLocaleFolder(true) + theFileName + aBuffer);
		}
		if (!unk)
		{
#endif
			if (!required)
				return true;
			else
			{
				Popup(GetString("UNABLE_OPEN_PROPERTIES", _S("Unable to open properties file ")) + StringToSexyString(theFileName));
				return false;
			}
#ifdef _SEXYDECOMP_USE_LATEST_CODE
		}
#endif
		if (checkSig)
		{
			if (!CheckSignature(aBuffer, theFileName))
			{
				Popup(GetString("PROPERTIES_SIG_FAILED", _S("Signature check failed on ")) + StringToSexyString(theFileName + "'"));
				return false;
			}
		}

		PropertiesParser aPropertiesParser(this);

		// Load required language-file properties
		if (!aPropertiesParser.ParsePropertiesBuffer(aBuffer))
		{
			Popup(aPropertiesParser.GetErrorText());
			return false;
		}
		else
			return true;
	}
}

bool SexyAppBase::LoadProperties() //1093-1096
{
	// Load required language-file properties
#ifdef _SEXYDECOMP_USE_LATEST_CODE //Get locales dir
	return LoadProperties("properties/default.xml", true, false, true);
#else
	return LoadProperties("properties/default.xml", true, false); //Now uses a single slash, probably to make multiplat better
#endif
}

void SexyAppBase::LoadResourceManifest() //1099-1102
{
	if (!mResourceManager->ParseResourcesFile("properties/resources.xml")) //Was changed, probably to make multiplat better
		ShowResourceError(true);
}

void SexyAppBase::ShowResourceError(bool doExit) //1105-1110
{
	OutputDebugStrF(mResourceManager->GetErrorText().c_str());
	Popup(mResourceManager->GetErrorText());	
	if (doExit)
		DoExit(0);
}

//////////////////////////////////////////////////////////////////////////
bool SexyAppBase::ReloadAllResources() //1114-1116
{
	return mAppDriver->ReloadAllResources();
}

bool SexyAppBase::GetBoolean(const std::string& theId) //1119-1127
{
	StringBoolMap::iterator anItr = mBoolProperties.find(theId);
	DBG_ASSERTE(anItr != mBoolProperties.end()); //1121
	
	if (anItr != mBoolProperties.end())	
		return anItr->second;
	else
		return false;
}

bool SexyAppBase::GetBoolean(const std::string& theId, bool theDefault) //1130-1137
{
	StringBoolMap::iterator anItr = mBoolProperties.find(theId);	
	
	if (anItr != mBoolProperties.end())	
		return anItr->second;
	else
		return theDefault;	
}

int SexyAppBase::GetInteger(const std::string& theId) //1130-1148
{
	StringIntMap::iterator anItr = mIntProperties.find(theId);
	DBG_ASSERTE(anItr != mIntProperties.end()); //1142
	
	if (anItr != mIntProperties.end())	
		return anItr->second;
	else
		return false;
}

int SexyAppBase::GetInteger(const std::string& theId, int theDefault) //1152-1158
{
	StringIntMap::iterator anItr = mIntProperties.find(theId);	
	
	if (anItr != mIntProperties.end())	
		return anItr->second;
	else
		return theDefault;	
}

double SexyAppBase::GetDouble(const std::string& theId) //1161-1169
{
	StringDoubleMap::iterator anItr = mDoubleProperties.find(theId);
	DBG_ASSERTE(anItr != mDoubleProperties.end()); //1163
	
	if (anItr != mDoubleProperties.end())	
		return anItr->second;
	else
		return false;
}

double SexyAppBase::GetDouble(const std::string& theId, double theDefault) //1172-1179
{
	StringDoubleMap::iterator anItr = mDoubleProperties.find(theId);	
	
	if (anItr != mDoubleProperties.end())	
		return anItr->second;
	else
		return theDefault;	
}

SexyString SexyAppBase::GetString(const std::string& theId) //1182-1190
{
	StringWStringMap::iterator anItr = mStringProperties.find(theId);
	DBG_ASSERTE(anItr != mStringProperties.end()); //1184
	
	if (anItr != mStringProperties.end())	
		return WStringToSexyString(anItr->second);
	else
		return _S("");
}

SexyString SexyAppBase::GetString(const std::string& theId, const SexyString& theDefault) //1193-1200
{
	StringWStringMap::iterator anItr = mStringProperties.find(theId);	
	
	if (anItr != mStringProperties.end())	
		return WStringToSexyString(anItr->second);
	else
		return theDefault;	
}

StringVector SexyAppBase::GetStringVector(const std::string& theId) //Not on iOS? | 1203-1211
{
	StringStringVectorMap::iterator anItr = mStringVectorProperties.find(theId);
	DBG_ASSERTE(anItr != mStringVectorProperties.end()); //1205
	
	if (anItr != mStringVectorProperties.end())	
		return anItr->second;
	else
		return StringVector();
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
void SexyAppBase::SetString(const std::string& anID, const std::wstring& value) //Recover from XNA, is this correct
{
	//TODO
}

void SexyAppBase::SetBoolean(const std::string& anID, bool boolValue) //Recover from XNA, is this correct
{
	//TODO
}

void SexyAppBase::SetInteger(const std::string& anID, int anInt) //Recover from XNA, is this correct
{
	//TODO
}

void SexyAppBase::SetDouble(const std::string& anID, double aDouble) //Recover from XNA, is this correct
{
	//TODO
}

#else
void SexyAppBase::SetString(const std::string& theId, const std::wstring& theValue) //1214-1224
{
	std::pair<StringWStringMap::iterator, bool> aPair = mStringProperties.insert(StringWStringMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
	int anIntValue;
	if (StringToInt(theId, &anIntValue))
		mPopLoc.SetString(anIntValue, ToSexyString(theValue), false);
}


void SexyAppBase::SetBoolean(const std::string& theId, bool theValue) //1228-1232
{
	std::pair<StringBoolMap::iterator, bool> aPair = mBoolProperties.insert(StringBoolMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

void SexyAppBase::SetInteger(const std::string& theId, int theValue) //1235-1239
{
	std::pair<StringIntMap::iterator, bool> aPair = mIntProperties.insert(StringIntMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}

void SexyAppBase::SetDouble(const std::string& theId, double theValue) //1242-1246
{
	std::pair<StringDoubleMap::iterator, bool> aPair = mDoubleProperties.insert(StringDoubleMap::value_type(theId, theValue));
	if (!aPair.second) // Found it, change value
		aPair.first->second = theValue;
}
#endif

void SexyAppBase::DoParseCmdLine() //1249-1251
{
	mAppDriver->DoParseCmdLine();
}

void SexyAppBase::ParseCmdLine(const std::string& theCmdLine) //1254-1256
{
	mAppDriver->ParseCmdLine(theCmdLine);
}


void SexyAppBase::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) //1260-1262
{
	mAppDriver->HandleCmdLineParam(theParamName, theParamValue);
}

void SexyAppBase::PreDisplayHook() //1265-1266
{
}

void SexyAppBase::PreDDInterfaceInitHook() //1269-1270
{
}

void SexyAppBase::PostDDInterfaceInitHook() //1273-1274
{
}

bool SexyAppBase::ChangeDirHook(const char *theIntendedPath) //1277-1279
{
	return false;
}

MusicInterface* SexyAppBase::CreateMusicInterface() //1282-1287
{
	if (mNoSoundNeeded && mAudioDriver == NULL)
		return new MusicInterface;
	else 
		return mAudioDriver->CreateMusicInterface();
}

void SexyAppBase::InitPropertiesHook() //1290-1291
{
}

void SexyAppBase::InitHook() //1294-1295
{
}

void SexyAppBase::Init() //1298-1300
{
	mAppDriver->Init();
}

void SexyAppBase::HandleGameAlreadyRunning() //1303-1305
{
}

void SexyAppBase::CopyToClipboard(const std::string& theString) //1308-1310
{
	mAppDriver->CopyToClipboard(theString);
}

std::string	SexyAppBase::GetClipboard() //1313-1315
{
	return mAppDriver->GetClipboard();
}

void SexyAppBase::SetCursor(int theCursorNum) //1318-1320
{
	mAppDriver->SetCursor(theCursorNum);
}

int SexyAppBase::GetCursor() //1323-1325
{
	return mAppDriver->GetCursor();
}

void SexyAppBase::EnableCustomCursors(bool enabled) //1328-1330
{
	mAppDriver->EnableCustomCursors(enabled);
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
Sexy::DeviceImage* SexyAppBase::GetImage(const std::string& theFileName, bool commitBits, bool allowTriReps, bool isInAtlas)
#else
Sexy::DeviceImage* SexyAppBase::GetImage(const std::string& theFileName, bool commitBits, bool allowTriReps) //Correct? TODO multiplat | 1333-1396
#endif
{
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	if (isInAtlas)
	{
		allowTriReps = false;
	}
	//DBG_ASSERT("SexyAppBase::GetImage from mobile is not yet decompiled.");
#else

	DeviceImage* aCachedImage = DeviceImage::ReadFromCache(GetFullPath(theFileName), "GetImage");
	if (aCachedImage)
	{
		aCachedImage->mFilePath = theFileName;
		if (!allowTriReps)
			aCachedImage->AddImageFlags(ImageFlag_NoTriRep);
		return aCachedImage;
	}
	else
	{
		if (mResStreamsManager && mResStreamsManager->IsInitialized())
		{
			std::string anRSBFileName = theFileName + ".ptx";
			ulong theGroupId = mResStreamsManager->GetGroupForFile(anRSBFileName);
			if (theGroupId != -1 && (mResStreamsManager->IsGroupLoaded(theGroupId) || mResStreamsManager->ForceLoadGroup(theGroupId)))
			{
				DBG_ASSERTE(mResStreamsManager->IsGroupLoaded(theGroupId)); //1359 | 1490 in BejLiveWin8
				Image* anRSBImage = NULL;
				if (mResStreamsManager->GetImage(theGroupId, anRSBFileName, &anRSBImage))
					return (DeviceImage*)anRSBImage;
				else
					DBG_ASSERTE("Could not load group\n"); //1368 | 1499 in BejLiveWin8
			}
			DeviceImage* aOptImage = NULL;
			aOptImage = mAppDriver->GetOptimizedImage(theFileName, commitBits, allowTriReps);
			if (aOptImage)
				return aOptImage;
			else
			{
				AutoCrit anAutoCrit(mGetImageCritSect);
				ImageLib::Image* aLoadedImage = ImageLib::GetImage(theFileName);

				if (aLoadedImage == NULL)
					return NULL;

				DeviceImage* anImage = new DeviceImage();
				if (!allowTriReps)
					anImage->AddImageFlags(ImageFlag_NoTriRep);
				anImage->mFilePath = theFileName;
				anImage->SetBits(aLoadedImage->GetBits(), aLoadedImage->GetWidth(), aLoadedImage->GetHeight(), commitBits);
				anImage->mFilePath = theFileName;
				delete aLoadedImage;

				if (mWriteToSexyCache)
					anImage->WriteToCache(GetFullPath(theFileName), "GetImage");
				return anImage;
			}
		}
	}
#endif
}

Sexy::DeviceImage* SexyAppBase::CreateCrossfadeImage(Sexy::Image* theImage1, const Rect& theRect1, Sexy::Image* theImage2, const Rect& theRect2, double theFadeFactor) //Correct? C++ only | 1399-1462
{
	MemoryImage* aMemoryImage1 = theImage1 ? theImage1->AsMemoryImage() : NULL;
	MemoryImage* aMemoryImage2 = theImage2 ? theImage2->AsMemoryImage() : NULL;

	if ((aMemoryImage1 == NULL) || (aMemoryImage2 == NULL))
		return NULL;

	if ((theRect1.mX < 0) || (theRect1.mY < 0) ||
		(theRect1.mX + theRect1.mWidth > theImage1->GetWidth()) ||
		(theRect1.mY + theRect1.mHeight > theImage1->GetHeight()))
	{
		DBG_ASSERTE("Crossfade Rect1 out of bounds"); //1410
		return NULL;
	}

	if ((theRect2.mX < 0) || (theRect2.mY < 0) ||
		(theRect2.mX + theRect2.mWidth > theImage2->GetWidth()) ||
		(theRect2.mY + theRect2.mHeight > theImage2->GetHeight()))
	{
		DBG_ASSERTE("Crossfade Rect2 out of bounds"); //1418
		return NULL;
	}

	int aWidth = theRect1.mWidth;
	int aHeight = theRect1.mHeight;

	DeviceImage* anImage = new DeviceImage(this);
	anImage->Create(aWidth, aHeight);

	ulong* aDestBits = anImage->GetBits();
	ulong* aSrcBits1 = aMemoryImage1->GetBits();
	ulong* aSrcBits2 = aMemoryImage2->GetBits();

	int aSrc1Width = aMemoryImage1->GetWidth();
	int aSrc2Width = aMemoryImage2->GetWidth();
	ulong aMult = (int)(theFadeFactor * 256);
	ulong aOMM = (256 - aMult);

	for (int y = 0; y < aHeight; y++)
	{
		ulong* s1 = &aSrcBits1[(y + theRect1.mY) * aSrc1Width + theRect1.mX];
		ulong* s2 = &aSrcBits2[(y + theRect2.mY) * aSrc2Width + theRect2.mX];
		ulong* d = &aDestBits[y * aWidth];

		for (int x = 0; x < aWidth; x++)
		{
			ulong p1 = *s1++;
			ulong p2 = *s2++;

			//p1 = 0;
			//p2 = 0xFFFFFFFF;

			*d++ =
				((((p1 & 0x000000FF) * aOMM + (p2 & 0x000000FF) * aMult) >> 8) & 0x000000FF) |
				((((p1 & 0x0000FF00) * aOMM + (p2 & 0x0000FF00) * aMult) >> 8) & 0x0000FF00) |
				((((p1 & 0x00FF0000) * aOMM + (p2 & 0x00FF0000) * aMult) >> 8) & 0x00FF0000) |
				((((p1 >> 24) * aOMM + (p2 >> 24) * aMult) << 16) & 0xFF000000);
		}
	}

	return anImage;
}

void SexyAppBase::ColorizeImage(Image* theImage, const Color& theColor) //NULL on XNA | 1465-1524
{
	MemoryImage* aSrcMemoryImage = theImage ? aSrcMemoryImage->AsMemoryImage() : NULL;

	if (aSrcMemoryImage == NULL)
		return;

	ulong* aBits;	
	int aNumColors;

	if (aSrcMemoryImage->mColorTable == NULL)
	{
		aBits = aSrcMemoryImage->GetBits();		
		aNumColors = theImage->GetWidth()*theImage->GetHeight();				
	}
	else
	{
		aBits = aSrcMemoryImage->mColorTable;		
		aNumColors = 256;				
	}
						
	if ((theColor.mAlpha <= 255) && (theColor.mRed <= 255) && 
		(theColor.mGreen <= 255) && (theColor.mBlue <= 255))
	{
		for (int i = 0; i < aNumColors; i++)
		{
			ulong aColor = aBits[i];

			aBits[i] = 
				((((aColor & 0xFF000000) >> 8) * theColor.mAlpha) & 0xFF000000) |
				((((aColor & 0x00FF0000) * theColor.mRed) >> 8) & 0x00FF0000) |
				((((aColor & 0x0000FF00) * theColor.mGreen) >> 8) & 0x0000FF00)|
				((((aColor & 0x000000FF) * theColor.mBlue) >> 8) & 0x000000FF);
		}
	}
	else
	{
		for (int i = 0; i < aNumColors; i++)
		{
			ulong aColor = aBits[i];

			int aAlpha = ((aColor >> 24) * theColor.mAlpha) / 255;
			int aRed = (((aColor >> 16) & 0xFF) * theColor.mRed) / 255;
			int aGreen = (((aColor >> 8) & 0xFF) * theColor.mGreen) / 255;
			int aBlue = ((aColor & 0xFF) * theColor.mBlue) / 255;

			if (aAlpha > 255)
				aAlpha = 255;
			if (aRed > 255)
				aRed = 255;
			if (aGreen > 255)
				aGreen = 255;
			if (aBlue > 255)
				aBlue = 255;

			aBits[i] = (aAlpha << 24) | (aRed << 16) | (aGreen << 8) | (aBlue);
		}
	}	

	aSrcMemoryImage->BitsChanged();
}

DeviceImage* SexyAppBase::CreateColorizedImage(Image* theImage, const Color& theColor) //Correct? Null on XNA | 1527-1598
{
	MemoryImage* aSrcMemoryImage = theImage ? aSrcMemoryImage->AsMemoryImage() : NULL;

	if (aSrcMemoryImage == NULL)
		return NULL;

	DeviceImage* anImage = new DeviceImage(this);
	
	anImage->Create(theImage->GetWidth(), theImage->GetHeight());
	
	ulong* aSrcBits;
	ulong* aDestBits;
	int aNumColors;

	if (aSrcMemoryImage->mColorTable == NULL)
	{
		aSrcBits = aSrcMemoryImage->GetBits();
		aDestBits = anImage->GetBits();
		aNumColors = theImage->GetWidth()*theImage->GetHeight();				
	}
	else
	{
		aSrcBits = aSrcMemoryImage->mColorTable;
		aDestBits = anImage->mColorTable = new ulong[256];
		aNumColors = 256;
		
		anImage->mColorIndices = new uchar[anImage->mWidth*theImage->mHeight];
		memcpy(anImage->mColorIndices, aSrcMemoryImage->mColorIndices, anImage->mWidth*theImage->mHeight);
	}
						
	if ((theColor.mAlpha <= 255) && (theColor.mRed <= 255) && 
		(theColor.mGreen <= 255) && (theColor.mBlue <= 255))
	{
		for (int i = 0; i < aNumColors; i++)
		{
			ulong aColor = aSrcBits[i];

			aDestBits[i] = 
				((((aColor & 0xFF000000) >> 8) * theColor.mAlpha) & 0xFF000000) |
				((((aColor & 0x00FF0000) * theColor.mRed) >> 8) & 0x00FF0000) |
				((((aColor & 0x0000FF00) * theColor.mGreen) >> 8) & 0x0000FF00)|
				((((aColor & 0x000000FF) * theColor.mBlue) >> 8) & 0x000000FF);
		}
	}
	else
	{
		for (int i = 0; i < aNumColors; i++)
		{
			ulong aColor = aSrcBits[i];

			int aAlpha = ((aColor >> 24) * theColor.mAlpha) / 255;
			int aRed = (((aColor >> 16) & 0xFF) * theColor.mRed) / 255;
			int aGreen = (((aColor >> 8) & 0xFF) * theColor.mGreen) / 255;
			int aBlue = ((aColor & 0xFF) * theColor.mBlue) / 255;

			if (aAlpha > 255)
				aAlpha = 255;
			if (aRed > 255)
				aRed = 255;
			if (aGreen > 255)
				aGreen = 255;
			if (aBlue > 255)
				aBlue = 255;

			aDestBits[i] = (aAlpha << 24) | (aRed << 16) | (aGreen << 8) | (aBlue);
		}
	}	

	anImage->BitsChanged();

	return anImage;
}

DeviceImage* SexyAppBase::CopyImage(Image* theImage, const Rect& theRect) //Null on XNA, not on iOS? | 1601-1612
{
	DeviceImage* anImage = new DeviceImage(this);

	anImage->Create(theRect.mWidth, theRect.mHeight);
	
	Graphics g(anImage);
	g.DrawImage(theImage, -theRect.mX, -theRect.mY);

	anImage->CopyAttributes(theImage);

	return anImage;
}

DeviceImage* SexyAppBase::CopyImage(Image* theImage) //Null on XNA | 1615-1617
{
	return CopyImage(theImage, Rect(0, 0, theImage->GetWidth(), theImage->GetHeight()));
}

void SexyAppBase::MirrorImage(Image* theImage) //Empty on XNA | 1620-1641
{
	MemoryImage* aSrcMemoryImage = theImage ? aSrcMemoryImage->AsMemoryImage() : NULL;

	ulong* aSrcBits = aSrcMemoryImage->GetBits();

	int aPhysSrcWidth = aSrcMemoryImage->mWidth;
	for (int y = 0; y < aSrcMemoryImage->mHeight; y++)
	{
		ulong* aLeftBits = aSrcBits + (y * aPhysSrcWidth);		
		ulong* aRightBits = aLeftBits + (aPhysSrcWidth - 1);

		for (int x = 0; x < (aPhysSrcWidth >> 1); x++)
		{
			ulong aSwap = *aLeftBits;

			*(aLeftBits++) = *aRightBits;
			*(aRightBits--) = aSwap;
		}
	}

	aSrcMemoryImage->BitsChanged();	
}

void SexyAppBase::FlipImage(Image* theImage) //Null on XNA, not on iOS? | 1644-1668
{
	MemoryImage* aSrcMemoryImage = theImage ? aSrcMemoryImage->AsMemoryImage() : NULL;

	ulong* aSrcBits = aSrcMemoryImage->GetBits();

	int aPhysSrcHeight = aSrcMemoryImage->mHeight;
	int aPhysSrcWidth = aSrcMemoryImage->mWidth;
	for (int x = 0; x < aPhysSrcWidth; x++)
	{
		ulong* aTopBits    = aSrcBits + x;
		ulong* aBottomBits = aTopBits + (aPhysSrcWidth * (aPhysSrcHeight - 1));

		for (int y = 0; y < (aPhysSrcHeight >> 1); y++)
		{
			ulong aSwap = *aTopBits;

			*aTopBits = *aBottomBits;
			aTopBits += aPhysSrcWidth;
			*aBottomBits = aSwap;
			aBottomBits -= aPhysSrcWidth;
		}
	}

	aSrcMemoryImage->BitsChanged();	
}

void SexyAppBase::RotateImageHue(Sexy::MemoryImage *theImage, int theDelta) //Correct? Empty on XNA | 1671-1740
{
	while (theDelta < 0)
		theDelta += 256;

	int aSize = theImage->mWidth * theImage->mHeight;
	DWORD *aPtr = theImage->GetBits();
	for (int i=0; i<aSize; i++)
	{
		DWORD aPixel = *aPtr;
		int alpha = aPixel&0xff000000;
		int r = (aPixel>>16)&0xff;
		int g = (aPixel>>8) &0xff;
		int b = aPixel&0xff;

		int maxval = max(r, max(g, b));
		int minval = min(r, min(g, b));
		int h = 0;
		int s = 0;
		int l = (minval+maxval)/2;
		int delta = maxval - minval;

		if (delta != 0)
		{			
			s = (delta * 256) / ((l <= 128) ? (minval + maxval) : (512 - maxval - minval));
			
			if (r == maxval)
				h = (g == minval ? 1280 + (((maxval-b) * 256) / delta) :  256 - (((maxval - g) * 256) / delta));
			else if (g == maxval)
				h = (b == minval ?  256 + (((maxval-r) * 256) / delta) :  768 - (((maxval - b) * 256) / delta));
			else
				h = (r == minval ?  768 + (((maxval-g) * 256) / delta) : 1280 - (((maxval - r) * 256) / delta));
			
			h /= 6;
		}

		h += theDelta;
		if (h >= 256)
			h -= 256;

		double v= (l < 128) ? (l * (255+s))/255 :
				(l+s-l*s/255);
		
		int y = (int) (2*l-v);

		int aColorDiv = (6 * h) / 256;
		int x = (int)(y+(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
		if (x > 255)
			x = 255;

		int z = (int) (v-(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
		if (z < 0)
			z = 0;
		
		switch (aColorDiv)
		{
			case 0: r = (int) v; g = x; b = y; break;
			case 1: r = z; g= (int) v; b = y; break;
			case 2: r = y; g= (int) v; b = x; break;
			case 3: r = y; g = z; b = (int) v; break;
			case 4: r = x; g = y; b = (int) v; break;
			case 5: r = (int) v; g = y; b = z; break;
			default: r = (int) v; g = x; b = y; break;
		}

		*aPtr++ = alpha | (r<<16) | (g << 8) | (b);	 

	}

	theImage->BitsChanged();
}

ulong SexyAppBase::HSLToRGB(int h, int s, int l) //Seems accurate | 1743-1774
{
	int r;
	int g;
	int b;

	double v= (l < 128) ? (l * (255+s))/255 :
			(l+s-l*s/255);
	
	int y = (int) (2*l-v);

	int aColorDiv = (6 * h) / 256;
	int x = (int)(y+(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
	if (x > 255)
		x = 255;

	int z = (int) (v-(v-y)*((h - (aColorDiv * 256 / 6)) * 6)/255);
	if (z < 0)
		z = 0;
	
	switch (aColorDiv)
	{
		case 0: r = (int) v; g = x; b = y; break;
		case 1: r = z; g= (int) v; b = y; break;
		case 2: r = y; g= (int) v; b = x; break;
		case 3: r = y; g = z; b = (int) v; break;
		case 4: r = x; g = y; b = (int) v; break;
		case 5: r = (int) v; g = y; b = z; break;
		default: r = (int) v; g = x; b = y; break;
	}

	return 0xFF000000 | (r << 16) | (g << 8) | (b);
}
//I guess they're not on iOS?
ulong SexyAppBase::RGBToHSL(int r, int g, int b) //Seems accurate | 1777-1800
{					
	int maxval = max(r, max(g, b));
	int minval = min(r, min(g, b));
	int hue = 0;
	int saturation = 0;
	int luminosity = (minval+maxval)/2;
	int delta = maxval - minval;

	if (delta != 0)
	{			
		saturation = (delta * 256) / ((luminosity <= 128) ? (minval + maxval) : (512 - maxval - minval));
		
		if (r == maxval)
			hue = (g == minval ? 1280 + (((maxval-b) * 256) / delta) :  256 - (((maxval - g) * 256) / delta));
		else if (g == maxval)
			hue = (b == minval ?  256 + (((maxval-r) * 256) / delta) :  768 - (((maxval - b) * 256) / delta));
		else
			hue = (r == minval ?  768 + (((maxval-g) * 256) / delta) : 1280 - (((maxval - r) * 256) / delta));
		
		hue /= 6;
	}

	return 0xFF000000 | (hue) | (saturation << 8) | (luminosity << 16);	 
}

void SexyAppBase::HSLToRGB(const ulong* theSource, ulong* theDest, int theSize) //1803-1809
{
	for (int i = 0; i < theSize; i++)
	{
		ulong src = theSource[i];
		theDest[i] = (src & 0xFF000000) | (HSLToRGB((src & 0xFF), (src >> 8) & 0xFF, (src >> 16) & 0xFF) & 0x00FFFFFF);
	}
}

void SexyAppBase::RGBToHSL(const ulong* theSource, ulong* theDest, int theSize) //1812-1818
{
	for (int i = 0; i < theSize; i++)
	{
		ulong src = theSource[i];
		theDest[i] = (src & 0xFF000000) | (RGBToHSL(((src >> 16) & 0xFF), (src >> 8) & 0xFF, (src & 0xFF)) & 0x00FFFFFF);
	}
}

void SexyAppBase::PrecacheAdditive(MemoryImage* theImage) //C++ only | 1821-1823
{
	theImage->GetRLAdditiveData(mGraphicsDriver->GetNativeDisplayInfo());
}

void SexyAppBase::PrecacheAlpha(MemoryImage* theImage) //C++ only | 1826-1828
{
	theImage->GetRLAlphaData();
}

void SexyAppBase::PrecacheNative(MemoryImage* theImage) //C++ only | 1831-1833
{
	theImage->GetNativeAlphaData(mGraphicsDriver->GetNativeDisplayInfo());
}

void SexyAppBase::PlaySample(int theSoundNum) //1837-1846
{
	if (!mSoundManager)
		return;

	SoundInstance* aSoundInstance = mSoundManager->GetSoundInstance(theSoundNum);
	if (aSoundInstance != NULL)
	{
		aSoundInstance->Play(false, true);
	}
}


void SexyAppBase::PlaySample(int theSoundNum, int thePan) //1850-1860
{
	if (!mSoundManager)
		return;

	SoundInstance* aSoundInstance = mSoundManager->GetSoundInstance(theSoundNum);
	if (aSoundInstance != NULL)
	{
		aSoundInstance->SetPan(thePan);
		aSoundInstance->Play(false, true);
	}
}

bool SexyAppBase::IsMuted() //1863-1865
{
	return mMuteCount > 0;
}

void SexyAppBase::Mute(bool autoMute) //1868-1875
{	
	mMuteCount++;
	if (autoMute)
		mAutoMuteCount++;

	SetMusicVolume(mMusicVolume);
	SetSfxVolume(mSfxVolume);
}

void SexyAppBase::Unmute(bool autoMute) //1878-1888
{	
	if (mMuteCount > 0)
	{
		mMuteCount--;
		if (autoMute)
			mAutoMuteCount--;
	}

	SetMusicVolume(mMusicVolume);
	SetSfxVolume(mSfxVolume);
}


double SexyAppBase::GetMusicVolume() //1892-1894
{
	return mMusicVolume;
}

void SexyAppBase::SetMusicVolume(double theVolume) //1897-1902
{
	mMusicVolume = theVolume;

	if (mMusicInterface != NULL)
		mMusicInterface->SetVolume((mMuteCount > 0) ? 0.0 : mMusicVolume);
}

double SexyAppBase::GetSfxVolume() //1905-1907
{
	return mSfxVolume;
}

void SexyAppBase::SetSfxVolume(double theVolume) //1910-1915
{
	mSfxVolume = theVolume;

	if (mSoundManager != NULL)
		mSoundManager->SetVolume((mMuteCount > 0) ? 0.0 : mSfxVolume);
}

double SexyAppBase::GetMasterVolume() //1918-1920
{
	return mSoundManager->GetMasterVolume();
}

void SexyAppBase::SetMasterVolume(double theMasterVolume) //1923-1926
{
	mSfxVolume = theMasterVolume;
	mSoundManager->SetMasterVolume(mSfxVolume);
}

void SexyAppBase::AddMemoryImage(MemoryImage* theMemoryImage) //1929-1933
{
	if (mGraphicsDriver == NULL) return;
	AutoCrit anAutoCrit(mImageSetCritSect);
	mMemoryImageSet.insert(theMemoryImage);
}

void SexyAppBase::RemoveMemoryImage(MemoryImage* theMemoryImage) //1936-1944
{
	if (mGraphicsDriver == NULL) return;
	AutoCrit anAutoCrit(mCritSect);
	MemoryImageSet::iterator anItr = mMemoryImageSet.find(theMemoryImage);
	if (anItr != mMemoryImageSet.end())
		mMemoryImageSet.erase(anItr);

	Remove3DData(theMemoryImage);
}

void SexyAppBase::Remove3DData(MemoryImage* theMemoryImage) //1947-1949
{
	mAppDriver->Remove3DData(theMemoryImage);
}


bool SexyAppBase::Is3DAccelerated() //1953-1955
{
	return mAppDriver->Is3DAccelerated();
}

bool SexyAppBase::Is3DAccelerationSupported() //1958-1960
{
	return mAppDriver->Is3DAccelerationSupported();
}

bool SexyAppBase::Is3DAccelerationRecommended() //1963-1965
{
	return mAppDriver->Is3DAccelerationRecommended();
}


void SexyAppBase::Set3DAcclerated(bool is3D, bool reinit) //Bruh | 1969-1972
{
	mAppDriver->Set3DAcclerated(is3D, reinit);
}

SharedImageRef SexyAppBase::SetSharedImage(const std::string& theFileName, const std::string& theVariant, DeviceImage* theImage, bool* isNew) //1975-1998
{
	std::string anUpperFileName = StringToUpper(theFileName);
	std::string anUpperVariant = StringToUpper(theVariant);

	std::pair<SharedImageMap::iterator, bool> aResultPair;
	SharedImageRef aSharedImageRef;

	{
		AutoCrit anAutoCrit(mCritSect);
		aResultPair = mSharedImageMap.insert(SharedImageMap::value_type(SharedImageMap::key_type(anUpperFileName, anUpperVariant), SharedImage()));
		aSharedImageRef = &aResultPair.first->second;
	}

	if (isNew != NULL)
		*isNew = aResultPair.second;

	if (aSharedImageRef.mSharedImage->mImage != theImage)
	{
		delete aSharedImageRef.mSharedImage->mImage;
		aSharedImageRef.mSharedImage->mImage = theImage;
	}

	return aSharedImageRef;
}

SharedImageRef SexyAppBase::CheckSharedImage(const std::string& theFileName, const std::string& theVariant) //Correct? | 2001-2029
{
	std::string aFileName;
	int aBarPos = theFileName.find("|", 0);
	if (aBarPos != -1)
	{
		ResourceRef aResourceRef = mResourceManager->GetImageRef(theFileName.substr(aBarPos + 1));
		if (aResourceRef.HasResource())
			return (SharedImageRef&)aResourceRef; //?
		aFileName = theFileName.substr(0, aBarPos);
	}
	else
		aFileName = theFileName;

	std::string anUpperFileName = StringToUpper(aFileName);
	std::string anUpperVariant = StringToUpper(theVariant);

	std::pair<SharedImageMap::iterator, bool> aResultPair;

	SharedImageRef aSharedImageRef;

	{
		AutoCrit anAutoCrit(mCritSect);
		SharedImageMap::iterator anItr = mSharedImageMap.find(SharedImageMap::key_type(anUpperFileName, theVariant)); //?
		if (anItr != mSharedImageMap.end())
			aSharedImageRef = &anItr->second;
	}


	return aSharedImageRef;
}

SharedImageRef SexyAppBase::GetSharedImage(const std::string& theFileName, const std::string& theVariant, bool* isNew, bool allowTriReps) //Correct? | 2032-2091
{
	std::string aFileName;
	int aBarPos = theFileName.find('|');
	if (aBarPos != -1)
	{
		ResourceRef aResourceRef = mResourceManager->GetImageRef(theFileName.substr(aBarPos + 1));
		if (aResourceRef.HasResource())
			return (SharedImageRef&)aResourceRef;
		aFileName = theFileName.substr(0, aBarPos);
	}
	else
		aFileName = theFileName;

	std::string anUpperFileName = StringToUpper(theFileName);
	std::string anUpperVariant = StringToUpper(theVariant);

	std::pair<SharedImageMap::iterator, bool> aResultPair;
	SharedImageRef aSharedImageRef;

	{
		AutoCrit anAutoCrit(mCritSect);	
		aResultPair = mSharedImageMap.insert(SharedImageMap::value_type(SharedImageMap::key_type(anUpperFileName, anUpperVariant), SharedImage()));
		aSharedImageRef = &aResultPair.first->second;
		if (&aResultPair.second)
			aSharedImageRef.mSharedImage->mLoading = true;
	}

	if (isNew != NULL)
		*isNew = aResultPair.second;

	if (aResultPair.second)
	{
		// Pass in a '!' as the first char of the file name to create a new image
		if ((theFileName.length() > 0) && (theFileName[0] == '!'))
		{
			aSharedImageRef.mSharedImage->mImage = new DeviceImage();
			if (!allowTriReps)
				aSharedImageRef->AddImageFlags(ImageFlag_NoTriRep);
		}
		else
			aSharedImageRef.mSharedImage->mImage = GetImage(theFileName,false,allowTriReps);
		aSharedImageRef.mSharedImage->mLoading = false;
		mSharedImageEvent.Notify();
	}
	else
	{
		while (aSharedImageRef.mSharedImage->mLoading)
			mSharedImageEvent.Wait(20);
	}

	return aSharedImageRef;
}

void SexyAppBase::CleanSharedImages() //2094-2119
{
	AutoCrit anAutoCrit(mCritSect);	

	if (mCleanupSharedImages)
	{
		// Delete shared images with reference counts of 0
		// This doesn't occur in ~SharedImageRef because sometimes we can not only access the image
		//  through the SharedImageRef returned by GetSharedImage, but also by calling GetSharedImage
		//  again with the same params -- so we can have instances where we do the 'final' deref on
		//  an image but immediately re-request it via GetSharedImage
		SharedImageMap::iterator aSharedImageItr = mSharedImageMap.begin();
		while (aSharedImageItr != mSharedImageMap.end())
		{
			SharedImage* aSharedImage = &aSharedImageItr->second;
			if (aSharedImage->mRefCount == 0)
			{
				delete aSharedImage->mImage;
				mSharedImageMap.erase(aSharedImageItr++);
			}
			else
				++aSharedImageItr;
		}

		mCleanupSharedImages = false;
	}
}

//////////////////////////////////////////////////////////////////////////
SharedRenderTarget::Pool* SexyAppBase::GetSharedRenderTargetPool() //2124-2128
{
	if (mSharedRTPool == NULL)
		mSharedRTPool = new SharedRenderTarget::Pool();
	return mSharedRTPool;
}

void SexyAppBase::RehupFocus() // MOVED TO *AppDriver | 2131-2133
{
}

#ifndef __APPLE__ //Windows or non-iOS only.
Reflection::CRefSymbolDb* SexyAppBase::GetReflection() //2139-2141
{
	return mAppDriver->GetReflection();
}
#endif

void SexyAppBase::LowMemoryWarning() //2145-2146
{
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
void SexyAppBase::GamepadButtonDown(GamepadButton theButton, int thePlayer, ulong theFlags)
#else
void SexyAppBase::GamepadButtonDown(int theButton, int thePlayer, ulong theFlags) //2149-2151
#endif
{
	mWidgetManager->GamepadButtonDown(theButton, thePlayer, theFlags);
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
void SexyAppBase::GamepadButtonUp(GamepadButton theButton, int thePlayer, ulong theFlags)
#else
void SexyAppBase::GamepadButtonUp(int theButton, int thePlayer, ulong theFlags) //2154-2156
#endif
{
	mWidgetManager->GamepadButtonUp(theButton, thePlayer, theFlags);
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
void SexyAppBase::GamepadAxisMove(GamepadAxis theAxis, int thePlayer, int theAxisValue) //Float in XNA | 2159-2161
#else
void SexyAppBase::GamepadAxisMove(int theAxis, int thePlayer, int theAxisValue)
#endif
{
	mWidgetManager->GamepadAxisMove(theAxis, thePlayer, theAxisValue);
}

bool SexyAppBase::IsUIOrientationAllowed(UI_ORIENTATION theOrientation) //2164-2167
{
	return theOrientation == UI_ORIENTATION_LANDSCAPE_LEFT || theOrientation == UI_ORIENTATION_LANDSCAPE_RIGHT;
}

void SexyAppBase::UIOrientationChanged(UI_ORIENTATION theOrientation) //2170-2172
{
}

bool SexyAppBase::IsMainThread() //2175-2177
{
	return mMainThreadId == GetCurrentRunningThread();
}

#if !defined(_WIN32) || defined(__APPLE__) //Only on mobile and XNA
UI_ORIENTATION SexyAppBase::GetUIOrientation()
{
	return mAppDriver->GetUIOrientation();
}
#endif

#ifdef _SEXYDECOMP_USE_LATEST_CODE
bool SexyAppBase::FrameNeedsSwapScreenImage() //C++ only, is this part of iOS BejClassic? BejLivePlus puts it in the game's App class.
{
	return true;
}

SexyAppBase::OnResourcesUpdated() //C++ only
{

}

SexyAppBase::OpenImagePicker() //C++ only
{
	//?
}

SexyAppBase::ImagePickerCompleted() //C++ only
{
	//?
}

SexyAppBase::AppEnteredForeground() //C++ only
{
}

void SexyAppBase::DrawSpecial(Graphics g) //C++ only, is this part of iOS BejClassic? BejLivePlus puts it in the game's App class. It's not in iOS PVZ, why is it here
{
}

#endif
