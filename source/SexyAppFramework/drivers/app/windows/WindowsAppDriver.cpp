#include "WindowsAppDriver.h"
#include "../../graphics/windows/WindowsGraphicsDriver.h"
#include "../../graphics/windows/D3DTester.h"
#include "../../audio/windows/WindowsAudioDriver.h"
#include "../../../ResourceManager.h"
#include "../../../DeviceImage.h"
#include "../../../SexyCache.h"
#include "../../../SEHCatcher.h"
#include "../../../PixelTracer.h"
#include "../../../PerfTimer.h"
#include "../../../ImageLib/ImageLib.h"
#include "../../../MusicInterface.h"
#include "../../../SoundManager.h"
#include "../../../WidgetManager.h"
#include "../../../SysFont.h"
#include "../../../RenderDevice.h"
#include "../../../SexyLocale.h"
#include "../../../SexyThread.h"
#include "../../../Mesh.h"
#include "../../../CfgMachine.h"
#include "../../../Reflection.h"
//#include "../../graphics/windows/D3DInterface.h"
#include <string>
#include <fstream>
#include <regstr.h>
#include <windows.h>
#include "../../../AutoCrit.h"
#include "../../../PakLib/PakInterface.h"
#include <direct.h>
//#include <d3dtypes.h>
//#include <ddraw.h>

using namespace Sexy;
using namespace Reflection; //Using?

HMODULE gDDrawDLL = NULL;
HMODULE gDSoundDLL = NULL;
HMODULE gVersionDLL = NULL;
static bool gScreenSaverActive = false;

static DeviceImage* gFPSImage = NULL;
static DeviceImage* gFPSHistoryImage = NULL;

typedef BOOL(WINAPI* GetLastInputInfoFunc)(LASTINPUTINFO* plii);
GetLastInputInfoFunc gGetLastInputInfoFunc = NULL;

//HotSpot: 11 4
//Size: 32 32
unsigned char gFingerCursorData[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xe7, 0xff, 0xff, 0xff, 0xc3, 0xff, 0xff, 0xff, 0xc3, 0xff, 0xff, 0xff, 0xc3,
	0xff, 0xff, 0xff, 0xc3, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xc0, 0x1f, 0xff, 0xff,
	0xc0, 0x07, 0xff, 0xff, 0xc0, 0x03, 0xff, 0xfc, 0x40, 0x01, 0xff, 0xfc, 0x00, 0x01, 0xff,
	0xfc, 0x00, 0x01, 0xff, 0xfc, 0x00, 0x01, 0xff, 0xff, 0x00, 0x01, 0xff, 0xff, 0x00, 0x01,
	0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0x80, 0x03, 0xff, 0xff, 0xc0, 0x03, 0xff, 0xff, 0xc0,
	0x03, 0xff, 0xff, 0xe0, 0x07, 0xff, 0xff, 0xe0, 0x07, 0xff, 0xff, 0xe0, 0x07, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18,
	0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
	0x18, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x1b, 0x60, 0x00, 0x00, 0x1b, 0x68, 0x00,
	0x00, 0x1b, 0x6c, 0x00, 0x01, 0x9f, 0xec, 0x00, 0x01, 0xdf, 0xfc, 0x00, 0x00, 0xdf, 0xfc,
	0x00, 0x00, 0x5f, 0xfc, 0x00, 0x00, 0x7f, 0xfc, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x3f,
	0xf8, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00,
	0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00
};

//HotSpot: 15 10
//Size: 32 32
unsigned char gDraggingCursorData[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xfc, 0x0f, 0xff, 0xff, 0xf0, 0x07, 0xff, 0xff, 0xe0,
	0x01, 0xff, 0xff, 0xe0, 0x00, 0xff, 0xff, 0xe0, 0x00, 0xff, 0xff, 0xe0, 0x00, 0xff, 0xff,
	0xe0, 0x00, 0xff, 0xfe, 0x60, 0x00, 0xff, 0xfc, 0x20, 0x00, 0xff, 0xfc, 0x00, 0x00, 0xff,
	0xfe, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x80, 0x00,
	0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0xc0, 0x01, 0xff, 0xff, 0xe0, 0x01, 0xff, 0xff, 0xf0,
	0x03, 0xff, 0xff, 0xf8, 0x03, 0xff, 0xff, 0xf8, 0x03, 0xff, 0xff, 0xf8, 0x03, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x80, 0x00, 0x00, 0x01, 0xb0, 0x00, 0x00, 0x0d, 0xb0, 0x00, 0x00, 0x0d, 0xb6, 0x00, 0x00,
	0x0d, 0xb6, 0x00, 0x00, 0x0d, 0xb6, 0x00, 0x00, 0x0d, 0xb6, 0x00, 0x00, 0x0d, 0xb6, 0x00,
	0x01, 0x8d, 0xb6, 0x00, 0x01, 0xcf, 0xfe, 0x00, 0x00, 0xef, 0xfe, 0x00, 0x00, 0xff, 0xfe,
	0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x3f, 0xfe, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x00, 0x1f,
	0xfc, 0x00, 0x00, 0x0f, 0xfc, 0x00, 0x00, 0x07, 0xf8, 0x00, 0x00, 0x03, 0xf8, 0x00, 0x00,
	0x03, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00
};

#ifndef SPI_GETSCREENSAVERRUNNING
#define SPI_GETSCREENSAVERRUNNING 114
#endif

void Sexy::SexyAppRun(SexyAppBase* appBase) //69-74
{
	appBase->Init();
	appBase->Start();
	appBase->Shutdown();
}

IAppDriver* IAppDriver::CreateAppDriver(SexyAppBase* appBase) //77-79
{
	return new WindowsAppDriver(appBase);
}

WindowsAppDriver::WindowsAppDriver(SexyAppBase* appBase) //172-175
{
	mWindowsGraphicsDriver = NULL;
	mApp = appBase;
}


bool WindowsAppDriver::InitAppDriver() //TODO | 179-418
{
	gSEHCatcher.Init();

	gVersionDLL = LoadLibraryA("version.dll");
	gDDrawDLL = LoadLibraryA("ddraw.dll");
	gDSoundDLL = LoadLibraryA("dsound.dll");
	gGetLastInputInfoFunc = (GetLastInputInfoFunc)GetProcAddress(GetModuleHandleA("user32.dll"), "GetLastInputInfo");

	ImageLib::InitJPEG2000();

	mApp->mMutex = NULL;
	mApp->mNotifyGameMessage = 0;

#ifdef _DEBUG //Prob was kept, XNA kept it so prob correct
	mApp->mOnlyAllowOneCopyToRun = false;
#else
	mApp->mOnlyAllowOneCopyToRun = true;
#endif

	// Extract product version
	char aPath[_MAX_PATH];
	GetModuleFileNameA(NULL, aPath, 256);
	mApp->mProductVersion = GetProductVersion(aPath);
	mApp->mChangeDirTo = GetFileDir(aPath);

	mApp->mNoDefer = false;
	mApp->mFullScreenPageFlip = true; // should we page flip in fullscreen?
	mApp->mTimeLoaded = GetTickCount();
	mApp->mSEHOccured = false;
	mApp->mProdName = "Product";
	mApp->mTitle = _S("SexyApp");
	mApp->mShutdown = false;
	mApp->mExitToTop = false;
	mApp->mWidth = 640;
	mApp->mHeight = 480;
	mApp->mFullscreenBits = 16;
	mApp->mIsWindowed = true;
	mApp->mIsPhysWindowed = true;
	mApp->mFullScreenWindow = false;
	mApp->mPreferredX = -1;
	mApp->mPreferredY = -1;
	mApp->mPreferredWidth = -1;
	mApp->mPreferredHeight = -1;
	mApp->mIsScreenSaver = false;
	mApp->mAllowMonitorPowersave = true;
	mApp->mHWnd = NULL;
	mApp->mMusicInterface = NULL;
	mApp->mInvisHWnd = NULL;
	mApp->mFrameTime = 10;
	mApp->mNonDrawCount = 0;
	mApp->mDrawCount = 0;
	mApp->mSleepCount = 0;
	mApp->mUpdateCount = 0;
	mApp->mUpdateAppState = 0;
	mApp->mUpdateAppDepth = 0;
	mApp->mPendingUpdatesAcc = 0.0;
	mApp->mUpdateFTimeAcc = 0.0;
	mApp->mHasPendingDraw = true;
	mApp->mIsDrawing = false;
	mApp->mLastDrawWasEmpty = false;
	mApp->mLastTimeCheck = 0;
	mApp->mUpdateMultiplier = 1;
	mApp->mMaxNonDrawCount = 10;
	mApp->mPaused = false;
	mApp->mFastForwardToUpdateNum = 0;
	mApp->mFastForwardToMarker = false;
	mApp->mFastForwardStep = false;
	mApp->mSoundManager = NULL;
	mApp->mCursorNum = CURSOR_POINTER;
	mApp->mMouseIn = false;
	mApp->mRunning = false;
	mApp->mActive = true;
	mApp->mProcessInTimer = false;
	mApp->mMinimized = false;
	mApp->mPhysMinimized = false;
	mApp->mIsDisabled = false;
	mApp->mLoaded = false;
	mApp->mReloadingResources = false;
	mApp->mReloadPct = 0;
	mApp->mYieldMainThread = false;
	mApp->mLoadingFailed = false;
	mApp->mLoadingThreadStarted = false;
	mApp->mAutoStartLoadingThread = true;
	mApp->mLoadingThreadCompleted = false;
	mApp->mCursorThreadRunning = false;
	mApp->mNumLoadingThreadTasks = 0;
	mApp->mCompletedLoadingThreadTasks = 0;
	mApp->mLastDrawTick = timeGetTime();
	mApp->mNextDrawTick = timeGetTime();
	mApp->mSysCursor = true;
	mApp->mForceFullscreen = false;
	mApp->mForceWindowed = false;
	mApp->mHasFocus = true;
	mApp->mCustomCursorsEnabled = false;
	mApp->mCustomCursorDirty = false;
	mApp->mOverrideCursor = NULL;
	mApp->mIsOpeningURL = false;
	mApp->mInitialized = false;
	mApp->mLastShutdownWasGraceful = true;
	mApp->mReadFromRegistry = false;
	mApp->mCmdLineParsed = false;
	mApp->mSkipSignatureChecks = false;
	mApp->mCtrlDown = false;
	mApp->mAltDown = false;
	mApp->mAllowAltEnter = true;
	mApp->mStepMode = 0;
	mApp->mCleanupSharedImages = false;
	mApp->mStandardWordWrap = true;
	mApp->mbAllowExtendedChars = true;
	mApp->mEnableMaximizeButton = false;

	mApp->mWriteToSexyCache = true;
	mApp->mSexyCacheBuffers = false;
	mApp->mWriteFontCacheDir = true;

	mApp->mMusicVolume = 0.85;
	mApp->mSfxVolume = 0.85;
	mApp->mMuteCount = 0;
	mApp->mAutoMuteCount = 0;
	mApp->mDemoMute = false;
	mApp->mMuteOnLostFocus = true;
	mApp->mCurHandleNum = 0;
	mApp->mFPSTime = 0;
	mApp->mFPSStartTick = GetTickCount();
	mApp->mFPSFlipCount = 0;
	mApp->mFPSCount = 0;
	mApp->mFPSDirtyCount = 0;
	mApp->mShowFPS = false;
	mApp->mShowFPSMode = FPS_ShowFPS;
	mApp->mVFPSUpdateTimes = 0.0;
	mApp->mVFPSUpdateCount = 0;
	mApp->mVFPSDrawTimes = 0.0;
	mApp->mVFPSDrawCount = 0;
	mApp->mCurVFPS = 0.0;
	mApp->mDrawTime = 0;
	mApp->mScreenBltTime = 0;
	mApp->mDebugKeysEnabled = false;
	mApp->mOldWndProc = 0;
	mApp->mNoSoundNeeded = false;
	mApp->mWantFMod = false;

	mApp->mSyncRefreshRate = 100;
	mApp->mVSyncUpdates = false;
	mApp->mNoVSync = true;
	mApp->mVSyncBroken = false;
	mApp->mVSyncBrokenCount = 0;
	mApp->mVSyncBrokenTestStartTick = 0;
	mApp->mVSyncBrokenTestUpdates = 0;
	mApp->mWaitForVSync = false;
	mApp->mSoftVSyncWait = true;
	mApp->mAutoEnable3D = false;
	mApp->mTest3D = false;
	mApp->mNoD3D9 = false;
	mApp->mMinVidMemory3D = 6;
	mApp->mRecommendedVidMemory3D = 14;
	mApp->mRelaxUpdateBacklogCount = 0;
	mApp->mWidescreenAware = false;
	mApp->mWidescreenTranslate = true;
	mApp->mEnableWindowAspect = false;
	mApp->mWindowAspect.Set(4, 3);
	mApp->mMinAspect.Set(4, 3);
	mApp->mMaxAspect.Set(16, 10);
	mApp->mIsWideWindow = false;
	mApp->mOrigScreenWidth = GetSystemMetrics(0);
	mApp->mOrigScreenHeight = GetSystemMetrics(1);
	mApp->mIsSizeCursor = false;

	int i;

	for (i = 0; i < NUM_CURSORS; i++)
		mApp->mCursorImages[i] = NULL;

	for (i = 0; i < 256; i++)
		mApp->mAdd8BitMaxTable[i] = i;

	for (i = 256; i < 512; i++)
		mApp->mAdd8BitMaxTable[i] = 255;

	// Set default strings.  Init could read in overrides from partner.xml

	mApp->SetString("UPDATE_CHECK_TITLE", L"Update Check");
	mApp->SetString("UPDATE_CHECK_BODY", L"Checking if there are any updates available for this product ...");

	mApp->SetString("UP_TO_DATE_TITLE", L"Up to Date");
	mApp->SetString("UP_TO_DATE_BODY", L"There are no updates available for this product at this time.");
	mApp->SetString("NEW_VERSION_TITLE", L"New Version");
	mApp->SetString("NEW_VERSION_BODY", L"There is an update available for this product.  Would you like to visit the web site to download it?");

	mApp->mPrimaryThreadId = 0;

	if (GetSystemMetrics(86)) // check for tablet pc
	{
		mApp->mTabletPC = true;
		mApp->mFullScreenPageFlip = false; // so that tablet keyboard can show up
	}
	else
		mApp->mTabletPC = false;

	gSEHCatcher.mApp = this->mApp;
	mApp->mSharedRTPool = NULL;
	mApp->mAllowWindowResize = true;
	mApp->mCfgCompiler = ICfgCompiler::CompilerCreate();
	std::string aCompatCfgErrorStr;
	mApp->mCompatCfgMachine = mApp->mCfgCompiler->CompilerCreateMachineFromFile("compat.cfg", aCompatCfgErrorStr);
#ifdef _DEBUG
	if (mApp->mCompatCfgMachine)
	{
		mApp->mCompatCfgMachine->MachineDisassembleToFile("compat.cfgasm"); //Debug only
		mApp->mCompatCfgMachine->MachineExecuteFunction(NULL, (CfgMachineValue*)CFGMVT_None);
	}
	else if (!aCompatCfgErrorStr.empty())
	{
		Popup(StrFormat("Configuration file parse error: %s", aCompatCfgErrorStr.c_str()));
	}
#endif

	mApp->mShowCompatInfoMode = SexyAppBase::SHOWCOMPATINFOMODE_OFF;
	mApp->mShowWidgetInspector = false;
	mApp->mWidgetInspectorCurWidget = NULL;
	mApp->mWidgetInspectorScrollOffset = 0;
	mApp->mWidgetInspectorPickWidget = NULL;
	mApp->mWidgetInspectorPickMode = false;
	mApp->mWidgetInspectorLeftAnchor = false;
	mApp->mWidgetInspectorClickPos.mX = -1;
	mApp->mWidgetInspectorClickPos.mY = -1;


	//TODO
	mApp->mRefSymbolDb = NULL;

}

WindowsAppDriver::~WindowsAppDriver() //Added FPSHistory image from SAB | 421-564
{
	Shutdown();

	// Check if we should write the current 3d setting
	bool showedMsgBox = false;
	if (SexyAppBase::sAttemptingNonRecommended3D)
	{
		bool writeToRegistry = true;
		bool is3D = false;
		bool is3DOptionSet = ConfigReadBoolean("Is3D", &is3D);
		if (!is3DOptionSet) // should we write the option?
		{
			if (!Is3DAccelerationRecommended()) // may need to prompt user if he wants to keep 3d acceleration on (LHR: is this needed)
			{
				if (Is3DAccelerated())
				{
					showedMsgBox = true;
					int aResult = MessageBox(NULL,
						mApp->GetString("HARDWARE_ACCEL_SWITCHED_ON",
							_S("3D acceleration was switched on during this session.\r\n") //Changed to 3D
							_S("If this resulted in slower performance or other display problems,\r\n")
							_S("it should be switched off.\r\n") //?
							_S("\r\n")
							_S("Would you like to keep 3D acceleration switched on?")).c_str(),
						(StringToSexyString(mApp->mCompanyName) + _S(" ") +
							mApp->GetString("HARDWARE_ACCEL_CONFIRMATION", _S("3D Acceleration Confirmation"))).c_str(),
						MB_YESNO | MB_ICONQUESTION);

					mWindowsGraphicsDriver->mIs3D = aResult == IDYES ? true : false;
					if (aResult != IDYES)
					{
						writeToRegistry = false;
						mApp->sAttemptingNonRecommended3D = false;
					}
				}
				else
					writeToRegistry = false;
			}
		}

		if (writeToRegistry)
			ConfigWriteBoolean("Is3D", mWindowsGraphicsDriver->mIs3D);
	}

	extern bool gD3DInterfacePreDrawError;
	if (!showedMsgBox && gD3DInterfacePreDrawError && !IsScreenSaver())
	{
		int aResult = MessageBox(NULL,
			mApp->GetString("HARDWARE_ACCEL_NOT_WORKING",
				_S("3D acceleration may not have been working correctly during this session.\r\n")
				_S("If you noticed graphics problems, you may want to turn off 3D acceleration.\r\n")
				_S("Would you like to keep 3D acceleration switched on?")).c_str(),
			(StringToSexyString(mApp->mCompanyName) + _S(" ") +
				mApp->GetString("HARDWARE_ACCEL_CONFIRMATION", _S("3D Acceleration Confirmation"))).c_str(),
			MB_YESNO | MB_ICONQUESTION);

		if (aResult == IDNO)
			ConfigWriteBoolean("Is3D", false);
	}

	if (mApp->mInvisHWnd != NULL)
	{
		HWND aWindow = mApp->mInvisHWnd;
		mApp->mInvisHWnd = NULL;
		SetWindowLong(aWindow, GWL_USERDATA, NULL);
		DestroyWindow(aWindow);
	}

	gFPSImage->DeleteExtraBuffers();
	gFPSImage = NULL;
	gFPSHistoryImage->DeleteExtraBuffers();
	gFPSHistoryImage = NULL;

	if (mWindowsGraphicsDriver)
	{
		DBG_ASSERTE(mWindowsGraphicsDriver->mMeshSet.size() == 0); //497
		mWindowsGraphicsDriver->CleanupMeshes();
	}

	delete mApp->mGraphicsDriver;
	delete mApp->mMusicInterface;
	delete mApp->mSoundManager;

	mApp->mGraphicsDriver = NULL;
	mApp->mMusicInterface = NULL;
	mApp->mSoundManager = NULL;

	if (mApp->mHWnd != NULL)
	{
		HWND hWnd = mApp->mHWnd;
		mApp->mHWnd = NULL;

		SetWindowLong(hWnd, GWL_USERDATA, NULL);

		/*char aStr[256];
		sprintf(aStr, "HWND: %d\r\n", aWindow);
		OutputDebugString(aStr);*/

		DestroyWindow(hWnd);
	}

	WaitForLoadingThread();
	ImageLib::CloseJPEG2000();
	DestroyCursor(mApp->mHandCursor);
	DestroyCursor(mApp->mDraggingCursor);

	if (mApp->mCompatCfgMachine)
	{
		mApp->mCompatCfgMachine->MachineDestroy();
		delete mApp->mCompatCfgMachine;
	}
	if (mApp->mCfgCompiler)
	{
		mApp->mCfgCompiler->CompilerDestroy();
		delete mApp->mCfgCompiler;
	}
	if (mApp->mRefSymbolDb)
	{
		delete mApp->mRefSymbolDb;
	}

	if (mApp->mMutex != NULL)
		::CloseHandle(mApp->mMutex);

	FreeLibrary(gDDrawDLL);
	FreeLibrary(gDSoundDLL);
	FreeLibrary(gVersionDLL);
	UnregisterClass("MainWindow", gHInstance);
	UnregisterClass("InvisWindow", gHInstance);
}

static BOOL CALLBACK ChangeDisplayWindowEnumProc(HWND hwnd, LPARAM lParam) //567-597
{
	typedef std::map<HWND,RECT> WindowMap;
	static WindowMap aMap;

	if (lParam==0 && aMap.find(hwnd)==aMap.end()) // record
	{
		RECT aRect;
		if (!IsIconic(hwnd) && IsWindowVisible(hwnd))
		{
			if (GetWindowRect(hwnd, &aRect))
			{
				//				char aBuf[4096];
				//				GetWindowText(hwnd,aBuf,4000);
				//				DWORD aProcessId = 0;
				//				GetWindowThreadProcessId(hwnd,&aProcessId);
				//				SEXY_TRACE(StrFormat("%s %d - %d %d %d %d",aBuf,aProcessId,aRect.left,aRect.top,aRect.right,aRect.bottom).c_str());
				aMap[hwnd] = aRect;
			}
		}
	}
	else
	{
		WindowMap::iterator anItr = aMap.find(hwnd);
		if (anItr != aMap.end())
		{
			RECT& r = anItr->second;
			MoveWindow(hwnd, r.left, r.top, abs(r.right - r.left), abs(r.bottom - r.top), TRUE);
		}
	}
	return TRUE;
}

void WindowsAppDriver::ClearUpdateBacklog(bool relaxForASecond) //600-606
{
	mApp->mLastTimeCheck = timeGetTime();
	mApp->mUpdateFTimeAcc = 0.0;

	if (relaxForASecond)
		mApp->mRelaxUpdateBacklogCount = 1000;
}

bool WindowsAppDriver::IsScreenSaver() //609-611
{
	return mApp->mIsScreenSaver;
}

bool WindowsAppDriver::AppCanRestore() //614-616
{
	return !mApp->mIsDisabled;
}

bool WindowsAppDriver::ProcessMessages() //619-627
{
	tagMSG msg;
	while ((PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) && (!mApp->mShutdown))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return true;
}

bool WindowsAppDriver::OpenURL(const std::string& theURL, bool shutdownOnOpen) //632-652
{
	if ((!mApp->mIsOpeningURL) || (theURL != mApp->mOpeningURL))
	{
		mApp->mShutdownOnURLOpen = shutdownOnOpen;
		mApp->mIsOpeningURL = true;
		mApp->mOpeningURL = theURL;
		mApp->mOpeningURLTime = GetTickCount();

		if ((int)ShellExecuteA(NULL, "open", theURL.c_str(), NULL, NULL, SW_SHOWNORMAL) > 32)
		{
			return true;
		}
		else
		{
			mApp->URLOpenFailed(theURL);
			return false;
		}
	}

	return true;
}

std::string WindowsAppDriver::GetProductVersion(const std::string& thePath) //655-700
{
	// Dynamically Load Version.dll
	typedef DWORD(APIENTRY* GetFileVersionInfoSizeFunc)(LPSTR lptstrFilename, LPDWORD lpdwHandle);
	typedef BOOL(APIENTRY* GetFileVersionInfoFunc)(LPSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);
	typedef BOOL(APIENTRY* VerQueryValueFunc)(const LPVOID pBlock, LPSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen);

	static GetFileVersionInfoSizeFunc aGetFileVersionInfoSizeFunc = NULL;
	static GetFileVersionInfoFunc aGetFileVersionInfoFunc = NULL;
	static VerQueryValueFunc aVerQueryValueFunc = NULL;

	if (aGetFileVersionInfoSizeFunc == NULL)
	{
		aGetFileVersionInfoSizeFunc = (GetFileVersionInfoSizeFunc)GetProcAddress(gVersionDLL, "GetFileVersionInfoSizeA");
		aGetFileVersionInfoFunc = (GetFileVersionInfoFunc)GetProcAddress(gVersionDLL, "GetFileVersionInfoA");
		aVerQueryValueFunc = (VerQueryValueFunc)GetProcAddress(gVersionDLL, "VerQueryValueA");
	}

	// Get Product Version
	std::string aProductVersion;

	uint aSize = aGetFileVersionInfoSizeFunc((char*)thePath.c_str(), 0);
	if (aSize > 0)
	{
		uchar* aVersionBuffer = new uchar[aSize];
		aGetFileVersionInfoFunc((char*)thePath.c_str(), 0, aSize, aVersionBuffer);
		char* aBuffer;
		if (aVerQueryValueFunc(aVersionBuffer,
			"\\StringFileInfo\\040904B0\\ProductVersion",
			(void**)&aBuffer,
			&aSize))
		{
			aProductVersion = aBuffer;
		}
		else if (aVerQueryValueFunc(aVersionBuffer,
			"\\StringFileInfo\\040904E4\\ProductVersion",
			(void**)&aBuffer,
			&aSize))
		{
			aProductVersion = aBuffer;
		}

		delete aVersionBuffer;
	}

	return aProductVersion;
}

void WindowsAppDriver::WaitForLoadingThread() //703-706
{
	while ((mApp->mLoadingThreadStarted) && (!mApp->mLoadingThreadCompleted))
		Sleep(20); //Would be SexySleep but I guess they moved this 1:1 from Windows-only designed SAB
}

void WindowsAppDriver::SetCursorImage(int theCursorNum, Image* theImage) //709-715
{
	if ((theCursorNum >= 0) && (theCursorNum < NUM_CURSORS))
	{
		mApp->mCursorImages[theCursorNum] = theImage;
		EnforceCursor();
	}
}

void WindowsAppDriver::TakeScreenshot() //Still here, just inaccessible | 718-809
{
	if (mWindowsGraphicsDriver == NULL || mWindowsGraphicsDriver->mDrawSurface == NULL)
		return;

	// Get free image name
	std::string anImageDir = GetAppDataFolder() + "_screenshots";
	MkDir(anImageDir);
	anImageDir += "/";

	WIN32_FIND_DATAA aData;
	int aMaxId = 0;
	std::string anImagePrefix = "image";
	HANDLE aHandle = FindFirstFileA((anImageDir + "*.png").c_str(), &aData);
	if (aHandle != INVALID_HANDLE_VALUE)
	{
		do {
			int aNum = 0;
			if (sscanf(aData.cFileName, (anImagePrefix + "%d.png").c_str(), &aNum) == 1)
			{
				if (aNum > aMaxId)
					aMaxId = aNum;
			}

		} while (FindNextFileA(aHandle, &aData));
		FindClose(aHandle);
	}
	std::string anImageName = anImageDir + anImagePrefix + StrFormat("%d.png", aMaxId + 1);

	// Capture screen
	IUnknown* aSurface = mWindowsGraphicsDriver->mDrawSurface;

	// Temporarily set the mDrawSurface to NULL so DDImage::Check3D 
	// returns false so we can lock the surface.
	mWindowsGraphicsDriver->mDrawSurface = NULL;

	DeviceImage anImage(this->mApp);
	anImage.SetSurface(aSurface);
	anImage.GetBits();
	anImage.DeleteDeviceSurface();
	mWindowsGraphicsDriver->mDrawSurface = aSurface;

	if (anImage.mBits == NULL)
		return;

	// Write image
	ImageLib::Image aSaveImage;
	aSaveImage.mBits = anImage.mBits;
	aSaveImage.mWidth = anImage.mWidth;
	aSaveImage.mHeight = anImage.mHeight;
	ImageLib::WritePNGImage(anImageName, &aSaveImage);
	aSaveImage.mBits = NULL;
	ClearUpdateBacklog();
}

void WindowsAppDriver::DumpProgramInfo() //TODO | 812-1043
{
	Deltree(GetAppDataFolder() + "_dump");

	for (;;)
	{
		if (mkdir((GetAppDataFolder() + "_dump").c_str()))
			break;
		Sleep(100);
	}

	std::fstream aDumpStream((GetAppDataFolder() + "_dump\\imagelist.html").c_str(), std::ios::out);

	time_t aTime;
	time(&aTime);
	tm* aTM = localtime(&aTime);

	aDumpStream << "<HTML><BODY BGCOLOR=EEEEFF><CENTER><FONT SIZE=+2><B>" << asctime(aTM) << "</B></FONT><BR>" << std::endl;

	int anImgNum = 0;

	int aThumbWidth = 64;
	int aThumbHeight = 64;

	ImageLib::Image anImageLibImage;
	anImageLibImage.mWidth = aThumbWidth;
	anImageLibImage.mHeight = aThumbHeight;
	anImageLibImage.mBits = new unsigned long[aThumbWidth * aThumbHeight];

	typedef std::multimap<int, MemoryImage*, std::greater<int> > SortedImageMap;

	int aTotalMemory = 0;

	SortedImageMap aSortedImageMap;
	MemoryImageSet::iterator anItr = mApp->mMemoryImageSet.begin();
	while (anItr != mApp->mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;

		int aNumPixels = aMemoryImage->mWidth * aMemoryImage->mHeight;

		DeviceImage* aDDImage = dynamic_cast<DeviceImage*>(aMemoryImage);

		int aBitsMemory = 0;
		int aSurfaceMemory = 0;
		int aPalletizedMemory = 0;
		int aNativeAlphaMemory = 0;
		int aRLAlphaMemory = 0;
		int aRLAdditiveMemory = 0;
		int aTextureMemory = 0;

		int aMemorySize = 0;
		if (aMemoryImage->mBits != NULL)
			aBitsMemory = aNumPixels * 4;
		if ((aDDImage != NULL) && (aDDImage->mSurface != NULL))
			aSurfaceMemory = aNumPixels * 4; // Assume 32bit screen...
		if (aMemoryImage->mColorTable != NULL)
			aPalletizedMemory = aNumPixels + 256 * 4;
		if (aMemoryImage->mNativeAlphaData != NULL)
		{
			if (aMemoryImage->mColorTable != NULL)
				aNativeAlphaMemory = 256 * 4;
			else
				aNativeAlphaMemory = aNumPixels * 4;
		}
		if (aMemoryImage->mRLAlphaData != NULL)
			aRLAlphaMemory = aNumPixels;
		if (aMemoryImage->mRLAdditiveData != NULL)
			aRLAdditiveMemory = aNumPixels;
		if (mWindowsGraphicsDriver->mRenderDevice3D != NULL)
			aTextureMemory += mWindowsGraphicsDriver->mRenderDevice3D->GetTextureMemorySize(aMemoryImage);

		aMemorySize = aBitsMemory + aSurfaceMemory + aPalletizedMemory + aNativeAlphaMemory + aRLAlphaMemory + aRLAdditiveMemory + aTextureMemory;
		aTotalMemory += aMemorySize;

		aSortedImageMap.insert(SortedImageMap::value_type(aMemorySize, aMemoryImage));

		++anItr;
	}

	aDumpStream << "Total Image Allocation: " << CommaSeperate(aTotalMemory).c_str() << " bytes<BR>";
	aDumpStream << "<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=4>";

	int aTotalMemorySize = 0;
	int aTotalBitsMemory = 0;
	int aTotalSurfaceMemory = 0;
	int aTotalPalletizedMemory = 0;
	int aTotalNativeAlphaMemory = 0;
	int aTotalRLAlphaMemory = 0;
	int aTotalRLAdditiveMemory = 0;
	int aTotalTextureMemory = 0;

	SortedImageMap::iterator aSortedItr = aSortedImageMap.begin();
	while (aSortedItr != aSortedImageMap.end())
	{
		MemoryImage* aMemoryImage = aSortedItr->second;

		char anImageName[256];
		sprintf(anImageName, "img%04d.png", anImgNum);

		char aThumbName[256];
		sprintf(aThumbName, "thumb%04d.jpg", anImgNum);

		aDumpStream << "<TR>" << std::endl;

		aDumpStream << "<TD><A HREF=" << anImageName << "><IMG SRC=" << aThumbName << " WIDTH=" << aThumbWidth << " HEIGHT=" << aThumbHeight << "></A></TD>" << std::endl;

		int aNumPixels = aMemoryImage->mWidth * aMemoryImage->mHeight;

		DeviceImage* aDDImage = dynamic_cast<DeviceImage*>(aMemoryImage);

		int aMemorySize = aSortedItr->first;

		int aBitsMemory = 0;
		int aSurfaceMemory = 0;
		int aPalletizedMemory = 0;
		int aNativeAlphaMemory = 0;
		int aRLAlphaMemory = 0;
		int aRLAdditiveMemory = 0;
		int aTextureMemory = 0;
		std::string aTextureFormatName;

		if (aMemoryImage->mBits != NULL)
			aBitsMemory = aNumPixels * 4;
		if ((aDDImage != NULL) && (aDDImage->mSurface != NULL))
			aSurfaceMemory = aNumPixels * 4; // Assume 32bit screen...
		if (aMemoryImage->mColorTable != NULL)
			aPalletizedMemory = aNumPixels + 256 * 4;
		if (aMemoryImage->mNativeAlphaData != NULL)
		{
			if (aMemoryImage->mColorTable != NULL)
				aNativeAlphaMemory = 256 * 4;
			else
				aNativeAlphaMemory = aNumPixels * 4;
		}
		if (aMemoryImage->mRLAlphaData != NULL)
			aRLAlphaMemory = aNumPixels;
		if (aMemoryImage->mRLAdditiveData != NULL)
			aRLAdditiveMemory = aNumPixels;
		if (mWindowsGraphicsDriver->mRenderDevice3D != NULL)
		{
			aTextureMemory += mWindowsGraphicsDriver->mRenderDevice3D->GetTextureMemorySize(aMemoryImage);

			switch (mWindowsGraphicsDriver->mRenderDevice3D->GetTextureFormat(aMemoryImage))
			{
			case PixelFormat_A8R8G8B8: aTextureFormatName = "A8R8G8B8"; break;
			case PixelFormat_A4R4G4B4: aTextureFormatName = "A4R4G4B4"; break;
			case PixelFormat_R5G6B5: aTextureFormatName = "R5G6B5"; break;
			case PixelFormat_Palette8: aTextureFormatName = "Palette8"; break;
			}
		}

		aTotalMemorySize += aMemorySize;
		aTotalBitsMemory += aBitsMemory;
		aTotalTextureMemory += aTextureMemory;
		aTotalSurfaceMemory += aSurfaceMemory;
		aTotalPalletizedMemory += aPalletizedMemory;
		aTotalNativeAlphaMemory += aNativeAlphaMemory;
		aTotalRLAlphaMemory += aRLAlphaMemory;
		aTotalRLAdditiveMemory += aRLAdditiveMemory;



		char aStr[256];
		sprintf(aStr, "%d x %d<BR>%s bytes", aMemoryImage->mWidth, aMemoryImage->mHeight, CommaSeperate(aMemorySize).c_str());
		aDumpStream << "<TD ALIGN=RIGHT>" << aStr << "</TD>" << std::endl;

		aDumpStream << "<TD>" << SexyStringToString(((aBitsMemory != 0) ? _S("mBits<BR>") + CommaSeperate(aBitsMemory) : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aPalletizedMemory != 0) ? _S("Palletized<BR>") + CommaSeperate(aPalletizedMemory) : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aSurfaceMemory != 0) ? _S("DDSurface<BR>") + CommaSeperate(aSurfaceMemory) : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aMemoryImage->GetRenderData() != NULL) ? _S("Texture<BR>") + StringToSexyString(aTextureFormatName) + _S("<BR>") + CommaSeperate(aTextureMemory) : _S("&nbsp;"))) << "</TD>" << std::endl;

		aDumpStream << "<TD>" << SexyStringToString(((aMemoryImage->mIsVolatile) ? _S("Volatile") : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aMemoryImage->mForcedMode) ? _S("Forced") : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aMemoryImage->mHasAlpha) ? _S("HasAlpha") : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aMemoryImage->mHasTrans) ? _S("HasTrans") : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aNativeAlphaMemory != 0) ? _S("NativeAlpha<BR>") + CommaSeperate(aNativeAlphaMemory) : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aRLAlphaMemory != 0) ? _S("RLAlpha<BR>") + CommaSeperate(aRLAlphaMemory) : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << SexyStringToString(((aRLAdditiveMemory != 0) ? _S("RLAdditive<BR>") + CommaSeperate(aRLAdditiveMemory) : _S("&nbsp;"))) << "</TD>" << std::endl;
		aDumpStream << "<TD>" << (aMemoryImage->mFilePath.empty() ? "&nbsp;" : aMemoryImage->mFilePath) << "</TD>" << std::endl;

		aDumpStream << "</TR>" << std::endl;

		// Write thumb

		MemoryImage aCopiedImage(*aMemoryImage);

		ulong* aBits = aCopiedImage.GetBits();

		ulong* aThumbBitsPtr = anImageLibImage.mBits;

		for (int aThumbY = 0; aThumbY < aThumbHeight; aThumbY++)
			for (int aThumbX = 0; aThumbX < aThumbWidth; aThumbX++)
			{
				int aSrcX = (int)(aCopiedImage.mWidth * (aThumbX + 0.5)) / aThumbWidth;
				int aSrcY = (int)(aCopiedImage.mHeight * (aThumbY + 0.5)) / aThumbHeight;

				*(aThumbBitsPtr++) = aBits[aSrcX + (aSrcY * aCopiedImage.mWidth)];
			}

		ImageLib::WriteJPEGImage((GetAppDataFolder() + std::string("_dump\\") + aThumbName).c_str(), &anImageLibImage);

		// Write high resolution image

		ImageLib::Image anFullImage;
		anFullImage.mBits = aCopiedImage.GetBits();
		anFullImage.mWidth = aCopiedImage.GetWidth();
		anFullImage.mHeight = aCopiedImage.GetHeight();

		ImageLib::WritePNGImage((GetAppDataFolder() + std::string("_dump\\") + anImageName).c_str(), &anFullImage);

		anFullImage.mBits = NULL;

		anImgNum++;

		aSortedItr++;
	}

	aDumpStream << "<TD>Totals</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalMemorySize)) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalBitsMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalPalletizedMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalSurfaceMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalTextureMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalNativeAlphaMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalRLAlphaMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>" << SexyStringToString(CommaSeperate(aTotalRLAdditiveMemory)) << "</TD>" << std::endl;
	aDumpStream << "<TD>&nbsp;</TD>" << std::endl;

	aDumpStream << "</TABLE></CENTER></BODY></HTML>" << std::endl;
}

double WindowsAppDriver::GetLoadingThreadProgress() //1046-1054
{
	if (mApp->mLoaded || (mApp->mNumLoadingThreadTasks / mApp->mNumLoadingThreadTasks >= 1.0))
		return 1.0;
	if (!mApp->mLoadingThreadStarted || !mApp->mNumLoadingThreadTasks)
		return 0.0;
	else
		return mApp->mCompletedLoadingThreadTasks / mApp->mNumLoadingThreadTasks;
}

bool WindowsAppDriver::ConfigWrite(const std::string& theValueName, ulong theType, const uchar* theValue, ulong theLength) //CHECK? | 1057-1095
{
	if (mApp->mRegKey.length() == 0)
		return false;

	HKEY aGameKey;

	std::string aKeyName = RemoveTrailingSlash("SOFTWARE\\" + mApp->mRegKey);
	std::string aValueName;

	int aSlashPos = (int)theValueName.rfind('\\');
	if (aSlashPos != -1)
	{
		aKeyName += "\\" + theValueName.substr(0, aSlashPos);
		aValueName = theValueName.substr(aSlashPos + 1);
	}
	else
	{
		aValueName = theValueName;
	}

	int aResult = RegOpenKeyExA(HKEY_CURRENT_USER, aKeyName.c_str(), 0, KEY_WRITE, &aGameKey);
	if (aResult != ERROR_SUCCESS)
	{
		ulong aDisp;
		aResult = RegCreateKeyExA(HKEY_CURRENT_USER, aKeyName.c_str(), 0, "Key", REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, NULL, &aGameKey, &aDisp);
	}

	if (aResult != ERROR_SUCCESS)
	{
		return false;
	}

	RegSetValueExA(aGameKey, aValueName.c_str(), 0, theType, theValue, theLength);
	RegCloseKey(aGameKey);

	return true;
}

bool WindowsAppDriver::ConfigWriteString(const std::string& theValueName, const std::string& theString) //1098-1100
{
	return ConfigWrite(theValueName, REG_SZ, (uchar*)StringToSexyStringFast(theString).c_str(), 2 * theString.length() + 2);
}

bool WindowsAppDriver::ConfigWriteString(const std::string& theValueName, const std::wstring& theString) //is SexyString? | 1103-1105
{
	return ConfigWrite(theValueName, REG_SZ, (uchar*)theString.c_str(), theString.length());
}

bool WindowsAppDriver::ConfigWriteInteger(const std::string& theValueName, int theValue) //1108-1110
{
	return ConfigWrite(theValueName, REG_DWORD, (uchar*)&theValue, sizeof(int));
}

bool WindowsAppDriver::ConfigWriteBoolean(const std::string& theValueName, bool theValue) //1113-1116
{
	int aValue = theValue ? 1 : 0;
	return ConfigWrite(theValueName, REG_DWORD, (uchar*)&aValue, sizeof(int));
}

bool WindowsAppDriver::ConfigWriteData(const std::string& theValueName, const uchar* theValue, ulong theLength) //1119-1121
{
	return ConfigWrite(theValueName, REG_BINARY, (uchar*)theValue, theLength);
}

bool WindowsAppDriver::ConfigEraseKey(const SexyString& _theKeyName) //1125-1139
{
	std::string theKeyName = SexyStringToStringFast(_theKeyName);
	if (mApp->mRegKey.length() == 0)
		return false;

	std::string aKeyName = RemoveTrailingSlash("SOFTWARE\\" + mApp->mRegKey) + "\\" + theKeyName;

	int aResult = RegDeleteKeyA(HKEY_CURRENT_USER, aKeyName.c_str());
	if (aResult != ERROR_SUCCESS)
	{
		return false;
	}
	return true;
}

void WindowsAppDriver::ConfigEraseValue(const SexyString& _theValueName) //1142-1168
{
	std::string theValueName = SexyStringToStringFast(_theValueName);
	if (mApp->mRegKey.length() == 0)
		return;

	HKEY aGameKey;
	std::string aKeyName = RemoveTrailingSlash("SOFTWARE\\" + mApp->mRegKey);
	std::string aValueName;

	int aSlashPos = (int)theValueName.rfind('\\');
	if (aSlashPos != -1)
	{
		aKeyName += "\\" + theValueName.substr(0, aSlashPos);
		aValueName = theValueName.substr(aSlashPos + 1);
	}
	else
	{
		aValueName = theValueName;
	}

	int aResult = RegOpenKeyExA(HKEY_CURRENT_USER, aKeyName.c_str(), 0, KEY_WRITE, &aGameKey);
	if (aResult == ERROR_SUCCESS)
	{
		RegDeleteValueA(aGameKey, aValueName.c_str());
		RegCloseKey(aGameKey);
	}
}

bool WindowsAppDriver::ConfigGetSubKeys(const std::string& theKeyName, StringVector* theSubKeys) //1171-1207
{
	theSubKeys->clear();

	if (mApp->mRegKey.length() == 0)
		return false;

	HKEY aKey;

	std::string aKeyName = RemoveTrailingSlash(RemoveTrailingSlash("SOFTWARE\\" + mApp->mRegKey) + "\\" + theKeyName);
	int aResult = RegOpenKeyExA(HKEY_CURRENT_USER, aKeyName.c_str(), 0, KEY_READ, &aKey);

	if (aResult == ERROR_SUCCESS)
	{
		for (int anIdx = 0; ; anIdx++)
		{
			char aStr[1024];

			aResult = RegEnumKeyA(aKey, anIdx, aStr, 1024);
			if (aResult != ERROR_SUCCESS)
				break;

			theSubKeys->push_back(aStr);
		}

		RegCloseKey(aKey);
		return true;
	}
	else
	{
		return false;
	}
}

bool WindowsAppDriver::ConfigRead(const std::string& theValueName, ulong* theType, uchar* theValue, ulong* theLength) //1210-1212
{
	return RegistryReadKey(theValueName, theType, theValue, theLength, HKEY_CURRENT_USER);
}

bool WindowsAppDriver::RegistryReadKey(const std::string& theValueName, ulong* theType, uchar* theValue, ulong* theLength, HKEY theKey) //1215-1253
{
	if (mApp->mRegKey.length() == 0)
		return false;

	HKEY aGameKey;

	std::string aKeyName = RemoveTrailingSlash("SOFTWARE\\" + mApp->mRegKey);
	std::string aValueName;

	int aSlashPos = (int)theValueName.rfind('\\');
	if (aSlashPos != -1)
	{
		aKeyName += "\\" + theValueName.substr(0, aSlashPos);
		aValueName = theValueName.substr(aSlashPos + 1);
	}
	else
	{
		aValueName = theValueName;
	}

	if (RegOpenKeyExA(theKey, aKeyName.c_str(), 0, KEY_READ, &aGameKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueExA(aGameKey, aValueName.c_str(), 0, theType, (uchar*)theValue, theLength) == ERROR_SUCCESS)
		{
			RegCloseKey(aGameKey);
			return true;
		}

		RegCloseKey(aGameKey);
	}
	return false;
}

bool WindowsAppDriver::ConfigReadString(const std::string& theKey, std::string* theString) //Correct? | 1256-1284
{
	wchar_t aStr[1024];
	wchar_t* aStrP = aStr;

	ulong aType;
	ulong aLen = sizeof(aStr) - 2;
	if (!ConfigRead(theKey, &aType, (uchar*)aStr, &aLen))
		return false;

	if (aType != REG_SZ)
		return false;

	aStrP = new wchar_t[aLen >> 1];

	*theString = ToString(aStrP);
	if (aStrP != aStr)
		delete aStrP;

	return true;
}

bool WindowsAppDriver::ConfigReadString(const std::string& theKey, std::wstring* theString) //Widestring? Correct? | 1287-1315
{
	wchar_t aStr[1024];
	wchar_t* aStrP = aStr;

	ulong aType;
	ulong aLen = sizeof(aStr) - 2;
	if (!ConfigRead(theKey, &aType, (uchar*)aStr, &aLen))
		return false;

	if (aType != REG_SZ)
		return false;

	aStrP = new wchar_t[aLen >> 1];

	*theString = aStrP;
	if (aStrP != aStr)
		delete aStrP;

	return true;
}

bool WindowsAppDriver::ConfigReadInteger(const std::string& theKey, int* theValue) //1318-1330
{
	ulong aType;
	ulong aLong;
	ulong aLen = 4;
	if (!ConfigRead(theKey, &aType, (uchar*)&aLong, &aLen))
		return false;

	if (aType != REG_DWORD)
		return false;

	*theValue = aLong;
	return true;
}

bool WindowsAppDriver::ConfigReadBoolean(const std::string& theKey, bool* theValue) //1333-1340
{
	int aValue;
	if (!ConfigReadInteger(theKey, &aValue))
		return false;

	*theValue = aValue != 0;
	return true;
}

bool WindowsAppDriver::ConfigReadData(const std::string& theKey, uchar* theValue, ulong* theLength) //1343-1353
{
	ulong aType;
	ulong aLen = *theLength;
	if (!ConfigRead(theKey, &aType, (uchar*)theValue, theLength))
		return false;

	if (aType != REG_BINARY)
		return false;

	return true;
}


bool WindowsAppDriver::WriteBytesToFile(const std::string& theFileName, const void* theData, unsigned long theDataLen) //1357-1383
{
	MkDir(GetFileDir(theFileName));
	FILE* aFP = fopen(theFileName.c_str(), "w+b");

	if (aFP == NULL)
		return false;

	fwrite(theData, 1, theDataLen, aFP);
	fclose(aFP);

	if (mApp->mSexyCacheBuffers && mApp->mWriteToSexyCache)
	{
		void* thePtr = gSexyCache.AllocSetData(GetFullPath(theFileName), "Buffer", theDataLen + 1);
		if (thePtr)
		{
			*(uchar*)thePtr = 1;
			memcpy((char*)thePtr + 1, theData, theDataLen);
			gSexyCache.SetData(thePtr);
			gSexyCache.FreeSetData(thePtr);
			gSexyCache.SetFileDeps(GetFullPath(theFileName), "Buffer", GetFullPath(theFileName));
		}
	}

	return true;
}

bool WindowsAppDriver::WriteBufferToFile(const std::string& theFileName, const Buffer* theBuffer) //1386-1388
{
	return WriteBytesToFile(theFileName, theBuffer->GetDataPtr(), theBuffer->GetDataLen());
}

bool WindowsAppDriver::ReadBufferFromFile(const std::string& theFileName, Buffer* theBuffer, bool dontWriteToDemo) //TODO! | 1392-1452
{
	/*void* thePtr;
	int theSize;
	if (mApp->mSexyCacheBuffers)
	{
		if (!gSexyCache.GetData(GetFullPath(theFileName), "Buffer", &thePtr, &theSize)) //?
			return false;
	}

	if (thePtr)
	{
		theBuffer->Clear();
		theBuffer->SetData((uchar*)thePtr + 1, theSize - 1);
		gSexyCache.FreeGetData(thePtr);
		return true;
	}
	else
	{
		return false;
	}

	PFILE* aFP = p_fopen(theFileName.c_str(), "rb");

	if (aFP == NULL)
		return false;

	p_fseek(aFP, 0, SEEK_END);
	int aFileSize = p_ftell(aFP);
	p_fseek(aFP, 0, SEEK_SET);

	uchar* aData = new uchar[aFileSize];

	p_fread(aData, 1, aFileSize, aFP);
	p_fclose(aFP);

	theBuffer->Clear();
	theBuffer->SetData(aData, aFileSize);

	if (mApp->mSexyCacheBuffers && mApp->mWriteToSexyCache)
	{
		thePtr = gSexyCache.AllocSetData(GetFullPath(theFileName), "Buffer", aFileSize + 1);
		if (thePtr)
		{
			*(uchar*)thePtr = 1;
			memcpy((char*)thePtr + 1, aData, aFileSize);
			gSexyCache.SetData(thePtr);
			gSexyCache.FreeSetData(thePtr);
			gSexyCache.SetFileDeps(GetFullPath(theFileName), "Buffer", GetFullPath(theFileName));
		}
	}

	delete aData;

	return true;

	if (mApp->mSexyCacheBuffers && mApp->mWriteToSexyCache)
	{
		thePtr = gSexyCache.AllocSetData(GetFullPath(theFileName), "Buffer", aFileSize + 1);
		if (thePtr)
		{
			*(uchar*)thePtr = 1;
			gSexyCache.SetData(thePtr);
			gSexyCache.FreeSetData(thePtr);
			gSexyCache.SetFileDeps(GetFullPath(theFileName), "Buffer", GetFullPath(theFileName));
		}
	}

	return false;*/
}

void WindowsAppDriver::SEHOccured() //1456-1461
{
	mApp->SetMusicVolume(0);
	::ShowWindow(mApp->mHWnd, SW_HIDE);
	mApp->mSEHOccured = true;
	EnforceCursor();
}

std::string WindowsAppDriver::GetGameSEHInfo() //1464-1491
{
	int aSecLoaded = (GetTickCount() - mApp->mTimeLoaded) / 1000;

	char aTimeStr[16];
	sprintf(aTimeStr, "%02d:%02d:%02d", (aSecLoaded / 60 / 60), (aSecLoaded / 60) % 60, aSecLoaded % 60);

	char aThreadIdStr[16];
	sprintf(aThreadIdStr, "%X", mApp->mPrimaryThreadId);

	std::string anInfoString =
		"Product: " + mApp->mProdName + "\r\n" +
		"Version: " + mApp->mProductVersion + "\r\n";

	anInfoString +=
		"Time Loaded: " + std::string(aTimeStr) + "\r\n"
		"Fullscreen: " + (mApp->mIsWindowed ? std::string("No") : std::string("Yes")) + "\r\n"
		"Primary ThreadId: " + aThreadIdStr + "\r\n"
		"Resolution: " + StrFormat("%dx%d\r\n", mApp->mWidth, mApp->mHeight) + "\r\n"
		"3D Mode: " + (mApp->Is3DAccelerated() ? mWindowsGraphicsDriver->mIsD3D8Or9 ? mWindowsGraphicsDriver->mIsD3D9 ? "DX9" : "DX8" : "DX7" : "OFF") + "\r\n";
	if (mWindowsGraphicsDriver && mWindowsGraphicsDriver->mRenderDevice3D)
	{
		anInfoString +=
			"D3D Dll Version: : " + mWindowsGraphicsDriver->mRenderDevice3D->GetInfoString(RenderDevice3D::INFOSTRING_DrvProductVersion) + "\r\n"
			"Adapter: " + mWindowsGraphicsDriver->mRenderDevice3D->GetInfoString(RenderDevice3D::INFOSTRING_Adapter) + "\r\n";
	}

	return anInfoString;
}

void WindowsAppDriver::GetSEHWebParams(DefinesMap* theDefinesMap) //1494-1495
{
}

void WindowsAppDriver::Shutdown() //1498-1533
{
	if ((mApp->mPrimaryThreadId != 0) && (GetCurrentThreadId() != mApp->mPrimaryThreadId))
	{
		mApp->mLoadingFailed = true;
	}
	else if (!mApp->mShutdown)
	{
		mApp->mExitToTop = true;
		mApp->mShutdown = true;
		mApp->ShutdownHook();

		// Blah
		while (mApp->mCursorThreadRunning)
		{
			Sleep(10);
		}

		if (mApp->mMusicInterface != NULL)
			mApp->mMusicInterface->StopAllMusic();

		if ((!mApp->mIsPhysWindowed) && (mWindowsGraphicsDriver != NULL) && (mWindowsGraphicsDriver->mDD != NULL))
		{
			mWindowsGraphicsDriver->mDD->RestoreDisplayMode();
		}

		if (mApp->mHWnd != NULL)
		{
			ShowWindow(mApp->mHWnd, SW_HIDE);
		}

		RestoreScreenResolution();

		if (mApp->mReadFromRegistry)
			mApp->WriteToRegistry();
	}
}

void WindowsAppDriver::RestoreScreenResolution() //1536-1544
{
	if (mApp->mFullScreenWindow)
	{
		EnumWindows(ChangeDisplayWindowEnumProc, 0); // get any windows that appeared while we were running
		ChangeDisplaySettings(NULL, 0);
		EnumWindows(ChangeDisplayWindowEnumProc, 1); // restore window pos
		mApp->mFullScreenWindow = false;
	}
}

void WindowsAppDriver::DoExit(int theCode) //1547-1550
{
	RestoreScreenResolution();
	exit(theCode);
}

void WindowsAppDriver::DoUpdateFramesF(float theFrac) //1554-1557
{
	if ((mApp->mVSyncUpdates) && (!mApp->mMinimized))
		mApp->mWidgetManager->UpdateFrameF(theFrac);
}

bool WindowsAppDriver::DoUpdateFrames() //1560-1578
{
	SexyAutoPerf anAutoPerfUNIQUE = "SexyAppBase::DoUpdateFrames";
	SEXY_AUTO_PERF(anAutoPerfUNIQUE);

	if (gScreenSaverActive)
		return false;

	else
	{
		if ((mApp->mLoadingThreadCompleted) && (!mApp->mLoaded))
		{
			::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_NORMAL);
			mApp->mLoaded = true;
			mApp->mYieldMainThread = false;
			mApp->LoadingThreadCompleted();

		}

		mApp->UpdateFrames();
		return true;
	}
}

bool gIsFailing = false;

void WindowsAppDriver::Redraw(Rect* theClipRect) //TODO | 1583-2102
{
	/*SEXY_AUTO_PERF("SexyAppBase::Redraw");
	int widgetX;
	int widgetY;

	// Do mIsDrawing check because we could enter here at a bad time if any windows messages
	//  are processed during WidgetManager->Draw
	if (mApp->mIsDrawing || mApp->mShutdown || mApp->mMinimized)
		return;

	if (gScreenSaverActive)
		return;

	if (mApp->mShowWidgetInspector && mApp->mWidgetManager)
	{
		int cursorX = mWindowsGraphicsDriver->mCursorX;
		int cursorY = mWindowsGraphicsDriver->mCursorY;
		mApp->mWidgetManager->RemapMouse(cursorX, cursorY);
		if (mApp->mWidgetInspectorPickWidget ? mApp->mWidgetInspectorPickWidget : mApp->mWidgetManager->GetWidgetAt(cursorX, cursorY, &widgetX, &widgetY))
		{
			cursorX -= mApp->mScreenBounds.mX;
			if (mApp->mWidgetInspectorCurWidget != mApp->mWidgetInspectorPickWidget ? mApp->mWidgetInspectorPickWidget : mApp->mWidgetManager->GetWidgetAt(cursorX, cursorY, &widgetX, &widgetY))
			{
				mApp->mWidgetInspectorCurWidget = mApp->mWidgetInspectorPickWidget ? mApp->mWidgetInspectorPickWidget : mApp->mWidgetManager->GetWidgetAt(cursorX, cursorY, &widgetX, &widgetY);
				mApp->mWidgetInspectorScrollOffset = 0;
			}
			Graphics g = mWindowsGraphicsDriver->GetScreenImage();
			g.SetDrawMode(DRAWMODE_NORMAL);
			g.SetColor(0xFF00FF00);
			mApp->mWidgetInspectorCurWidget->GetAbsPos();
			if (mApp->mWidgetInspectorCurWidget->mWidth - 1 <= 0)
			{
				? = 0;
			}
			else
			{
				? = mApp->mWidgetInspectorCurWidget->mWidth - 1;
			}
			theWidth = ? ;
			if (mApp->mWidgetInspectorCurWidget->mHeight - 1 <= 0)
			{
				? = 0;
			}
			else
			{
				? = mApp->mWidgetInspectorCurWidget->mHeight - 1;
			}
			theHeight = ? ;
			g.DrawRect( [0] - mApp->mScreenBounds.mX, ? [1], theWidth, ? );
		}
	}*/

	//DBG_ASSERTE(aWidgetType && (aWidgetType->GetTypeCategory() == Reflection::RType::TC_Named_Class)); // 1658
	//DBG_ASSERTE(aWidgetType && (aWidgetType->GetTypeCategory() == Reflection::RType::TC_Named_Class)); // 1679
	/*SEXY_AUTO_PERF("anAutoPerfUNIQUE"); //Lol

	// Do mIsDrawing check because we could enter here at a bad time if any windows messages
	//  are processed during WidgetManager->Draw
	if (mApp->mIsDrawing || mApp->mShutdown || mApp->mMinimized || gScreenSaverActive)
		return;

	static DWORD aRetryTick = 0;
	if (!mWindowsGraphicsDriver->Redraw(theClipRect))
	{
		extern bool gD3DInterfacePreDrawError;
		gD3DInterfacePreDrawError = false; // this predraw error happens naturally when ddraw is failing
		if (!gIsFailing)
		{
			//gDebugStream << GetTickCount() << " Redraw failed!" << std::endl;
			gIsFailing = true;
		}

		WINDOWPLACEMENT aWindowPlacement;
		ZeroMemory(&aWindowPlacement, sizeof(aWindowPlacement));
		aWindowPlacement.length = sizeof(aWindowPlacement);
		::GetWindowPlacement(mHWnd, &aWindowPlacement);

		DWORD aTick = GetTickCount();
		if ((mActive || (aTick-aRetryTick>1000 && mIsPhysWindowed)) && (aWindowPlacement.showCmd != SW_SHOWMINIMIZED) && (!mMinimized))
		{
			aRetryTick = aTick;

			mWidgetManager->mImage = NULL;

			// Re-check resolution at this point, because we hit here when you change your resolution.
			if (((mWidth >= GetSystemMetrics(SM_CXFULLSCREEN)) || (mHeight >= GetSystemMetrics(SM_CYFULLSCREEN))) && (mIsWindowed))
			{
				if (mForceWindowed)
				{
					Popup(mApp->GetString("PLEASE_SET_COLOR_DEPTH", _S("Please set your desktop color depth to 16 bit.")));
					Shutdown();
					return;
				}
				mForceFullscreen = true;

				SwitchScreenMode(false);
				return;
			}


			int aResult = InitDDInterface();

			//gDebugStream << GetTickCount() << " ReInit..." << std::endl;

			if ((mIsWindowed) && (aResult == DDInterface::RESULT_INVALID_COLORDEPTH))
			{
				//gDebugStream << GetTickCount() << "ReInit Invalid Colordepth" << std::endl;
				if (!mActive) // don't switch to full screen if not active app
					return;

				SwitchScreenMode(false);
				mForceFullscreen = true;
				return;
			}
			else if (aResult == DDInterface::RESULT_3D_FAIL)
			{
				Set3DAcclerated(false);
				return;
			}
			else if (aResult != DDInterface::RESULT_OK)
			{
				//gDebugStream << GetTickCount() << " ReInit Failed" << std::endl;
				//Fail("Failed to initialize DirectDraw");
				//Sleep(1000);

				return;
			}

			mApp->ReInitImages();

			mApp->mWidgetManager->mImage = mAppDriver->GetScreenImage();
			mApp->mWidgetManager->MarkAllDirty();

			mLastTime = timeGetTime();
		}
	}
	else
	{
		if (gIsFailing)
		{
			//gDebugStream << GetTickCount() << " Redraw succeeded" << std::endl;
			gIsFailing = false;
			aRetryTick = 0;
		}
	}

	mApp->mFPSFlipCount++;*/
}

///////////////////////////// FPS Stuff
static PerfTimer gFPSTimer; //2105
static int gFrameCount;
static int gFPSDisplay;
static double gFPSDisplayRecipMS;
static bool gForceDisplay = false;

HistoryGraph::HistoryGraph() //2133-2136
{
	NextFrame();
}

void HistoryGraph::NextFrame() //2139-2143
{
	if (mFrames.size() >= 1000)
	mFrames.push_back(Frame());
}

void HistoryGraph::AddItem(double inTimeMS, ulong inColor) //2145-2151
{
	Frame* frame = &mFrames.back();
	memset(0, 0, 0); //?
	//frame->mItems.push_back();
	Frame::FrameItem* item = &frame->mItems.back();
	item->mTimeMS = inTimeMS;
	item->mColor = inColor;
}

HistoryGraph gHistoryGraph; //2156

static void CalculateFPS(bool showHistory) //2158-2279
{
	gFrameCount++;
	if (gSexyAppBase->mVFPSUpdateCount >= 75 && gSexyAppBase->mVFPSDrawCount < 5)
	{
		gSexyAppBase->mCurVFPS = 0.0;
		gSexyAppBase->mVFPSDrawTimes = 0.0;
		gSexyAppBase->mVFPSDrawCount = 0;
		gSexyAppBase->mVFPSUpdateTimes = 0.0;
		gSexyAppBase->mVFPSUpdateCount = 0;
		gForceDisplay = 1;
	}
	if (gSexyAppBase->mVFPSUpdateCount >= 50 && gSexyAppBase->mVFPSDrawCount >= 5 || gSexyAppBase->mStepMode && gSexyAppBase->mVFPSUpdateCount >= 1 && gSexyAppBase->mVFPSDrawCount >= 1)
	{
		float anAvgUpdateTime = gSexyAppBase->mVFPSUpdateTimes / gSexyAppBase->mVFPSUpdateCount;
		float anAvgDrawTime = gSexyAppBase->mVFPSDrawCount / gSexyAppBase->mVFPSDrawCount;
		gSexyAppBase->mCurVFPS = 1000.0 / (anAvgUpdateTime + anAvgDrawTime);
		if (gSexyAppBase->mCurVFPS > 999.9000244140625)
			gSexyAppBase->mCurVFPS = 999.90002;
		gSexyAppBase->mVFPSDrawTimes = 0.0;
		gSexyAppBase->mVFPSDrawCount = 0;
		gSexyAppBase->mVFPSUpdateTimes = 0.0;
		gSexyAppBase->mVFPSUpdateCount = 0;
		gForceDisplay = true;
	}

	static SysFont aFont(gSexyAppBase, "Tahoma", 8);
	if (gFPSImage == NULL)
	{
		gFPSImage = new DeviceImage(gSexyAppBase);
		gFPSImage->Create(74, 2 * aFont.GetHeight() + 2);
		gFPSImage->SetImageMode(false, false);
		gFPSImage->SetVolatile(true);
		gFPSImage->mPurgeBits = false;
		gFPSImage->mWantDeviceSurface = true;
		gFPSImage->PurgeBits();
	}

	bool ready = (showHistory ? 100 : 1000) <= gFPSTimer.GetDuration();

	if (ready || gForceDisplay || showHistory)
	{
		if (ready)
		{
			double aDuration = gFPSTimer.GetDuration();
			gFPSTimer.Stop();
			gFPSDisplay = (int)(gFrameCount * 1000 / gFPSTimer.GetDuration() + 0.5f);
			gFPSDisplayRecipMS = aDuration / gFrameCount;
			gFPSTimer.Start();
		}

		gForceDisplay = false;
		if (showHistory)
		{
			gHistoryGraph.NextFrame();
			if (gFPSDisplayRecipMS >= 99)
				gFPSDisplayRecipMS = 99;
			gHistoryGraph.AddItem(gFPSDisplayRecipMS, gSexyAppBase->HSLToRGB(100 - gFPSDisplayRecipMS, 255, 128));
			if (gFPSHistoryImage == NULL)
			{
				gFPSHistoryImage = new DeviceImage(gSexyAppBase);
				gFPSHistoryImage->Create(1000, 200);
				gFPSHistoryImage->SetImageMode(false, false);
				gFPSHistoryImage->SetVolatile(true);
				gFPSHistoryImage->mPurgeBits = false;
				gFPSHistoryImage->mWantDeviceSurface = true;
				gFPSHistoryImage->PurgeBits();
			}
			Graphics aDrawG(gFPSHistoryImage);
			aDrawG.SetColor(0x000000);
			aDrawG.FillRect(0, 0, gFPSImage->GetWidth(), gFPSImage->GetHeight());
			int historyCount = gHistoryGraph.mFrames.size();
			for (int i = 0; i < historyCount; ++i)
			{
				HistoryGraph::Frame* frame = &gHistoryGraph.mFrames[i];
				ulong vStart = 0;
				int itemCount = frame->mItems.size();
				for (int j = 0; j < itemCount; ++j)
				{
					HistoryGraph::Frame::FrameItem* item = &frame->mItems[j];
					int v = item->mTimeMS;
					int vEnd = v + vStart;
					aDrawG.SetColor(Color(item->mColor));
					aDrawG.DrawLine(i, 2 * (100 - vStart), i, 2 * (100 - vEnd));
					vStart = vEnd;
				}
			}
			++gFPSHistoryImage->mBitsChangedCount;
		}
	}
	if (gFPSTimer.GetDuration() >= 1000 || gForceDisplay)
	{
		Graphics aDrawG(gFPSImage);
		aDrawG.SetFont(&aFont);
		SexyString aFPS = StrFormat(_S("ActFPS: %d"), gFPSDisplay);
		SexyString aVFPS = StrFormat(_S("VirtFPS: %1.f"), gSexyAppBase->mCurVFPS);
		if (gSexyAppBase->mCurVFPS == 0.0)
			aVFPS = _S("VirtFPS: -");
		aDrawG.SetColor(0x000000);
		aDrawG.FillRect(0, 0, gFPSImage->GetWidth(), gFPSImage->GetHeight());
		aDrawG.SetColor(0xFFFFFF);
		aDrawG.DrawString(aFPS, 3, aFont.GetAscent() + 1);
		aDrawG.DrawString(aFPS, 3, aFont.GetHeight() + aFont.GetAscent());
		gFPSImage->mBitsChangedCount++;
	}
}

///////////////////////////// FPS Stuff to draw mouse coords
static void FPSDrawCoords(int theX, int theY) //2283-2304
{
	static SysFont aFont(gSexyAppBase, "Tahoma", 8);
	if (gFPSImage == NULL)
	{
		gFPSImage = new DeviceImage(gSexyAppBase);
		gFPSImage->Create(74, 2 * aFont.GetHeight() + 2);
		gFPSImage->SetImageMode(false, false);
		gFPSImage->SetVolatile(true);
		gFPSImage->mPurgeBits = false;
		gFPSImage->mWantDeviceSurface = true;
		gFPSImage->PurgeBits();
	}

	Graphics aDrawG(gFPSImage);
	aDrawG.SetFont(&aFont);
	SexyString aFPS = StrFormat(_S("%d,%d"), theX, theY);
	aDrawG.SetColor(0x000000);
	aDrawG.FillRect(0, 0, gFPSImage->GetWidth(), gFPSImage->GetHeight());
	aDrawG.SetColor(0xFFFFFF);
	aDrawG.DrawString(aFPS, 2, aFont.GetAscent());
	gFPSImage->mBitsChangedCount++;
}

static void UpdateScreenSaverInfo(DWORD theTick) //2308-2371
{
	if (gSexyAppBase->IsScreenSaver() || !gSexyAppBase->mIsPhysWindowed)
		return;

	// Get screen saver timeout		
	static DWORD aPeriodicTick = 0;
	static DWORD aScreenSaverTimeout = 60000;
	static BOOL aScreenSaverEnabled = TRUE;

	if (theTick - aPeriodicTick > 10000)
	{
		aPeriodicTick = theTick;

		int aTimeout = 0;

		SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &aTimeout, 0);
		SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &aScreenSaverEnabled, 0);
		aTimeout -= 2;

		if (aTimeout < 1)
			aTimeout = 1;

		aScreenSaverTimeout = aTimeout * 1000;

		if (!aScreenSaverEnabled)
			gScreenSaverActive = false;
	}

	// Get more accurate last user input time
	if (gGetLastInputInfoFunc)
	{
		LASTINPUTINFO anInfo;
		anInfo.cbSize = sizeof(anInfo);
		if (gGetLastInputInfoFunc(&anInfo))
		{
			if (anInfo.dwTime > theTick)
				anInfo.dwTime = theTick;

			gSexyAppBase->mLastUserInputTick = anInfo.dwTime;
		}
	}

	if (!aScreenSaverEnabled)
		return;

	DWORD anIdleTime = theTick - gSexyAppBase->mLastUserInputTick;
	if (gScreenSaverActive)
	{
		BOOL aBool = FALSE;
		if (SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &aBool, 0))
		{
			if (aBool) // screen saver not off yet
				return;
		}

		if (anIdleTime < aScreenSaverTimeout)
		{
			gScreenSaverActive = false;
			gSexyAppBase->mWidgetManager->MarkAllDirty();
		}
	}
	else if (anIdleTime > aScreenSaverTimeout)
		gScreenSaverActive = true;
}

bool WindowsAppDriver::DrawDirtyStuff() //2374-2575
{
	SEXY_AUTO_PERF("SexyAppBase::DrawDirtyStuff");
	MTAutoDisallowRand aDisallowRand;

	if (gIsFailing) // just try to reinit
	{
		Redraw(NULL);
		mApp->mHasPendingDraw = false;
		mApp->mLastDrawWasEmpty = true;
		return false;
	}

	if (SexyPerf::IsPerfOn()) //Makes sense
	{
		SexyPerf::ClearFrameInfo();
	}

	if (mApp->mShowFPS)
	{
		switch (mApp->mShowFPSMode)
		{
		case FPS_ShowFPS: CalculateFPS(false); break;
		case FPS_ShowFPSWithHistory: CalculateFPS(true);
		case FPS_ShowCoords:
			if (mApp->mWidgetManager != NULL)
				FPSDrawCoords(mApp->mWidgetManager->mLastMouseX, mApp->mWidgetManager->mLastMouseY);
			break;
		}

	}

	DWORD aStartTime = timeGetTime();

	// Update user input and screen saver info
	static DWORD aPeriodicTick = 0;
	if (aStartTime - aPeriodicTick > 1000)
	{
		aPeriodicTick = aStartTime;
		UpdateScreenSaverInfo(aStartTime);
	}

	if (gScreenSaverActive)
	{
		mApp->mHasPendingDraw = false;
		mApp->mLastDrawWasEmpty = true;
		return false;
	}

	PerfTimer aTimer; //?

	if (mApp->mShowFPS && mApp->mShowFPSMode <= FPS_ShowFPSWithHistory)
		aTimer.Start();

	mApp->mIsDrawing = true;
	bool drewScreen = mApp->mWidgetManager->DrawScreen();
	mApp->mIsDrawing = false;

	if (mApp->mShowFPS && mApp->mShowFPSMode <= FPS_ShowFPSWithHistory && drewScreen)
	{
		if (mWindowsGraphicsDriver->mRenderDevice3D)
		{
			mWindowsGraphicsDriver->mRenderDevice3D->Flush(RenderDevice3D::FLUSHF_BufferedTris);
			mApp->mVFPSDrawCount++;
			mApp = this->mApp; //?
			mApp->mVFPSDrawTimes += aTimer.GetDuration();
		}
	}

	if ((drewScreen || (aStartTime - mApp->mLastDrawTick >= 1000) || (mApp->mCustomCursorDirty)) &&
		((int)(aStartTime - mApp->mNextDrawTick) >= 0))
	{
		mApp->mLastDrawWasEmpty = false;

		mApp->mDrawCount++;

		DWORD aMidTime = timeGetTime();

		mApp->mFPSCount++;
		mApp->mFPSTime += aMidTime - aStartTime;

		mApp->mDrawTime += aMidTime - aStartTime;

		static SysFont aSysFont(mApp, "Tahoma", 12);

		if (mApp->mShowFPS || mApp->mShowFPSMode <= FPS_ShowFPSWithHistory && SexyPerf::IsPerfRecording())
		{
			Graphics g(mWindowsGraphicsDriver->GetScreenImage());
			bool aWasHalfTris = (mWindowsGraphicsDriver->mRenderModeFlags & WindowsGraphicsDriver::RENDERMODEF_HalfTris) != 0;
			bool aWasNoBatching = (mWindowsGraphicsDriver->mRenderModeFlags & WindowsGraphicsDriver::RENDERMODEF_NoBatching) != 0;
			if (aWasHalfTris)
			{
				if (mWindowsGraphicsDriver->mRenderDevice3D)
					mWindowsGraphicsDriver->mRenderDevice3D->Flush(WindowsGraphicsDriver::RENDERMODEF_NoBatching);
				mWindowsGraphicsDriver->mRenderModeFlags &= ~WindowsGraphicsDriver::RENDERMODEF_HalfTris;
				mWindowsGraphicsDriver->mRenderModeFlags |= WindowsGraphicsDriver::RENDERMODEF_NoBatching;
			}
			if (mApp->mShowFPS)
			{
				g.DrawImage(gFPSImage, mApp->mScreenBounds.mWidth - gFPSImage->GetWidth() - 6, mApp->mScreenBounds.mHeight - gFPSImage->GetHeight() - 6);
				if (gFPSHistoryImage)
				{
					if (mApp->mShowFPSMode == FPS_ShowFPSWithHistory)
					{
						g.PushState();
						g.SetColorizeImages(true);
						g.SetColor(0xFFFFFFFF); //Has 128?
						g.DrawImage(gFPSHistoryImage, mApp->mScreenBounds.mWidth - gFPSHistoryImage->GetWidth() - 6, mApp->mScreenBounds.mHeight - gFPSHistoryImage->GetHeight() - 16);
						g.PopState();
					}
				}
			}
			if (SexyPerf::IsPerfOn() && SexyPerf::IsPerfRecording() && mApp->mUpdateCount & 64 != 0)
			{
				g.PushState();
				Font* aFont = &aSysFont;
				g.SetFont(&aSysFont);
				g.SetColor(0xFFFF0000);
				g.DrawString(_S("PERF RECORD"), 10, mApp->mScreenBounds.mHeight - 15);
				g.PopState();
			}

			if (aWasHalfTris)
			{
				if (mWindowsGraphicsDriver->mRenderDevice3D)
				{
					mWindowsGraphicsDriver->mRenderDevice3D->Flush(WindowsGraphicsDriver::RENDERMODEF_NoBatching);
					mWindowsGraphicsDriver->mRenderModeFlags |= WindowsGraphicsDriver::RENDERMODEF_HalfTris;
					if (!aWasNoBatching) //Correct?
					{
						mWindowsGraphicsDriver->mRenderModeFlags &= ~WindowsGraphicsDriver::RENDERMODEF_NoBatching;
					}
				}
			}
		}

		if (mApp->mWaitForVSync && mApp->mIsPhysWindowed && mApp->mSoftVSyncWait)
		{
			DWORD aTick = timeGetTime();
			if (aTick - mApp->mLastDrawTick < mWindowsGraphicsDriver->mMillisecondsPerFrame)
				Sleep(mWindowsGraphicsDriver->mMillisecondsPerFrame - (aTick - mApp->mLastDrawTick));
		}

		DWORD aPreScreenBltTime = timeGetTime();
		mApp->mLastDrawTick = aPreScreenBltTime;

		Redraw(NULL);

		// This is our one UpdateFTimeAcc if we are vsynched
		UpdateFTimeAcc();

		DWORD aEndTime = timeGetTime();

		mApp->mScreenBltTime = aEndTime - aPreScreenBltTime;

#ifdef _DEBUG
		/*if (mFPSTime >= 5000) // Show FPS about every 5 seconds
		{
			ulong aTickNow = GetTickCount();

			OutputDebugString(StrFormat(_S("Theoretical FPS: %d\r\n"), (int) (mFPSCount * 1000 / mFPSTime)).c_str());
			OutputDebugString(StrFormat(_S("Actual      FPS: %d\r\n"), (mFPSFlipCount * 1000) / max((aTickNow - mFPSStartTick), 1)).c_str());
			OutputDebugString(StrFormat(_S("Dirty Rate     : %d\r\n"), (mFPSDirtyCount * 1000) / max((aTickNow - mFPSStartTick), 1)).c_str());

			mFPSTime = 0;
			mFPSCount = 0;
			mFPSFlipCount = 0;
			mFPSStartTick = aTickNow;
			mFPSDirtyCount = 0;
		}*/
#endif

		if ((mApp->mLoadingThreadStarted) && (!mApp->mLoadingThreadCompleted))
		{
			int aTotalTime = aEndTime - aStartTime;

			mApp->mNextDrawTick += 35 + max(aTotalTime, 15);

			if ((int)(aEndTime - mApp->mNextDrawTick) >= 0)
				mApp->mNextDrawTick = aEndTime;

			/*char aStr[256];
			sprintf(aStr, "Next Draw Time: %d\r\n", mNextDrawTick);
			OutputDebugString(aStr);*/
		}
		else
			mApp->mNextDrawTick = aEndTime;

		mApp->mHasPendingDraw = false;
		mApp->mCustomCursorDirty = false;
		if (gTracingPixels)
			PixelTracerStop();

		return true;
	}
	else
	{
		mApp->mHasPendingDraw = false;
		mApp->mLastDrawWasEmpty = true;
		return false;
	}
}

void WindowsAppDriver::LogScreenSaverError(const std::string& theError) //2578-2593
{
	static bool firstTime = true;
	char aBuf[512];
	std::string aFileName;

	const char* aFlag = firstTime ? "w" : "a+";
	firstTime = false;

	aFileName = GetAppDataFolder();
	MkDir(aFileName);

	FILE* aFile = fopen("ScrError.txt", aFlag);
	if (aFile != NULL)
	{
		fprintf(aFile, "%s %s %u\n", theError.c_str(), _strtime(aBuf), GetTickCount());
		fclose(aFile);
	}
}

void WindowsAppDriver::BeginPopup() //2596-2608
{
	if (!mApp->mIsPhysWindowed && mWindowsGraphicsDriver)
	{
		if (mWindowsGraphicsDriver->mDD)
			mWindowsGraphicsDriver->mDD->FlipToGDISurface();
		else if (mApp->mWantsDialogCompatibility)
			ShowWindow(mApp->mHWnd, SW_MINIMIZE);
		mApp->mNoDefer = true;
	}
}

void WindowsAppDriver::EndPopup() //2611-2627
{
	if (!mApp->mIsPhysWindowed)
		if (!mWindowsGraphicsDriver->mDD && !mApp->mWantsDialogCompatibility)
			SetForegroundWindow(mApp->mHWnd);
	mApp->mNoDefer = false;

	ClearUpdateBacklog();
	ClearKeysDown();

	if (mApp->mWidgetManager->mDownButtons)
	{
		mApp->mWidgetManager->DoMouseUps();
		ReleaseCapture();
	}
}

int WindowsAppDriver::MsgBox(const std::string& theText, const std::string& theTitle, int theFlags) //2630-2644
{
	if (IsScreenSaver())
	{
		LogScreenSaverError(theText);
		return IDOK;
	}

	BeginPopup();
	int aResult = MessageBoxA(mApp->mHWnd, theText.c_str(), theTitle.c_str(), theFlags);
	EndPopup();

	return aResult;
}

int WindowsAppDriver::MsgBox(const std::wstring& theText, const std::wstring& theTitle, int theFlags) //2647-2661
{
	if (IsScreenSaver())
	{
		LogScreenSaverError(SexyStringToStringFast(theText)); //change to fast?
		return IDOK;
	}

	BeginPopup();
	int aResult = MessageBoxW(mApp->mHWnd, theText.c_str(), theTitle.c_str(), theFlags);
	EndPopup();

	return aResult;
}

void WindowsAppDriver::Popup(const std::string& theString) //2664-2675
{
	if (IsScreenSaver())
	{
		LogScreenSaverError(theString);
		return;
	}

	BeginPopup();
	if (!mApp->mShutdown)
		::MessageBoxA(mApp->mHWnd, theString.c_str(), SexyStringToString(mApp->GetString("FATAL_ERROR", _S("FATAL ERROR"))).c_str(), MB_APPLMODAL | MB_ICONSTOP);
	EndPopup();
}

void WindowsAppDriver::Popup(const std::wstring& theString) //2678-2689
{
	if (IsScreenSaver())
	{
		LogScreenSaverError(SexyStringToStringFast(theString));
		return;
	}

	BeginPopup();
	if (!mApp->mShutdown)
		::MessageBoxW(mApp->mHWnd, theString.c_str(), SexyStringToWString(mApp->GetString("FATAL_ERROR", _S("FATAL ERROR"))).c_str(), MB_APPLMODAL | MB_ICONSTOP);
	EndPopup();
}

BOOL CALLBACK EnumCloseThing2(HWND hwnd, LPARAM lParam) //Why | 2693-2709
{
	//CloseWindow(hwnd);
	char aClassName[256];
	if (GetClassNameA(hwnd, aClassName, 256) != 0)
	{
		if (strcmp(aClassName, "Internet Explorer_Server") == 0)
		{
			DestroyWindow(hwnd);
		}
		else
		{
			EnumChildWindows(hwnd, EnumCloseThing2, lParam);
		}
	}

	return TRUE;
}

BOOL CALLBACK EnumCloseThing(HWND hwnd, LPARAM lParam) //2712-2724
{
	//CloseWindow(hwnd);
	char aClassName[256];
	if (GetClassNameA(hwnd, aClassName, 256) != 0)
	{
		if (strcmp(aClassName, "AmWBC_WClass") == 0)
		{
			EnumChildWindows(hwnd, EnumCloseThing2, lParam);
		}
	}

	return TRUE;
}

static DWORD gPowerSaveTick = 0;
bool WindowsAppDriver::ScreenSaverWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& theResult) //Todo | 2742-2943
{
	static bool gCreated = false;
	static int gMouseMoveCount = 0;
	static int gLastMouseX = 0, gLastMouseY = 0;
	static bool gClosed = false;
	typedef BOOL(WINAPI* VERIFYPWDPROC)(HWND);
	static VERIFYPWDPROC aPasswordFunc = NULL;
	HMODULE aPasswordLib = NULL;

	if (gClosed)
		return false;

	switch (uMsg)
	{
	case WM_CREATE:
	{
		if (gCreated)
			return false;

		gCreated = true;
		POINT aMousePoint;
		GetCursorPos(&aMousePoint);
		gLastMouseX = aMousePoint.x;
		gLastMouseY = aMousePoint.y;

		// Password checking stuff for 95/98/ME
		OSVERSIONINFO aVersion;
		aVersion.dwOSVersionInfoSize = sizeof(aVersion);
		GetVersionEx(&aVersion);
		if (aVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		{
			HKEY hKey;
			if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_SCREENSAVE, &hKey) == ERROR_SUCCESS)
			{
				DWORD aCheckPwd = 0;
				DWORD aSize = sizeof(DWORD);
				DWORD aType;
				LONG aResult = RegQueryValueEx(hKey, REGSTR_VALUE_USESCRPASSWORD, NULL, &aType, (PBYTE)&aCheckPwd, &aSize);
				if (aResult == ERROR_SUCCESS && aCheckPwd)
				{
					aPasswordLib = LoadLibrary(TEXT("PASSWORD.CPL"));
					if (aPasswordLib)
					{
						aPasswordFunc = (VERIFYPWDPROC)GetProcAddress(aPasswordLib, "VerifyScreenSavePwd");
						// prevents user from ctrl-alt-deleting the screensaver etc to avoid typing in a password
						int aPrev;
						SystemParametersInfo(SPI_SCREENSAVERRUNNING, TRUE, &aPrev, 0);
					}

				}
				RegCloseKey(hKey);
			}
		}
		return false;
	}
	break;

	case WM_SYSCOMMAND:
	{
		switch (wParam)
		{
		case SC_CLOSE:
		case SC_SCREENSAVE:
		case SC_NEXTWINDOW:
		case SC_PREVWINDOW:
			theResult = FALSE;
			return true;

		default:
			return false;
		}
	}
	break;

	case WM_MOUSEMOVE:
	{
		int aMouseX = LOWORD(lParam);
		int aMouseY = HIWORD(lParam);
		//			SEXY_TRACE(StrFormat("SCR MouseMove: %d %d",aMouseX,aMouseY).c_str());
		if (aMouseX != gLastMouseX || aMouseY != gLastMouseY)
		{
			gLastMouseX = aMouseX;
			gLastMouseY = aMouseY;
			gMouseMoveCount++;
		}

		if (gMouseMoveCount < 4)
		{
			theResult = 0;
			return true;
		}
	}
	break;

	case WM_NCACTIVATE:
	case WM_ACTIVATE:
	case WM_ACTIVATEAPP:
	{
		if (wParam != FALSE)
			return false;
	}
	break;

	case WM_CLOSE:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		break;

	default:
		return false;
	}

	if (gSexyAppBase != NULL && gSexyAppBase->mHWnd != hWnd) // wrong window
		return false;

	if (GetTickCount() - gPowerSaveTick < 1000) // powersave just went on so ignore certain messages that seem to come on certain os's at that time
	{
		switch (uMsg)
		{
		case WM_MOUSEMOVE:
		case WM_NCACTIVATE:
		case WM_ACTIVATE:
		case WM_ACTIVATEAPP:
		case WM_CLOSE:
			return false;
		}
	}

	if (aPasswordFunc && gSexyAppBase != NULL && gSexyAppBase->mInitialized) // need to verify password before closing
	{
		WindowsGraphicsDriver* windowsGraphicsDriver = NULL;
		if (gSexyAppBase != NULL)
		{
			windowsGraphicsDriver = (WindowsGraphicsDriver*)gSexyAppBase->mGraphicsDriver;
			windowsGraphicsDriver->mDD->FlipToGDISurface();	// so we can see the password dialog
			gSexyAppBase->mNoDefer = true;							// so the app doesn't draw over the password dialog
		}

		gClosed = true; // prevent this function from doing anything while in the password dialog
		BOOL aPasswordResult = aPasswordFunc(hWnd);
		gClosed = false; // let this functino work again

		if (gSexyAppBase != NULL)
		{
			gSexyAppBase->mNoDefer = false;
			gSexyAppBase->ClearUpdateBacklog();
		}

		if (!aPasswordResult) // bad password
		{
			// Get new mouse coordinate
			POINT aMousePoint;
			GetCursorPos(&aMousePoint);
			gLastMouseX = aMousePoint.x;
			gLastMouseY = aMousePoint.y;
			gMouseMoveCount = 0;

			return false;
		}


		// can turn this SPI_SCREENSAVERRUNNING off now since screensaver is about to stop
		int aPrev;
		SystemParametersInfo(SPI_SCREENSAVERRUNNING, FALSE, &aPrev, 0);

		// good password -> close and unload dll
		FreeLibrary(aPasswordLib);
		aPasswordLib = NULL;
		aPasswordFunc = NULL;
	}

	// Screen saver should shutdown
	gClosed = true;
	PostMessage(hWnd, WM_CLOSE, 0, 0);
	return false;
}

LRESULT CALLBACK WindowsAppDriver::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) //HELP | 2946-3383
{
	if (gSexyAppBase != NULL && gSexyAppBase->IsScreenSaver())
	{
		LRESULT aResult;
		if (ScreenSaverWindowProc(hWnd, uMsg, wParam, lParam, aResult))
			return aResult;
	}

	WindowsAppDriver* driver = (WindowsAppDriver*)GetWindowLong(hWnd, GWL_USERDATA);
	switch (uMsg)
	{

	case WM_ACTIVATEAPP:
		if (driver && hWnd == driver->mApp->mHWnd)
		{
			driver->mApp->mActive = wParam != 0;
		}
		//Fallthrough	

	case WM_SIZE:
	case WM_MOVE:
	case WM_TIMER:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
	case WM_CHAR:
	case WM_CLOSE:
	case WM_MOUSEWHEEL:
	case WM_DISPLAYCHANGE:
	case WM_SYSCOLORCHANGE:
	{
		if (driver && !driver->mApp->mNoDefer)
		{
			bool keyDown = (uMsg == WM_KEYDOWN) || (uMsg == WM_SYSKEYDOWN);

			if ((keyDown) || (uMsg == WM_KEYUP) || (uMsg == WM_SYSKEYUP))
			{
				if (wParam == VK_CONTROL)
					driver->mApp->mCtrlDown = keyDown;
				if (wParam == VK_MENU)
					driver->mApp->mAltDown = keyDown;
			}

			if ((keyDown) && (driver->DebugKeyDownAsync(wParam, driver->mApp->mCtrlDown, driver->mApp->mAltDown)))
				return 0;

			bool pushMessage = true;

			if (driver->mApp->mDeferredMessages.size() > 0)
			{
				// Don't add any more messages after WM_CLOSE
				MSG* aMsg = &driver->mApp->mDeferredMessages.back();

				if (aMsg->message == WM_CLOSE)
					pushMessage = false;
				if ((uMsg == WM_TIMER) && (uMsg == aMsg->message))
					pushMessage = false; // Don't need more timer messages


				if (pushMessage && (uMsg == WM_SYSCOLORCHANGE || uMsg == WM_DISPLAYCHANGE)) // kill duplicate SysColorChange() events.
				{
					WindowsMessageList::iterator aMsgListItr = driver->mApp->mDeferredMessages.begin();
					while (pushMessage && aMsgListItr != driver->mApp->mDeferredMessages.end())
					{
						MSG& aMsg = *aMsgListItr;

						if (aMsg.message == WM_SYSCOLORCHANGE || aMsg.message == WM_DISPLAYCHANGE)
							pushMessage = false;

						++aMsgListItr;
					}
				}
			}

			if (pushMessage)
			{
				MSG msg;
				msg.hwnd = hWnd;
				msg.message = uMsg;
				msg.lParam = lParam;
				msg.wParam = wParam;

				driver->mApp->mDeferredMessages.push_back(msg);
			}

			if (uMsg == WM_WINDOWPOSCHANGING) //TODO
			{
				/*LPWINDOWPOS wp = lParam;
				RECT aCurrentRect;
				GetWindowRect(hWnd, &aCurrentRect);
				RECT aClientRect;
				GetWindowRect(hWnd, &aClientRect);
				RECT aDesiredRect;
				aDesiredRect.left = wp->x;
				aDesiredRect.top = wp->y;
				aDesiredRect.right = wp->cx + wp->x;
				aDesiredRect.bottom = wp->cy + wp->y;
				float aCurrentAspect = (double)(aCurrentRect.right - aCurrentRect.left) / (double)(aCurrentRect.bottom - aCurrentRect.top);
				float aDesiredAspect = (double)(aDesiredRect.right - aDesiredRect.left) / (double)(aDesiredRect.bottom - aDesiredRect.top);
				Rect screenRect(0, 0, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
				bool bothRectsOnScreen = false;
				if (screenRect.Intersects(aCurrentRect))
				{

				}*/
			}

			if (uMsg == WM_SIZING) //TODO
			{
				if (driver->mApp->mWidescreenAware)
				{
					//GetWindowRect(hWnd, &? );
					//RECT aWindowRect = lParam;
				}
			}

			if (uMsg == WM_SIZE)
			{
				driver->mApp->mPhysMinimized = wParam == SIZE_MINIMIZED;
				if (wParam != 0 && driver->mApp->mWidescreenAware)
				{
					RECT aRect;
					GetClientRect(hWnd, &aRect);
					driver->mWindowsGraphicsDriver->WindowResize(aRect.right, aRect.bottom);
					driver->mApp->mWidgetManager->mMouseSourceRect = driver->mWindowsGraphicsDriver->mPresentationRect;
					InvalidateRect(hWnd, NULL, false);
				}
			}
			else if (uMsg == WM_SYSKEYDOWN)
			{
				if (wParam != VK_F4)
					return 0;
			}
			else if (uMsg == WM_CLOSE)
			{
				/*char aStr[256];
				sprintf(aStr, "CLOSED HWND: %d\r\n", hWnd);
				OutputDebugString(aStr);*/

				driver->mApp->CloseRequestAsync();
				return 0;
			}
		}
	}
	break;

	case WM_ENABLE:
		if (driver != NULL)
		{
			driver->mApp->mIsDisabled = wParam == 0;
		}
		break;

	case WM_QUERYOPEN:
		if ((driver != NULL) && (!driver->AppCanRestore()))
			return 0;
		break;

	case WM_SYSCHAR:
		if ((driver != NULL) && (driver->IsAltKeyUsed(wParam)))
			return 0;
		break;

	case WM_NCLBUTTONDOWN:
		if (driver != NULL)
		{
			//			aSexyApp->mProcessInTimer = true;
			LRESULT aResult = DefWindowProc(hWnd, uMsg, wParam, lParam);
			//			aSexyApp->mProcessInTimer = false;
			driver->ClearUpdateBacklog();
			return aResult;
		}
		break;


	case WM_SYSCOMMAND:
		if (wParam == SC_MONITORPOWER)
		{
			gPowerSaveTick = GetTickCount();
			if (driver != NULL && (!driver->mApp->mAllowMonitorPowersave || !driver->mApp->mLoaded))
				return FALSE;
		}
		if (wParam == SC_SCREENSAVE && driver != NULL && (!driver->mApp->mLoaded || !driver->mApp->mIsPhysWindowed))
			return FALSE;

		break;

		/*	case WM_DISPLAYCHANGE:
				SEXY_TRACE("WM_DISPLAYCHANGE 1");
				if (aSexyApp!=NULL && aSexyApp->mIsWindowed && aSexyApp->mAppDriver!=NULL && aSexyApp->mHWnd==hWnd && aSexyApp->mLoaded)
				{
					SEXY_TRACE("WM_DISPLAYCHANGE 2");
					aSexyApp->mAppDriver->Init(aSexyApp->mHWnd,aSexyApp->mIsWindowed);
					aSexyApp->mWidgetManager->mImage = aSexyApp->mAppDriver->GetScreenImage();
					aSexyApp->mWidgetManager->MarkAllDirty();
				}
				break;*/

	case WM_DESTROY:
	{
		char aStr[256];
		sprintf(aStr, "DESTROYED HWND: %d\r\n", hWnd);
		OutputDebugString(aStr);
	}
	break;
	case WM_SETCURSOR:
		if (driver && !driver->mApp->mSEHOccured)
			driver->EnforceCursor();
		return TRUE;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_ENDSESSION:
		if (driver)
			driver->mApp->Shutdown();
		break;
	case WM_PAINT:
		if (!driver || !driver->mApp->mInitialized || gInAssert || driver->mApp->mSEHOccured)
		{
			RECT aClientRect;
			GetClientRect(hWnd, &aClientRect);

			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);

			if (driver->mApp->mRunning)
				driver->Redraw(NULL);

			EndPaint(hWnd, &ps);

			return 0;
		}
		break;
	}

	if ((driver != NULL) && (uMsg == driver->mApp->mNotifyGameMessage) && (hWnd == driver->mApp->mHWnd))
	{
		// Oh, we are trying to open another instance of ourselves.
		// Bring up the original window instead
		driver->HandleNotifyGameMessage(wParam, lParam);
		return 0;
	}

	if (driver && driver->mApp->mIsWideWindow)
		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	else
		return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

void WindowsAppDriver::HandleNotifyGameMessage(int theType, int theParam) //3386-3398
{
	if (theType == 0) // bring to front message
	{
		WINDOWPLACEMENT aWindowPlacement;
		aWindowPlacement.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(mApp->mHWnd, &aWindowPlacement);

		if (aWindowPlacement.showCmd == SW_SHOWMINIMIZED)
			ShowWindow(mApp->mHWnd, SW_RESTORE);

		::SetForegroundWindow(mApp->mHWnd);
	}
}

void WindowsAppDriver::ShowMemoryUsage() //Correct? | 3409-3507
{
	DWORD aTotal = 0;
	DWORD aFree = 0;

	if (mWindowsGraphicsDriver->mDD7 != NULL)
	{
		DDSCAPS2 aCaps;
		ZeroMemory(&aCaps, sizeof(aCaps));
		aCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
		mWindowsGraphicsDriver->mDD7->GetAvailableVidMem(&aCaps, &aTotal, &aFree);
	}

	MemoryImageSet::iterator anItr = mApp->mMemoryImageSet.begin();
	typedef std::pair<int, int> FormatUsage;
	typedef std::map<PixelFormat, FormatUsage> FormatMap;
	FormatMap aFormatMap;
	int anImageMemory = 0;
	int aTextureMemory = 0;
	while (anItr != mApp->mMemoryImageSet.end())
	{
		MemoryImage* aMemoryImage = *anItr;
		DeviceImage* aDDImage = dynamic_cast<DeviceImage*>(aMemoryImage);
		int aNumPixels = aMemoryImage->mWidth * aMemoryImage->mHeight;

		if (aMemoryImage->mBits != NULL)
			anImageMemory += aNumPixels * 4;
		if ((aDDImage != NULL) && (aDDImage->mSurface != NULL))
			anImageMemory += aNumPixels * 4; // Assume 32bit screen...
		if (aMemoryImage->mColorTable != NULL)
			anImageMemory += aNumPixels + 256 * 4;
		if (aMemoryImage->mNativeAlphaData != NULL)
		{
			if (aMemoryImage->mColorTable != NULL)
				anImageMemory += 256 * 4;
			else
				anImageMemory += aNumPixels * 4;
		}
		if (aMemoryImage->mRLAlphaData != NULL)
			anImageMemory += aNumPixels;
		if (aMemoryImage->mRLAdditiveData != NULL)
			anImageMemory += aNumPixels;

		if (mWindowsGraphicsDriver != NULL && mWindowsGraphicsDriver->mRenderDevice3D != NULL)
		{
			anImageMemory += mWindowsGraphicsDriver->mRenderDevice3D->GetTextureMemorySize(aMemoryImage);
			int aUsedMem = mWindowsGraphicsDriver->mRenderDevice3D->GetTextureMemorySize(aMemoryImage);
			PixelFormat aFormat = mWindowsGraphicsDriver->mRenderDevice3D->GetTextureFormat(aMemoryImage);
			aTextureMemory += aUsedMem;
			if (aFormat)
			{
				FormatUsage& aUsage = aFormatMap[aFormat];
				aUsage.first++;
				aUsage.second += aUsedMem;
			}
		}

		++anItr;
	}

	std::string aStr;

	const char* aDesc;
	if (Is3DAccelerationRecommended())
		aDesc = "Recommended";
	else if (Is3DAccelerationSupported())
		aDesc = "Supported";
	else
		aDesc = "Unsupported";

	aStr += StrFormat("3D-Mode is %s (3D is %s on this system)\r\n\r\n", Is3DAccelerated() ? "On" : "Off", aDesc);

	if (mWindowsGraphicsDriver->mIs3D)
		aStr += StrFormat("Direct3D Version: %d\r\n", mWindowsGraphicsDriver->mIsD3D8Or9 ? mWindowsGraphicsDriver->mIsD3D9 + 8 : 7, aDesc);

	aStr += StrFormat("Num Images: %d\r\n", (int)mApp->mMemoryImageSet.size());
	aStr += StrFormat("Num Sounds: %d\r\n", mApp->mSoundManager->GetNumSounds());
	aStr += StrFormat("Video Memory: %s/%s KB\r\n", ToString(CommaSeperate((aTotal - aFree) / 1024)).c_str(), ToString(CommaSeperate(aTotal / 1024))).c_str();
	aStr += StrFormat("Image Memory: %s KB\r\n", CommaSeperate(anImageMemory / 1024).c_str()); //Added in ZR

	if (mWindowsGraphicsDriver->mIs3D)
	{
		aStr += StrFormat("Texture Memory: %s KB\r\n", CommaSeperate(aTextureMemory / 1024).c_str()); //Only visible with 3D as of BejBlitz PC (ZR was the last game to have them visible outside)
		FormatUsage aUsage = aFormatMap[PixelFormat_A8R8G8B8];
		aStr += StrFormat("A8R8G8B8: %d - %s KB\r\n", aUsage.first, ToString(CommaSeperate(aUsage.second / 1024)).c_str());
		aUsage = aFormatMap[PixelFormat_A4R4G4B4];
		aStr += StrFormat("A4R4G4B4: %d - %s KB\r\n", aUsage.first, ToString(CommaSeperate(aUsage.second / 1024)).c_str());
		aUsage = aFormatMap[PixelFormat_R5G6B5];
		aStr += StrFormat("R5G6B5: %d - %s KB\r\n", aUsage.first, ToString(CommaSeperate(aUsage.second / 1024)).c_str());
		aUsage = aFormatMap[PixelFormat_Palette8];
		aStr += StrFormat("Palette8: %d - %s KB\r\n", aUsage.first, ToString(CommaSeperate(aUsage.second / 1024)).c_str());
#ifdef SEXYDECOMP_ENABLE_CUSTOM_MODS //Display the other textures!
		aUsage = aFormatMap[PixelFormat_X8R8G8B8];
		aStr += StrFormat("X8R8G8B8: %d - %s KB\r\n", aUsage.first, ToString(CommaSeperate(aUsage.second / 1024)).c_str());
#ifdef SEXYDECOMP_USE_LATEST_CODE
		aUsage = aFormatMap[PixelFormat_DXT5];
		aStr += StrFormat("DXT5: %d - %s KB\r\n", aUsage.first, ToString(CommaSeperate(aUsage.second / 1024)).c_str());
#endif
#endif
	}

	MsgBox(aStr, "Video Stats", MB_OK);
	mApp->mLastTime = timeGetTime();
}

bool WindowsAppDriver::IsAltKeyUsed(WPARAM wParam) //3510-3523
{
	int aChar = tolower(wParam);
	switch (aChar) //Not sure why these were added, use the keycode
	{
	case 13: // alt-enter
	case 97: // NumPad1
	case 109: // NumPadMinus
	case 'r': // F3
	case 115: // F4
		return true;
	default:
		return false;
	}
}

bool WindowsAppDriver::KeyDown(int theKey) //3526-3540
{
	if (theKey != 'p')
		return mApp->mDebugKeysEnabled && mApp->DebugKeyDown(theKey);
	if (mApp->mLoadingThreadCompleted)
		mApp->mShowCompatInfoMode = (SexyAppBase::EShowCompatInfoMode)((mApp->mShowCompatInfoMode + 1) % SexyAppBase::SHOWCOMPATINFOMODE_COUNT); //Yeah weird
	return true;
}

bool WindowsAppDriver::DebugKeyDown(int theKey) //3543-3814
{
	if ((theKey == 'R') && (mApp->mWidgetManager->mKeyDown[KEYCODE_MENU]))
	{
#ifndef RELEASEFINAL
		if (ReparseModValues())
			PlaySoundA("c:\\windows\\media\\Windows XP Menu Command.wav", NULL, SND_ASYNC);
		else
		{
			for (int aKeyNum = 0; aKeyNum < 0xFF; aKeyNum++) // prevent alt from getting stuck
				mApp->mWidgetManager->mKeyDown[aKeyNum] = false;
		}
	}
#endif
	if ((theKey == 'S') && (mApp->mWidgetManager->mKeyDown[KEYCODE_MENU])) //Seems this reloads the scenery
	{
#ifndef RELEASEFINAL
		if (mWindowsGraphicsDriver && mWindowsGraphicsDriver->mRenderDevice3D && mWindowsGraphicsDriver->mRenderDevice3D->ReloadEffects())
			PlaySoundA("c:\\windows\\media\\Windows XP Menu Command.wav", NULL, SND_ASYNC);
		else
		{
			for (int aKeyNum = 0; aKeyNum < 0xFF; aKeyNum++) // prevent alt from getting stuck
				mApp->mWidgetManager->mKeyDown[aKeyNum] = false;
		}
	}
#endif
	if ((theKey == 'A') && (mApp->mWidgetManager->mKeyDown[KEYCODE_MENU])) //Seems to reload assets
	{
#ifndef RELEASEFINAL
		if (ReloadAllResources)
			PlaySoundA("c:\\windows\\media\\Windows XP Menu Command.wav", NULL, SND_ASYNC);
		else
		{
			for (int aKeyNum = 0; aKeyNum < 0xFF; aKeyNum++) // prevent alt from getting stuck
				mApp->mWidgetManager->mKeyDown[aKeyNum] = false;
		}
	}
#endif
	if ((theKey == 'W') && (mApp->mWidgetManager->mKeyDown[KEYCODE_CONTROL]) && (mApp->mWidgetManager->mKeyDown[KEYCODE_MENU])) //Seems to be a widget inspector
	{
#ifndef RELEASEFINAL //Correct?
		mApp->mShowWidgetInspector ^= true;
		mApp->mWidgetInspectorCurWidget = NULL;
		mApp->mWidgetInspectorScrollOffset = 0;
		mApp->mWidgetInspectorPickWidget = NULL;
		mApp->mWidgetInspectorPickMode = false;
		mApp->mWidgetInspectorClickPos.mX = -1;
		mApp->mWidgetInspectorClickPos.mY = -1;
		return false;
	}
#endif
	if ((theKey == 'P') && (mApp->mWidgetManager->mKeyDown[KEYCODE_CONTROL]) && (mApp->mWidgetManager->mKeyDown[KEYCODE_MENU])) //PDB debug?
	{
#ifndef RELEASEFINAL
		CRefSymbolDb::ToggleStringFlag(1u);
	}
#endif
#ifdef SEXYDECOMP_ENABLE_CUSTOM MODS
	if ((theKey == 'I') && (mApp->mWidgetManager->mKeyDown[KEYCODE_MENU])) //Custom mod add: PIEffect debug draw, TODO
	{
		PIEffect::mDebug = true;
	}
#endif

	else if (theKey == VK_F3)
	{
		if (!mApp->mWidgetManager->mKeyDown[KEYCODE_CONTROL])
		{
			if (mApp->mWidgetManager->mKeyDown[KEYCODE_SHIFT])
			{
				mApp->mShowFPS = true;
				if (++mApp->mShowFPSMode >= Num_FPS_Types)
					mApp->mShowFPSMode = FPS_ShowFPS;
			}
			else
				mApp->mShowFPS = !mApp->mShowFPS;

			mApp->mWidgetManager->MarkAllDirty();

			if (mApp->mShowFPS)
			{
				gFPSTimer.Start();
				gFrameCount = 0;
				gFPSDisplay = 0;
				gFPSDisplayRecipMS = 0;
				gForceDisplay = true;
				mApp->mVFPSUpdateTimes = 0.0;
				mApp->mVFPSUpdateCount = 0;
				mApp->mVFPSDrawTimes = 0.0;
				mApp->mVFPSDrawCount = 0;
				mApp->mCurVFPS = 0.0;
			}
			DeviceImage* aTarget = new DeviceImage();
			aTarget->SetImageMode(false, false);
			aTarget->ReplaceImageFlags(0x18u);
			aTarget->Create(mApp->mScreenBounds.mWidth, mApp->mScreenBounds.mHeight);
			if (mWindowsGraphicsDriver->mRenderDevice3D)
				mWindowsGraphicsDriver->mRenderDevice3D->Flush(RenderDevice3D::FLUSHF_CurrentScene);
			mWindowsGraphicsDriver->SetRenderMode(WindowsGraphicsDriver::RENDERMODE_OverdrawExact);
			Graphics aDrawG(aTarget);
			aDrawG.Translate(-mApp->mWidgetManager->mMouseDestRect.mX, -mApp->mWidgetManager->mMouseDestRect.mY);
			aDrawG.SetFastStretch(false);
			aDrawG.SetLinearBlend(true);
			mApp->mWidgetManager->DrawWidgetsTo(&aDrawG);
			if (mWindowsGraphicsDriver->mRenderDevice3D)
				mWindowsGraphicsDriver->mRenderDevice3D->Flush(2u);
			mWindowsGraphicsDriver->SetRenderMode(WindowsGraphicsDriver::RENDERMODE_Default);
			int aScreenPixels = aTarget->mHeight * aTarget->mWidth;
			int aRCount = 0;
			int aGCount = 0;
			ulong* aBits = aTarget->GetBits(); //?
			for (int aPixelIdx = 0; aPixelIdx < aScreenPixels; ++aPixelIdx)
			{
				aRCount += aBits[aPixelIdx];
				aGCount += aBits[aPixelIdx];
			}
			if (!aRCount)
				aRCount = aGCount / 4;
			MsgBox(StrFormat("Frame overdraw: %.2fx\r\n", aRCount / aScreenPixels), "Overdraw", MB_OK);
			if (aTarget != NULL)
				aTarget->DeleteExtraBuffers();
			return true;
		}
	}
	else if (theKey == VK_F8)
	{
		if (mApp->mWidgetManager->mKeyDown[KEYCODE_SHIFT])
		{
			Set3DAcclerated(!Is3DAccelerated());

			char aBuf[512];
			sprintf(aBuf, "3D-Mode: %s", Is3DAccelerated() ? mWindowsGraphicsDriver->mIsD3D8Or9 ? mWindowsGraphicsDriver->mIsD3D9 ? "DX9" : "DX8" : "DX7" : "Off"); //Added
			MsgBox(aBuf, "Mode Switch", MB_OK);
			mApp->mLastTime = timeGetTime();
		}
		else if (mApp->mWidgetManager->mKeyDown[KEYCODE_CONTROL])
		{
			mWindowsGraphicsDriver->mWantD3D9 = !mWindowsGraphicsDriver->mWantD3D9;
			mWindowsGraphicsDriver->mIs3D = false;
			Set3DAcclerated(true, true);

			char aBuf[512];
			sprintf(aBuf, "3D-Mode: %s", Is3DAccelerated() ? mWindowsGraphicsDriver->mIsD3D8Or9 ? mWindowsGraphicsDriver->mIsD3D9 ? "DX9" : "DX8" : "DX7" : "Off");
			MsgBox(aBuf, "Mode Switch", MB_OK);
			mApp->mLastTime = timeGetTime();
		}
		else if (mApp->mWidgetManager->mKeyDown[KEYCODE_MENU]) //ALT F8 is VSync
		{
			mApp->mNoVSync = !mApp->mNoVSync;
			mWindowsGraphicsDriver->mIs3D = false;
			Set3DAcclerated(true, true);

			char aBuf[512];
			sprintf(aBuf, "VSync: %s", mApp->mNoVSync ? "ON" : "OFF");
			MsgBox(aBuf, "VSync Switch", MB_OK);
			mApp->mLastTime = timeGetTime();
		}
		else
			ShowMemoryUsage();

		return true;
	}
	else if (theKey == VK_F9) //Add F9 for debug
	{
		if (mApp->mWidgetManager->mKeyDown[KEYCODE_CONTROL])
		{
#ifndef RELEASEFINAL
			if (++sCurRenderModeFlagBit >= WindowsGraphicsDriver::RENDERMODEF_USEDBITS)
				sCurRenderModeFlagBit = 0;
			char aBuf[512];
			sprintf(aBuf, "Current Flag: %s", mWindowsGraphicsDriver->GetRenderModeString(mWindowsGraphicsDriver->mRenderMode, 1 << sCurRenderModeFlagBit, true, false).c_str());
			MsgBox(aBuf, "Cycle Current Render Mode Flag", MB_OK);
			mApp->mLastTime = timeGetTime();
		}
#endif
		else if (mApp->mWidgetManager->mKeyDown[KEYCODE_SHIFT])
		{
#ifndef RELEASEFINAL
			if ((mWindowsGraphicsDriver->mRenderModeFlags & (1 << sCurRenderModeFlagBit)) != 0)
				mWindowsGraphicsDriver->mRenderModeFlags = mWindowsGraphicsDriver->mRenderModeFlags & ~(1 << sCurRenderModeFlagBit);
			else
				mWindowsGraphicsDriver->mRenderModeFlags = mWindowsGraphicsDriver->mRenderModeFlags | (1 << sCurRenderModeFlagBit);
			//?
			char aBuf[512];
			sprintf(aBuf, "%s: %s", mWindowsGraphicsDriver->GetRenderModeString(mWindowsGraphicsDriver->mRenderMode, 1 << sCurRenderModeFlagBit, true, false).c_str() ? "ON" : "OFF");
			MsgBox(aBuf, "Toggle Render Mode Flag", MB_OK);
		}
#endif
		else if (mApp->mWidgetManager->mKeyDown[KEYCODE_MENU])
		{
#ifndef RELEASEFINAL
			if (++mWindowsGraphicsDriver->mRenderMode >= WindowsGraphicsDriver::RENDERMODE_OverdrawExact)
				mWindowsGraphicsDriver->mRenderMode = WindowsGraphicsDriver::RENDERMODE_Default;
			mWindowsGraphicsDriver->SetRenderMode(mWindowsGraphicsDriver->mRenderMode);
			char aBuf[512];
			sprintf(aBuf, "Render Mode: %s", mWindowsGraphicsDriver->GetRenderModeString(mWindowsGraphicsDriver->mRenderMode, 1 << sCurRenderModeFlagBit, true, false).c_str());
			MsgBox(aBuf, "Cycle Current Render Mode", MB_OK);
			mApp->mLastTime = timeGetTime();
		}
#endif
	}

	else if (theKey == VK_F10)
	{
#ifndef RELEASEFINAL
		if (mApp->mWidgetManager->mKeyDown[KEYCODE_CONTROL])
		{
			if (mApp->mUpdateMultiplier == 0.25)
				mApp->mUpdateMultiplier = 1.0;
			else
				mApp->mUpdateMultiplier = 0.25;
		}
		else if (mApp->mWidgetManager->mKeyDown[KEYCODE_SHIFT])
		{
			mApp->mStepMode = 0;
			ClearUpdateBacklog();
		}
		else
			mApp->mStepMode = 1;
#endif

		return true;
	}
	else if (theKey == VK_F11) //This was completely removed in ZR, why idk
	{
		if (mApp->mWidgetManager->mKeyDown[KEYCODE_SHIFT])
			DumpProgramInfo();
#ifdef _SEXYDECOMP_USE_DEPRECATED_CODE //PopCap seems to have removed the standalone key to take a screenshot in Twist+ (and also BejBlitz and carried over in Prime), not just cut from retail, why idk, this was useful
		else
			TakeScreenshot();
#endif

		return true;
	}
	else if (theKey == VK_F2) //Changed
	{
		bool isPerfOn = !SexyPerf::IsPerfOn();
		if (isPerfOn)
		{
			//			MsgBox("Perf Monitoring: ON", "Perf Monitoring", MB_OK);
			ClearUpdateBacklog();
			SexyPerf::BeginPerf(false, mApp->mWidgetManager->mKeyDown[KEYCODE_SHIFT]);
		}
		else
		{
			bool showResults = SexyPerf::IsPerfRecording();
			SexyPerf::EndPerf();
			if (showResults)
				MsgBox(SexyPerf::GetResults().c_str(), "Perf Results", MB_OK);
			ClearUpdateBacklog();
		}
	}
	else
		return false;

	return false;
}

bool WindowsAppDriver::DebugKeyDownAsync(int theKey, bool ctrlDown, bool altDown) //3817-3819
{
	return false;
}

void WindowsAppDriver::CloseRequestAsync() //3822-3823
{
}

// Why did I defer messages?  Oh, incase a dialog comes up such as a crash
//  it won't keep crashing and stuff
bool WindowsAppDriver::ProcessDeferredMessages(bool singleMessage) //Todo | 3828-4130
{
	PerfTimer aTimer = PerfTimer();
	if (mApp->mShowFPS && mApp->mShowFPSMode <= FPS_ShowFPSWithHistory)
		aTimer.Start();
	while (mApp->mDeferredMessages.size() > 0)
	{
		MSG aMsg = mApp->mDeferredMessages.front();
		mApp->mDeferredMessages.pop_front();

		UINT uMsg = aMsg.message;
		LPARAM lParam = aMsg.lParam;
		WPARAM wParam = aMsg.wParam;
		HWND hWnd = aMsg.hwnd;

		switch (uMsg)
		{
			//  TODO: switch to killfocus/setfocus?
			//			case WM_KILLFOCUS:
			//			case WM_SETFOCUS:
		case WM_ACTIVATEAPP:
			if ((hWnd == mApp->mHWnd) && (!gInAssert) && (!mApp->mSEHOccured) && (!mApp->mShutdown))
			{
				//					mActive = uMsg==WM_SETFOCUS;

				RehupFocus();

				if ((mApp->mActive) && (!mApp->mIsWindowed))
					mApp->mWidgetManager->MarkAllDirty();

				if ((mApp->mIsOpeningURL) && (!mApp->mActive))
					mApp->URLOpenSucceeded(mApp->mOpeningURL);
			}
			break;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE:
			if ((!gInAssert) && (!mApp->mSEHOccured))
			{
				int x = (short)LOWORD(lParam);
				int y = (short)HIWORD(lParam);
				mApp->mWidgetManager->RemapMouse(x, y);

				mApp->mLastUserInputTick = mApp->mLastTimerTime;

				mApp->mWidgetManager->MouseMove(x, y);

				if (!mApp->mMouseIn)
				{
					//More demo code gone

					mApp->mMouseIn = true;
					EnforceCursor();
				}

				switch (uMsg)
				{
				case WM_LBUTTONDOWN:
					SetCapture(hWnd);
					if (mApp->mShowWidgetInspector)
					{
						mApp->mWidgetInspectorClickPos.mX = x - mApp->mScreenBounds.mX;
						mApp->mWidgetInspectorClickPos.mY = y - mApp->mScreenBounds.mY; //?
					}
					else if ((mApp->mWidgetManager != NULL) && (mApp->mWidgetManager->mKeyDown[KEYCODE_CONTROL]) && (mApp->mWidgetManager->mKeyDown[KEYCODE_MENU]))
					{
						if (!gTracingPixels)
						{
							mApp->mWidgetManager->MarkAllDirty();
							PixelTracerStart(x - mApp->mScreenBounds.mX, y - mApp->mScreenBounds.mY);
						}
					}
					mApp->mWidgetManager->MouseDown(x, y, 1);
					break;
				case WM_RBUTTONDOWN:
					SetCapture(hWnd);
					mApp->mWidgetManager->MouseDown(x, y, -1);
					break;
				case WM_MBUTTONDOWN:
					SetCapture(hWnd);
					mApp->mWidgetManager->MouseDown(x, y, 3);
					break;
				case WM_LBUTTONDBLCLK:
					SetCapture(hWnd);
					mApp->mWidgetManager->MouseDown(x, y, 2);
					break;
				case WM_RBUTTONDBLCLK:
					SetCapture(hWnd);
					mApp->mWidgetManager->MouseDown(x, y, -2);
					break;
				case WM_LBUTTONUP:
					if ((mApp->mWidgetManager->mDownButtons & ~1) == 0)
						ReleaseCapture();
					mApp->mWidgetManager->MouseUp(x, y, 1);
					break;
				case WM_RBUTTONUP:
					if ((mApp->mWidgetManager->mDownButtons & ~2) == 0)
						ReleaseCapture();
					mApp->mWidgetManager->MouseUp(x, y, -1);
					break;
				case WM_MBUTTONUP:
					if ((mApp->mWidgetManager->mDownButtons & ~4) == 0)
						ReleaseCapture();
					mApp->mWidgetManager->MouseUp(x, y, 3);
					break;
				}
			}
			break;
		case WM_MOUSEWHEEL:
		{
			char aZDelta = ((short)HIWORD(wParam)) / 120;
			if (mApp->mShowWidgetInspector)
				mApp->mWidgetInspectorScrollOffset -= aZDelta;
			else
				mApp->mWidgetManager->MouseWheel(aZDelta);
		}
		break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			mApp->mLastUserInputTick = mApp->mLastTimerTime;

			if (wParam == VK_RETURN && uMsg == WM_SYSKEYDOWN && !mApp->mForceFullscreen && !mApp->mForceWindowed && mApp->mAllowAltEnter)
			{
				mApp->SwitchScreenMode(!mApp->mIsWindowed);
				ClearKeysDown();
				break;
			}
			else if ((wParam == 'D') && (mApp->mWidgetManager != NULL) && (mApp->mWidgetManager->mKeyDown[KEYCODE_CONTROL]) && (mApp->mWidgetManager->mKeyDown[KEYCODE_MENU]))
			{
				PlaySoundA("c:\\windows\\media\\Windows XP Menu Command.wav", NULL, SND_ASYNC);
				mApp->mDebugKeysEnabled = !mApp->mDebugKeysEnabled;
			}

			/*if (mApp->mDebugKeysEnabled)
			{
				if (DebugKeyDown(wParam))
					break;
			}*/

			mApp->mWidgetManager->KeyDown((KeyCode)wParam);
			break;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			mApp->mLastUserInputTick = mApp->mLastTimerTime;
			mApp->mWidgetManager->KeyUp((KeyCode)wParam);
			break;
		case WM_CHAR:
			mApp->mLastUserInputTick = mApp->mLastTimerTime;
			mApp->mWidgetManager->KeyChar((SexyChar)wParam);
			break;
		case WM_MOVE:
		{
			if ((hWnd == mApp->mHWnd) && (mApp->mIsWindowed))
			{
				WINDOWPLACEMENT aWindowPlacment;
				aWindowPlacment.length = sizeof(aWindowPlacment);

				GetWindowPlacement(hWnd, &aWindowPlacment);
				if ((aWindowPlacment.showCmd == SW_SHOW) ||
					(aWindowPlacment.showCmd == SW_SHOWNORMAL))
				{
					mApp->mPreferredX = aWindowPlacment.rcNormalPosition.left;
					mApp->mPreferredY = aWindowPlacment.rcNormalPosition.top;
				}
			}
		}
		break;
		case WM_SIZE:
		{
			bool isMinimized = wParam == SIZE_MINIMIZED;
			int aNewWidth = lParam;
			int aNewHeight = HIWORD(lParam); //?

			if ((hWnd == mApp->mHWnd) && (!mApp->mShutdown) && (isMinimized != mApp->mMinimized))
			{
				mApp->mMinimized = isMinimized;

				// We don't want any sounds (or music) playing while its minimized
				if (mApp->mMinimized)
				{
					if (mApp->mMuteOnLostFocus)
						mApp->Mute(true);
				}
				else
				{
					if (mApp->mMuteOnLostFocus)
						mApp->Unmute(true);

					mApp->mWidgetManager->MarkAllDirty();
				}
			}

			RehupFocus();
			if (wParam == SIZE_MAXIMIZED)
				mApp->SwitchScreenMode(false);
			else if (wParam != FALSE && hWnd == mApp->mHWnd && mApp->mIsWindowed)
			{
				mApp->mPreferredWidth = aNewWidth;
				mApp->mPreferredHeight = aNewHeight;
			}
		}
		break;
		case WM_TIMER:
			if ((!gInAssert) && (!mApp->mSEHOccured) && (mApp->mRunning))
			{
				DWORD aTimeNow = GetTickCount();
				if (aTimeNow - mApp->mLastTimerTime > 500)
					mApp->mLastBigDelayTime = aTimeNow;

				mApp->mLastTimerTime = aTimeNow;

				if ((mApp->mIsOpeningURL) &&
					(aTimeNow - mApp->mLastBigDelayTime > 5000))
				{
					if ((aTimeNow - mApp->mOpeningURLTime > 8000) && (!mApp->mActive))
					{
						//TODO: Have some demo message thing
						mApp->URLOpenSucceeded(mApp->mOpeningURL);
					}
					else if ((aTimeNow - mApp->mOpeningURLTime > 12000) && (mApp->mActive))
					{
						mApp->URLOpenFailed(mApp->mOpeningURL);
					}
				}

				RECT aRect;
				GetClientRect(hWnd, &aRect);

				POINT aULCorner = { 0, 0 };
				::ClientToScreen(hWnd, &aULCorner);

				POINT aBRCorner = { aRect.right }; //?
				::ClientToScreen(hWnd, &aBRCorner);

				POINT aPoint;
				::GetCursorPos(&aPoint);

				HWND aWindow = ::WindowFromPoint(aPoint);
				bool isMouseIn = (aWindow == hWnd) &&
					(aPoint.x >= aULCorner.x) && (aPoint.y >= aULCorner.y) &&
					(aPoint.x < aBRCorner.x) && (aPoint.y < aBRCorner.y);

				if (mApp->mMouseIn != isMouseIn)
				{
					//More demo code gone

					if (!isMouseIn)
					{
						int x = aPoint.x - aULCorner.x;
						int y = aPoint.y - aULCorner.y;
						mApp->mWidgetManager->RemapMouse(x, y);
						mApp->mWidgetManager->MouseExit(x, y);
					}

					mApp->mMouseIn = isMouseIn;
					EnforceCursor();
				}
			}
			break;

		case WM_SYSCOLORCHANGE:
		case WM_DISPLAYCHANGE:
			mApp->mWidgetManager->SysColorChangedAll();
			mApp->mWidgetManager->MarkAllDirty();
			break;
		}

		switch (uMsg)
		{
		case WM_CLOSE:
			if ((hWnd == mApp->mHWnd) || (hWnd == mApp->mInvisHWnd))
			{
				// This should short-circuit all demo calls, otherwise we will get
				//  all sorts of weird asserts because we are changing
				//  program flow
				//mApp->mManualShutdown = true;

				Shutdown();
			}
			break;
		}

		if (singleMessage)
			break;
	}

	return (mApp->mDeferredMessages.size() > 0);
}

void WindowsAppDriver::Done3dTesting() //4133-4134
{
}

// return file name that you want to upload
std::string	WindowsAppDriver::NotifyCrashHook() //Identical to SAB, why | 4138-4140
{
	return "";
}

void WindowsAppDriver::MakeWindow() //TODO FIX | 4143-4486
{
	//OutputDebugString("MAKING WINDOW\r\n");

	bool wasActive = false;
	if (mApp->mHWnd != NULL)
	{
		SetWindowLong(mApp->mHWnd, GWL_USERDATA, NULL);
		HWND anOldWindow = mApp->mHWnd;
		mApp->mHWnd = NULL;
		DestroyWindow(anOldWindow);
		mApp->mWidgetManager->mImage = NULL;
	}


	if (mApp->mIsWindowed && !mApp->mFullScreenWindow)
	{
		DWORD aWindowStyle = WS_CLIPCHILDREN | WS_POPUP | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
		if (mApp->mEnableMaximizeButton)
			aWindowStyle |= WS_MAXIMIZEBOX;

		RECT aRect;
		aRect.left = 0;
		aRect.top = 0;
		aRect.right = mApp->mWidth;
		aRect.bottom = mApp->mHeight;

		BOOL worked = AdjustWindowRect(&aRect, aWindowStyle, FALSE);

		int aWidth = aRect.right - aRect.left;
		int aHeight = aRect.bottom - aRect.top;

		// Get the work area of the desktop to allow us to center
		RECT aDesktopRect;
		::SystemParametersInfo(SPI_GETWORKAREA, NULL, &aDesktopRect, NULL);

		int aPlaceX = 64;
		int aPlaceY = 64;

		if (mApp->mPreferredX != -1)
		{
			aPlaceX = mApp->mPreferredX;
			aPlaceY = mApp->mPreferredY;

			int aSpacing = 4;

			if (aPlaceX < aDesktopRect.left + aSpacing)
				aPlaceX = aDesktopRect.left + aSpacing;

			if (aPlaceY < aDesktopRect.top + aSpacing)
				aPlaceY = aDesktopRect.top + aSpacing;

			if (aPlaceX + aWidth >= aDesktopRect.right - aSpacing)
				aPlaceX = aDesktopRect.right - aWidth - aSpacing;

			if (aPlaceY + aHeight >= aDesktopRect.bottom - aSpacing)
				aPlaceY = aDesktopRect.bottom - aHeight - aSpacing;
		}

		if (CheckFor98Mill())
		{
			mApp->mHWnd = CreateWindowExA(
				0,
				"MainWindow",
				SexyStringToStringFast(mApp->mTitle).c_str(),
				aWindowStyle,
				aPlaceX,
				aPlaceY,
				aWidth,
				aHeight,
				NULL,
				NULL,
				gHInstance,
				0);
		}
		else
		{
			mApp->mHWnd = CreateWindowEx(
				0,
				_S("MainWindow"),
				mApp->mTitle.c_str(),
				aWindowStyle,
				aPlaceX,
				aPlaceY,
				aWidth,
				aHeight,
				NULL,
				NULL,
				gHInstance,
				0);
		}

		if (mApp->mPreferredX == -1)
		{
			::MoveWindow(mApp->mHWnd,
				aDesktopRect.left + ((aDesktopRect.right - aDesktopRect.left) - aWidth) / 2,
				aDesktopRect.top + (int)(((aDesktopRect.bottom - aDesktopRect.top) - aHeight) * 0.382),
				aWidth, aHeight, FALSE);
		}

		mApp->mIsPhysWindowed = true;
	}
	else
	{
		if (CheckFor98Mill())
		{
			mApp->mHWnd = CreateWindowExA(
				WS_EX_TOPMOST,
				"MainWindow",
				SexyStringToStringFast(mApp->mTitle).c_str(),
				WS_POPUP | WS_VISIBLE,
				0,
				0,
				mApp->mWidth,
				mApp->mHeight,
				NULL,
				NULL,
				gHInstance,
				0);
		}
		else
		{
			mApp->mHWnd = CreateWindowEx(
				WS_EX_TOPMOST,
				_S("MainWindow"),
				mApp->mTitle.c_str(),
				WS_POPUP | WS_VISIBLE,
				0,
				0,
				mApp->mWidth,
				mApp->mHeight,
				NULL,
				NULL,
				gHInstance,
				0);
		}

		mApp->mIsPhysWindowed = false;
	}

	/*char aStr[256];
	sprintf(aStr, "HWND: %d\r\n", mHWnd);
	OutputDebugString(aStr);*/

	SetWindowLong(mApp->mHWnd, GWL_USERDATA, (LONG)this);

	if (mWindowsGraphicsDriver == NULL)
	{
		mWindowsGraphicsDriver = new WindowsGraphicsDriver(mApp);

		// Enable 3d setting
		bool is3D = false;
		bool is3DOptionSet = mApp->RegistryReadBoolean("Is3D", &is3D);
		if (is3DOptionSet)
		{
			if (mApp->mAutoEnable3D)
			{
				mApp->mAutoEnable3D = false;
				mApp->mTest3D = true;
			}

			if (is3D)
				mApp->mTest3D = true;

			mWindowsGraphicsDriver->mIs3D = is3D;
		}
	}

	int aResult = InitGraphicsInterface();

	if ((mApp->mIsWindowed) && (aResult == WindowsGraphicsDriver::RESULT_INVALID_COLORDEPTH))
	{
		if (mApp->mForceWindowed)
		{
			Popup(mApp->GetString("PLEASE_SET_COLOR_DEPTH", _S("Please set your desktop color depth to 16 bit.")));
			DoExit(1);
		}
		else
		{
			mApp->mForceFullscreen = true;
			mApp->SwitchScreenMode(false);
		}
		return;
	}
	else if ((!mApp->mIsWindowed) &&
		((aResult == WindowsGraphicsDriver::RESULT_EXCLUSIVE_FAIL) ||
			(aResult == WindowsGraphicsDriver::RESULT_DISPCHANGE_FAIL)))
	{
		mApp->mForceWindowed = true;
		mApp->SwitchScreenMode(true);
	}
	else if (aResult == WindowsGraphicsDriver::RESULT_3D_FAIL)
	{
		Set3DAcclerated(false);
		return;
	}
	else if (aResult != WindowsGraphicsDriver::RESULT_OK)
	{
		if (Is3DAccelerated())
		{
			Set3DAcclerated(false);
			return;
		}
		else
		{
			Popup(mApp->GetString("FAILED_INIT_DIRECTDRAW", _S("Failed to initialize DirectDraw: ")) + StringToSexyString(WindowsGraphicsDriver::ResultToString(aResult) + " " + mWindowsGraphicsDriver->mErrorString));
			DoExit(1);
		}
	}

	if (wasActive)
		::SetFocus(mApp->mHWnd);
	bool isActive = mApp->mActive;
	mApp->mActive = GetActiveWindow() == mApp->mHWnd;

	mApp->mPhysMinimized = false;
	if (mApp->mMinimized)
	{
		if (mApp->mMuteOnLostFocus)
			mApp->Unmute(true);

		mApp->mMinimized = false;
		isActive = mApp->mActive; // set this here so we don't call RehupFocus again.
		RehupFocus();
	}

	if (isActive != mApp->mActive)
		RehupFocus();

	mApp->ReInitImages();

	mApp->mWidgetManager->mImage = mWindowsGraphicsDriver->GetScreenImage();
	mApp->mWidgetManager->MarkAllDirty();

	SetTimer(mApp->mHWnd, 100, mApp->mFrameTime, NULL);
}

void WindowsAppDriver::LoadingThreadProcStub(void* theArg) //TODO | 4491-4501
{
	SexyAppBase* aSexyApp = (SexyAppBase*)theArg;

	aSexyApp->LoadingThreadProc();

	char aStr[256];
	sprintf(aStr, "Resource Loading Time: %d\r\n", (GetTickCount() - aSexyApp->mTimeLoaded));
	OutputDebugStringA(aStr);

	aSexyApp->mLoadingThreadCompleted = true;
}

void WindowsAppDriver::StartLoadingThread() //4504-4512
{
	if (!mApp->mLoadingThreadStarted)
	{
		mApp->mYieldMainThread = true;
		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
		mApp->mLoadingThreadStarted = true;
		_beginthread(LoadingThreadProcStub, 0, this);
	}
}

void WindowsAppDriver::CursorThreadProc() //4514-4560
{
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	POINT aLastCursorPos = { 0, 0 };
	int aLastDrawCount = 0;

	while (!mApp->mShutdown)
	{
		//		if (mProcessInTimer)
		//			PostMessage(mHWnd,WM_TIMER,101,0);

		POINT aCursorPos;

		//{
		::GetCursorPos(&aCursorPos);
		::ScreenToClient(mApp->mHWnd, &aCursorPos);
		//}

		if (aLastDrawCount != mApp->mDrawCount)
		{
			// We did a draw so we may have committed a pending mNextCursorX/Y 
			aLastCursorPos.x = mWindowsGraphicsDriver->mCursorX;
			aLastCursorPos.y = mWindowsGraphicsDriver->mCursorY;
		}

		if ((aCursorPos.x != aLastCursorPos.x) ||
			(aCursorPos.y != aLastCursorPos.y))
		{
			DWORD aTimeNow = timeGetTime();
			if (gIsFailing || aTimeNow - mApp->mNextDrawTick > mWindowsGraphicsDriver->mMillisecondsPerFrame + 5)
			{
				// Set them up to get assigned in the next screen redraw
				mWindowsGraphicsDriver->mNextCursorX = aCursorPos.x;
				mWindowsGraphicsDriver->mNextCursorY = aCursorPos.y;
			}
			else
			{
				// Do the special drawing if we are rendering at less than full framerate				
				mWindowsGraphicsDriver->SetCursorPos(aCursorPos.x, aCursorPos.y);
				aLastCursorPos = aCursorPos;
			}
		}

		Sleep(10);
	}

	mApp->mCursorThreadRunning = false;
}

void WindowsAppDriver::CursorThreadProcStub(void* theArg) //4563-4567
{
	CoInitialize(NULL);
	WindowsAppDriver* driver = (WindowsAppDriver*)theArg;
	driver->CursorThreadProc();
}

void WindowsAppDriver::StartCursorThread() //4570-4576
{
	if (!mApp->mCursorThreadRunning)
	{
		mApp->mCursorThreadRunning = true;
		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
		_beginthread(CursorThreadProcStub, 0, this);
	}
}

void WindowsAppDriver::SwitchScreenMode(bool wantWindowed, bool is3d, bool force) //4579-4619
{
	if (mApp->mForceFullscreen)
		wantWindowed = false;

	if (mApp->mIsWindowed == wantWindowed && !force)
	{
		Set3DAcclerated(is3d);
		return;
	}

	// Set 3d acceleration preference
	Set3DAcclerated(is3d, false);

	// Always make the app windowed when playing demos, in order to
	//  make it easier to track down bugs.  We place this after the
	//  sanity check just so things get re-initialized and stuff
	//if (mPlayingDemoBuffer)
	//	wantWindowed = true;

	mApp->mIsWindowed = wantWindowed;

	MakeWindow();

	// We need to do this check to allow IE to get focus instead of
	//  stealing it away for ourselves
	if (!mApp->mIsOpeningURL)
	{
		::ShowWindow(mApp->mHWnd, SW_NORMAL);
		::SetForegroundWindow(mApp->mHWnd);
	}
	else
	{
		// Show it but don't activate it
		::ShowWindow(mApp->mHWnd, SW_SHOWNOACTIVATE);
	}

	mApp->mLastTime = timeGetTime();
}

void WindowsAppDriver::SwitchScreenMode(bool wantWindowed) //4622-4624
{
	SwitchScreenMode(wantWindowed, Is3DAccelerated());
}

void WindowsAppDriver::SwitchScreenMode() //4627-4629
{
	SwitchScreenMode(mApp->mIsWindowed, Is3DAccelerated(), true);
}

void WindowsAppDriver::EnforceCursor() //TODO FIX | 4632-4704
{
	if (mApp->mIsSizeCursor) //?
		return;

	bool wantSysCursor = true;

	if (mWindowsGraphicsDriver == NULL)
		return;

	if ((mApp->mSEHOccured) || (!mApp->mMouseIn))
	{
		::SetCursor(::LoadCursor(NULL, IDC_ARROW));
		if (mWindowsGraphicsDriver->SetCursorImage(NULL))
			mApp->mCustomCursorDirty = true;
	}
	else
	{
		if ((mApp->mCursorImages[mApp->mCursorNum] == NULL) ||
			((!mApp->mCustomCursorsEnabled) && (mApp->mCursorNum != CURSOR_CUSTOM)))
		{
			if (mApp->mOverrideCursor != NULL)
				::SetCursor(mApp->mOverrideCursor);
			else if (mApp->mCursorNum == CURSOR_POINTER)
				::SetCursor(::LoadCursor(NULL, IDC_ARROW));
			else if (mApp->mCursorNum == CURSOR_HAND)
				::SetCursor(mApp->mHandCursor);
			else if (mApp->mCursorNum == CURSOR_TEXT)
				::SetCursor(::LoadCursor(NULL, IDC_IBEAM));
			else if (mApp->mCursorNum == CURSOR_DRAGGING)
				::SetCursor(mApp->mDraggingCursor);
			else if (mApp->mCursorNum == CURSOR_CIRCLE_SLASH)
				::SetCursor(::LoadCursor(NULL, IDC_NO));
			else if (mApp->mCursorNum == CURSOR_SIZEALL)
				::SetCursor(::LoadCursor(NULL, IDC_SIZEALL));
			else if (mApp->mCursorNum == CURSOR_SIZENESW)
				::SetCursor(::LoadCursor(NULL, IDC_SIZENESW));
			else if (mApp->mCursorNum == CURSOR_SIZENS)
				::SetCursor(::LoadCursor(NULL, IDC_SIZENS));
			else if (mApp->mCursorNum == CURSOR_SIZENWSE)
				::SetCursor(::LoadCursor(NULL, IDC_SIZENWSE));
			else if (mApp->mCursorNum == CURSOR_SIZEWE)
				::SetCursor(::LoadCursor(NULL, IDC_SIZEWE));
			else if (mApp->mCursorNum == CURSOR_WAIT)
				::SetCursor(::LoadCursor(NULL, IDC_WAIT));
			else if (mApp->mCursorNum == CURSOR_CUSTOM)
				::SetCursor(NULL); // Default to not showing anything
			else if (mApp->mCursorNum == CURSOR_NONE)
				::SetCursor(NULL);
			else
				::SetCursor(::LoadCursor(NULL, IDC_ARROW));

			if (mWindowsGraphicsDriver->SetCursorImage(NULL))
				mApp->mCustomCursorDirty = true;
		}
		else
		{
			if (mWindowsGraphicsDriver->SetCursorImage(mApp->mCursorImages[mApp->mCursorNum]))
				mApp->mCustomCursorDirty = true;

			::SetCursor(NULL); //?
			wantSysCursor = false;
		}
	}

	if (wantSysCursor != mApp->mSysCursor)
	{
		mApp->mSysCursor = wantSysCursor;

		// Don't hide the hardware cursor when playing back a demo buffer
//		if (!mPlayingDemoBuffer)
//			::ShowCursor(mSysCursor);
	}
}

void WindowsAppDriver::UpdateFTimeAcc() //4708-4722
{
	DWORD aCurTime = timeGetTime();

	if (mApp->mLastTimeCheck != 0)
	{
		int aDeltaTime = aCurTime - mApp->mLastTimeCheck;

		mApp->mUpdateFTimeAcc = min(mApp->mUpdateFTimeAcc + aDeltaTime, 200.0, mApp->mMaxUpdateBacklog); //?

		if (mApp->mRelaxUpdateBacklogCount > 0)
			mApp->mRelaxUpdateBacklogCount = max(mApp->mRelaxUpdateBacklogCount - aDeltaTime, 0);
	}

	mApp->mLastTimeCheck = aCurTime;
}

//int aNumCalls = 0;
//DWORD aLastCheck = 0;

bool WindowsAppDriver::Process(bool allowSleep) //4728-4920
{
	/*DWORD aTimeNow = GetTickCount();
	if (aTimeNow - aLastCheck >= 10000)
	{
		OutputDebugString(StrFormat(_S("FUpdates: %d\n"), aNumCalls).c_str());
		aLastCheck = aTimeNow;
		aNumCalls = 0;
	}*/

	if (mApp->mLoadingFailed)
		mApp->Shutdown();

	bool isVSynched = /*(!mPlayingDemoBuffer) &&*/ (mApp->mVSyncUpdates) && (!mApp->mLastDrawWasEmpty) && (!mApp->mVSyncBroken) &&
		(!mApp->mIsPhysWindowed || mApp->mWaitForVSync && !mApp->mSoftVSyncWait); //obsoleted demo
	double aFrameFTime;
	double anUpdatesPerUpdateF;

	if (mApp->mVSyncUpdates)
	{
		aFrameFTime = (1000.0 / mApp->mSyncRefreshRate) / mApp->mUpdateMultiplier;
		anUpdatesPerUpdateF = (float)(1000.0 / (mApp->mFrameTime * mApp->mSyncRefreshRate));
	}
	else
	{
		aFrameFTime = mApp->mFrameTime / mApp->mUpdateMultiplier;
		anUpdatesPerUpdateF = 1.0;
	}

	//Removed demo code check 1.30

	// Make sure we're not paused
	if ((!mApp->mPaused) && (mApp->mUpdateMultiplier > 0))
	{
		ulong aStartTime = timeGetTime();

		ulong aCurTime = aStartTime;
		int aCumSleepTime = 0;

		// When we are VSynching, only calculate this FTimeAcc right after drawing

		if (!isVSynched)
			UpdateFTimeAcc();

		// mNonDrawCount is used to make sure we draw the screen at least
		// 10 times per second, even if it means we have to slow down
		// the updates to make it draw 10 times per second in "game time"

		bool didUpdate = false;

		if (mApp->mUpdateAppState == UPDATESTATE_PROCESS_1)
		{
			if ((++mApp->mNonDrawCount < (int)ceil(mApp->mMaxNonDrawCount * mApp->mUpdateMultiplier)) || (!mApp->mLoaded)) //Changed 10 to mMaxNonDrawCount
			{
				bool doUpdate = false;

				if (isVSynched)
				{
					// Synch'ed to vertical refresh, so update as soon as possible after draw
					doUpdate = (!mApp->mHasPendingDraw) || (mApp->mUpdateFTimeAcc >= (int)(aFrameFTime * 0.75));
				}
				else if (mApp->mUpdateFTimeAcc >= aFrameFTime)
				{
					doUpdate = true;
				}

				if (doUpdate)
				{
					// Do VSyncBroken test.  This test fails if we're in fullscreen and
					// "don't vsync" has been forced in Advanced settings up Display Properties
					if (mApp->mUpdateMultiplier == 1.0) //Removed demo code
					{
						mApp->mVSyncBrokenTestUpdates++;
						if (mApp->mVSyncBrokenTestUpdates >= (DWORD)((1000 + mApp->mFrameTime - 1) / mApp->mFrameTime)) //or 999
						{
							// It has to be running 33% fast to be "broken" (25% = 1/0.800)
							if (aStartTime - mApp->mVSyncBrokenTestStartTick <= 800) //Was this changed to greater to, prob not according to WP7
							{
								// The test has to fail 3 times in a row before we decide that
								//  vsync is broken overall
								mApp->mVSyncBrokenCount++;
								if (mApp->mVSyncBrokenCount >= 3)
									mApp->mVSyncBroken = true;
							}
							else
								mApp->mVSyncBrokenCount = 0;

							mApp->mVSyncBrokenTestStartTick = aStartTime;
							mApp->mVSyncBrokenTestUpdates = 0;
						}
					}

					bool hadRealUpdate = DoUpdateFrames();
					if (hadRealUpdate)
						mApp->mUpdateAppState = UPDATESTATE_PROCESS_2;
					mApp->mHasPendingDraw = true;
					didUpdate = true;
				}
			}
		}
		else if (mApp->mUpdateAppState == UPDATESTATE_PROCESS_2)
		{
			mApp->mUpdateAppState = UPDATESTATE_PROCESS_DONE;

			mApp->mPendingUpdatesAcc += anUpdatesPerUpdateF;
			mApp->mPendingUpdatesAcc -= 1.0;
			mApp->ProcessSafeDeleteList();

			// Process any extra updates
			while (mApp->mPendingUpdatesAcc >= 1.0)
			{
				// These should just be IDLE commands we have to clear out
				//Removed demo code
				++mApp->mNonDrawCount;
				bool hasRealUpdate = DoUpdateFrames();
				DBG_ASSERTE(hasRealUpdate); //4842

				if (!hasRealUpdate)
					break;

				mApp->ProcessSafeDeleteList();
				mApp->mPendingUpdatesAcc -= 1.0;
			}

			//aNumCalls++;
			//Does something go here
			DoUpdateFramesF((float)anUpdatesPerUpdateF);
			mApp->ProcessSafeDeleteList();

			// Don't let mUpdateFTimeAcc dip below 0
			//  Subtract an extra 0.2ms, because sometimes refresh rates have some
			//  fractional component that gets truncated, and it's better to take off
			//  too much to keep our timing tending toward occuring right after 
			//  redraws
			//Was this changed
			if (isVSynched)
				mApp->mUpdateFTimeAcc = max(mApp->mUpdateFTimeAcc - aFrameFTime - 0.2f, 0.0);
			else
				mApp->mUpdateFTimeAcc -= aFrameFTime;

			if (mApp->mRelaxUpdateBacklogCount > 0)
				mApp->mUpdateFTimeAcc = 0;

			didUpdate = true;
		}

		if (!didUpdate)
		{
			mApp->mUpdateAppState = UPDATESTATE_PROCESS_DONE;

			mApp->mNonDrawCount = 0;

			if (mApp->mHasPendingDraw)
			{
				DrawDirtyStuff();
			}
			else
			{
				// Let us take into account the time it took to draw dirty stuff			
				int aTimeToNextFrame = (int)(aFrameFTime - mApp->mUpdateFTimeAcc);
				if (aTimeToNextFrame > 0)
				{
					if (!allowSleep)
						return false;

					// Wait till next processing cycle
					++mApp->mSleepCount;
					Sleep(aTimeToNextFrame);

					aCumSleepTime += aTimeToNextFrame;
				}
			}
		}

		if (mApp->mYieldMainThread)
		{
			// This is to make sure that the title screen doesn't take up any more than 
			// 1/3 of the processor time

			ulong anEndTime = timeGetTime();
			int anElapsedTime = (anEndTime - aStartTime) - aCumSleepTime;
			int aLoadingYieldSleepTime = min(250, (anElapsedTime * 2) - aCumSleepTime);

			if (aLoadingYieldSleepTime >= 0)
			{
				if (!allowSleep)
					return false;

				Sleep(aLoadingYieldSleepTime);
			}
		}
	}

	mApp->ProcessSafeDeleteList();
	return true;
}

void WindowsAppDriver::DoMainLoop() //4924-4931
{
	while (!mApp->mShutdown)
	{
		if (mApp->mExitToTop)
			mApp->mExitToTop = false;
		UpdateAppStep(false);
	}
}

bool WindowsAppDriver::UpdateAppStep(bool* updated) //4934-4993
{
	if (updated != NULL)
		*updated = false;

	if (mApp->mExitToTop)
		return false;

	if (mApp->mUpdateAppState == UPDATESTATE_PROCESS_DONE)
		mApp->mUpdateAppState = UPDATESTATE_MESSAGES;

	mApp->mUpdateAppDepth++;

	// We update in two stages to avoid doing a Process if our loop termination
	//  condition has already been met by processing windows messages		
	if (mApp->mUpdateAppState == UPDATESTATE_MESSAGES)
	{
		MSG msg;
		while ((PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) && (!mApp->mShutdown))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		//Bye bye demo
		if (!ProcessDeferredMessages(true))
		{
			mApp->mUpdateAppState = UPDATESTATE_PROCESS_1;
		}
	}
	else
	{
		// Process changes state by itself
		if (mApp->mStepMode)
		{
			if (mApp->mStepMode == 2)
			{
				Sleep(mApp->mFrameTime);
				mApp->mUpdateAppState = UPDATESTATE_PROCESS_DONE; // skip actual update until next step
			}
			else
			{
				mApp->mStepMode = 2;
				DoUpdateFrames();
				DoUpdateFramesF(1.0f);
				DrawDirtyStuff();
			}
		}
		else
		{
			int anOldUpdateCnt = mApp->mUpdateCount;
			Process();
			if (updated != NULL)
				*updated = mApp->mUpdateCount != anOldUpdateCnt;
		}
	}

	mApp->mUpdateAppDepth--;

	return true;
}


int WindowsAppDriver::InitGraphicsInterface() //TODO | 4997-5030
{
	mApp->PreDDInterfaceInitHook();
	AutoCrit anAutoCrit(mWindowsGraphicsDriver->mCritSect);
	mApp->DeleteNativeImageData();
	int aResult = mWindowsGraphicsDriver->Init(mApp->mHWnd, mApp->mIsPhysWindowed);
	if (WindowsGraphicsDriver::RESULT_OK == aResult)
	{
		if (mApp->mWidescreenTranslate)
		{
			mApp->mScreenBounds.mX = (mApp->mWidth - mWindowsGraphicsDriver->mWidth) / 2;
			mApp->mScreenBounds.mY = (mApp->mHeight - mWindowsGraphicsDriver->mHeight) / 2;
		}
		else
		{
			mApp->mScreenBounds.mX = 0;
			mApp->mScreenBounds.mY = 0;
		}
		mApp->mScreenBounds.mWidth = mWindowsGraphicsDriver->mWidth;
		mApp->mScreenBounds.mHeight = mWindowsGraphicsDriver->mHeight;
		mApp->mWidgetManager->Resize(mApp->mScreenBounds, mWindowsGraphicsDriver->mPresentationRect);
		if (mWindowsGraphicsDriver && mWindowsGraphicsDriver->mRenderDevice3D && (mWindowsGraphicsDriver->mRenderDevice3D->GetCapsFlags() & RenderDevice3D::CAPF_AutoWindowedVSync) != 0)
		{
			mApp->mSoftVSyncWait = false;
		}
		mApp->PostDDInterfaceInitHook();
	}
	return aResult;
}

void WindowsAppDriver::Start() //5034-5091
{
	if (mApp->mShutdown)
		return;

	StartCursorThread();

	if (mApp->mAutoStartLoadingThread)
		StartLoadingThread();

	::ShowWindow(mApp->mHWnd, SW_SHOW);
	::SetFocus(mApp->mHWnd);

	timeBeginPeriod(1);

	int aCount = 0;
	int aSleepCount = 0;

	DWORD aStartTime = timeGetTime();

	mApp->mRunning = true;
	mApp->mLastTime = aStartTime;
	mApp->mLastUserInputTick = aStartTime;
	mApp->mLastTimerTime = aStartTime;

	DoMainLoop();
	mApp->ProcessSafeDeleteList();

	mApp->mRunning = false;

	WaitForLoadingThread();

	char aString[256];
	sprintf(aString, "Seconds       = %g\r\n", (timeGetTime() - aStartTime) / 1000.0);
	OutputDebugStringA(aString);
	//sprintf(aString, "Count         = %d\r\n", aCount);
	//OutputDebugString(aString);
	sprintf(aString, "Sleep Count   = %d\r\n", mApp->mSleepCount);
	OutputDebugStringA(aString);
	sprintf(aString, "Update Count  = %d\r\n", mApp->mUpdateCount);
	OutputDebugStringA(aString);
	sprintf(aString, "Draw Count    = %d\r\n", mApp->mDrawCount);
	OutputDebugStringA(aString);
	sprintf(aString, "Draw Time     = %d\r\n", mApp->mDrawTime);
	OutputDebugStringA(aString);
	sprintf(aString, "Screen Blt    = %d\r\n", mApp->mScreenBltTime);
	OutputDebugStringA(aString);
	if (mApp->mDrawTime + mApp->mScreenBltTime > 0)
	{
		sprintf(aString, "Avg FPS       = %d\r\n", (mApp->mDrawCount * 1000) / (mApp->mDrawTime + mApp->mScreenBltTime));
		OutputDebugStringA(aString);
	}

	timeEndPeriod(1);

	mApp->PreTerminate();

	mApp->WriteToRegistry();
}

bool WindowsAppDriver::CheckSignature(const Buffer& theBuffer, const std::string& theFileName) //5094-5097
{
	// Add your own signature checking code here
	return false;
}

#ifndef _SEXYDECOMP_DUMMY_USELESS_CODE
void WindowsAppDriver::ReloadAllResources_DrawStateUpdate(const std::string& theHeader, const std::string& theSubText, float thePct) //Almost, sets mImage to mApp->mWidgetManager->mImage in XNA | 5101-5136
{
	if (mApp->mWidgetManager->mImage)
	{
		static SysFont aBigFont("Arial", 14);
		static SysFont aSmallFont("Arial", 10);
		Graphics g(mApp->mWidgetManager->mImage);
		g.SetColor(Color::White);
		g.FillRect(mApp->mWidgetManager->mImage->mWidth / 2 - 150, mApp->mWidgetManager->mImage->mHeight / 2 - 75, 300, 150);
		g.SetColor(0);
		g.FillRect(mApp->mWidgetManager->mImage->mWidth / 2 - 148, mApp->mWidgetManager->mImage->mHeight / 2 - 73, 296, 146);
		g.SetColor(Color::White);
		g.SetFont(&aBigFont);
		g.WriteString(ToSexyString(theHeader), mApp->mWidgetManager->mImage->mWidth / 2, mApp->mWidgetManager->mImage->mHeight / 2 - 30, 0);
		g.SetFont(&aBigFont);
		g.WriteString(StringToSexyStringFast(theSubText), mApp->mWidgetManager->mImage->mWidth / 2, mApp->mWidgetManager->mImage->mHeight / 2, 0);
		Rect aDoneRect(mApp->mWidgetManager->mImage->mWidth / 2 - 125, mApp->mWidgetManager->mImage->mHeight / 2 + 20, 250, 20);
		g.SetColor(Color::White);
		//g.FillRect(aDoneRect.mX, aDoneRect.mY,2,2); //?
		g.SetColor(Color(32, 32, 32));
		g.FillRect(aDoneRect);
		g.SetColor(Color(128, 128, 128));
		aDoneRect.mWidth *= thePct;
		g.FillRect(aDoneRect);
		if (!gSexyCache.Connected())
		{
			g.SetColor(Color::Red);
			g.WriteString(_S("Performance warning: not loaded with SexyCache"), mApp->mWidgetManager->mImage->mWidth / 2, mApp->mWidgetManager->mImage->mHeight / 2 + 65, 0);
		}
		InvalidateRect(mApp->mHWnd, NULL, false);
	}
}



void WindowsAppDriver::ReloadAllResourcesProc() //TODO | 5146-5414
{
	int aTotalCount;
	int aDoneCount;
	StringIntMap aTypeCountMap;
	StringIntMap aTypeDoneMap;
	ulong anExitCode;
	mApp->mReloadSubText = "";
	if (!mApp->mResourceManager->mResGenExePath.empty())
	{
		mApp->mReloadText = "Generating Resources...";
		std::string aParams;
		if (mApp->mResourceManager->mResGenMajorVersion == 2)
		{
			aParams = "\"" + mApp->mResourceManager->mResPropsUsed;
			if (mApp->mResourceManager->mResGenMinorVersion > 0)
				aParams += StrFormat(" -ver%d", mApp->mResourceManager->mResGenMinorVersion);
		}
		else if (mApp->mResourceManager->mResGenMajorVersion == 3)
		{
			aParams = "\"" + GetFullPath(mApp->mResourceManager->mResWatchFileUsed);
			if (mApp->mResourceManager->mResGenMinorVersion > 0)
				aParams += StrFormat(" -ver%d", mApp->mResourceManager->mResGenMinorVersion);
		}
		bool found = false;
		std::string pendingExePath = mApp->mResourceManager->mResGenExePath;
		while (!pendingExePath.empty())
		{
			std::string curExePath;
			uint semiColonPos = pendingExePath.find(";", 0);
			if (semiColonPos == std::string::npos)
			{
				curExePath = pendingExePath;
				pendingExePath.clear();
			}
			else
			{
				curExePath = pendingExePath.substr(0, semiColonPos);
				pendingExePath = pendingExePath.substr(semiColonPos + 1);
			}
			curExePath = Trim(curExePath);
			SHELLEXECUTEINFO anInfo;
			memset(&anInfo, 0, sizeof(anInfo));
			anInfo.cbSize = sizeof(anInfo);
			anInfo.lpVerb = "open";
			anInfo.lpFile = curExePath.c_str();
			anInfo.lpParameters = aParams.c_str();
			anInfo.lpDirectory = 0;
			anInfo.nShow = 5;
			anInfo.fMask = 1088;
			if (ShellExecuteEx(&anInfo))
			{
				found = true;

				HANDLE aHInstance = anInfo.hProcess;
				while (GetExitCodeProcess(aHInstance, &anExitCode))
				{
					if (anExitCode != ERROR_NO_MORE_ITEMS || mApp->mShutdown)
					{
						CloseHandle(aHInstance);
						break;
					}
					Sleep(20);
				}
				break;
			}
		}
		if (!found)
			MsgBox("Failed to execute ResGen at: " + mApp->mResourceManager->mResGenExePath, "Reload Error", 16);
	}
	if (mApp->mResourceManager->mLastXMLFileName.empty())
	{
		mApp->mReloadText = "Reparsing resources.xml...";
		mApp->mReloadSubText = "";
		mApp->mResourceManager->ReparseResourcesFile(mApp->mResourceManager->mLastXMLFileName);
	}
	std::set<void*> aPtrDirtySet;
	for (int aPass = 0; aPass < 3; ++aPass)
	{
		PopAnimSet::iterator aPopAnimItr = mApp->mPopAnimSet.begin();
		while (aPopAnimItr != mApp->mPopAnimSet.end())
		{
			PopAnim* aPopAnim;
			if (aPass)
			{
				if (aPtrDirtySet.find(aPopAnim) != aPtrDirtySet.end())
				{
					if (aPass)
					{
						if (aPass == 1)
						{
							mApp->mReloadText = StrFormat("Loading %ss...", "PopAnim");
							mApp->mReloadSubText = StrFormat("%s %d of %d", "PopAnim", aTypeDoneMap["PopAnim"], aTypeCountMap["PopAnim"]);
							mApp->mReloadPct = (float)aDoneCount++ / (float)aTotalCount;
							++aTypeDoneMap["PopAnim"];
						}
					}
					else
					{
						aTotalCount++;
						aTypeCountMap["PopAnim"]++;
					}
					if (aPass == 1)
					{
						std::string aPath = aPopAnim->mLoadedPamFile;
						Buffer aSaveData;
						aPopAnim->SaveState(aSaveData);
						aPopAnim->Clear();
						aPopAnim->LoadState(aSaveData);
					}
				}
			}
			else
			{
				aTotalCount++;
				aTypeCountMap["PopAnim"]++;
				aPtrDirtySet.insert(aPopAnim);
			}
			aPopAnimItr++;
		}

		PIEffectSet::iterator aPIEffectItr = mApp->mPIEffectSet.begin();
		while (aPIEffectItr != mApp->mPIEffectSet.end())
		{
			PIEffect* aPIEffect;
			if (aPIEffect->CheckCache())
				break;

			if (aPass)
			{
				if (aPass == 1)
				{
					mApp->mReloadText = StrFormat("Loading %ss...", "PIEffect");
					mApp->mReloadSubText = StrFormat("%s %d of %d", "PIEffect", aTypeDoneMap["PIEffect"], aTypeCountMap["PIEffect"]);
					mApp->mReloadPct = (float)aDoneCount++ / (float)aTotalCount;
					++aTypeDoneMap["PIEffect"];
				}
				else
				{
					aTotalCount++;
					aTypeCountMap["PIEffect"]++;
				}
				if (aPass == 1)
				{
					Buffer aSaveData;
					aPIEffect->SaveState(aSaveData);
					aPIEffect->Clear();
					aPIEffect->LoadState(aSaveData);
				}
				else if (aPass == 2)
				{
					aPIEffect->SetCacheUpToDate();
				}
				aPIEffectItr++;
			}
		}

		DDImageSet::iterator anImageItr = mWindowsGraphicsDriver->mDDImageSet.begin();
		while (anImageItr != mWindowsGraphicsDriver->mDDImageSet.end() || mApp->mShutdown)
		{
			DeviceImage* aDDImage(&anImageItr); //?
			if (!aPass && &aDDImage->mFilePath[0] != "!")
			{
				if (strncmp(aDDImage->mFilePath.c_str(), "IMAGE_", 6) == 0)
				{
					if (!DeviceImage::CheckCache(GetAppFullPath(aDDImage->mFilePath), "*"))
					{
						if (aPass == 1)
						{
							mApp->mReloadText = StrFormat("Loading %ss...", "Image");
							mApp->mReloadSubText = StrFormat("%s %d of %d", "Image", aTypeDoneMap["Image"], aTypeCountMap["Image"]);
							mApp->mReloadPct = (float)aDoneCount++ / (float)aTotalCount;
							++aTypeDoneMap["Image"];
						}
						else
						{
							aTotalCount++;
							aTypeCountMap["Image"]++;
						}
						aPtrDirtySet.insert(aDDImage);
					}
					else
					{
						if (aPtrDirtySet.find(aDDImage) != aPtrDirtySet.end())
						{
							if (aPass == 1)
							{
								mApp->mReloadText = StrFormat("Loading %ss...", "Image");
								mApp->mReloadSubText = StrFormat("%s %d of %d", "Image", aTypeDoneMap["Image"], aTypeCountMap["Image"]);
								mApp->mReloadPct = (float)aDoneCount++ / (float)aTotalCount;
								++aTypeDoneMap["Image"];
							}
							else
							{
								aTotalCount++;
								aTypeCountMap["Image"]++;
							}
							if (aPass == 1)
							{
								ImageLib::Image* aLoadedImage = ImageLib::GetImage(aDDImage->mFilePath);
								if (aLoadedImage != NULL)
								{
									aDDImage->SetBits(aLoadedImage->mBits, aLoadedImage->mWidth, aLoadedImage->mHeight);
									mWindowsGraphicsDriver->Remove3DData(aDDImage);
									delete aLoadedImage;
								}
								else
								{
									bool foundEmpty = false;
									SharedImageMap::iterator anItr = mApp->mSharedImageMap.begin();
									while (anItr != mApp->mSharedImageMap.end())
									{
										if (anItr->second.mImage == aDDImage && anItr->second.mRefCount)
											foundEmpty = true;
									}
									if (!foundEmpty)
										MsgBox("Failed to reload image: " + aDDImage->mFilePath, "Reload Error", 16);
								}
							}
							else if(aPass == 2)
								DeviceImage::SetCacheUpToDate(GetAppFullPath(aDDImage->mFilePath), "*");
						}
					}
					anImageItr++;
				}
			}
		}
		ImageFontSet::iterator aFontItr = mApp->mImageFontSet.begin();
		while (aFontItr != mApp->mImageFontSet.end() || mApp->mShutdown)
		{
			ImageFont* aFont(*aFontItr); //?
			if (aFont->mFontData->mSourceFile.length() > 0)
			{
				if (!ImageFont::CheckCache(aFont->mFontData->mSourceFile, "*"))
				{
					if (aPass == 1)
					{
						mApp->mReloadText = StrFormat("Loading %ss...", "Font");
						mApp->mReloadSubText = StrFormat("%s %d of %d", "Font", aTypeDoneMap["Font"], aTypeCountMap["Image"]);
						mApp->mReloadPct = (float)aDoneCount++ / (float)aTotalCount;
						++aTypeDoneMap["Font"];
					}
					else
					{
						aTotalCount++;
						aTypeCountMap["Font"]++;
					}
					if (aPass == 1)
					{
						ImageFont aNewImageFont(mApp, aFont->mFontData->mSourceFile);
						if (aNewImageFont.mFontData->mInitialized)
						{
							aFont->mFontData->DeRef();
							aFont->mFontData = aNewImageFont.mFontData;
							aFont->mFontData->Ref();
							bool wasValid = aFont->mFontData;
							if (wasValid)
								aFont->Prepare();
						}
						else
							MsgBox("Failed to reload font: " + aFont->mFontData->mSourceFile, "Reload Error", 16);
					}
					else if (aPass == 2)
						ImageFont::SetCacheUpToDate(GetAppFullPath(aFont->mFontData->mSourceFile), "*");
				}
				aFontItr++;
			}
		}
	}
	mApp->mResourceManager->ReapplyConfigs();
	mApp->mReloadingResources = false;
}

void WindowsAppDriver::ReloadAllResourcesProcStub(void* theArg) //5417-5419
{
	((WindowsAppDriver*) theArg)->ReloadAllResourcesProc();
}

bool WindowsAppDriver::ReloadAllResources() //5422-5495
{
	bool wasFullscreen = false;
	if (!mApp->mIsWindowed)
	{
		mApp->SwitchScreenMode(true);
		wasFullscreen = true;
	}
	mApp->mReloadingResources = true;
	mApp->mReloadPct = 0;
	mApp->mReloadSubText = "";
	mApp->mReloadText = "";
	_beginthread(ReloadAllResourcesProcStub, 0, this);
	bool wasRunning = mApp->mRunning;
	mApp->mRunning = true;
	DeviceImage anImage(mWindowsGraphicsDriver);
	Rect aRect;
	if (mApp->mWidgetManager->mImage)
	{
		aRect = Rect(mApp->mWidgetManager->mImage->mWidth / 2 - 150, mApp->mWidgetManager->mImage->mHeight / 2 - 75, 300, 150);
		anImage.AddImageFlags(ImageFlag_RenderTarget);
		anImage.Create(aRect.mWidth, aRect.mHeight);
		anImage.SetImageMode(false, false);
		Graphics aGetG(&anImage);
		aGetG.DrawImage(mApp->mWidgetManager->mImage, 0, 0, aRect);
	}
	float aLastPct = -1.0;
	std::string aLastReloadText;
	while (mApp->mReloadingResources)
	{
		if (aLastPct != mApp->mReloadPct || mApp->mReloadText != aLastReloadText)
		{
			ReloadAllResources_DrawStateUpdate(mApp->mReloadText, mApp->mReloadSubText, mApp->mReloadPct);
			aLastPct = mApp->mReloadPct;
			aLastReloadText = mApp->mReloadText;
			MSG msg;
			while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE) && mApp->mReloadingResources)
			{
				TranslateMessage(&msg);
				DispatchMessageA(&msg);
			}
			Sleep(20);
		}
		mApp->mRunning = wasRunning;
		mApp->mDeferredMessages.clear();
		if (mApp->mWidgetManager->mImage)
		{
			Graphics aRestoreG(mApp->mWidgetManager->mImage);
			aRestoreG.DrawImage(&anImage, aRect.mX, aRect.mY);
		}
		mApp->mWidgetManager->MarkAllDirty();
		InvalidateRect(mApp->mHWnd, NULL, false);
		if (wasFullscreen)
		{
			mApp->SwitchScreenMode(0);
			mApp->mHasFocus = false;
			mApp->mActive = true;
			mApp->mMinimized = false;
			RehupFocus();
		}
	}
	return true;
}
#endif

void WindowsAppDriver::DoParseCmdLine() //5500-5518
{
	char* aCmdLine = GetCommandLineA();
	char* aCmdLinePtr = aCmdLine;
	if (aCmdLinePtr[0] == '"')
	{
		aCmdLinePtr = strchr(aCmdLinePtr + 1, '"');
		if (aCmdLinePtr != NULL)
			aCmdLinePtr++;
	}

	if (aCmdLinePtr != NULL)
	{
		aCmdLinePtr = strchr(aCmdLinePtr, ' ');
		if (aCmdLinePtr != NULL)
			ParseCmdLine(aCmdLinePtr + 1);
	}

	mApp->mCmdLineParsed = true;
}

void WindowsAppDriver::ParseCmdLine(const std::string& theCmdLine) //5521-5563
{
	// Command line example:  -play -demofile="game demo.dmo"
	// Results in HandleCmdLineParam("-play", ""); HandleCmdLineParam("-demofile", "game demo.dmo"); //Supposedly they left the comment here, rip demo playback
	std::string aCurParamName;
	std::string aCurParamValue;

	int aSpacePos = 0;
	bool inQuote = false;
	bool onValue = false;

	for (int i = 0; i < (int)theCmdLine.length(); i++)
	{
		char c = theCmdLine[i];
		bool atEnd = false;

		if (c == '"')
		{
			inQuote = !inQuote;

			if (!inQuote)
				atEnd = true;
		}
		else if ((c == ' ') && (!inQuote))
			atEnd = true;
		else if (c == '=')
			onValue = true;
		else if (onValue)
			aCurParamValue += c;
		else
			aCurParamName += c;

		if (i == theCmdLine.length() - 1)
			atEnd = true;

		if (atEnd && !aCurParamName.empty())
		{
			HandleCmdLineParam(aCurParamName, aCurParamValue);
			aCurParamName = "";
			aCurParamValue = "";
			onValue = false;
		}
	}
}

void WindowsAppDriver::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) //5566-5595
{
	if (gSEHCatcher.HandleCmdLineParam(theParamName, theParamValue))
		return;

	if (theParamName == "-crash")
	{
		// Try to access NULL
		char* a = 0;
		*a = '!';
	}
	else if (theParamName == "-screensaver")
		mApp->mIsScreenSaver = true;
	else if (theParamName == "-changedir")
		mApp->mChangeDirTo = theParamValue;
	else if (theParamName == "-nod3d9")
		mApp->mNoD3D9 = true;
	else
	{
		Popup(mApp->GetString("INVALID_COMMANDLINE_PARAM", _S("Invalid command line parameter: ")) + StringToSexyString(theParamName));
		DoExit(0);
	}
}

void WindowsAppDriver::Init()
{
	mApp->mPrimaryThreadId = GetCurrentThreadId();

	if (mApp->mShutdown)
		return;

	if (gDDrawDLL == NULL || gDSoundDLL == NULL)
	{
		MessageBox(NULL,
			mApp->GetString("APP_REQUIRES_DIRECTX", _S("This application requires DirectX to run.  You can get DirectX at http://www.microsoft.com/directx")).c_str(),
			mApp->GetString("YOU_NEED_DIRECTX", _S("You need DirectX")).c_str(),
			MB_OK | MB_ICONERROR);
		DoExit(0);
	}

	mApp->InitPropertiesHook();
	mApp->ReadFromRegistry();
	mApp->mFileDriver->InitSaveDataFolder();

	if (!mApp->mCmdLineParsed)
		mApp->DoParseCmdLine();

	if (IsScreenSaver())
		mApp->mOnlyAllowOneCopyToRun = false;


	if (gHInstance == NULL)
		gHInstance = (HINSTANCE)GetModuleHandle(NULL);

	// Change directory
	if (!mApp->ChangeDirHook(mApp->mChangeDirTo.c_str()))
	{
		chdir(mApp->mChangeDirTo.c_str());
		AddTrailingSlash(mApp->mChangeDirTo);
	}

	//TODO
	ImageLib::InitJPEG2000();
	gPakInterface->AddPakFile(mApp->mFileDriver->GetLoadDataPath() + "main.pak");

	// Create a message we can use to talk to ourselves inter-process
	mApp->mNotifyGameMessage = RegisterWindowMessage((_S("Notify") + StringToSexyString(mApp->mProdName)).c_str());

	// Create a globally unique mutex
	mApp->mMutex = CreateMutex(NULL, TRUE, (StringToSexyString(mApp->mProdName) + _S("Mutex")).c_str());
	if (::GetLastError() == ERROR_ALREADY_EXISTS)
		HandleGameAlreadyRunning();

	mApp->mRandSeed = GetTickCount();
	SRand(mApp->mRandSeed);

	srand(GetTickCount());

	if (CheckFor98Mill())
	{
		mApp->mIsWideWindow = false;

		WNDCLASSA wc;
		wc.style = CS_DBLCLKS;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hbrBackground = NULL;
		wc.hCursor = NULL;
		wc.hIcon = ::LoadIconA(gHInstance, "IDI_MAIN_ICON");
		wc.hInstance = gHInstance;
		wc.lpfnWndProc = WindowProc;
		wc.lpszClassName = "MainWindow";
		wc.lpszMenuName = NULL;
		bool success = RegisterClassA(&wc) != 0;
		DBG_ASSERTE(success); //5675

		if (!mApp->mInvisHWnd)
		{
			wc.style = 0;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hbrBackground = NULL;
			wc.hCursor = NULL;
			wc.hIcon = NULL;
			wc.hInstance = gHInstance;
			wc.lpfnWndProc = WindowProc;
			wc.lpszClassName = "InvisWindow";
			wc.lpszMenuName = NULL;
			success = RegisterClassA(&wc) != 0;
			DBG_ASSERTE(success); //5690

			mApp->mInvisHWnd = CreateWindowExA(
				0,
				"InvisWindow",
				SexyStringToStringFast(mApp->mTitle).c_str(),
				0,
				0,
				0,
				0,
				0,
				NULL,
				NULL,
				gHInstance,
				0);
			SetWindowLong(mApp->mInvisHWnd, GWL_USERDATA, (LONG)this);
		}
	}
	else
	{
		mApp->mIsWideWindow = sizeof(SexyChar) == sizeof(wchar_t); //Is this supposed to be 1?

		WNDCLASS wc;
		wc.style = CS_DBLCLKS;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hbrBackground = NULL;
		wc.hCursor = NULL;
		wc.hIcon = ::LoadIconA(gHInstance, "IDI_MAIN_ICON");
		wc.hInstance = gHInstance;
		wc.lpfnWndProc = WindowProc;
		wc.lpszClassName = _S("MainWindow");
		wc.lpszMenuName = NULL;
		bool success = RegisterClass(&wc) != 0;
		DBG_ASSERTE(success); //5724

		if (!mApp->mInvisHWnd)
		{
			wc.style = 0;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hbrBackground = NULL;
			wc.hCursor = NULL;
			wc.hIcon = NULL;
			wc.hInstance = gHInstance;
			wc.lpfnWndProc = WindowProc;
			wc.lpszClassName = _S("InvisWindow");
			wc.lpszMenuName = NULL;
			success = RegisterClass(&wc) != 0;
			DBG_ASSERTE(success); //5739

			mApp->mInvisHWnd = CreateWindowEx(
				0,
				_S("InvisWindow"),
				mApp->mTitle.c_str(),
				0,
				0,
				0,
				0,
				0,
				NULL,
				NULL,
				gHInstance,
				0);
			SetWindowLong(mApp->mInvisHWnd, GWL_USERDATA, (LONG)this);
		}
	}

	mApp->mHandCursor = CreateCursor(gHInstance, 11, 4, 32, 32, gFingerCursorData, gFingerCursorData + sizeof(gFingerCursorData) / 2);
	mApp->mDraggingCursor = CreateCursor(gHInstance, 15, 10, 32, 32, gDraggingCursorData, gDraggingCursorData + sizeof(gDraggingCursorData) / 2);

	// Let app do something before showing window, or switching to fullscreen mode
	// NOTE: Moved call to PreDisplayHook above mIsWindowed and GetSystemsMetrics
	// checks because the checks below use values that could change in PreDisplayHook.
	// PreDisplayHook must call mWidgetManager->Resize if it changes mWidth or mHeight.
	mApp->PreDisplayHook();

	mApp->mWidgetManager->Resize(Rect(0, 0, mApp->mWidth, mApp->mHeight), Rect(0, 0, mApp->mWidth, mApp->mHeight));

	if (mApp->mFullScreenWindow) // change resoultion using ChangeDisplaySettings
	{
		EnumWindows(ChangeDisplayWindowEnumProc, 0); // record window pos
		DEVMODE dm;
		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);

		// Switch resolutions
		if (dm.dmPelsWidth != mApp->mWidth || dm.dmPelsHeight != mApp->mHeight || (dm.dmBitsPerPel != 16 && dm.dmBitsPerPel != 32))
		{
			dm.dmPelsWidth = mApp->mWidth;
			dm.dmPelsHeight = mApp->mHeight;
			dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

			if (dm.dmBitsPerPel != 16 && dm.dmBitsPerPel != 32) // handle 24-bit/256 color case
			{
				dm.dmBitsPerPel = 16;
				dm.dmFields |= DM_BITSPERPEL;
			}

			if (ChangeDisplaySettings(&dm, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			{
				mApp->mFullScreenWindow = false;
				mApp->mIsWindowed = false;
			}
		}
	}

	MakeWindow();

	//More demo code gone

	mApp->mSoundManager = mApp->mAudioDriver->CreateSoundManager();

	mApp->SetSfxVolume(mApp->mSfxVolume);

	mApp->mMusicInterface = mApp->CreateMusicInterface();

	mApp->SetMusicVolume(mApp->mMusicVolume);

	if (IsScreenSaver())
	{
		SetCursor(CURSOR_NONE);
	}

	mApp->InitHook();

	Locale::SetLocale(mApp->GetString("Locale", _S("English_United_States")));

	mApp->mInitialized = true;
}

void WindowsAppDriver::HandleGameAlreadyRunning() //5821-5830
{
	if (mApp->mOnlyAllowOneCopyToRun)
	{
		// Notify the other window and then shut ourselves down
		if (mApp->mNotifyGameMessage != 0)
			PostMessage(HWND_BROADCAST, mApp->mNotifyGameMessage, 0, 0);

		DoExit(0);
	}
}

void WindowsAppDriver::CopyToClipboard(const std::string& theString) //5833-5860
{
	HGLOBAL				aGlobalHandle;
	char* theData;
	WCHAR* theWData;

	//More demo code gone

	if (OpenClipboard(mApp->mHWnd))
	{
		EmptyClipboard();

		aGlobalHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, theString.length() + 1);
		theData = (char*)GlobalLock(aGlobalHandle);
		strcpy(theData, theString.c_str());
		GlobalUnlock(aGlobalHandle);
		SetClipboardData(CF_TEXT, aGlobalHandle);
		SetClipboardData(CF_OEMTEXT, aGlobalHandle);
		SetClipboardData(CF_LOCALE, aGlobalHandle);

		int aSize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, theString.c_str(), theString.length(), NULL, 0);
		aGlobalHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (aSize + 1) * sizeof(WCHAR));
		theWData = (WCHAR*)GlobalLock(aGlobalHandle);
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, theString.c_str(), theString.length(), theWData, aSize);
		theWData[aSize] = '\0';
		GlobalUnlock(aGlobalHandle);
		SetClipboardData(CF_UNICODETEXT, aGlobalHandle);

		CloseClipboard();
	}
}

std::string WindowsAppDriver::GetClipboard() //5863-5884
{
	HGLOBAL				aGlobalHandle;
	std::string			aString;
	if (OpenClipboard(mApp->mHWnd)) //More demo code goes bye bye
	{
		aGlobalHandle = GetClipboardData(CF_TEXT);
		if (aGlobalHandle != NULL)
		{
			char* theData = (char*)GlobalLock(aGlobalHandle);
			if (theData != NULL)
			{
				aString = theData;
				GlobalUnlock(aGlobalHandle);
			}
		}
		CloseClipboard();
	}
	return aString;
}

void WindowsAppDriver::SetCursor(int theCursorNum) //5887-5890
{
	mApp->mCursorNum = theCursorNum;
	EnforceCursor();
}

int WindowsAppDriver::GetCursor() //5893-5895
{
	return mApp->mCursorNum;
}

void WindowsAppDriver::EnableCustomCursors(bool enabled) //5898-5901
{
	mApp->mCustomCursorsEnabled = enabled;
	EnforceCursor();
}

void WindowsAppDriver::Remove3DData(MemoryImage* theMemoryImage) //5906-5909
{
	if (mWindowsGraphicsDriver)
		mWindowsGraphicsDriver->Remove3DData(theMemoryImage);
}

bool WindowsAppDriver::Is3DAccelerated() //5913-5915
{
	return mWindowsGraphicsDriver->mIs3D;
}

bool WindowsAppDriver::Is3DAccelerationSupported() //5918-5923
{
	if (mWindowsGraphicsDriver->mD3DTester)
		return mWindowsGraphicsDriver->mD3DTester->Is3DSupported();
	else
		return false;
}

bool WindowsAppDriver::Is3DAccelerationRecommended() //5926-5931
{
	if (mWindowsGraphicsDriver->mD3DTester)
		return mWindowsGraphicsDriver->mD3DTester->Is3DRecommended();
	else
		return false;
}

void WindowsAppDriver::Set3DAcclerated(bool is3D, bool reinit) //Lol Acclerated | 5935-5961
{
	if (mWindowsGraphicsDriver->mIs3D == is3D)
		return;

	mWindowsGraphicsDriver->mIs3D = is3D;

	if (reinit)
	{
		int aResult = InitGraphicsInterface();

		if (is3D && aResult)
		{
			Set3DAcclerated(false, reinit);
			return;
		}
		else if (aResult)
		{
			Popup(mApp->GetString("FAILED_INIT_DIRECTDRAW", _S("Failed to initialize DirectDraw: ")) + StringToSexyString(WindowsGraphicsDriver::ResultToString(aResult) + " " + mWindowsGraphicsDriver->mErrorString));
			DoExit(1);
		}

		mApp->ReInitImages();

		mApp->mWidgetManager->mImage = mWindowsGraphicsDriver->GetScreenImage();
		mApp->mWidgetManager->MarkAllDirty();
	}
}

static int sCurRenderModeFlagBit;





bool WindowsAppDriver::OverrideWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &theResult) //5968-5970
{
	return false;
}


void WindowsAppDriver::RehupFocus() //5973-6000
{
	bool wantHasFocus = mApp->mActive && !mApp->mMinimized;

	if (wantHasFocus != mApp->mHasFocus)
	{
		mApp->mHasFocus = wantHasFocus;

		if (mApp->mHasFocus)
		{
			if (mApp->mMuteOnLostFocus)
				mApp->Unmute(true);

			mApp->mWidgetManager->GotFocus();
			mApp->GotFocus();
		}
		else
		{
			if (mApp->mMuteOnLostFocus)
				mApp->Mute(true);

			mApp->mWidgetManager->LostFocus();
			mApp->LostFocus();

			ReleaseCapture();
			mApp->mWidgetManager->DoMouseUps();
		}
	}
}

void WindowsAppDriver::ClearKeysDown() //6003-6011
{
	if (mApp->mWidgetManager != NULL) // fix stuck alt-key problem
	{
		for (int aKeyNum = 0; aKeyNum < 0xFF; aKeyNum++)
			mApp->mWidgetManager->mKeyDown[aKeyNum] = false;
	}
	mApp->mCtrlDown = false;
	mApp->mAltDown = false;
}

DeviceImage* WindowsAppDriver::GetOptimizedImage(const std::string& theFileName, bool commitBits, bool allowTriReps) //6015-6017
{
	return NULL; //Unsupported on Windows.
}

#ifdef _DEBUG
CRefSymbolDb* WindowsAppDriver::GetReflection() //Check? | 6023-6034
{
	if (!mApp->mRefSymbolDb)
	{
		mApp->mRefSymbolDb = new CRefSymbolDb();
		if (!mApp->mRefSymbolDb->InitFromModule(GetModuleHandleA(0), false, "", ""))
			MessageBoxA(NULL, "Reflection initialization failed (reflection data will be unavailable)", "Error", MB_OK | MB_ICONERROR); //I bet it's error icon
	}
	return mApp->mRefSymbolDb->GetIsInitialized() ? mApp->mRefSymbolDb : NULL;
}
#endif