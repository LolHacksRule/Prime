#include "DXSurface8.h"
#include "WindowsGraphicsDriver.h"
#include "../../../AutoCrit.h"
#include "D3D8Interface.h"

using namespace Sexy;

IDXSurface* CreateDXSurface8(WindowsGraphicsDriver* theDriver) //20-24
{
	return new DXSurface8(theDriver);
}

DXSurface8::DXSurface8(WindowsGraphicsDriver* theDriver) //28-37
{
	mDriver = theDriver;
	mD3DInterface = dynamic_cast<D3D8Interface*>(theDriver->mD3DInterface); //?
	mSurface = NULL;
	mSurfaceFormat = 0;
	mTextureSurface = NULL;

	mImageFlags = 0;
	mIsTexture = 0;
}

DXSurface8::~DXSurface8() //42-48
{
	if (mSurface)
		mSurface->Release();
}

bool DXSurface8::Lock(_DEVICESURFACEDESC* theDesc) //53-92
{
	if (mSurface == NULL)
		return false;

	if (mIsTexture == NULL)
		return false;

	D3DSURFACE_DESC aD3DDesc;

	HRESULT aResult;

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

	D3DLOCKED_RECT aLockedRect;

	aResult = _getSurface()->LockRect(&aLockedRect, NULL, 0);

	if (FAILED(aResult))
		return false;

	*(D3DLOCKED_RECT*)*&theDesc->lPitch = aLockedRect;
	return true;
}

void DXSurface8::Unlock(void* __formal) //97-99
{
	if (!mIsTexture)_getSurface()->UnlockRect();
}

bool DXSurface8::GenerateDeviceSurface(DeviceImage* theImage) //Similar to DDImage::GenerateDDSurface | 104-270
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
		LPDIRECT3DTEXTURE8 aTexture = (LPDIRECT3DTEXTURE8)&aData->mTextures.begin()->mTexture;
		if (SUCCEEDED(aTexture->GetSurfaceLevel(0, &mSurface)))
			return true;
		else
			return false;
	}
	else
	{
		HRESULT aResult = mD3DInterface->mD3DDevice->CreateImageSurface(theImage->mWidth, theImage->mHeight, (D3DFORMAT)mSurfaceFormat, (LPDIRECT3DSURFACE8*)&mSurface);

		if (FAILED(aResult))
			return false;

		mIsTexture = false;
		D3DLOCKED_RECT aLockedRect;
		aResult = ((LPDIRECT3DRESOURCE8)_getSurface())->PreLoad(); //&aLockedRect, 0, 0x2000?
		if (FAILED(aResult))
		{
			mSurface->Release();
			mSurface = 0;
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
		else if (aNumBits == 32)
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
		((LPDIRECT3DRESOURCE8)_getSurface())->GetType();
		return true;
	}
}

void DXSurface8::Release() //275-282
{
	if (mSurface)
	{
		mSurface->Release();
		mSurface = NULL;
	}
}

ulong* DXSurface8::GetBits(DeviceImage* theImage) //287-381
{
	LPDIRECT3DSURFACE8 aSurface = NULL; //?
	if (mIsTexture)
	{
		LPDIRECT3DSURFACE8 aSurface = (LPDIRECT3DSURFACE8)_getTexture()->GetSurfaceLevel(0, &aSurface);
		if (FAILED(aSurface))
			return 0;
	}
	else
	{
		aSurface = _getSurface();
	}
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

	if (FAILED(aResult))
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

	if (mBlueBits + mGreenBits + mRedBits == 24)
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

HDC DXSurface8::GetDC() //386-417 (Probably commented out code)
{
	return 0;
}

void DXSurface8::ReleaseDC(HDC theDC) //422-436 (Probably commented out code)
{
}

void DXSurface8::SetSurface(void* theSurface) //441-484
{
	LPDIRECT3DRESOURCE8 aResource = (LPDIRECT3DRESOURCE8)theSurface;

	if (theSurface == NULL)
		mSurface = NULL;

	D3DRESOURCETYPE aResType = aResource->GetType();
	if (mSurface != aResource && (aResType == D3DRTYPE_TEXTURE || aResType == D3DRTYPE_SURFACE))
	{
		mSurface = aResource;
		if (aResType == D3DRTYPE_TEXTURE)
		{
			mIsTexture = true;
			D3DSURFACE_DESC aDesc;
			if (SUCCEEDED(_getTexture()->GetLevelDesc(0, &aDesc)))
				mSurfaceFormat = aDesc.Format;
			else
			{
				mSurfaceFormat = mD3DInterface->mDisplayFormat;
				if (mSurfaceFormat == D3DFMT_X8R8G8B8)
					mSurfaceFormat = D3DFMT_A8R8G8B8;
			}
		}
		else
		{
			mIsTexture = false;
			if (SUCCEEDED(((LPDIRECT3DRESOURCE8)_getSurface())->GetPriority())) //?
				mSurfaceFormat = mD3DInterface->mDisplayFormat;
			else
				mSurfaceFormat = aDesc.Format;
		}
	}
}

void DXSurface8::GetDimensions(int* theWidth, int* theHeight) //489-509
{
	*theHeight = 0;
	*theWidth = 0;
	if (mSurface == NULL)
		return;

	D3DSURFACE_DESC aDesc;

	if (mIsTexture)
	{
		if (SUCCEEDED(_getTexture()->GetLevelDesc(0, &aDesc)))
		{
			*theWidth = aDesc.Width;
			*theHeight = aDesc.Height;
		}
	}

	else
	{
		if (SUCCEEDED(((LPDIRECT3DRESOURCE8)_getSurface())->GetPriority()))
		{
			*theWidth = aDesc.Width;
			*theHeight = aDesc.Height;
		}
	}
}

HRESULT DXSurface8::Blt(LPRECT theDestRect, void* theSurface, LPRECT theSrcRect, DWORD theFlags, LPDDBLTFX theBltFx) //TODO | 514-710
{
	if(!mSurface || !theSurface)
		return D3DERR_INVALIDCALL;
	DXSurface8* aSurface = (DXSurface8*)theSurface;
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

	D3DSURFACE_DESC aDestDesc = _getSurface()->GetPriority();
	D3DSURFACE_DESC aSourceDesc = _getSurface()->GetPriority();

	assert(aSourceDesc.Format == aDestDesc.Format); //637

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

	((LPDIRECT3DRESOURCE8)aSurface->_getSurface())->GetType();
	((LPDIRECT3DRESOURCE8)_getSurface())->GetType();
	return 0;
}

void DXSurface8::AddRef() //715-721
{
	if (mSurface)
	{

		mSurface->AddRef();
	}
}
