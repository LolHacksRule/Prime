#ifndef __SEXYAPPBASE_H__
#define __SEXYAPPBASE_H__

#include "Common.h"
#include "Rect.h"
#include "Color.h"
#include "ButtonListener.h"
#include "DialogListener.h"
#include "Buffer.h"
#include "CritSect.h"
#include "SharedImage.h"
#include "Ratio.h"

//Prime stuff
#include "ImageFont.h"
#include "PopAnim.h"
#include "PopLoc.h"
#include "SharedRenderTarget.h"
#include "SexyThread.h"

namespace ImageLib
{
	class Image;
};

namespace Reflection
{
	class CRefSymbolDb;
};

namespace Sexy //Decompilation by LolHacksRule, original engine belongs to PopCap. Support PopCap by remembering to buy the things they made.
{

class WidgetManager;
class Image;
class Widget;
class SoundManager;
class MusicInterface;
class MemoryImage;
class HTTPTransfer;
class Dialog;

class ResourceManager;

class WidgetSafeDeleteInfo
{
public:
	int						mUpdateAppDepth;
	Widget*					mWidget;
};

//Prime stuff
class DeviceImage;

class IAppDriver;
class IAudioDriver;
class IFileDriver;
class IGamepadDriver;
class IGraphicsDriver;
class IResStreamsDriver;

class ICfgMachine;
class ICfgCompiler;

class ResStreamsManager;
class SafeStrFntRcd;

enum _TouchPhase
{
	TOUCH_BEGAN,
	TOUCH_MOVED,
	TOUCH_STATIONARY,
	TOUCH_ENDED,
	TOUCH_CANCELLED
};

enum GamepadButton
{
	GAMEPAD_BUTTON_UP,
	GAMEPAD_BUTTON_DOWN,
	GAMEPAD_BUTTON_LEFT,
	GAMEPAD_BUTTON_RIGHT,
	GAMEPAD_BUTTON_BACK,
	GAMEPAD_BUTTON_START,
	GAMEPAD_BUTTON_A,
	GAMEPAD_BUTTON_B,
	GAMEPAD_BUTTON_X,
	GAMEPAD_BUTTON_Y,
	GAMEPAD_BUTTON_LB,
	GAMEPAD_BUTTON_RB,
	GAMEPAD_BUTTON_LTRIGGER,
	GAMEPAD_BUTTON_RTRIGGER,
	GAMEPAD_BUTTON_LSTICK,
	GAMEPAD_BUTTON_RSTICK,
	GAMEPAD_BUTTON_DPAD_UP,
	GAMEPAD_BUTTON_DPAD_DOWN,
	GAMEPAD_BUTTON_DPAD_LEFT,
	GAMEPAD_BUTTON_DPAD_RIGHT,
	_GAMEPAD_BUTTON_NUM, //Why
	GAMEPAD_BUTTON_PS_X = 6,
	GAMEPAD_BUTTON_PS_SQUARE = 8,
	GAMEPAD_BUTTON_PS_CIRCLE = 7,
	GAMEPAD_BUTTON_PS_TRIANGLE = 9,
	GAMEPAD_BUTTON_PS_L1 = 10,
	GAMEPAD_BUTTON_PS_L2 = 12,
	GAMEPAD_BUTTON_PS_L3 = 14,
	GAMEPAD_BUTTON_PS_R1 = 11,
	GAMEPAD_BUTTON_PS_R2 = 13,
	GAMEPAD_BUTTON_PS_R3 = 15,
	GAMEPAD_BUTTON_PS_START = 5,
	GAMEPAD_BUTTON_PS_SELECT = 4
};
#ifdef _SEXYDECOMP_USE_LATEST_CODE
enum GamepadAxis //Not in Windows Prime (pre Mobile/XNA)
{
	GAMEPAD_AXIS_LSTICK_X,
	GAMEPAD_AXIS_LSTICK_Y,
	GAMEPAD_AXIS_RSTICK_X,
	GAMEPAD_AXIS_RSTICK_Y,
	_GAMEPAD_AXIS_NUM
};
enum GamepadButtonExta //Not in Windows Prime (pre Mobile/XNA)
{
	GAMEPAD_BUTTON_REPEAT = 1,
	GAMEPAD_BUTTON_IS_DPAD,
	GAMEPAD_BUTTON_JUST_BOUND = 4,
	GAMEPAD_BUTTON_FROM_CHILD = 8
};
#endif

class Touch //Has a backslash in XNA
{
public:
	void* ident; //Point/CGPoint?
	void* event; //Point/CGPoint?
	Point location;
	Point previousLocation;
	int tapCount;
	double timestamp;
	_TouchPhase phase;
#if defined(_WIN32_) || defined(_MAC) //Only on Prime Mobile
	void SetTouchInfo(Point loc, _TouchPhase _phase, double _timestamp)
	{
		previousLocation = location;
		location = loc;
		phase = _phase;
		timestamp = _timestamp / 1000;
	}
#endif
};

#if defined(_WIN32_)
enum MessageBoxFlags //Add
{
	MSGBOX_OK = 0,
	MSGBOX_OKCANCEL = 1,
	MSGBOX_ABORTRETRYIGNORE = 2,
	MSGBOX_YESNOCANCEL = 3,
	MSGBOX_YESNO = 4,
	MSGBOX_RETRYCANCEL = 5,
	MSGBOX_CANCELTRYCONTINUE = 6,
	MSGBOX_ICONERROR = 16,
	MSGBOX_ICONQUESTION = 32,
	MSGBOX_ICONWARNING = 48,
	MSGBOX_ICONINFORMATION = 64
};

enum MessageBoxResults
{
	MSGBOX_RESULT_OK = 1,
	MSGBOX_RESULT_CANCEL = 2,
	MSGBOX_RESULT_ABORT = 3,
	MSGBOX_RESULT_RETRY = 4,
	MSGBOX_RESULT_IGNORE = 5,
	MSGBOX_RESULT_YES = 6,
	MSGBOX_RESULT_NO = 7,
	MSGBOX_RESULT_TRYAGAIN = 10,
	MSGBOX_RESULT_CONTINUE = 11
};
#endif

typedef std::list<WidgetSafeDeleteInfo> WidgetSafeDeleteList;
typedef std::set<MemoryImage*> MemoryImageSet;
typedef std::set<ImageFont*> ImageFontSet;
typedef std::set<PopAnim*> PopAnimSet;
typedef std::set<PIEffect*> PIEffectSet;
typedef std::map<int, Dialog*> DialogMap;
typedef std::list<Dialog*> DialogList;
#ifdef WIN32
	typedef std::list<MSG> WindowsMessageList;
#endif

//typedef std::basic_string<TCHAR> tstring; // string of TCHARs

typedef std::map<std::string, SexyString> StringSexyStringMap;
typedef std::map<std::string, std::string> StringStringMap;
typedef std::map<std::string, std::wstring> StringWStringMap;
typedef std::map<std::string, bool> StringBoolMap;
typedef std::map<std::string, int> StringIntMap;
typedef std::map<std::string, double> StringDoubleMap;
typedef std::map<std::string, StringVector> StringStringVectorMap;

enum //Prob goes here
{
	CURSOR_POINTER,
	CURSOR_HAND,
	CURSOR_DRAGGING,
	CURSOR_TEXT,
	CURSOR_CIRCLE_SLASH,
	CURSOR_SIZEALL,
	CURSOR_SIZENESW,
	CURSOR_SIZENS,
	CURSOR_SIZENWSE,
	CURSOR_SIZEWE,
	CURSOR_WAIT,
	CURSOR_NONE,
	CURSOR_CUSTOM,
	NUM_CURSORS
};

//The demo enums are gone.

enum //Add the other one, introduced in BejBlitz, FPS_SHOW on XNA
{
	FPS_ShowFPS,
	FPS_ShowFPSWithHistory, //Fancy bars
	FPS_ShowCoords,
	Num_FPS_Types
};

enum
{
	UPDATESTATE_MESSAGES,
	UPDATESTATE_PROCESS_1,
	UPDATESTATE_PROCESS_2,
	UPDATESTATE_PROCESS_DONE
};

enum UI_ORIENTATION
{
	UI_ORIENTATION_UNKNOWN,
	UI_ORIENTATION_PORTRAIT,
	UI_ORIENTATION_PORTRAIT_UPSIDE_DOWN,
	UI_ORIENTATION_LANDSCAPE_RIGHT,
	UI_ORIENTATION_LANDSCAPE_LEFT,
	UI_ORIENTATION_FACE_UP,
	UI_ORIENTATION_FACE_DOWN
};

#ifdef WIN32
typedef std::map<HANDLE, int> HandleToIntMap;
#endif

typedef std::map<int, int>	  IntToIntMap;

class SexyAppBase : public ButtonListener, public DialogListener
{
public:	
	IAppDriver*							mAppDriver;
	IAudioDriver*						mAudioDriver;
	IGraphicsDriver*					mGraphicsDriver;
	IFileDriver*						mFileDriver;
	IGamepadDriver*						mGamepadDriver;
	IResStreamsDriver*					mResStreamsDriver;
	ulong								mRandSeed;
		
	std::string							mCompanyName;
	std::string							mFullCompanyName;
	std::string							mProdName;	
	SexyString							mTitle;	
	std::string							mRegKey;
	std::string							mChangeDirTo;
	
	int									mRelaxUpdateBacklogCount; // app doesn't try to catch up for this many frames
	int									mMaxUpdateBacklog;
	bool								mPauseWhenMoving;
	int									mPreferredX;
	int									mPreferredY;
	int									mPreferredWidth;
	int									mPreferredHeight;
	int									mWidth;
	int									mHeight;
	int									mFullscreenBits;
	double								mMusicVolume;
	double								mSfxVolume;
	double								mDemoMusicVolume;
	double								mDemoSfxVolume;
	bool								mNoSoundNeeded;
	bool								mWantFMod;
	bool								mCmdLineParsed;
	bool								mSkipSignatureChecks;
	bool								mStandardWordWrap;
	bool								mbAllowExtendedChars;

	bool								mOnlyAllowOneCopyToRun;
	UINT								mNotifyGameMessage;
	CritSect							mCritSect;
	CritSect							mGetImageCritSect;
	bool								mBetaValidate;
	uchar								mAdd8BitMaxTable[512];
	WidgetManager*						mWidgetManager;
	DialogMap							mDialogMap;
	DialogList							mDialogList;
	DWORD								mPrimaryThreadId;
	bool								mSEHOccured;
	bool								mShutdown;
	bool								mExitToTop;
	bool								mIsWindowed;
	bool								mIsPhysWindowed;
	bool								mFullScreenWindow; // uses ChangeDisplaySettings to run fullscreen with mIsWindowed true
	bool								mForceFullscreen;
	bool								mForceWindowed;	
	bool								mInitialized;	
	bool								mProcessInTimer;
	DWORD								mTimeLoaded;

	bool								mIsScreenSaver;
	bool								mAllowMonitorPowersave;
	bool								mWantsDialogCompatibility;

	bool								mNoDefer;	
	bool								mFullScreenPageFlip;	
	bool								mTabletPC;
	MusicInterface*						mMusicInterface;	
	bool								mReadFromRegistry;
	std::string							mRegisterLink;
	std::string							mProductVersion;	
	Image*								mCursorImages[NUM_CURSORS];

	bool								mIsOpeningURL;
	bool								mShutdownOnURLOpen;
	std::string							mOpeningURL;
	DWORD								mOpeningURLTime;
	DWORD								mLastTimerTime;
	DWORD								mLastBigDelayTime;	
	double								mUnmutedMusicVolume;
	double								mUnmutedSfxVolume;	
	int									mMuteCount;
	int									mAutoMuteCount;
	bool								mDemoMute;
	bool								mMuteOnLostFocus;
	MemoryImageSet						mMemoryImageSet;
	CritSect							mImageSetCritSect;
	ImageFontSet						mImageFontSet;
	PIEffectSet							mPIEffectSet;
	PopAnimSet							mPopAnimSet;
	SharedImageMap						mSharedImageMap;
	Condition							mSharedImageEvent;

#ifdef _WIN32
	HANDLE								mMutex; //C++ only
	WindowsMessageList					mDeferredMessages; ///Why like literally why is this still here
#endif

	bool								mCleanupSharedImages;
	
	int									mNonDrawCount;
	int									mFrameTime;

	bool								mIsDrawing;
	bool								mLastDrawWasEmpty;
	bool								mHasPendingDraw;
	double								mPendingUpdatesAcc;
	double								mUpdateFTimeAcc;
	uint64								mLastTimeCheck;
	uint64								mLastTime;
	uint64								mLastUserInputTick;

	int									mSleepCount;
	int									mDrawCount;
	int									mUpdateCount;
	int									mUpdateAppState;
	int									mUpdateAppDepth;
	int									mMaxNonDrawCount;
	double								mUpdateMultiplier;		
	bool								mPaused;
	int									mFastForwardToUpdateNum;
	bool								mFastForwardToMarker;
	bool								mFastForwardStep;
	uint64								mLastDrawTick;
	uint64								mNextDrawTick;
	int									mStepMode;  // 0 = off, 1 = step, 2 = waiting for step

	int									mCursorNum;
	SoundManager*						mSoundManager;
	int									mGamepadLocked;

#ifdef _WIN32
	HCURSOR								mOverrideCursor;
	HCURSOR								mHandCursor;
	HCURSOR								mDraggingCursor;
	HWND								mHWnd;
	HWND								mInvisHWnd;
	LONG								mOldWndProc;
	SafeStrFntRcd*						mStrFntRcd; //Not in PVZ2, XNA, PVZF, Blitz+W8+Mobile, or Peg2, is this specific to pl_d?
	HandleToIntMap						mHandleToIntMap; // For waiting on handles
#endif
	int									mCurHandleNum;

	WidgetSafeDeleteList				mSafeDeleteList;
	bool								mMouseIn;	
	bool								mRunning;
	bool								mActive;
	bool								mMinimized;
	bool								mPhysMinimized;
	bool								mIsDisabled;
	bool								mHasFocus;
	int									mDrawTime;
	ulong								mFPSStartTick;
	int									mFPSFlipCount;
	int									mFPSDirtyCount;
	int									mFPSTime;
	int									mFPSCount;
	bool								mShowFPS;
	int									mShowFPSMode;
	float								mVFPSUpdateTimes;
	int									mVFPSUpdateCount;
	int									mVFPSDrawTimes;
	int									mVFPSDrawCount;
	float								mCurVFPS;
	int									mScreenBltTime;
	bool								mAutoStartLoadingThread;
	bool								mLoadingThreadStarted;
	bool								mLoadingThreadCompleted;
	bool								mLoaded;
	bool								mReloadingResources;
	float								mReloadPct;
	std::string							mReloadText;
	std::string							mReloadSubText;
	bool								mYieldMainThread;
	bool								mLoadingFailed;
	bool								mCursorThreadRunning;
	bool								mSysCursor;	
	bool								mCustomCursorsEnabled;
	bool								mCustomCursorDirty;	
	bool								mLastShutdownWasGraceful;
	bool								mIsWideWindow;
	bool								mWriteToSexyCache;
	bool								mSexyCacheBuffers;
	bool								mWriteFontCacheDir;

	int									mNumLoadingThreadTasks;
	int									mCompletedLoadingThreadTasks;

	bool								mDebugKeysEnabled;
	bool								mEnableMaximizeButton;
	bool								mCtrlDown;
	bool								mAltDown;
	bool								mAllowAltEnter;
	
	int									mSyncRefreshRate;
	bool								mVSyncUpdates;
	bool								mNoVSync;
	bool								mVSyncBroken;
	int									mVSyncBrokenCount;
	DWORD								mVSyncBrokenTestStartTick;
	DWORD								mVSyncBrokenTestUpdates;
	bool								mWaitForVSync;
	bool								mSoftVSyncWait;

	bool								mAutoEnable3D;
	bool								mTest3D;
	bool								mNoD3D9;
	DWORD								mMinVidMemory3D;
	DWORD								mRecommendedVidMemory3D;

	bool								mWidescreenAware;
	bool								mWidescreenTranslate;
	Rect								mScreenBounds;
	bool								mEnableWindowAspect;
	Ratio								mWindowAspect;
	Ratio								mMinAspect;
	Ratio								mMaxAspect;
	bool								mAllowWindowResize;
	int									mOrigScreenWidth;
	int									mOrigScreenHeight;
	bool								mIsSizeCursor;

	StringWStringMap					mStringProperties;
	StringBoolMap						mBoolProperties;
	StringIntMap						mIntProperties;
	StringDoubleMap						mDoubleProperties;
	StringStringVectorMap				mStringVectorProperties;
	ResourceManager*					mResourceManager;

#ifdef ZYLOM
	uint								mZylomGameId;
#endif

	PopLoc								mPopLoc; //Not in OSX, Peg2 or PVZ2
	SharedRenderTarget::Pool*			mSharedRTPool;
	ICfgCompiler*						mCfgCompiler;
	ICfgMachine*						mCompatCfgMachine;

	enum EShowCompatInfoMode
	{
		SHOWCOMPATINFOMODE_OFF,
		SHOWCOMPATINFOMODE_BOTTOM,
		SHOWCOMPATINFOMODE_TOP,
		SHOWCOMPATINFOMODE_COUNT
	};

	EShowCompatInfoMode					mShowCompatInfoMode;
	bool								mShowWidgetInspector;
	bool								mWidgetInspectorPickMode;
	bool								mWidgetInspectorLeftAnchor;
	WidgetContainer*					mWidgetInspectorPickWidget;
	WidgetContainer*					mWidgetInspectorCurWidget;
	int									mWidgetInspectorScrollOffset;
	Point								mWidgetInspectorClickPos;
	ResStreamsManager*					mResStreamsManager;
	SexyThreadId						mMainThreadId; //Or it is DWORD like it originally is?
	Reflection::CRefSymbolDb*			mRefSymbolDb;

	static bool							sAttemptingNonRecommended3D;

public:
	void						ProcessSafeDeleteList();
	virtual void				ReInitImages();
	virtual void				DeleteNativeImageData();
	virtual void				DeleteExtraImageData();
	virtual void				LoadingThreadCompleted();
	virtual void				UpdateFrames();
	void						DoExit(int theCode);

public:
	SexyAppBase();
	virtual ~SexyAppBase();
	static void 				InitFileDriver();
	

	// Common overrides:
	virtual MusicInterface*		CreateMusicInterface(); //Change HWND, called in Driver
	virtual void				InitHook();
	virtual void				ShutdownHook();	
	virtual void				PreTerminate();
	virtual void				LoadingThreadProc();
	virtual void				WriteToRegistry();
	virtual void				ReadFromRegistry();
	virtual Dialog*				NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode);		
	virtual void				PreDisplayHook();

	virtual bool				IsUIOrientationAllowed(UI_ORIENTATION theOrientation);
	virtual void				UIOrientationChanged(UI_ORIENTATION theOrientation);
	virtual void				LowMemoryWarning(); //Why
	virtual void				AppEnteredBackground() {} //Why (maybe there's code in mobile) | Line 562
	virtual bool				ShouldReInit() { return false; } //Line 568

	// Public methods
	virtual void				BeginPopup();
	virtual void				EndPopup();
	virtual int					MsgBox(const std::string &theText, const std::string &theTitle = "Message", int theFlags = MB_OK);
	virtual int					MsgBox(const std::wstring &theText, const std::wstring &theTitle = L"Message", int theFlags = MB_OK);
	virtual void				Popup(const std::string& theString);
	virtual void				Popup(const std::wstring& theString);
	virtual void				SafeDeleteWidget(Widget* theWidget);	

	virtual void				URLOpenFailed(const std::string& theURL);
	virtual void				URLOpenSucceeded(const std::string& theURL);
	virtual bool				OpenURL(const std::string& theURL, bool shutdownOnOpen = false);	

	virtual std::string			GetGameSEHInfo();
	virtual	void				Shutdown();	//Correct?

	virtual void				DoParseCmdLine();
	virtual void				ParseCmdLine(const std::string& theCmdLine);
	virtual void				HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue);
	virtual void				HandleGameAlreadyRunning(); 

	virtual void				Start();	
	virtual void				Init();	
	virtual void				PreDDInterfaceInitHook();
	virtual void				PostDDInterfaceInitHook();
	virtual bool				ChangeDirHook(const char *theIntendedPath);
	virtual void				PlaySample(int theSoundNum);
	virtual void				PlaySample(int theSoundNum, int thePan);

	virtual double				GetMasterVolume();
	virtual double				GetMusicVolume();
	virtual double				GetSfxVolume();
	virtual bool				IsMuted();

	virtual void				SetMasterVolume(double theVolume);
	virtual void				SetMusicVolume(double theVolume);
	virtual void				SetSfxVolume(double theVolume);	
	virtual void				Mute(bool autoMute = false);
	virtual void				Unmute(bool autoMute = false);

	void						StartLoadingThread();
	virtual double				GetLoadingThreadProgress();	

	void						CopyToClipboard(const std::string& theString);
	std::string					GetClipboard();

	void						SetCursor(int theCursorNum);
	int							GetCursor();
	void						EnableCustomCursors(bool enabled);
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	virtual DeviceImage*		GetImage(const std::string& theFileName, bool commitBits = true, bool allowTriReps = true, bool isInAtlas = false);	//DeviceImage now
#else
	virtual DeviceImage*		GetImage(const std::string& theFileName, bool commitBits = true, bool allowTriReps = true);	//DeviceImage now
#endif
	virtual SharedImageRef		SetSharedImage(const std::string& theFileName, const std::string& theVariant = "", DeviceImage *theImage, bool* isNew = NULL ); //Todo
	virtual SharedImageRef		CheckSharedImage(const std::string& theFileName, const std::string& theVariant = ""); //Todo
	virtual SharedImageRef		GetSharedImage(const std::string& theFileName, const std::string& theVariant = "", bool* isNew = NULL, bool allowTriReps = NULL); //Todo
	SharedRenderTarget::Pool*	GetSharedRenderTargetPool();

	void						CleanSharedImages();
	void						PrecacheAdditive(MemoryImage* theImage);
	void						PrecacheAlpha(MemoryImage* theImage);
	void						PrecacheNative(MemoryImage* theImage);
	void						SetCursorImage(int theCursorNum, Image* theImage); //Not present on iOS? iPhoneOSAppDriver::SetCursorImage exists though.

	DeviceImage*				CreateCrossfadeImage(Image* theImage1, const Rect& theRect1, Image* theImage2, const Rect& theRect2, double theFadeFactor); //Not present on iOS?
	void						ColorizeImage(Image* theImage, const Color& theColor);
	DeviceImage*				CreateColorizedImage(Image* theImage, const Color& theColor);
	DeviceImage*				CopyImage(Image* theImage, const Rect& theRect);
	DeviceImage*				CopyImage(Image* theImage);
	void						MirrorImage(Image* theImage);
	void						FlipImage(Image* theImage);
	void						RotateImageHue(Sexy::MemoryImage *theImage, int theDelta);
	ulong						HSLToRGB(int h, int s, int l);
	ulong						RGBToHSL(int r, int g, int b);
	void						HSLToRGB(const ulong* theSource, ulong* theDest, int theSize);
	void						RGBToHSL(const ulong* theSource, ulong* theDest, int theSize);

	void						AddMemoryImage(MemoryImage* theMemoryImage);
	void						RemoveMemoryImage(MemoryImage* theMemoryImage);
	void						Remove3DData(MemoryImage* theMemoryImage);
	virtual void				SwitchScreenMode();
	virtual void				SwitchScreenMode(bool wantWindowed);
	virtual void				SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false);
	
	virtual Dialog*				DoDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode);
	virtual Dialog*				GetDialog(int theDialogId);
	virtual void				AddDialog(int theDialogId, Dialog* theDialog);
	virtual void				AddDialog(Dialog* theDialog);
	virtual bool				KillDialog(int theDialogId, bool removeWidget, bool deleteWidget);
	virtual bool				KillDialog(int theDialogId);
	virtual bool				KillDialog(Dialog* theDialog);
	virtual int					GetDialogCount();
	virtual void				ModalOpen();
	virtual void				ModalClose();	
	virtual void				DialogButtonPress(int theDialogId, int theButtonId);
	virtual void				DialogButtonDepress(int theDialogId, int theButtonId);

	virtual void				GotFocus();
	virtual void				LostFocus();	
	virtual bool				KeyDown(int theKey);
	virtual bool				DebugKeyDown(int theKey);	
	virtual bool				DebugKeyDownAsync(int theKey, bool ctrlDown, bool altDown);
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	virtual void				ShowKeyboard();
	virtual void				HideKeyboard();
#endif
	virtual void				CloseRequestAsync();
	bool						Is3DAccelerated();
	bool						Is3DAccelerationSupported();
	bool						Is3DAccelerationRecommended();
	void						Set3DAcclerated(bool is3D, bool reinit = true); //Lol
	virtual void				Done3dTesting();
	virtual std::string			NotifyCrashHook(); // return file name that you want to upload
	
	virtual bool				CheckSignature(const Buffer& theBuffer, const std::string& theFileName);

	// Properties access methods
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	bool						LoadProperties(const std::string& theFileName, bool required, bool checkSig, bool needsLocaleCorrection);
#else
	bool						LoadProperties(const std::string& theFileName, bool required, bool checkSig);
#endif
	bool						LoadProperties();
	virtual void				InitPropertiesHook();

	// Resource access methods
	void						LoadResourceManifest();
	void						ShowResourceError(bool doExit = false);
	bool						ReloadAllResources();
	
	bool						GetBoolean(const std::string& theId);
	bool						GetBoolean(const std::string& theId, bool theDefault);	
	int							GetInteger(const std::string& theId);
	int							GetInteger(const std::string& theId, int theDefault);
	double						GetDouble(const std::string& theId);
	double						GetDouble(const std::string& theId, double theDefault);
	SexyString					GetString(const std::string& theId);
	SexyString					GetString(const std::string& theId, const SexyString& theDefault);

#ifndef _IPHONEOS
	StringVector				GetStringVector(const std::string& theId);
#endif

	void						SetBoolean(const std::string& theId, bool theValue);
	void						SetInteger(const std::string& theId, int theValue);
	void						SetDouble(const std::string& theId, double theValue);
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	void						SetString(const std::string& anId, const std::wstring& value);
#else
	void						SetString(const std::string& theId, const std::wstring& theValue);
#endif

	// Registry access methods
	bool						RegistryGetSubKeys(const std::string& theKeyName, StringVector* theSubKeys); //Not on iOS?
	bool						RegistryReadString(const std::string& theValueName, std::string* theString);
	bool						RegistryReadString(const std::string& theValueName, std::wstring* theString);
	bool						RegistryReadInteger(const std::string& theValueName, int* theValue);
	bool						RegistryReadBoolean(const std::string& theValueName, bool* theValue);
	bool						RegistryReadData(const std::string& theValueName, uchar* theValue, ulong* theLength); //Not on iOS?
	bool						RegistryWriteString(const std::string& theValueName, const std::string& theString);
	bool						RegistryWriteString(const std::string& theValueName, const std::wstring& theString);
	bool						RegistryWriteInteger(const std::string& theValueName, int theValue);
	bool						RegistryWriteBoolean(const std::string& theValueName, bool theValue);
	bool						RegistryWriteData(const std::string& theValueName, const uchar* theValue, ulong theLength);	
	bool						RegistryEraseKey(const SexyString& theKeyName); //Not on iOS?
	void						RegistryEraseValue(const SexyString& theValueName); //Not on iOS?

	// File access methods
	bool						WriteBufferToFile(const std::string& theFileName, const Buffer* theBuffer);
	bool						ReadBufferFromFile(const std::string& theFileName, Buffer* theBuffer, bool dontWriteToDemo = true);//UNICODE
	bool						WriteBytesToFile(const std::string& theFileName, const void *theData, unsigned long theDataLen);
	bool						FileExists(const std::string& theFileName);
	bool						EraseFile(const std::string& theFileName);

	// Misc methods
	virtual bool				UpdateAppStep(bool* updated);
	virtual bool				UpdateApp();
	void						ClearUpdateBacklog(bool relaxForASecond = false);
	bool						IsScreenSaver();
	virtual bool				AppCanRestore();
	void						RehupFocus();
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	virtual void				GamepadButtonDown(GamepadButton theButton, int thePlayer, ulong theFlags);
	virtual void				GamepadButtonUp(GamepadButton theButton, int thePlayer, ulong theFlags);
	virtual void				GamepadAxisMove(GamepadAxis theAxis, int thePlayer, int theAxisValue);
	virtual UI_ORIENTATION		GetUIOrientation();
#else
	virtual void				GamepadButtonDown(int theButton, int thePlayer, ulong theFlags); //Int on PC
	virtual void				GamepadButtonUp(int theButton, int thePlayer, ulong theFlags);
	virtual void				GamepadAxisMove(int theAxis, int thePlayer, int theAxisValue);
#endif
#ifdef _WIN32 //Not in BejW8 or OSX
	Reflection::CRefSymbolDb* GetReflection();
#endif
	bool						IsMainThread();
	
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	void CleanupSysFont();
	void TouchBegan(Touch* theTouch);
	void TouchEnded(Touch* theTouch);
	void TouchMoved(Touch* theTouch);
	void TouchesCanceled();
#endif

};

extern SexyAppBase* gSexyAppBase;
extern IFileDriver* gFileDriver;

extern bool gIs3D; 


};

#endif //__SEXYAPPBASE_H__
