#include "DXSurface9.h"
#include "WindowsGraphicsDriver.h"
#include "../../../AutoCrit.h"

using namespace Sexy;

IDXSurface* CreateDXSurface9(WindowsGraphicsDriver* theDriver) //20-22
{
	return new DXSurface9(theDriver);
}

DXSurface9::DXSurface9(WindowsGraphicsDriver* theDriver) //26-35
{
	mDriver = theDriver;
	mD3DInterface = (D3D9Interface*)dynamic_cast<D3DInterface*>(mDriver->mD3DInterface);
	mSurface = NULL;
	mSurfaceFormat = D3DFMT_UNKNOWN;
	mTextureSurface = NULL;

	mImageFlags = 0;
	mIsTexture = false;
}

DXSurface9::~DXSurface9() //40-46
{
	if (mSurface)
		mSurface->Release();
}

bool DXSurface9::Lock(_DEVICESURFACEDESC *theDesc) //51-91
{
	D3DSURFACE_DESC aD3DDesc;
	D3DLOCKED_RECT aLockedRect;
	HRESULT aResult;

	if (mSurface == NULL || mIsTexture)
		return false;

	aResult = _getSurface()->GetDesc(&aD3DDesc);

	if (FAILED(aResult))
		return false;

	theDesc->dwWidth = aD3DDesc.Width;
	theDesc->dwHeight = aD3DDesc.Height;

	if (aD3DDesc.Format == D3DFMT_X8R8G8B8)
	{
		theDesc->ddpfPixelFormat.dwRGBBitCount = 32;
		theDesc->ddpfPixelFormat.dwRBitMask = 0xFF0000;
		theDesc->ddpfPixelFormat.dwGBitMask = 0xFF00;
		theDesc->ddpfPixelFormat.dwBBitMask = 0xFF;
	}
	else if (aD3DDesc.Format == D3DFMT_R5G6B5)
	{
		theDesc->ddpfPixelFormat.dwRGBBitCount = 16;
		theDesc->ddpfPixelFormat.dwRBitMask = 0xF800;
		theDesc->ddpfPixelFormat.dwGBitMask = 0x7E0;
		theDesc->ddpfPixelFormat.dwBBitMask = 0x1F;
	}
	else
		return false;

	aResult = _getSurface()->LockRect(&aLockedRect, NULL, 0);

	if (FAILED(aResult))
		return false;

	*(D3DLOCKED_RECT*)theDesc->lPitch = aLockedRect;
	return true;
}

void DXSurface9::Unlock(void* formal) //96-98
{
	if (mIsTexture)
		_getSurface()->UnlockRect();
}

bool DXSurface9::GenerateDeviceSurface(DeviceImage* theImage) //103-268
{
	if (mSurface != NULL)
		return true;

	theImage->MemoryImage::CommitBits();

	if (theImage->mHasAlpha)
		return false;

	theImage->mWantDeviceSurface = true;

	// Force into non-palletized mode for this
	if (theImage->mColorTable != NULL)
		theImage->MemoryImage::GetBits();

	AutoCrit aCrit(mDriver->mCritSect); // prevent mSurface from being released while we're in this code

	mSurfaceFormat = mD3DInterface->mDisplayFormat;

	if (mSurfaceFormat == D3DFMT_X8R8G8B8) //It's D3DFMT according to the casts, assuming
	{
		mRedBits = 8;
		mRedShift = 16;
		mRedMask = 0xFF0000;
		mGreenBits = 8;
		mGreenShift = 8;
		mGreenMask = 0xFF00;
		mBlueBits = 8;
		mBlueShift = 0;
		mBlueMask = 0xFF;
	}
	else if (mSurfaceFormat == D3DFMT_R5G6B5)
	{
		mRedBits = 5;
		mRedShift = 11;
		mRedMask = 0xF800;
		mGreenBits = 6;
		mGreenShift = 5;
		mGreenMask = 0x7E0;
		mBlueBits = 5;
		mBlueShift = 0;
		mBlueMask = 0x1F;
	}

	if ((mImageFlags & ImageFlag_RenderTarget) != 0 && theImage->GetRenderData())
	{
		mIsTexture = false;
		D3DTextureData* aData = (D3DTextureData*)theImage->GetRenderData();
		IDirect3DTexture9* aTexture = (LPDIRECT3DTEXTURE9)aData->mTextures.begin()->mTexture;
		if (SUCCEEDED(aTexture->GetSurfaceLevel(0, (LPDIRECT3DSURFACE9*)&mSurface)))
			return true;
		else
			return false;
	}
	else
	{
		HRESULT aResult = mD3DInterface->mD3DDevice->CreateOffscreenPlainSurface(theImage->mWidth, theImage->mHeight, (D3DFORMAT)mSurfaceFormat, (mImageFlags & ImageFlag_RenderTarget) != 0 ? D3DPOOL_DEFAULT : D3DPOOL_SYSTEMMEM, (LPDIRECT3DSURFACE9*)&mSurface, 0);
		if (FAILED(aResult))
			return false;

		mIsTexture = false;
		D3DLOCKED_RECT aLockedRect;
		aResult = _getSurface()->LockRect(&aLockedRect, 0, D3DLOCK_DISCARD);

		if (FAILED(aResult))
		{
			mSurface->Release();
			mSurface = NULL;
			return false;
		}

		const int rRightShift = 16 + (8 - mRedBits);
		const int gRightShift = 8 + (8 - mGreenBits);
		const int bRightShift = 0 + (8 - mBlueBits);

		const int rLeftShift = mRedShift;
		const int gLeftShift = mGreenShift;
		const int bLeftShift = mBlueShift;

		const int rMask = mRedMask;
		const int gMask = mGreenMask;
		const int bMask = mBlueMask;

		int aNumBits = mSurfaceFormat != 22 ? 16 : 32;

		if (aNumBits == 16)
		{
			ushort* mSurfaceBits = (ushort*)aLockedRect.pBits;
			if (mSurfaceBits != NULL)
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

						aDestPtr += aLockedRect.Pitch / 2 - theImage->mWidth;
					}
				}

				delete a16Bits;
			}
		}
		else
		{
			ulong* mSurfaceBits = (ulong*)aLockedRect.pBits;
			if (mSurfaceBits != NULL)
			{
				int i;
				bool firstTrans = true;

				ulong aTransColor = 0;
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

						aDestPtr += aLockedRect.Pitch / 4 - theImage->mWidth;
					}
				}
			}
		}
		_getSurface()->UnlockRect();
		return true;
	}
}

void DXSurface9::Release() //273-280
{
	if (mSurface)
	{
		mSurface->Release();
		mSurface = NULL;
	}
}

ulong* DXSurface9::GetBits(DeviceImage* theImage) //285-379
{
	LPDIRECT3DSURFACE9 aSurface = NULL; //?
	if (mIsTexture)
	{
		LPDIRECT3DSURFACE9 aSurface = (LPDIRECT3DSURFACE9)_getTexture()->GetSurfaceLevel(0, &aSurface);
		if (!SUCCEEDED(aSurface))
			return 0;
	}
	else
		aSurface = _getSurface();

	D3DSURFACE_DESC aDesc;
	HRESULT aResult = aSurface->GetDesc(&aDesc);
	if (FAILED(aResult))
		return 0;

	mSurfaceFormat = aDesc.Format;

	if (mSurfaceFormat == D3DFMT_X8R8G8B8) //It's D3DFMT according to the casts, assuming
	{
		mRedBits = 8;
		mRedShift = 16;
		mRedMask = 0xFF0000;
		mGreenBits = 8;
		mGreenShift = 8;
		mGreenMask = 0xFF00;
		mBlueBits = 8;
		mBlueShift = 0;
		mBlueMask = 0xFF;
	}
	else if (mSurfaceFormat == D3DFMT_R5G6B5)
	{
		mRedBits = 5;
		mRedShift = 11;
		mRedMask = 0xF800;
		mGreenBits = 6;
		mGreenShift = 5;
		mGreenMask = 0x7E0;
		mBlueBits = 5;
		mBlueShift = 0;
		mBlueMask = 0x1F;
	}
	D3DLOCKED_RECT aLockedRect;
	aResult = aSurface->LockRect(&aLockedRect, 0, 0);

	if (!SUCCEEDED(aResult))
		return 0;

	ulong* aBits = new ulong[theImage->mWidth * theImage->mHeight + 1];
	aBits[theImage->mWidth * theImage->mHeight] = MEMORYCHECK_ID;

	if (mBlueBits + mGreenBits + mRedBits == 16)
	{
		ushort* aSrcPixelsRow = (ushort*)aLockedRect.pBits;
		ulong* aDest = theImage->mBits;
		for (int y = 0; y < theImage->mHeight; y++)
		{
			ushort* aSrcPixels = aSrcPixelsRow;

			for (int x = 0; x < theImage->mWidth; x++)
			{
				ulong src = *(aSrcPixels++);

				int r = ((src >> mRedShift << (8 - mRedBits)) & 0xFF);
				int g = ((src >> mGreenShift << (8 - mGreenBits)) & 0xFF);
				int b = ((src >> mBlueShift << (8 - mBlueBits)) & 0xFF);

				*aDest++ = 0xFF000000 | (r << 16) | (g << 8) | (b);
			}

			aSrcPixelsRow += aLockedRect.Pitch / 2;
		}
	}

	else if (mBlueBits + mGreenBits + mRedBits == 24)
	{
		ulong* aSrcPixelsRow = (ulong*)aLockedRect.pBits;
		ulong* aDest = theImage->mBits;
		for (int y = 0; y < theImage->mHeight; y++)
		{
			ulong* aSrcPixels = aSrcPixelsRow;

			for (int x = 0; x < theImage->mWidth; x++)
			{
				ulong src = *(aSrcPixels++);

				int r = ((src >> mRedShift << (8 - mRedBits)) & 0xFF);
				int g = ((src >> mGreenShift << (8 - mGreenBits)) & 0xFF);
				int b = ((src >> mBlueShift << (8 - mBlueBits)) & 0xFF);

				*aDest++ = 0xFF000000 | (r << 16) | (g << 8) | (b);
			}

			aSrcPixelsRow += 4 * (aLockedRect.Pitch / 4);
		}
	}

	aSurface->UnlockRect();
	if (mIsTexture)
		aSurface->Release();

	return aBits;
}

HDC DXSurface9::GetDC() //384-413
{
	if (!mSurface)
		return NULL;
	HDC aDC = NULL;
	if (mIsTexture)
	{
		LPDIRECT3DSURFACE9 aSurface;
		if (SUCCEEDED(_getTexture()->GetSurfaceLevel(0, &aSurface) >= 0))
		{
			mTextureSurface = aSurface;
			if (SUCCEEDED(mTextureSurface->GetDC(&aDC)))
			{
				mTextureSurface->Release();
				mTextureSurface = NULL;
				return NULL;
			}
			else
				return aDC;
		}
		else
			return NULL;
	}
	else
		return _getSurface()->GetDC(&aDC) > 0 ? aDC : 0;
}

void DXSurface9::ReleaseDC(HDC theDC) //418-431
{
	if (!mSurface)
		return;

	if (mIsTexture)
	{
		mTextureSurface->ReleaseDC(theDC);
		mTextureSurface->Release();
		mTextureSurface = NULL;
	}
	else
		_getSurface()->ReleaseDC(theDC);
}

void DXSurface9::SetSurface(void* theSurface) //436-479
{
	LPDIRECT3DRESOURCE9 aResource = (LPDIRECT3DRESOURCE9)theSurface;
	D3DSURFACE_DESC aDesc;
	if (theSurface)
	{
		D3DRESOURCETYPE aResType = aResource->GetType();
		if (mSurface != aResource && (aResType == D3DRTYPE_TEXTURE || aResType == D3DRTYPE_SURFACE))
		{
			mSurface = aResource;
			if (aResType == D3DRTYPE_TEXTURE)
			{
				mIsTexture = true;
				if (_getTexture()->GetLevelDesc(0, &aDesc) < 0)
				{
					mSurfaceFormat = mD3DInterface->mDisplayFormat;
					if (mSurfaceFormat == SEXY3DFMT_X8R8G8B8)
						mSurfaceFormat = SEXY3DFMT_A8R8G8B8;
					else
						mSurfaceFormat = aDesc.Format;
				}
			}
			else
			{
				mIsTexture = false;
				if (_getSurface()->GetDesc(&aDesc) < 0)
					mSurfaceFormat = mD3DInterface->mDisplayFormat;
				else
					mSurfaceFormat = aDesc.Format;
			}
		}
	}
	else
		mSurface = NULL;
}

void DXSurface9::GetDimensions(int* theWidth, int* theHeight) //484-504
{
	D3DSURFACE_DESC aDesc;
	*theHeight = 0;
	*theWidth = 0;
	if (!mSurface)
		return;

	if (mIsTexture)
	{
		if ((_getTexture()->GetLevelDesc)(0, &aDesc) >= 0)
		{
			*theWidth = aDesc.Width;
			*theHeight = aDesc.Height;
		}
	}
	else
	{
		if ((_getSurface()->GetDesc)(&aDesc) >= 0)
		{
			*theWidth = aDesc.Width;
			*theHeight = aDesc.Height;
		}
	}

}

HRESULT DXSurface9::Blt(RECT* theDestRect, void* theSurface, RECT* theSrcRect, ulong theFlags, DDBLTFX* theBltFx) //509-694
{
	/*if (!mSurface || !theSurface)
		return D3DERR_INVALIDCALL;

	DXSurface9* aSurface = (DXSurface9*)theSurface;

	if (!aSurface->mSurface) //?
		return D3DERR_INVALIDCALL;
	if (mIsTexture)
		return D3DERR_INVALIDCALL;
	if ((aSurface->mImageFlags & ImageFlag_RenderTarget) != 0 && (mImageFlags & ImageFlag_RenderTarget) == 0)
		return D3DERR_INVALIDCALL;
	if ((mImageFlags & ImageFlag_RenderTarget) != 0 && (aSurface->mImageFlags & ImageFlag_RenderTarget) == 0)
		return D3DERR_INVALIDCALL;
	if ((mImageFlags & ImageFlag_RenderTarget) != 0 || (aSurface->mImageFlags & ImageFlag_RenderTarget) != 0)
		return D3DERR_INVALIDCALL;

	_getSurface()->GetDesc();
	_getSurface()->GetDesc();

	assert(aSourceDesc.Format == aDestDesc.Format); //624

	if (FAILED(aSurface->_getSurface()->PreLoad()))
		return D3DERR_INVALIDCALL;

	if (FAILED(_getSurface()->PreLoad()))
	{
		((LPDIRECT3DRESOURCE8)aSurface->_getSurface())->GetType();
		return D3DERR_INVALIDCALL;
	}

	if (aSourceDesc.Format == D3DFMT_X8R8G8B8)
	{
		ulong* aSrcRow = (ulong*)aSourceRect.pBits;
		ulong* aDestRow = (ulong*)aDestRect.pBits;
		int aHeight = theDestRect->bottom - theDestRect->top;
		int aWidth = theDestRect->right - theDestRect->left;
		for (int y = 0; y < aHeight; y++)
		{
			for (int x = 0; x < aWidth; x++)
				aDestRow[x] = aSrcRow[x];
			aDestRow += aDestRect.Pitch / 4;
			aSrcRow += aSourceRect.Pitch / 4;
		}
	}

	else if (aSourceDesc.Format == D3DFMT_R5G6B5)
	{
		ushort* aSrcRow = (ushort*)aSourceRect.pBits;
		ushort* aDestRow = (ushort*)aDestRect.pBits;
		int aHeight = theDestRect->bottom - theDestRect->top;
		int aWidth = theDestRect->right - theDestRect->left;
		for (int y = 0; y < aHeight; y++)
		{
			for (int x = 0; x < aWidth; x++)
				aDestRow[x] = aSrcRow[x];
			aDestRow += aDestRect.Pitch / 4;
			aSrcRow += aSourceRect.Pitch / 4;
		}
	}

	((LPDIRECT3DRESOURCE9)aSurface->_getSurface())->GetType();
	((LPDIRECT3DRESOURCE9)_getSurface())->GetType();
	return 0;*/
}

void DXSurface9::AddRef() //699-705
{
	if (mSurface)
	{
		mSurface->AddRef();
	}
}
