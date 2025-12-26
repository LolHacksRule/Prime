#include "DXSurface7.h"
#include "WindowsGraphicsDriver.h"
#include "../../../AutoCrit.h"

using namespace Sexy;

IDXSurface* CreateDXSurface7(WindowsGraphicsDriver* theDriver) //15-17
{
	return new DXSurface7(theDriver);
}

DXSurface7::DXSurface7(WindowsGraphicsDriver* theDriver) //21-25
{
	mDriver = theDriver;

	mSurface = NULL;
}

DXSurface7::~DXSurface7() //30-33
{
	if (mSurface != NULL)
		mSurface->Release();
}

bool DXSurface7::Lock(DeviceSurfaceDesc* theParam) //38-62
{
	if (mSurface == NULL)
		return false;

	_DDSURFACEDESC theDesc;
	ZeroMemory(&theDesc, 0);
	theDesc.dwSize = sizeof(_DDSURFACEDESC);

	HRESULT aResult = mSurface->Lock(NULL, &theDesc, 1, NULL);

	if (!SUCCEEDED(aResult))
		return false;

	theParam->dwWidth = theDesc.dwWidth;
	theParam->dwHeight = theDesc.dwHeight;
	theParam->lPitch = theDesc.dwLinearSize;
	theParam->lpSurface = theDesc.lpSurface;
	theParam->ddpfPixelFormat.dwRGBBitCount = theDesc.ddpfPixelFormat.dwRGBBitCount;
	theParam->ddpfPixelFormat.dwRBitMask = theDesc.ddpfPixelFormat.dwRBitMask;
	theParam->ddpfPixelFormat.dwGBitMask = theDesc.ddpfPixelFormat.dwGBitMask;
	theParam->ddpfPixelFormat.dwBBitMask = theDesc.ddpfPixelFormat.dwBBitMask;

	return true;
}

void DXSurface7::Unlock(void* theParam) //68-71
{
	if (mSurface)
		mSurface->Unlock(theParam);
}

bool DXSurface7::GenerateDeviceSurface(DeviceImage* theImage) //Similar to GenerateDDSurface (Correct?) | 76-221
{
	if (mSurface == NULL)
		return false;

	theImage->CommitBits();

	if (theImage->mHasAlpha)
		return false;

	theImage->mWantDeviceSurface = true;

	// Force into non-palletized mode for this
	if (theImage->mColorTable || theImage->mNativeAlphaData)
		theImage->GetBits();

	HRESULT aResult;
	//  TDDSurfaceDesc aDesc;
	DDSURFACEDESC2 aDesc;

	ZeroMemory(&aDesc, sizeof(aDesc));
	aDesc.dwSize = sizeof(aDesc);
	aDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	aDesc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY;
	aDesc.dwWidth = theImage->mWidth;
	aDesc.dwHeight = theImage->mHeight;

	AutoCrit aCrit(mDriver->mCritSect); // prevent mSurface from being released while we're in this code

	aResult = mDriver->CreateSurface(&aDesc, mSurface, NULL);

	if (aResult != DD_OK)
		return false;

	DDSURFACEDESC aLockedSurfaceDesc;
	ZeroMemory(&aLockedSurfaceDesc, sizeof(aLockedSurfaceDesc));
	aLockedSurfaceDesc.dwSize = sizeof(aLockedSurfaceDesc);

	aResult = mSurface->Lock(NULL, &aLockedSurfaceDesc, 1, NULL);

	if (!SUCCEEDED(aResult))
	{
		mSurface->Release();
		mSurface = NULL;
		return false;
	}

	const int rRightShift = 16 + (8 - mDriver->GetNativeDisplayInfo()->mRedBits);
	const int gRightShift = 8 + (8 - mDriver->GetNativeDisplayInfo()->mGreenBits);
	const int bRightShift = 0 + (8 - mDriver->GetNativeDisplayInfo()->mBlueBits);

	const int rLeftShift = mDriver->GetNativeDisplayInfo()->mRedShift;
	const int gLeftShift = mDriver->GetNativeDisplayInfo()->mGreenShift;
	const int bLeftShift = mDriver->GetNativeDisplayInfo()->mBlueShift;

	const int rMask = aLockedSurfaceDesc.ddpfPixelFormat.dwRBitMask;
	const int gMask = aLockedSurfaceDesc.ddpfPixelFormat.dwGBitMask;
	const int bMask = aLockedSurfaceDesc.ddpfPixelFormat.dwBBitMask;

	if (aLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 16) //?
	{
		ushort* mSurfaceBits = (ushort*)aLockedSurfaceDesc.lpSurface;
		if (mSurfaceBits)
		{
			int i;
			bool firstTrans = true;

			ushort* a16Bits = NULL;
			ushort aTransColor = 0;

			if (theImage->mBits != NULL)
			{
				a16Bits = new ushort[theImage->mWidth * theImage->mHeight];
				ulong* aSrcPtr = theImage->mBits;
				ushort* a16SrcPtr = a16Bits;

				for (i = 0; i < theImage->mWidth * theImage->mHeight; i++)
				{
					ulong val = *(aSrcPtr++);

					*a16SrcPtr = (((val >> rRightShift) << rLeftShift) & rMask) |
						(((val >> gRightShift) << gLeftShift) & gMask) |
						(((val >> bRightShift) << bLeftShift) & bMask);

					int anAlpha = val >> 24;
					if ((firstTrans) && (anAlpha < 255))
					{
						firstTrans = false;
						aTransColor = *a16SrcPtr;
					}

					++a16SrcPtr;
				}
			}
			if (a16Bits != NULL)
			{
				ushort* aDestPtr = mSurfaceBits;
				ushort* a16SrcPtr = a16Bits;
				for (int aRow = 0; aRow < theImage->mHeight; aRow++)
				{
					for (int aCol = 0; aCol < theImage->mWidth; aCol++)
					{
						*(aDestPtr++) = *a16SrcPtr;
						++a16SrcPtr;
					}

					aDestPtr += aLockedSurfaceDesc.lPitch / 2 - theImage->mWidth;
				}
			}

			delete a16Bits;
		}
	}
	else if (aLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 32)
	{
		ulong* mSurfaceBits = (ulong*)aLockedSurfaceDesc.lpSurface;
		if (mSurfaceBits != NULL)
		{
			if (theImage->mBits != NULL)
			{
				ulong* aDestPtr = mSurfaceBits;
				ulong* aSrcPtr = theImage->mBits;
				for (int aRow = 0; aRow < theImage->mHeight; aRow++)
				{
					for (int aCol = 0; aCol < theImage->mWidth; aCol++)
					{
						ulong val = *(aSrcPtr++);

						*(aDestPtr++) =
							(((val >> rRightShift) << rLeftShift) & rMask) |
							(((val >> gRightShift) << gLeftShift) & gMask) |
							(((val >> bRightShift) << bLeftShift) & bMask);
					}

					aDestPtr += aLockedSurfaceDesc.lPitch / 4 - theImage->mWidth;
				}
			}
		}
	}

	return false;
}

void DXSurface7::Release() //226-232
{
	if (mSurface != NULL)
	{
		mSurface->Release();
		mSurface = NULL;
	}
}

ulong* DXSurface7::GetBits(DeviceImage* theImage) //237-306
{
	DDSURFACEDESC aLockedSurfaceDesc;
	ZeroMemory(&aLockedSurfaceDesc, sizeof(aLockedSurfaceDesc));
	aLockedSurfaceDesc.dwSize = sizeof(aLockedSurfaceDesc);

	HRESULT aResult;

	aResult = mSurface->Lock(NULL, &aLockedSurfaceDesc, 1, NULL);

	if (!SUCCEEDED(aResult))
		return false;

	ulong* aBits = new ulong[theImage->mWidth * theImage->mHeight + 1];
	aBits[theImage->mWidth * theImage->mHeight] = MEMORYCHECK_ID;

	int RedShift = mDriver->GetNativeDisplayInfo()->mRedShift;
	int GreenShift = mDriver->GetNativeDisplayInfo()->mGreenShift;
	int BlueShift = mDriver->GetNativeDisplayInfo()->mBlueShift;

	int RedBits = mDriver->GetNativeDisplayInfo()->mRedBits;
	int GreenBits = mDriver->GetNativeDisplayInfo()->mGreenBits;
	int BlueBits = mDriver->GetNativeDisplayInfo()->mBlueBits;

	if (aLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 16)
	{
		ushort* aSrcPixelsRow = (ushort*)aLockedSurfaceDesc.lpSurface;
		ulong* aDest = aBits;

		for (int y = 0; y < theImage->mHeight; y++)
		{
			ushort* aSrcPixels = aSrcPixelsRow;

			for (int x = 0; x < theImage->mWidth; x++)
			{
				ulong src = *(aSrcPixels++);

				int r = ((src >> RedShift << (8 - RedBits)) & 0xFF);
				int g = ((src >> GreenShift << (8 - GreenBits)) & 0xFF);
				int b = ((src >> BlueShift << (8 - BlueBits)) & 0xFF);

				*aDest++ = 0xFF000000 | (r << 16) | (g << 8) | (b);
			}

			aSrcPixelsRow += aLockedSurfaceDesc.lPitch / 2;
		}
	}
	else if (aLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 32)
	{
		ulong* aSrcPixelsRow = (ulong*)aLockedSurfaceDesc.lpSurface;
		ulong* aDest = aBits;

		for (int y = 0; y < theImage->mHeight; y++)
		{
			ulong* aSrcPixels = aSrcPixelsRow;

			for (int x = 0; x < theImage->mWidth; x++)
			{
				ulong src = *(aSrcPixels++);

				int r = (src >> RedShift << (8 - RedBits)) & 0xFF;
				int g = (src >> GreenShift << (8 - GreenBits)) & 0xFF;
				int b = (src >> BlueShift << (8 - BlueBits)) & 0xFF;

				*aDest++ = 0xFF000000 | (r << 16) | (g << 8) | (b);
			}

			aSrcPixelsRow += aLockedSurfaceDesc.lPitch / 4;
		}
	}
	mSurface->Unlock(NULL);
	return aBits;
}

HDC DXSurface7::GetDC() //311-320
{
	if (mSurface == NULL)
		return NULL;

	HDC aDC = NULL;
	if (mSurface->GetDC(&aDC) < 0)
		return NULL;
	else
		return aDC;
}

void DXSurface7::ReleaseDC(HDC theDC) //325-330 (Done for line integrity)
{
	if (mSurface != NULL)
	{
		mSurface->ReleaseDC(theDC);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void DXSurface7::SetSurface(void* theSurface) //Why is this in every one | 335-346
{
	if (mSurface != theSurface)
		mSurface = (LPDIRECTDRAWSURFACE)theSurface;
}

void DXSurface7::GetDimensions(int* theWidth, int* theHeight) //351-367
{
	theHeight = 0;
	theWidth = 0;

	if (mSurface != NULL)
		return;

	DDSURFACEDESC aDesc;
	ZeroMemory(&aDesc, sizeof(DDSURFACEDESC));
	aDesc.dwSize = sizeof(DDSURFACEDESC);
	aDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	if (SUCCEEDED(mSurface->GetSurfaceDesc(&aDesc)))
	{
		*theWidth = aDesc.dwWidth;
		*theHeight = aDesc.dwHeight;
	}
}

HRESULT DXSurface7::Blt(LPRECT theDestRect, void* theSurface, LPRECT theSrcRect, DWORD theFlags, LPDDBLTFX theBltFx) //372-379 (Unmatched lines)
{
	return mSurface != NULL ? mSurface->Blt(theDestRect, (LPDIRECTDRAWSURFACE)theSurface, theSrcRect, theFlags, theBltFx) : -1;
}

void DXSurface7::AddRef() //Why is this in every one | 384-387
{
	if (mSurface)
		mSurface->AddRef();
}