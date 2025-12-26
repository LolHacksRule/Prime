#define INITGUID

#include "WindowsGraphicsDriver.h"
#include "D3DTester.h"
#include "../../../Debug.h"
#include "../../../AutoCrit.h"
#include "DirectXErrorString.h"
#include "DXSurface7.h"
#include "DXSurface8.h"
#include "DXSurface9.h"
#include "../../../PixelTracer.h"

using namespace Sexy;


typedef HRESULT(WINAPI* DirectDrawCreateFunc)(GUID FAR* lpGUID, LPDIRECTDRAW FAR* lplpDD, IUnknown FAR* pUnkOuter);
typedef HRESULT(WINAPI* DirectDrawCreateExFunc)(GUID FAR* lpGUID, LPVOID* lplpDD, REFIID iid, IUnknown FAR* pUnkOuter);

extern HMODULE gDDrawDLL;
static DirectDrawCreateFunc gDirectDrawCreateFunc = NULL;
static DirectDrawCreateExFunc gDirectDrawCreateExFunc = NULL;

const char* sFlagStrs[7] = { "NoBatching", "HalfTris", "NoDynVB", "PreventLag", "NoStretchRectFromTextures", "HalfPresent" };
const char* sModeStrs[8] = { "Default", "Overdraw", "PseudoOverdraw", "BatchSize", "Wireframe", "WastedOverdraw", "TextureHash", "OverdrawExact"};

WindowsGraphicsDriver::WindowsGraphicsDriver(SexyAppBase* theApp) //Heavily based off DDInterface | 43-105
{
	mApp = theApp;
	mPrimarySurface = NULL;
	mDrawSurface = NULL;
	mSecondarySurface = NULL;
	mScreenImage = NULL;
	mDD = NULL;
	mDD7 = NULL;
	mRedAddTable = NULL;
	mGreenAddTable = NULL;
	mBlueAddTable = NULL;
	mInitialized = false;
	mScanLineFailCount = 0;

	mNextCursorX = 0;
	mNextCursorY = 0;
	mCursorX = 0;
	mCursorY = 0;
	mInRedraw = false;
	mInRedrawCursor = false;
	mCursorWidth = 64;
	mCursorHeight = 64;
	mCursorImage = NULL;
	mOldCursorArea = NULL;
	mNewCursorArea = NULL;
	mHasOldCursorArea = false;
	mOldCursorArea = NULL;
	mNewCursorArea = NULL;
	mNewCursorAreaImage = NULL;
	mOldCursorAreaImage = NULL;
	mInitCount = 0;
	mRefreshRate = 60;
	mMillisecondsPerFrame = 1000 / mRefreshRate;

	mD3DInterface = NULL;
	mRenderDevice3D = NULL;
	mIs3D = false;
	for (int i = 0; i < 4; ++i) //Four buffers?
		mWindowScaleBuffers[i] = NULL;
	mIs3D = false;
	mWantD3D9 = !mApp->mNoD3D9;
	mIsD3D9 = false;
	mIsD3D8 = false;
	mIsD3D8Or9 = false;
	mPrimaryDXSurface = NULL;
	mD3DTester = NULL;
	mFov = 0.78539819;
	mNearPlane = 0.1;
	mFarPlane = 100.0;
	gDirectDrawCreateFunc = (DirectDrawCreateFunc)GetProcAddress(gDDrawDLL, "DirectDrawCreate");
	gDirectDrawCreateExFunc = (DirectDrawCreateExFunc)GetProcAddress(gDDrawDLL, "DirectDrawCreateEx");
	mRenderMode = RENDERMODE_Default;
	mRenderModeFlags = RENDERMODEF_PreventLag | RENDERMODEF_NoStretchRectFromTextures;
}

WindowsGraphicsDriver::~WindowsGraphicsDriver() //108-118
{
	delete[] mRedAddTable;
	delete[] mGreenAddTable;
	delete[] mBlueAddTable;

	Cleanup();
	CleanupMeshes();

	delete mD3DInterface;
	delete mD3DTester;
}


bool WindowsGraphicsDriver::GotDXError(HRESULT theResult, const char* theContext) //122-132
{
	if (!SUCCEEDED(theResult))
	{
		std::string anError = GetDirectXErrorString(theResult);
		mErrorString = StrFormat("%s: %s", theContext, anError.c_str());

		return true;
	}
	else
		return false;
}


DeviceImage* WindowsGraphicsDriver::GetScreenImage() //136-138
{
	return mScreenImage;
}

HRESULT WindowsGraphicsDriver::CreateSurface(LPDDSURFACEDESC2 theDesc, IUnknown** theSurface, void*) //Correct? | 141-240
{
	HRESULT aResult;
	if (mIsD3D8Or9)
	{
		if (theDesc == NULL)
			aResult = DDERR_INVALIDPARAMS;

		if (!(theDesc->dwFlags & DDSD_CAPS | DDSD_HEIGHT) == (DDSD_CAPS | DDSD_HEIGHT))
			aResult = DDERR_INVALIDCAPS;

		int aWidth = theDesc->dwWidth;
		int aHeight = theDesc->dwHeight;
		bool renderTarget = false;

		if ((theDesc->dwFlags & DDSD_CAPS) != 0)
			renderTarget = (theDesc->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) != 0;

		if (theSurface == NULL)
			aResult = DDERR_INVALIDPARAMS;

		*theSurface = mD3DInterface->CreateSurface(aWidth, aHeight, renderTarget, false);
		aResult = *theSurface != 0 ? 0 : DDERR_GENERIC;

		gOptimizeSoftwareDrawing = true;
	}
	else
	{
		if (mDD7 != NULL)
		{
			LPDIRECTDRAWSURFACE7 aSurface;
			aResult = mDD7->CreateSurface(theDesc, &aSurface, NULL);
			if (!SUCCEEDED(aResult))
				return aResult;

			aResult = aSurface->QueryInterface(IID_IDirectDrawSurface, (LPVOID*)theSurface);
			aSurface->Release();

			if (!SUCCEEDED(aResult))
				return aResult;
		}
		else
		{
			DDSURFACEDESC aDesc;
			ZeroMemory(&aDesc, sizeof(aDesc));
			aDesc.dwSize = sizeof(aDesc);
			aDesc.dwFlags = theDesc->dwFlags;
			aDesc.dwHeight = theDesc->dwHeight;
			aDesc.dwWidth = theDesc->dwWidth;
			aDesc.lPitch = theDesc->lPitch;
			aDesc.dwBackBufferCount = theDesc->dwBackBufferCount;
			aDesc.dwRefreshRate = theDesc->dwRefreshRate;
			aDesc.dwAlphaBitDepth = theDesc->dwAlphaBitDepth;
			aDesc.dwReserved = theDesc->dwReserved;
			aDesc.lpSurface = theDesc->lpSurface;
			aDesc.ddpfPixelFormat = theDesc->ddpfPixelFormat;
			aDesc.ddsCaps.dwCaps = theDesc->ddsCaps.dwCaps;

			aResult = mDD->CreateSurface(&aDesc, (LPDIRECTDRAWSURFACE*)theSurface, NULL);
			if (!SUCCEEDED(aResult))
				return aResult;
		}

		// Make sure it's 32-bit or 16-bit
		DDSURFACEDESC aDesc;
		ZeroMemory(&aDesc, sizeof(aDesc));
		aDesc.dwSize = sizeof(aDesc);
		aDesc.dwFlags = DDSD_PIXELFORMAT;
		if (mPrimarySurface == NULL)
			return DDERR_INVALIDPARAMS;
		mPrimarySurface->GetSurfaceDesc(&aDesc);
#ifdef OPTIMIZE_SOFTWARE_DRAWING
		// If things are stored blue low, green middle, red high, we can optimize a lot of our software rendering based on bit patterns.
		// This of course does not matter for native data which is already in the correct order (and can be optimized similarly).
		gOptimizeSoftwareDrawing = aDesc.ddpfPixelFormat.dwBBitMask < aDesc.ddpfPixelFormat.dwGBitMask && aDesc.ddpfPixelFormat.dwGBitMask < aDesc.ddpfPixelFormat.dwRBitMask;
#endif
		int aNumBits = aDesc.ddpfPixelFormat.dwRGBBitCount;
		if (aNumBits != 16 && aNumBits != 32)
		{
			(*theSurface)->Release();
			*theSurface = NULL;
			return DDERR_INVALIDPIXELFORMAT;
		}

	}
	return aResult;
}

void WindowsGraphicsDriver::ClearSurface(LPDIRECTDRAWSURFACE theSurface) //243-262
{
	if (theSurface)
	{
		DDSURFACEDESC desc;
		memset(&desc, 0, sizeof desc);
		desc.dwSize = sizeof desc;
		HRESULT hr = theSurface->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL);
		if (DD_OK == hr)
		{
			DWORD pixelSize = desc.ddpfPixelFormat.dwRGBBitCount / 8;
			unsigned char* p = (unsigned char*)desc.lpSurface;
			for (DWORD row = 0; row < desc.dwHeight; ++row)
			{
				memset(p, 0, pixelSize * desc.dwWidth);
				p += desc.lPitch;
			}
			theSurface->Unlock(NULL);
		}
	}
}

bool WindowsGraphicsDriver::Do3DTest(HWND theHWND) //265-288
{
	if (mD3DTester == NULL)
	{
		if (mApp->mTest3D || mApp->mAutoEnable3D)
		{
			mD3DTester = new D3DTester;
			mD3DTester->SetVidMemoryConstraints(mApp->mMinVidMemory3D, mApp->mRecommendedVidMemory3D);
			mD3DTester->TestD3D(theHWND, mDD7, this);

			if (mApp->mAutoEnable3D && mD3DTester->Is3DRecommended())
				mIs3D = true;

			if (!mD3DTester->Is3DSupported())
				mIs3D = false;

			if (mD3DTester->ResultsChanged() && !mD3DTester->Is3DRecommended())
				mIs3D = false;

			return true;
		}
	}

	return false;
}

void WindowsGraphicsDriver::SetRenderMode(ERenderMode inRenderMode) //291-296
{
	mRenderMode = inRenderMode;
}

std::string WindowsGraphicsDriver::GetRenderModeString(ERenderMode inRenderMode, ulong inRenderModeFlags, bool inIgnoreMode, bool inIgnoreFlags) //Correct? 299-345
{
	std::string modeStr;
	std::string flagStr;
	if (!inIgnoreMode)
		modeStr = sModeStrs[inRenderMode];
	if (!inIgnoreFlags)
	{
		for (int i = 0; i < RENDERMODE_CYCLE_END; i++)
		{
			if ((inRenderModeFlags & (RENDERMODEF_NoBatching << i)) != 0)
			{
				if (!flagStr.empty())
					flagStr += "|";
				flagStr += sFlagStrs[i];
			}
		}
	}
	if (inIgnoreMode)
		return flagStr;
	else
	{
		return inIgnoreFlags ? modeStr : modeStr + " (" + flagStr + ")";
	}
}

int WindowsGraphicsDriver::Init(HWND theWindow, bool IsWindowed) //Correct? | 348-950
{
	AutoCrit anAutoCrit(mCritSect);

	mInitialized = false;

	Cleanup();

	HRESULT aResult;
	DDSURFACEDESC2 aDesc;

	delete mD3DInterface; //if?
	mD3DInterface = NULL;
	mRenderDevice3D = NULL;
	gSexyAppBase->SetInteger("compat_D3DInterface", 0);

	if (gDirectDrawCreateExFunc != NULL)
	{
		aResult = gDirectDrawCreateExFunc(NULL, (LPVOID*)&mDD7, IID_IDirectDraw7, NULL);
		if (GotDXError(aResult, "DirectDrawCreateEx"))
			return RESULT_DD_CREATE_FAIL;

		aResult = mDD7->QueryInterface(IID_IDirectDraw, (LPVOID*)&mDD);
		if (GotDXError(aResult, "QueryrInterface IID_IDirectDraw")) //Lol
			return RESULT_DD_CREATE_FAIL;
	}
	else
	{
		aResult = gDirectDrawCreateFunc(NULL, &mDD, NULL);
	}

	mHWnd = theWindow;
	mWidth = mApp->mWidth;
	mHeight = mApp->mHeight;
	mFullscreenBits = mApp->mFullscreenBits;
	mIsWindowed = IsWindowed;
	mDesktopWidth = GetSystemMetrics(SM_CXSCREEN);
	mDesktopHeight = GetSystemMetrics(SM_CYSCREEN);
	mDesktopAspect.Set(mDesktopWidth, mDesktopHeight);
	if (mDesktopAspect < mApp->mMinAspect)
	{
		mDesktopAspect.mNumerator = mApp->mMinAspect.mNumerator;
		mDesktopAspect.mDenominator = mApp->mMinAspect.mDenominator;
	}
	if (mDesktopAspect > mApp->mMaxAspect)
	{
		mDesktopAspect.mNumerator = mApp->mMaxAspect.mNumerator;
		mDesktopAspect.mDenominator = mApp->mMaxAspect.mDenominator;
	}
	if (Do3DTest(theWindow))
	{
		if (mD3DTester->ResultsChanged())
			mApp->Done3dTesting();
	}
	RECT aRect;
	GetClientRect(theWindow, &aRect);

	mHWnd = theWindow;
	mWidth = mApp->mWidth;
	mHeight = mApp->mHeight;
	mAspect.Set(mWidth, mHeight);
	mDesktopWidth = GetSystemMetrics(SM_CXSCREEN);
	mDesktopHeight = GetSystemMetrics(SM_CYSCREEN);
	mDesktopAspect.Set(mDesktopWidth, mDesktopHeight);
	if (mDesktopAspect < mApp->mMinAspect)
	{
		mDesktopAspect.mNumerator = mApp->mMinAspect.mNumerator;
		mDesktopAspect.mDenominator = mApp->mMinAspect.mDenominator;
	}
	if (mDesktopAspect > mApp->mMaxAspect)
	{
		mDesktopAspect.mNumerator = mApp->mMaxAspect.mNumerator;
		mDesktopAspect.mDenominator = mApp->mMaxAspect.mDenominator;
	}
	mDisplayWidth = mWidth;
	mDisplayHeight = mHeight;
	mDisplayAspect.mNumerator = mAspect.mNumerator;
	mDisplayAspect.mDenominator = mAspect.mDenominator;
	mDisplayAspect = mAspect; //?
	mPresentationRect = Rect(0, 0, mWidth, mHeight);
	mApp->mScreenBounds = mPresentationRect;
	mFullscreenBits = mApp->mFullscreenBits;
	mIsWindowed = IsWindowed;
	mHasOldCursorArea = false;

	OutputDebugStrF(_S("Application requests %4lu x %4lu [%2d:%2d]\n"), mWidth, mHeight, mAspect.mNumerator, mAspect.mDenominator);

	if (GotDXError(aResult, "DirectDrawCreate"))
		return RESULT_DD_CREATE_FAIL;

	if (IsWindowed)
	{
		OutputDebugStrF(_S("Hack aspect is                   [%2d:%2d]\n"), mApp->mWindowAspect.mNumerator, mApp->mWindowAspect.mDenominator);

		Ratio aWindowAspect;
		aWindowAspect.mNumerator = mApp->mMaxAspect.mNumerator;
		aWindowAspect.mDenominator = mApp->mMaxAspect.mDenominator;

		if (mDesktopAspect < mApp->mMinAspect)
		{
			mDesktopAspect.mNumerator = mApp->mMinAspect.mNumerator;
			mDesktopAspect.mDenominator = mApp->mMinAspect.mDenominator;
		}
		if (mDesktopAspect > mApp->mMaxAspect)
		{
			mDesktopAspect.mNumerator = mApp->mMaxAspect.mNumerator;
			mDesktopAspect.mDenominator = mApp->mMaxAspect.mDenominator;
		}
		int anExpectedWidth = mHeight * aWindowAspect;

		if (mApp->mEnableWindowAspect && anExpectedWidth < mWidth)
		{
			mIsWidescreen = true;

			mDisplayWidth = mHeight * mApp->mWindowAspect;
			mDisplayHeight = mHeight;
			mDisplayAspect = mApp->mWindowAspect;
			mDesktopAspect.mNumerator = mApp->mMinAspect.mNumerator;
			mDesktopAspect.mDenominator = mApp->mMinAspect.mDenominator;
			if (aWindowAspect != mApp->mWindowAspect)
			{
				RECT rc;
				POINT pt;
				WINDOWINFO info;
				::GetWindowInfo(theWindow, &info);
				::GetClientRect(theWindow, &rc);

				pt.x = rc.left;
				pt.y = rc.top;
				::ClientToScreen(theWindow, &pt);
				rc.left = pt.x - (mDisplayWidth - mWidth) / 2;
				rc.top = pt.y - (mDisplayHeight - mHeight) / 2;
				rc.right = rc.left + mDisplayWidth;
				rc.bottom = rc.top + mDisplayHeight;
				::AdjustWindowRectEx(&rc, info.dwStyle, false, info.dwExStyle);
				::MoveWindow(theWindow, max(0, rc.left), max(0, rc.top), rc.right - rc.left, rc.bottom - rc.top, false);
			}

			if (mApp->mWidescreenAware)
			{
				mWidth = mDisplayWidth;
				mHeight = mDisplayHeight;
				mAspect = mDisplayAspect;

				mDesktopAspect.mNumerator = mApp->mMinAspect.mNumerator;
				mDesktopAspect.mDenominator = mApp->mMinAspect.mDenominator;

				mPresentationRect.mWidth = mDisplayWidth;
				mPresentationRect.mHeight = mDisplayHeight;
				mPresentationRect.mX = 0;
			}
			else
			{
				// Set the dest rect for drawing the back buffer to the center of
				// the wide display.
				mPresentationRect.mWidth = mWidth;
				mPresentationRect.mHeight = mHeight;
				mPresentationRect.mX = (mDisplayWidth - mPresentationRect.mWidth) / 2;
			}
			mPresentationRect.mY = 0;
		}
		OutputDebugStrF(_S("Window is            %4lu x %4lu [%2d:%2d]\n"), mDisplayWidth, mDisplayHeight, mDisplayAspect.mNumerator, mDisplayAspect.mDenominator);
	}
	else
	{
		OutputDebugStrF(_S("Desktop is           %4lu x %4lu [%2d:%2d]\n"), mDesktopWidth, mDesktopHeight, mDesktopAspect.mNumerator, mDesktopAspect.mDenominator);
		if (mIs3D && mAspect != mDesktopAspect || mDisplayWidth != mDesktopWidth || mDisplayHeight != mDesktopHeight)
		{
			mIsWidescreen = true;

			mDisplayWidth = mDesktopWidth;
			mDisplayHeight = mDesktopHeight;
			mDisplayAspect = mDesktopAspect;

			mDesktopAspect.mNumerator = mApp->mMinAspect.mNumerator;
			mDesktopAspect.mDenominator = mApp->mMinAspect.mDenominator;

			if (mApp->mWidescreenAware)
			{
				mAspect = mDisplayAspect;
				mWidth = mHeight * mAspect;

				mPresentationRect.mWidth = mDisplayWidth;
				mPresentationRect.mHeight = mDisplayHeight;
				mPresentationRect.mX = 0;
				mPresentationRect.mY = 0;
			}
			else
			{
				// Set the dest rect for drawing the back buffer to the center of
				// the wide display.
				mPresentationRect.mWidth = mWidth * mDisplayHeight / mHeight;
				mPresentationRect.mHeight = mDisplayHeight;
				mPresentationRect.mX = (mDisplayWidth - mPresentationRect.mWidth) / 2;
				mPresentationRect.mY = 0;
			}
		}
		OutputDebugStrF(_S("Display is           %4lu x %4lu [%2d:%2d]\n"), mDisplayWidth, mDisplayHeight, mDisplayAspect.mNumerator, mDisplayAspect.mDenominator);
	}
	mIsD3D9 = false;
	mIsD3D8 = false;
	mIsD3D8Or9 = false;
	if (mIs3D)
	{
		mD3DInterface = NULL;
		mRenderDevice3D = NULL;
		if (mWantD3D9)
		{
			mD3DInterface = CreateD3D9Interface();
			mRenderDevice3D = mD3DInterface;
			if (!mD3DInterface->InitFromGraphicsDriver(this, (EResult*)RESULT_OK, false, false))
			{
				delete mD3DInterface;
				mRenderDevice3D = NULL;
				return RESULT_OK;
			}
		}
		if (mD3DInterface == NULL)
		{
			mD3DInterface = CreateD3D8Interface();
			mRenderDevice3D = mD3DInterface;
			if (!mD3DInterface->InitFromGraphicsDriver(this, (EResult*)RESULT_OK, false, false))
			{
				delete mD3DInterface;
				mRenderDevice3D = NULL;
				return RESULT_OK;
			}
		}
		if (mD3DInterface == NULL) //? Should be else?
			return RESULT_3D_FAIL;
		if (!IsWindowed)
		{
			ulong aBackBufferWidth;
			ulong aBackBufferHeight;
			mD3DInterface->GetBackBufferDimensions(aBackBufferWidth, aBackBufferHeight);
			if (aBackBufferWidth != mDisplayWidth || aBackBufferHeight != mDisplayHeight)
			{
				mDisplayWidth = aBackBufferWidth;
				mDisplayHeight = aBackBufferHeight;
				mDisplayAspect.Set(mDisplayWidth, mDisplayHeight);
				if (mApp->mWidescreenAware) //?
				{
					// Setup the draw buffer(s) at the same aspect ratio as the desktop,
					// but with the height requested by the application.
					mAspect = mDisplayAspect;
					mWidth = mHeight * mAspect;

					mPresentationRect.mWidth = mDisplayWidth;
					mPresentationRect.mHeight = mDisplayHeight;
					mPresentationRect.mX = 0;
					mPresentationRect.mY = 0;
				}
				else
				{
					// Set the dest rect for drawing the back buffer to the center of
					// the wide display.
					mPresentationRect.mWidth = mWidth * mDisplayHeight / mHeight;
					mPresentationRect.mHeight = mDisplayHeight;
					mPresentationRect.mX = (mDisplayWidth - mPresentationRect.mWidth) / 2;
					mPresentationRect.mY = 0;
				}
			}
		}

		if (mDD != NULL)
		{
			mDD->SetCooperativeLevel(mHWnd, DDSCL_NORMAL);
			mDD->Release();
			mDD = NULL;
		}

		if (mDD7 != NULL)
		{
			mDD7->Release();
			mDD7 = NULL;
		}
		mInitialized = true;
	}
	if (!mIsD3D8Or9)
	{
		if (IsWindowed)
		{
			aResult = mDD->SetCooperativeLevel(theWindow, DDSCL_NORMAL);

			ZeroMemory(&aDesc, sizeof(aDesc));
			aDesc.dwSize = sizeof(aDesc);
			aDesc.dwFlags = DDSD_CAPS;

			aDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
			if (mIs3D)
				aDesc.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;

			aDesc.dwBackBufferCount = 1;
			aResult = CreateSurface(&aDesc, (IUnknown**)mPrimarySurface, NULL);
			if (aResult == DDERR_INVALIDPIXELFORMAT) // check for non 16 or 32 bit primary surface
				return RESULT_INVALID_COLORDEPTH;
			else if (GotDXError(aResult, "CreateSurface Windowed Primary"))
				return RESULT_SURFACE_FAIL;

			mPrimaryDXSurface = new DXSurface7(this);
			mPrimaryDXSurface->SetSurface(mPrimarySurface);

			ZeroMemory(&aDesc, sizeof(aDesc));
			aDesc.dwSize = sizeof(aDesc);
			aDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
			aDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
			if (mIs3D)
				aDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;

			aDesc.dwWidth = mWidth;
			aDesc.dwHeight = mHeight;

			aResult = CreateSurface(&aDesc, &mDrawSurface, NULL);
			if (GotDXError(aResult, "CreateSurface Windowed DrawSurface"))
				return RESULT_SURFACE_FAIL;

			IDirectDrawClipper* aClipper;
			aResult = mDD->CreateClipper(0, &aClipper, NULL);
			if (GotDXError(aResult, "CreateClipper Windowed")) return RESULT_FAIL;

			// Associate the clipper with the window
			aResult = aClipper->SetHWnd(0, theWindow);
			if (SUCCEEDED(aResult))
				aResult = mPrimarySurface->SetClipper(aClipper);

			aClipper->Release(); //IDA says stuff is below this but I doubt it
		}
		else
		{
			aResult = mDD->SetCooperativeLevel(theWindow, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
			if (GotDXError(aResult, "SetCooperativeLevel FullScreen"))
				return RESULT_EXCLUSIVE_FAIL;

			aResult = mDD->SetDisplayMode(mDisplayWidth, mDisplayHeight, mFullscreenBits);

			if (GotDXError(aResult, "SetDisplayMode FullScreen"))
				return RESULT_DISPCHANGE_FAIL;

			ZeroMemory(&aDesc, sizeof(aDesc));
			aDesc.dwSize = sizeof(aDesc);
			aDesc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
			aDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
				DDSCAPS_FLIP |
				DDSCAPS_COMPLEX;
			aDesc.dwBackBufferCount = 1;

			if (mIs3D)
				aDesc.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;

			aResult = CreateSurface(&aDesc, (IUnknown**)mPrimarySurface, NULL);
			if (GotDXError(aResult, "CreateSurface FullScreen Primary"))
				return RESULT_SURFACE_FAIL;

			DDSCAPS aCaps;
			ZeroMemory(&aCaps, sizeof(aCaps));
			aCaps.dwCaps = DDSCAPS_BACKBUFFER;
			aResult = mPrimarySurface->GetAttachedSurface(&aCaps, &mSecondarySurface);

			if (GotDXError(aResult, "GetAttachedSurface"))
				return RESULT_SURFACE_FAIL;

			if (mIsWidescreen)
			{
				ClearSurface(mPrimarySurface);
				ClearSurface(mSecondarySurface);
			}

			ZeroMemory(&aDesc, sizeof(aDesc));
			aDesc.dwSize = sizeof(aDesc);
			aDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
			aDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
			if (mIs3D)
				aDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;

			aDesc.dwWidth = mWidth;
			aDesc.dwHeight = mHeight;

			aResult = CreateSurface(&aDesc, &mDrawSurface, NULL);
			if (GotDXError(aResult, "CreateSurface FullScreen DrawSurface"))
				return RESULT_SURFACE_FAIL;

			IDirectDrawClipper* aClipper;
			aResult = mDD->CreateClipper(0, &aClipper, NULL);
			if (GotDXError(aResult, "CreateClipper FullScreen")) return RESULT_FAIL;

			// Associate the clipper with the window
			aResult = aClipper->SetHWnd(0, theWindow);
			if (SUCCEEDED(aResult))
				aResult = mPrimarySurface->SetClipper(aClipper);

			mSecondarySurfaceImage = new DeviceImage(this);
			mSecondarySurfaceImage->SetSurface(mSecondarySurface);

			if (mIsWidescreen)
			{
				ClearSurface(mPrimarySurface);
				ClearSurface(mSecondarySurface);
			}

			aClipper->Release();

			if (!mApp->mFullScreenPageFlip)
				mDD->FlipToGDISurface();
		}
	}
	RECT aClientRect;
	GetClientRect(mHWnd, &aClientRect);
	OutputDebugStringA(StrFormat("mHWnd=%08X mW=%d mH=%d WW=%d WH=%d Window=%d D3D8=%d D3D9=%d\r\n", mHWnd, mWidth, mHeight, aRect.right, aRect.bottom, mIsWindowed, mIsD3D8, mIsD3D9).c_str());

	if ((mIsWindowed || mIsD3D8Or9) && (aClientRect.right != mWidth || aClientRect.bottom != mHeight))
		WindowResize(aRect.right, aRect.bottom);

	OutputDebugStrF(_S("Draw buffer is       %4lu x %4lu [%2d:%2d]\n"), mWidth, mHeight, mAspect.mNumerator, mAspect.mDenominator);

	if (FAILED(mDD->GetMonitorFrequency(&mRefreshRate)) || mRefreshRate < 60)
	{
		mApp->mVSyncBroken = true;
		mRefreshRate = 60;
	}

	if (mRefreshRate < 60)
	{
		mApp->mVSyncBroken = true;
		mRefreshRate = 60;
	}
	else if (mRefreshRate > 100) // We must have at least 1 Update per UpdateF for demo compatibility
	{
		mApp->mVSyncBroken = true;
		mRefreshRate = 100;
	}

	mMillisecondsPerFrame = 1000 / mRefreshRate;

	// Create the images needed for cursor stuff
	ZeroMemory(&aDesc, sizeof(aDesc));
	aDesc.dwSize = sizeof(aDesc);
	aDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	aDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	aDesc.dwWidth = mCursorWidth;
	aDesc.dwHeight = mCursorHeight;

	if (!mIsD3D8Or9)
	{
		IUnknown* aSurfaceTemp;
		aResult = CreateSurface(&aDesc, &aSurfaceTemp, NULL);
		if (GotDXError(aResult, "CreateSurface OldCursorArea"))
			return RESULT_SURFACE_FAIL;

		mOldCursorAreaImage = new DeviceImage(this);
		mOldCursorAreaImage->SetSurface(mOldCursorArea);
		mOldCursorAreaImage->SetImageMode(false, false);
		mOldCursorArea = (LPDIRECTDRAWSURFACE)aSurfaceTemp;

		mNewCursorAreaImage = new DeviceImage(this);
		mNewCursorAreaImage->SetSurface(mNewCursorArea);
		mNewCursorAreaImage->SetImageMode(false, false);
		mNewCursorArea = (LPDIRECTDRAWSURFACE)aSurfaceTemp;

		aResult = CreateSurface(&aDesc, &aSurfaceTemp, NULL);
		if (GotDXError(aResult, "CreateSurface NewCursorArea"))
			return RESULT_SURFACE_FAIL;
	}

	// Get data from the primary surface
	if (mPrimarySurface != NULL)
	{
		DDSURFACEDESC aDesc;

		ZeroMemory(&aDesc, sizeof(aDesc));
		aDesc.dwSize = sizeof(aDesc);
		aDesc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
		HRESULT aResult = mPrimarySurface->GetSurfaceDesc(&aDesc);

		if ((aDesc.ddpfPixelFormat.dwRGBBitCount != 16) &&
			(aDesc.ddpfPixelFormat.dwRGBBitCount != 32))
			return RESULT_INVALID_COLORDEPTH;

		mRGBBits = aDesc.ddpfPixelFormat.dwRGBBitCount;
		mRedMask = aDesc.ddpfPixelFormat.dwRBitMask;
		mGreenMask = aDesc.ddpfPixelFormat.dwGBitMask;
		mBlueMask = aDesc.ddpfPixelFormat.dwBBitMask;

		int i;
		for (i = 32; i >= 0; i--)
		{
			if (((mRedMask >> i) & 1) != 0)
				mRedShift = i;
			if (((mGreenMask >> i) & 1) != 0)
				mGreenShift = i;
			if (((mBlueMask >> i) & 1) != 0)
				mBlueShift = i;
		}

		for (i = 0; i < 32; i++)
		{
			if ((i + mRedShift < 32) && ((mRedMask >> (i + mRedShift)) != 0))
				mRedBits = i + 1;
			if ((i + mGreenShift < 32) && ((mGreenMask >> (i + mGreenShift)) != 0))
				mGreenBits = i + 1;
			if ((i + mBlueShift < 32) && ((mBlueMask >> (i + mBlueShift)) != 0))
				mBlueBits = i + 1;
		}

		delete[] mRedAddTable;
		delete[] mGreenAddTable;
		delete[] mBlueAddTable;

		int aMaxR = (1 << mRedBits) - 1;
		int aMaxG = (1 << mGreenBits) - 1;
		int aMaxB = (1 << mBlueBits) - 1;

		mRedAddTable = new int[aMaxR * 2 + 1];
		mGreenAddTable = new int[aMaxG * 2 + 1];
		mBlueAddTable = new int[aMaxB * 2 + 1];

		for (i = 0; i < aMaxR * 2 + 1; i++)
			mRedAddTable[i] = min(i, aMaxR);
		for (i = 0; i < aMaxG * 2 + 1; i++)
			mGreenAddTable[i] = min(i, aMaxG);
		for (i = 0; i < aMaxB * 2 + 1; i++)
			mBlueAddTable[i] = min(i, aMaxB);

		// Create the tables that we will use to convert from 
		// internal color representation to surface representation
		for (i = 0; i < 256; i++)
		{
			mRedConvTable[i] = ((i * mRedMask) / 255) & mRedMask;
			mGreenConvTable[i] = ((i * mGreenMask) / 255) & mGreenMask;
			mBlueConvTable[i] = ((i * mBlueMask) / 255) & mBlueMask;
		}
	}

	if (!mIs3D || mIsD3D8Or9 || mD3DInterface->InitFromGraphicsDriver(this, 0, 0, 0))
	{
		if (!mIs3D)
		{
			mScreenImage = new DeviceImage(this);
			mScreenImage->SetSurface(mDrawSurface);
			mScreenImage->SetImageMode(false, false);
		}
		mInitCount++;
		mInitialized = true;
		if (mApp->mCompatCfgMachine)
			mApp->mCompatCfgMachine->MachineExecuteFunction("DDInterfaceReady", 0);
		return RESULT_OK;
	}
	else
	{
		mErrorString = "3D init error: ";
		mErrorString += D3DInterface::sErrorString;
		return RESULT_3D_FAIL;
	}
}

void WindowsGraphicsDriver::WindowResize(int theWidth, int theHeight) //Correct? | 953-1011
{
	OutputDebugStringA(StrFormat("WindowResize(%d, %d)\r\n", theWidth, theHeight).c_str());
	HRESULT aResult;
	DDSURFACEDESC2 aDesc;
	for (int i = 0; i < 4; i++)
	{
		if (mWindowScaleBuffers[i])
		{
			mWindowScaleBuffers[i]->Release();
			mWindowScaleBuffers[i] = NULL;
		}
	}
	if (mWidth == theWidth && mHeight == theHeight)
	{
		if (mIsWindowed)
		{
			Rect(0, 0, theWidth, theHeight);
		}
	}
	ZeroMemory(&aDesc, sizeof(aDesc));
	aDesc.dwSize = sizeof(aDesc);
	aDesc.dwFlags = 7; //?
	aDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	if (mIs3D)
		aDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;
	int aCurWidth = mWidth;
	int aCurHeight = mHeight;
	int aResizeBufIdx = 0;
	if (mIs3D && mD3DInterface && mD3DInterface->CanBltSurface(false))
	{
		while (aCurHeight > 2 * theHeight && aResizeBufIdx < 3)
		{
			aCurWidth /= 2;
			aCurHeight /= 2;
			aDesc.dwWidth = aCurWidth;
			aDesc.dwHeight = aCurHeight;
			aResult = CreateSurface(&aDesc, &mWindowScaleBuffers[aResizeBufIdx++], 0);
			if (GotDXError(aResult, "CreateSurface Windowed DrawSurface"))
				return;
		}
	}
	if (mIsD3D8Or9)
	{
		aDesc.dwWidth = theWidth;
		aDesc.dwHeight = theHeight;
		aResult = CreateSurface(&aDesc, &mWindowScaleBuffers[aResizeBufIdx], 0);
		++aResizeBufIdx;
		if (GotDXError(aResult, "CreateSurface Windowed DrawSurface"))
			return;
		if (mIsWindowed)
		{
			Rect(0, 0, theWidth, theHeight);
		}
	}
}

void WindowsGraphicsDriver::RemapMouse(int& theX, int& theY) //1014-1020
{
	if (mInitialized)
	{
		theX = (theX - mPresentationRect.mX) * mWidth / mPresentationRect.mWidth;
		theY = (theY - mPresentationRect.mY) * mHeight / mPresentationRect.mHeight;
	}
}

ulong WindowsGraphicsDriver::GetColorRef(ulong theRGB) //1023-1041 (Matched probably)
{
	DDSURFACEDESC aDesc;

	ZeroMemory(&aDesc, sizeof(aDesc));
	aDesc.dwSize = sizeof(aDesc);
	aDesc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	HRESULT aResult = mPrimarySurface->GetSurfaceDesc(&aDesc);

	BYTE bRed = (BYTE)((theRGB >> 16) & 0xFF);
	BYTE bGreen = (BYTE)((theRGB >> 8) & 0xFF);
	BYTE bBlue = (BYTE)((theRGB) & 0xFF);

	ulong aColor;
	aColor = ((DWORD(((LONGLONG)bRed * (LONGLONG)aDesc.ddpfPixelFormat.dwRBitMask) / 255) & aDesc.ddpfPixelFormat.dwRBitMask) |
		(DWORD(((LONGLONG)bGreen * (LONGLONG)aDesc.ddpfPixelFormat.dwGBitMask) / 255) & aDesc.ddpfPixelFormat.dwGBitMask) |
		(DWORD(((LONGLONG)bBlue * (LONGLONG)aDesc.ddpfPixelFormat.dwBBitMask) / 255) & aDesc.ddpfPixelFormat.dwBBitMask));

	return aColor;
}

void WindowsGraphicsDriver::AddDeviceImage(DeviceImage* theDDImage) //1044-1048
{
	AutoCrit anAutoCrit(mApp->mImageSetCritSect);

	mDDImageSet.insert(theDDImage);
}

void WindowsGraphicsDriver::RemoveDeviceImage(DeviceImage* theDDImage) //1051-1057
{
	AutoCrit anAutoCrit(mApp->mImageSetCritSect);

	DDImageSet::iterator anItr = mDDImageSet.find(theDDImage);
	if (anItr != mDDImageSet.end())
		mDDImageSet.erase(anItr);
}

void WindowsGraphicsDriver::Remove3DData(MemoryImage* theImage) // for 3d texture cleanup | 1060-1063 (Matched)
{
	if (mD3DInterface)
		mD3DInterface->RemoveImageRenderData(theImage);
}

void WindowsGraphicsDriver::Cleanup() //1066-1169 (Prob correct)
{
	AutoCrit anAutoCrit(mCritSect);

	mInitialized = false;

	if (mOldCursorAreaImage != NULL)
	{
		delete mOldCursorAreaImage;
		mOldCursorAreaImage = NULL;
	}

	if (mNewCursorAreaImage != NULL)
	{
		delete mNewCursorAreaImage;
		mNewCursorAreaImage = NULL;
	}

	for (int i = 0; i < 4; ++i)
	{
		if (mWindowScaleBuffers[i] != NULL)
		{
			mWindowScaleBuffers[i]->Release();
			mWindowScaleBuffers[i] = NULL;
		}
	}

	if (mScreenImage != NULL) //Check
	{
		if (mScreenImage->mSurface != NULL && mScreenImage->mSurface->GetSurfacePtr() == mDrawSurface)
			mDrawSurface = NULL;
		delete mScreenImage;
		mScreenImage = NULL;
	}

	if (mSecondarySurfaceImage != NULL)
	{
		delete mSecondarySurfaceImage;
		mSecondarySurfaceImage = NULL;
	}

	if (mDrawSurface != NULL)
	{
		mDrawSurface->Release();
		mDrawSurface = NULL;
	}

	if (mSecondarySurface != NULL)
	{
		mSecondarySurface->Release();
		mSecondarySurface = NULL;
	}

	if (mPrimarySurface != NULL)
	{
		mPrimarySurface->Release();
		mPrimarySurface = NULL;
	}

	if (mDD != NULL)
	{
		mDD->SetCooperativeLevel(mHWnd, DDSCL_NORMAL);
		mDD->Release();
		mDD = NULL;
	}

	if (mDD7 != NULL)
	{
		mDD7->Release();
		mDD7 = NULL;
	}

	if (mPrimaryDXSurface != NULL)
	{
		mPrimaryDXSurface->SetSurface(NULL);
		delete mPrimaryDXSurface;
		mPrimaryDXSurface = NULL;
	}

	if (mD3DInterface != NULL)
		mD3DInterface->Cleanup();
	if (gSexyAppBase->mSharedRTPool)
		gSexyAppBase->mSharedRTPool->InvalidateSurfaces();
}

void WindowsGraphicsDriver::CleanupMeshes() //1172-1178
{
	while (mMeshSet.size())
	{
		Mesh* aMesh = *mMeshSet.begin();
		delete aMesh;
	}
}

bool WindowsGraphicsDriver::CopyBitmap(LPDIRECTDRAWSURFACE theSurface, HBITMAP theBitmap, int theX, int theY, int theWidth, int theHeight) //1181-1239
{
	AutoCrit anAutoCrit(mCritSect);

	HRESULT hr;

	if (theBitmap == NULL || theSurface == NULL) return false;

	// Make sure this surface is restored.
	theSurface->Restore();

	// Get size of the bitmap
	BITMAP bmBitmap;
	GetObject(theBitmap, sizeof(bmBitmap), &bmBitmap);
	theWidth = (theWidth == 0) ? bmBitmap.bmWidth : theWidth;
	theHeight = (theHeight == 0) ? bmBitmap.bmHeight : theHeight;

	DDSURFACEDESC aDesc;
	ZeroMemory(&aDesc, sizeof(aDesc));
	aDesc.dwSize = sizeof(aDesc);
	aDesc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
	hr = theSurface->GetSurfaceDesc(&aDesc);
	if (FAILED(hr)) return false;

	// Create memory DC
	HDC hdcImage = CreateCompatibleDC(NULL);
	if (hdcImage != NULL)
	{
		// Select bitmap into memory DC
		HBITMAP anOldBitmap = (HBITMAP)SelectObject(hdcImage, theBitmap);

		// Get surface DC
		HDC hdc;
		hr = theSurface->GetDC(&hdc);
		if (SUCCEEDED(hr))
		{
			// Copy the bitmap. Use BitBlt, if possible, otherwise use
			// StretchBlt
			if (theWidth == aDesc.dwWidth && theHeight == aDesc.dwHeight)
			{
				BitBlt(hdc, 0, 0, theWidth, theHeight, hdcImage, theX, theY, SRCCOPY);
			}
			else
			{
				StretchBlt(hdc, 0, 0, aDesc.dwWidth, aDesc.dwHeight, hdcImage,
					theX, theY, theWidth, theHeight, SRCCOPY);
			}

			// Release surface DC
			theSurface->ReleaseDC(hdc);
		}

		// Select old bitmap into the memory DC and delete the DC
		SelectObject(hdcImage, anOldBitmap);
		DeleteDC(hdcImage);
	}
	else return false;

	return true;
}

bool WindowsGraphicsDriver::Redraw(Rect* theClipRect) //Correct? | 1242-1705
{
	AutoCrit anAutoCrit(mCritSect);

	if (!mInitialized)
		return false;

	DDBLTFX aBltFX;
	ZeroMemory(&aBltFX, sizeof(aBltFX));
	aBltFX.dwSize = sizeof(aBltFX);

	mInRedraw = true;

	DWORD aTickNow = GetTickCount();

	if (mIs3D)
	{
		if (!mD3DInterface->sErrorString.empty())
		{
			OutputDebugStringA(mD3DInterface->sErrorString.c_str());
			OutputDebugStringA("\r\n");
			mInRedraw = false;
			mIs3D = false;
			return false;
		}

		mD3DInterface->Flush(RenderDevice3D::FLUSHF_CurrentScene);
	}

	Rect aDestRect;
	Rect aSrcRect;
	if (NULL == theClipRect || mIsWidescreen)
	{
		aDestRect = mPresentationRect;
		aSrcRect = Rect(0, 0, mWidth, mHeight);
	}
	else
	{
		aDestRect = *theClipRect;
		aSrcRect = *theClipRect;
	}
	if (mIsWidescreen)
	{
		aBltFX.dwDDFX = DDBLTFX_ARITHSTRETCHY;
	}
	mGraphicsMetrics.NextFrame();
	if (mIsD3D8Or9)
	{
		IUnknown* aCurSurface = mDrawSurface;
		if (mWindowScaleBuffers[0])
		{
			Rect anOldDestRect = aDestRect;
			for (int aBufferIdx = 0; aBufferIdx < 4 && mWindowScaleBuffers[aBufferIdx]; aBufferIdx++)
			{
				if (aBufferIdx != 3 && mWindowScaleBuffers[aBufferIdx + 1] || mIsD3D8Or9)
					aDestRect = Rect(0, 0, mWidth/2, mHeight/2);
				else
					aDestRect = mPresentationRect;
				mD3DInterface->SetRenderTargetSurface(mWindowScaleBuffers[aBufferIdx]);
				mD3DInterface->BltSurface(aCurSurface, Rect(0, 0, aDestRect.mWidth, aDestRect.mHeight), Rect(0, 0, aSrcRect.mWidth, aSrcRect.mHeight));
				aCurSurface = mWindowScaleBuffers[aBufferIdx];
				aSrcRect = aDestRect;
			}
			aDestRect = anOldDestRect;
		}
		float stretchFactorX = 1.0;
		float stretchFactorY = 1.0;
		if (aCurSurface != NULL)
		{
			mD3DInterface->SetRenderTargetSurface((mD3DInterface->GetBackBufferSurface()));
			Rect aSrcRect2(aSrcRect);
			Rect aDestRect2(aDestRect);
			if (mIsWindowed && (aDestRect2.mWidth > aSrcRect2.mWidth || aDestRect2.mHeight > aSrcRect2.mHeight))
			{
				mD3DInterface->BltSurface(aCurSurface, aDestRect2, aSrcRect2); //?
				if (aDestRect2.mWidth > aSrcRect2.mWidth)
					stretchFactorX = aDestRect2.mWidth / aSrcRect2.mWidth;
				if (aDestRect2.mHeight > aSrcRect2.mHeight)
					stretchFactorY = aDestRect2.mHeight / aSrcRect2.mHeight;
			}
			else
			{
				if (!mIsWindowed && mIsWidescreen && !mApp->mWidescreenAware)
					mD3DInterface->ClearColorBuffer(Color(0, 0, 0, 0));
				mD3DInterface->BltSurface(aCurSurface, aDestRect, aSrcRect); //?
				aSrcRect = aDestRect;
			}
		}
		mCursorX = mNextCursorX;
		mCursorY = mNextCursorY;
		if (!mInRedrawCursor && mD3DInterface->SceneBegun())
		{
			int oldCursorX = mCursorX;
			int oldCursorY = mCursorY;
			mCursorX /= stretchFactorX;
			mCursorY /= stretchFactorY;
			DrawCursorTo(NULL, false);
			mCursorX = oldCursorX;
			mCursorY = oldCursorY;
		}
		mD3DInterface->Flush(RenderDevice3D::FLUSHF_CurrentScene);
		bool result = mD3DInterface->Present(&aSrcRect, &aDestRect);
		mD3DInterface->SetRenderTargetSurface(mDrawSurface);
		if (mRenderMode && !gTracingPixels)
			mD3DInterface->ClearColorBuffer(Color::Black);
		mInRedraw = false;
		if (mD3DInterface->sErrorString.length())
		{
			OutputDebugStringA(mD3DInterface->sErrorString.c_str());
			OutputDebugStringA("\r\n");
		}
		return result;
	}
	else
	{
		POINT aPoint = { 0, 0 };

		ClientToScreen(mHWnd, &aPoint);

		RECT winDestRect = aDestRect.ToRECT();

		OffsetRect(&winDestRect, aPoint.x, aPoint.y);

		aDestRect = winDestRect;

		DDSURFACEDESC aDesc;
		ZeroMemory(&aDesc, sizeof(&aDesc));
		aDesc.dwSize = sizeof(aDesc);
		LPDIRECTDRAWSURFACE aDrawSurface = (LPDIRECTDRAWSURFACE)mDrawSurface;
		aDrawSurface->Lock(NULL, &aDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_READONLY, 0);
		aDrawSurface->Unlock(NULL);

		if (mIsWindowed)
		{
			HRESULT aResult;

			//DWORD aScanLine;
			//mDD->GetScanLine(&aScanLine);

			int aScreenHeight = GetSystemMetrics(SM_CYSCREEN);

			LPDIRECTDRAWSURFACE aCurSurface = (LPDIRECTDRAWSURFACE)mDrawSurface;

			if (mWindowScaleBuffers[0])
			{
				Rect anOldDestRect = aDestRect;
				for (int aBufferIdx = 0; aBufferIdx < 4 && mWindowScaleBuffers[aBufferIdx]; aBufferIdx++)
				{
					if (aBufferIdx != 3 && mWindowScaleBuffers[aBufferIdx + 1])
						aDestRect = Rect(0, 0, aSrcRect.mWidth / 2, aSrcRect.mHeight / 2);
					else
						aDestRect = mPresentationRect;
					RECT winSrcRect = aSrcRect.ToRECT();
					((LPDIRECTDRAWSURFACE)mWindowScaleBuffers[aBufferIdx])->Blt(&aDestRect.ToRECT(), aCurSurface, &winSrcRect, DDBLT_WAIT, &aBltFX);
					aCurSurface = (LPDIRECTDRAWSURFACE)mWindowScaleBuffers[aBufferIdx];
					aSrcRect = aDestRect;
				}
				aDestRect = anOldDestRect;
			}
			mCursorX = mNextCursorX;
			mCursorY = mNextCursorY;

			DrawCursorTo(aCurSurface, false);

			if ((mApp->mWaitForVSync) && (!mApp->mSoftVSyncWait))
			{
				bool scanLineFail = false;

				if (mScanLineFailCount >= 3)
					scanLineFail = true;

				if (!scanLineFail)
				{
					int aHalfMarkClient = mApp->mHeight / 2;
					int aHalfMarkScreen = aDestRect.mY + aHalfMarkClient;
					int aBotMarkScreen = aDestRect.mY + mHeight;

					DWORD aScanLine = 0x7FFFFFFF;

					bool wasLess = false;

					DWORD aTopStartTick = GetTickCount();

					for (;;)
					{
						aResult = mDD->GetScanLine(&aScanLine);

						if (aResult == DD_OK)
						{
							// Wait until we scan below half way on the window
							int aHalfMarkDist = aHalfMarkScreen - aScanLine;

							if ((aHalfMarkDist <= 0) || ((int)aScanLine >= aScreenHeight))
							{
								if (wasLess)
									break;
							}
							else
							{
								wasLess = true;
							}
						}
						else
						{
							if (aResult == DDERR_VERTICALBLANKINPROGRESS)
							{
								if (wasLess)
									break;
							}
							else
							{
								scanLineFail = true;
								break;
							}
						}

						DWORD aTickNow = GetTickCount();
						if (aTickNow - aTopStartTick >= 200)
						{
							// It shouldn't take this long
							scanLineFail = true;
							break;
						}

						if (!scanLineFail)
						{
							mScanLineFailCount = 0;

							RECT aTopDestRect = { aDestRect.ToRECT().left, aDestRect.ToRECT().top, aDestRect.ToRECT().right, aHalfMarkScreen };
							RECT aTopSrcRect = { aSrcRect.ToRECT().left, aSrcRect.ToRECT().top, aSrcRect.ToRECT().right, aHalfMarkClient };
							aResult = mPrimarySurface->Blt(&aTopDestRect, aCurSurface, &aTopSrcRect, DDBLT_WAIT, &aBltFX);

							DWORD aLastScanLine = aScanLine;

							for (;;)
							{
								if (SUCCEEDED(mDD->GetScanLine(&aScanLine)))
								{
									// Wait until we scan below the bottom of the window
									int aHalfMarkDist = aBotMarkScreen - aScanLine;
									if ((aScanLine < aLastScanLine) || (aHalfMarkDist <= 0))
										break;
								}
							}

							RECT aBotDestRect = { aDestRect.ToRECT().left, aHalfMarkScreen, aDestRect.ToRECT().right, aDestRect.ToRECT().bottom };
							RECT aBotSrcRect = { aSrcRect.ToRECT().left, aHalfMarkClient, aSrcRect.ToRECT().right, aSrcRect.ToRECT().bottom };
							aResult = mPrimarySurface->Blt(&aBotDestRect, aCurSurface, &aBotSrcRect, DDBLT_WAIT, &aBltFX);
						}
						else
						{
							mScanLineFailCount++;
						}
					}

					if (scanLineFail)
					{
						mDD->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);
						aResult = mPrimarySurface->Blt(&aDestRect.ToRECT(), aDrawSurface, &aSrcRect.ToRECT(), DDBLT_WAIT, &aBltFX);
					}
				}
				else
				{
					aResult = mPrimarySurface->Blt(&aDestRect.ToRECT(), aDrawSurface, &aSrcRect.ToRECT(), DDBLT_WAIT, &aBltFX);
				}

				if (mHasOldCursorArea)
				{
					// Restore from the drawn surface, incase we don't do a redraw
					//  of the drawn surface by next Redraw
					RestoreOldCursorAreaFrom(aDrawSurface, false);

					// Set it back to true so it gets removed from the primary 
					//  surface when we move the mouse
					mHasOldCursorArea = true;
				}
			}
			else
			{
				if ((mApp->mWaitForVSync) && (!mApp->mSoftVSyncWait))
					mDD->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);

				mCursorX = mNextCursorX;
				mCursorY = mNextCursorY;

				DrawCursorTo(mSecondarySurface, true);
				aResult = mPrimarySurface->Blt(&aDestRect.ToRECT(), mSecondarySurface, &aSrcRect.ToRECT(), DDBLT_WAIT, &aBltFX);
			}

			mInRedraw = false;
			return !GotDXError(aResult, "Redraw Windowed");
		}
		else
		{
			// Don't flip away from the GDI surface during the TryMedia purchasing process
			if (!mApp->mNoDefer && mApp->mFullScreenPageFlip)
			{
				HRESULT aResult = mSecondarySurface->Blt(&aDestRect.ToRECT(), aDrawSurface, &aSrcRect.ToRECT(), DDBLT_WAIT, &aBltFX);

				if (SUCCEEDED(aResult))
				{
					if (mIs3D && mIsWidescreen)
					{
						Rect aLeftBar(0, 0, aDestRect.mX, mDisplayHeight);
						Rect aRightBar(aDestRect.mWidth + aDestRect.mX, 0, mDisplayWidth - (aDestRect.mWidth + aDestRect.mX), mDisplayHeight);
						if (aLeftBar.mWidth > 0)
							mSecondarySurfaceImage->NormalFillRect(aLeftBar, Color::Black);
						if (aRightBar.mWidth > 0)
							mSecondarySurfaceImage->NormalFillRect(aRightBar, Color::Black);
					}
					mCursorX = mNextCursorX;
					mCursorY = mNextCursorY;

					DrawCursorTo(mSecondarySurface, true);

					HRESULT aResult = mPrimarySurface->Flip(NULL, gSexyAppBase->mNoVSync ? (DDFLIP_WAIT | DDFLIP_NOVSYNC) : DDFLIP_WAIT);

					mInRedraw = false;
					return !GotDXError(aResult, "Redraw FullScreen Flip (2)");
				}
			}
			else
			{
				HRESULT aResult = mPrimarySurface->Blt(&aDestRect.ToRECT(), aDrawSurface, &aSrcRect.ToRECT(), DDBLT_WAIT, &aBltFX);

				mInRedraw = false;
				return !GotDXError(aResult, "Redraw FullScreen Flip (1)");
			}
		}
	}
}

void WindowsGraphicsDriver::RestoreOldCursorAreaFrom(LPDIRECTDRAWSURFACE theSurface, bool adjust) //1708-1744
{
	if ((mHasOldCursorArea) && (mPrimarySurface != NULL))
	{
		Rect aSexyScreenRect(
			mCursorX - (mCursorWidth / 2),
			mCursorY - (mCursorHeight / 2),
			mCursorWidth,
			mCursorHeight);

		Rect aClippedScreenRect = aSexyScreenRect.Intersection(Rect(0, 0, mWidth, mHeight));

		Rect aSexyLocalRect(
			aClippedScreenRect.mX - aSexyScreenRect.mX,
			aClippedScreenRect.mY - aSexyScreenRect.mY,
			aClippedScreenRect.mWidth,
			aClippedScreenRect.mHeight);

		if (adjust)
		{
			POINT aPoint = { 0, 0 };
			ClientToScreen(mHWnd, &aPoint);
			aClippedScreenRect.Offset(aPoint.x, aPoint.y);
		}

		RECT aLocalRect = aSexyLocalRect.ToRECT();
		RECT aScreenRect = aClippedScreenRect.ToRECT();

		DDBLTFX aBltFX;
		ZeroMemory(&aBltFX, sizeof(aBltFX));
		aBltFX.dwSize = sizeof(aBltFX);

		// From mNewCursorArea to theSurface
		HRESULT aResult = theSurface->Blt(&aScreenRect, mOldCursorArea, &aLocalRect, DDBLT_WAIT, &aBltFX);

		mHasOldCursorArea = false;
	}
}

void WindowsGraphicsDriver::DrawCursorTo(LPDIRECTDRAWSURFACE theSurface, bool adjust) //Correct? | 1747-1864
{
	if ((mCursorImage != NULL) && (mPrimarySurface != NULL) || !mApp->mIsSizeCursor && (mOldCursorArea != NULL) && (mNewCursorArea != NULL) && !mIsD3D8Or9)
	{
		Rect aDisplayRect = mIsWindowed ? mPresentationRect : Rect(0, 0, mDisplayWidth, mDisplayHeight);
		DDSURFACEDESC aDesc;
		ZeroMemory(&aDesc, sizeof(aDesc));
		aDesc.dwSize = sizeof(aDesc);
		aDesc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
		HRESULT aResult = mPrimarySurface->GetSurfaceDesc(&aDesc);
		Rect aDestSurfaceRect(0, 0, aDesc.dwWidth, aDesc.dwHeight);

		Rect aSexyScreenRect(
			mCursorX - (mCursorWidth / 2),
			mCursorY - (mCursorHeight / 2),
			mCursorWidth,
			mCursorHeight);

		Rect aClippedScreenRect = aSexyScreenRect.Intersection(aDisplayRect); //?

		Rect aSexyLocalRect(
			aClippedScreenRect.mX - aSexyScreenRect.mX,
			aClippedScreenRect.mY - aSexyScreenRect.mY,
			aClippedScreenRect.mWidth,
			aClippedScreenRect.mHeight);

		if (adjust)
		{
			POINT aPoint = { 0, 0 };
			ClientToScreen(mHWnd, &aPoint);
			aClippedScreenRect.Offset(aPoint.x, aPoint.y);
		}

		RECT aLocalRect = aSexyLocalRect.ToRECT();
		RECT aScreenRect = aClippedScreenRect.ToRECT();

		DDBLTFX aBltFX;
		ZeroMemory(&aBltFX, sizeof(aBltFX));
		aBltFX.dwSize = sizeof(aBltFX);

		// From theSurface to mOldCursorArea
		aResult = mOldCursorArea->Blt(&aLocalRect, theSurface, &aScreenRect, DDBLT_WAIT, &aBltFX);
		if (aResult != DD_OK)
		{
			// Try to clip it now.  We don't ALWAYS want to clip it, though
			Rect aPrevRect = aClippedScreenRect;
			aClippedScreenRect = aClippedScreenRect.Intersection(aDestSurfaceRect);

			aSexyLocalRect.mX += (aClippedScreenRect.mX - aPrevRect.mX);
			aSexyLocalRect.mY += (aClippedScreenRect.mY - aPrevRect.mY);
			aSexyLocalRect.mWidth = aClippedScreenRect.mWidth;
			aSexyLocalRect.mHeight = aClippedScreenRect.mHeight;

			aLocalRect = aSexyLocalRect.ToRECT();
			aScreenRect = aClippedScreenRect.ToRECT();

			aResult = mOldCursorArea->Blt(&aLocalRect, theSurface, &aScreenRect, DDBLT_WAIT, &aBltFX);
			//DBG_ASSERT(aResult == DD_OK);
		}

		mHasOldCursorArea = aResult == DD_OK;

		// Kindof a hack, since we only use this for mDrawSurace
		if (theSurface == mDrawSurface && !mIs3D)
		{
			// Draw directly to the screen image, instead of indirectly through mNewCursorArea
			Graphics g(mScreenImage);
			g.DrawImage(mCursorImage,
				mCursorX - (mCursorWidth / 2) + (mCursorWidth - mCursorImage->mWidth) / 2,
				mCursorY - (mCursorHeight / 2) + (mCursorHeight - mCursorImage->mHeight) / 2);
		}
		else
		{
			// From mOldCursorArea to mNewCursorArea
			aResult = mNewCursorArea->Blt(&aLocalRect, mOldCursorArea, &aLocalRect, DDBLT_WAIT, &aBltFX);

			// Draw image to mNewCursorAreaImage
			Graphics aNewCursorAreaG(mNewCursorAreaImage);
			aNewCursorAreaG.DrawImage(mCursorImage,
				(mCursorWidth - mCursorImage->mWidth) / 2,
				(mCursorHeight - mCursorImage->mHeight) / 2);

			// From mNewCursorArea to theSurface
			aResult = theSurface->Blt(&aScreenRect, mNewCursorArea, &aLocalRect, DDBLT_WAIT, &aBltFX);
		}
	}
	else if (mIsD3D8Or9)
	{
		Rect aDisplayRect = mIsWindowed ? mPresentationRect : Rect(0, 0, mDisplayWidth, mDisplayHeight); //?

		mD3DInterface->BltF(mCursorImage, (float)(mCursorX - (mCursorWidth / 2)), (float)(mCursorY - (mCursorHeight / 2)), mPresentationRect, aDisplayRect, Color::White, 0); //?
	}
	else
		mHasOldCursorArea = false;
}

void WindowsGraphicsDriver::MoveCursorTo(LPDIRECTDRAWSURFACE theSurface, bool adjust, int theNewCursorX, int theNewCursorY) //Correct? | 1867-2026
{
	DBG_ASSERT(mHasOldCursorArea); //1868

	if (mIsD3D8Or9)
		return;

	if ((mCursorImage != NULL) && (mPrimarySurface != NULL) && (mOldCursorArea != NULL) && (mNewCursorArea != NULL))
	{
		Rect aDisplayRect = mIsWindowed ? mPresentationRect : Rect(0, 0, mDisplayWidth, mDisplayHeight);
		DDSURFACEDESC aDesc;
		ZeroMemory(&aDesc, sizeof(aDesc));
		aDesc.dwSize = sizeof(aDesc);
		aDesc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
		HRESULT aResult = mPrimarySurface->GetSurfaceDesc(&aDesc);
		Rect aDestSurfaceRect(0, 0, aDesc.dwWidth, aDesc.dwHeight);

		// OLD RECTANGLES
		Rect aSexyScreenRect(
			mCursorX - (mCursorWidth / 2),
			mCursorY - (mCursorHeight / 2),
			mCursorWidth,
			mCursorHeight);

		Rect aClippedScreenRect = aSexyScreenRect.Intersection(aDisplayRect);

		Rect aSexyLocalRect(
			aClippedScreenRect.mX - aSexyScreenRect.mX,
			aClippedScreenRect.mY - aSexyScreenRect.mY,
			aClippedScreenRect.mWidth,
			aClippedScreenRect.mHeight);
		if (adjust)
		{
			POINT aPoint = { 0, 0 };
			ClientToScreen(mHWnd, &aPoint);
			aClippedScreenRect.Offset(aPoint.x, aPoint.y);
			//aClippedScreenRect = aClippedScreenRect.Intersection(aDestSurfaceRect);   
		}

		aSexyLocalRect.mWidth = aClippedScreenRect.mWidth;
		aSexyLocalRect.mHeight = aClippedScreenRect.mHeight;

		RECT aLocalRect = aSexyLocalRect.ToRECT();
		RECT aScreenRect = aClippedScreenRect.ToRECT();

		// NEW RECTANGLES
		Rect aNewSexyScreenRect(
			theNewCursorX - (mCursorWidth / 2),
			theNewCursorY - (mCursorHeight / 2),
			mCursorWidth,
			mCursorHeight);

		Rect aNewClippedScreenRect = aNewSexyScreenRect.Intersection(mPresentationRect);
		Rect aNewSexyLocalRect(
			aNewClippedScreenRect.mX - aNewSexyScreenRect.mX,
			aNewClippedScreenRect.mY - aNewSexyScreenRect.mY,
			aNewClippedScreenRect.mWidth,
			aNewClippedScreenRect.mHeight);
		if (adjust)
		{
			POINT aPoint = { 0, 0 };
			ClientToScreen(mHWnd, &aPoint);
			aNewClippedScreenRect.Offset(aPoint.x, aPoint.y);
			//aNewClippedScreenRect = aNewClippedScreenRect.Intersection(aDestSurfaceRect);
		}

		aNewSexyLocalRect.mWidth = aNewClippedScreenRect.mWidth;
		aNewSexyLocalRect.mHeight = aNewClippedScreenRect.mHeight;

		RECT aNewLocalRect = aNewSexyLocalRect.ToRECT();
		RECT aNewScreenRect = aNewClippedScreenRect.ToRECT();

		// Do drawing stuff now

		DDBLTFX aBltFX;
		ZeroMemory(&aBltFX, sizeof(aBltFX));
		aBltFX.dwSize = sizeof(aBltFX);

		// From theSurface to mNewCursorArea
		//  It may still have some of the old cursor on it though, since we haven't restored
		//  that area yet

		aResult = mNewCursorArea->Blt(&aNewLocalRect, theSurface, &aNewScreenRect, DDBLT_WAIT, &aBltFX);
		if (aResult != DD_OK)
		{
			// Try to clip it now.  We don't ALWAYS want to clip it, though
			Rect aPrevRect = aNewClippedScreenRect;
			aNewClippedScreenRect = aNewClippedScreenRect.Intersection(aDestSurfaceRect);

			aNewSexyLocalRect.mX += (aNewClippedScreenRect.mX - aPrevRect.mX);
			aNewSexyLocalRect.mY += (aNewClippedScreenRect.mY - aPrevRect.mY);
			aNewSexyLocalRect.mWidth = aNewClippedScreenRect.mWidth;
			aNewSexyLocalRect.mHeight = aNewClippedScreenRect.mHeight;

			aNewLocalRect = aNewSexyLocalRect.ToRECT();
			aNewScreenRect = aNewClippedScreenRect.ToRECT();

			aResult = mNewCursorArea->Blt(&aNewLocalRect, theSurface, &aNewScreenRect, DDBLT_WAIT, &aBltFX);
			//DBG_ASSERT(aResult == DD_OK);
		}
		//DBG_ASSERT(aResult == DD_OK);

		// Figure out the overlapping area from the source
		Rect aCursorAreaRect(0, 0, mCursorWidth, mCursorHeight);
		Rect aSexyOrigSrcAreaRect(aCursorAreaRect);
		aSexyOrigSrcAreaRect.Offset(theNewCursorX - mCursorX, theNewCursorY - mCursorY);
		Rect aSexySrcAreaRect = aSexyOrigSrcAreaRect.Intersection(aCursorAreaRect);

		// Does the new cursor area overlap with the old one?
		if ((aSexySrcAreaRect.mWidth > 0) || (aSexySrcAreaRect.mHeight > 0))
		{
			Rect aSexyDestAreaRect(
				aSexySrcAreaRect.mX - aSexyOrigSrcAreaRect.mX,
				aSexySrcAreaRect.mY - aSexyOrigSrcAreaRect.mY,
				aSexySrcAreaRect.mWidth, aSexySrcAreaRect.mHeight);

			RECT aSrcAreaRect = aSexySrcAreaRect.ToRECT();
			RECT aDestAreaRect = aSexyDestAreaRect.ToRECT();

			// Restore old area from new area
			//  This will give us our new pure OLD buffer
			mNewCursorArea->Blt(&aDestAreaRect, mOldCursorArea, &aSrcAreaRect, DDBLT_WAIT, &aBltFX);
			//DBG_ASSERT(aResult == DD_OK);

			// Draw offset image to mOldCursorAreaImage.  This is to avoid flicker
			Graphics aOldCursorAreaG(mOldCursorAreaImage);
			aOldCursorAreaG.DrawImage(mCursorImage,
				theNewCursorX - mCursorX + (mCursorWidth - mCursorImage->mWidth) / 2,
				theNewCursorY - mCursorY + (mCursorHeight - mCursorImage->mHeight) / 2);
		}

		// Restore the old cursor area
		aResult = theSurface->Blt(&aScreenRect, mOldCursorArea, &aLocalRect, DDBLT_WAIT, &aBltFX);
		//DBG_ASSERT(aResult == DD_OK);

		// The screen is now PURE and restored

		// Move the new cursor area to the old one, since this is what we will have to
		//  use to redraw the old area
		RECT aFullAreaRect = { 0, 0, mCursorWidth, mCursorHeight };
		aResult = mOldCursorArea->Blt(&aFullAreaRect, mNewCursorArea, &aFullAreaRect, DDBLT_WAIT, &aBltFX);
		//DBG_ASSERT(aResult == DD_OK);

		// Draw image to mNewCursorAreaImage, preparing to draw it to the screen
		Graphics aNewCursorAreaG(mNewCursorAreaImage);
		aNewCursorAreaG.DrawImage(mCursorImage,
			(mCursorWidth - mCursorImage->mWidth) / 2,
			(mCursorHeight - mCursorImage->mHeight) / 2);

		// From mNewCursorArea to theSurface
		aResult = theSurface->Blt(&aNewScreenRect, mNewCursorArea, &aNewLocalRect, DDBLT_WAIT, &aBltFX);
		//DBG_ASSERT(aResult == DD_OK);

		// The cursor is now fully moved
		mCursorX = theNewCursorX;
		mCursorY = theNewCursorY;
	}
	else
		mHasOldCursorArea = false;
}

bool WindowsGraphicsDriver::SetCursorImage(Image* theImage) //2029-2040
{
	AutoCrit anAutoCrit(mCritSect);

	if (mCursorImage != theImage)
	{
		// Wait until next Redraw or cursor move to draw new cursor
		mCursorImage = theImage;
		return true;
	}
	else
		return false;
}

void WindowsGraphicsDriver::SetCursorPos(int theCursorX, int theCursorY) //2043-2068 (Matched?)
{
	AutoCrit anAutoCrit(mCritSect);

	mNextCursorX = theCursorX;
	mNextCursorY = theCursorY;

	if (mIsD3D8Or9)
		mApp->mCustomCursorDirty = true;
	else
	{
		if (mInRedraw)
			return;

		if (mHasOldCursorArea)
		{
			MoveCursorTo(mPrimarySurface, true, theCursorX, theCursorY);
		}
		else
		{
			mCursorX = theCursorX;
			mCursorY = theCursorY;
			DrawCursorTo(mPrimarySurface, true);
		}
	}
}

Mesh* WindowsGraphicsDriver::LoadMesh(const std::string& thePath, MeshListener* theListener) //2071-2085 (I'm honestly surprised it's engine specific despite being only used for Twist-3 [non-mobile], previously Load3DObject)
{
	AutoCrit anAutoCrit(mCritSect);

	Mesh* aMesh = new Mesh();
	aMesh->mListener = theListener;
	aMesh->mFileName = thePath;

	if (mRenderDevice3D != NULL || mRenderDevice3D->LoadMesh(aMesh))
		return aMesh;
	else
	{
		delete aMesh;
		return NULL;
	}
}

void WindowsGraphicsDriver::AddMesh(Mesh* theMesh) //2088-2090
{
	mMeshSet.insert(theMesh);
}

void WindowsGraphicsDriver::RemoveMesh(Mesh* theMesh) //2093-2097 (Matched probably)
{
	MeshSet::iterator anItr = mMeshSet.find(theMesh);
	if (anItr != mMeshSet.end())
		mMeshSet.erase(anItr);
}

bool WindowsGraphicsDriver::InitGraphicsDriver() //2100-2102
{
	return true;
}

int WindowsGraphicsDriver::GetVersion() //2105-2107
{
	return mIsD3D8Or9 ? mIsD3D9 + 8 : 7;
}

DeviceSurface* WindowsGraphicsDriver::CreateDeviceSurface() //Huh why | 2110-2121
{
	if (mIsD3D9)
		return CreateDXSurface9(this);
	if (mIsD3D8)
		return CreateDXSurface8(this);
	else
		return CreateDXSurface7(this);
}

NativeDisplay* WindowsGraphicsDriver::GetNativeDisplayInfo() //2124-2126
{
	return this ? &NativeDisplay() : NULL;
}

CritSect& WindowsGraphicsDriver::GetCritSect() //2129-2131
{
	return mCritSect;
}

RenderDevice* WindowsGraphicsDriver::GetRenderDevice() //2134-2136
{
	return mD3DInterface;
}

RenderDevice3D* WindowsGraphicsDriver::GetRenderDevice3D() //2134-2135
{
	return mD3DInterface;
}