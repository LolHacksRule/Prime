#include "DeviceImage.h"
#include "Debug.h"
#include "SexyCache.h"
#include "NativeDisplay.h"
//Seems to be based off from DDImage

using namespace Sexy;

DeviceImage::DeviceImage(SexyAppBase *theApp): //37-40
	MemoryImage(theApp)
{
	mDriver = theApp->mGraphicsDriver;
	Init();
}

DeviceImage::DeviceImage(IGraphicsDriver* theDriver) : //44-47
	MemoryImage(gSexyAppBase)
{
	mDriver = theDriver;
	Init();
}


DeviceImage::DeviceImage(): //53-56
	MemoryImage(gSexyAppBase)
{
	mDriver = gSexyAppBase->mGraphicsDriver;
	Init();
}

DeviceImage::~DeviceImage() //61-66
{
	if (mSurface != NULL)
		delete mSurface;
	mDriver->RemoveDeviceImage(this);

	DBG_ASSERTE(mLockCount == 0); //65 | 70 BejLiveWin8
}

void DeviceImage::Init() //71-82
{
	mSurface = NULL;
	if (mDriver)
		mDriver->AddDeviceImage(this);
	
	mNoLock = false;
	mWantDeviceSurface = false;			
	mDrawToBits = false;
	mSurfaceSet = false;

	mLockCount = 0;
}

bool DeviceImage::LockSurface() //True in XNA | 87-113
{
	if (mDriver->Is3D() && HasImageFlag(ImageFlag_RenderTarget))
		return false;

	if (mSurface == NULL)
		GenerateDeviceSurface();
	if (mSurface == NULL)
		return false;

	if (mLockCount == 0)
	{
		if (!mSurface->Lock(&mLockedSurfaceDesc)); //?
			return false;
	}

	mLockCount++;

	DBG_ASSERTE(mLockCount < 8); //110 | 115 BejLiveWin8

	return true;
}

bool DeviceImage::UnlockSurface() //Seems accurate | 118-132
{
	if (mDriver->Is3D() && HasImageFlag(ImageFlag_RenderTarget))
		return false;

	--mLockCount;

	if (mLockCount == 0)
	{
		mSurface->Unlock(NULL);
	}

	DBG_ASSERTE(mLockCount >= 0); //129 | 134 BejLiveWin8

	return true;
}

void DeviceImage::SetSurface(void *theSurface) //137-174
{
	mSurfaceSet = true;
	if (mSurface)
	{
		int aVersion = mDriver->GetVersion();
		if (mSurface->GetVersion() != aVersion)
		{
			if (mSurface)
				delete mSurface;
			mSurface = NULL;
		}
	}
	if (theSurface)
	{
		if (!mSurface)
			mSurface = mDriver->CreateDeviceSurface();
		mSurface->SetSurface(theSurface);
		mSurface->GetDimensions(&mWidth, &mHeight);
		mNoLock = false;
	}
	else
	{
		if (mSurface) //Why
		{
			delete mSurface;
			mSurface = NULL;
		}
		mWidth = 0;
		mHeight = 0;

		mNoLock = true;
	}
}

bool DeviceImage::GenerateDeviceSurface() //179-195
{
	if (mSurface != NULL)
		return false;

	DeviceSurface* aSurface = mDriver->CreateDeviceSurface();
	if (aSurface == NULL)
		return false;
	aSurface->mImageFlags = GetImageFlags();
	bool aResult = aSurface->GenerateDeviceSurface(this);
	mSurface = aSurface;
	return aResult;
}

void DeviceImage::DeleteDeviceSurface() //201-210
{
	if (mSurface != NULL)
	{
		if ((mColorTable == NULL) && (mBits == NULL) && (GetRenderData() == NULL))
			GetBits();
		delete mSurface;
		mSurface = NULL;
	}
}

void DeviceImage::ReInit() //235-240
{
	MemoryImage::ReInit();

	if (mWantDeviceSurface)
		GenerateDeviceSurface();
}

void DeviceImage::PurgeBits() //245-279
{
	if (mSurfaceSet)
		return;

	mPurgeBits = true;

	MemoryImage::CommitBits(); //Done by memoryimage

	if (!mApp->Is3DAccelerated())
	{
		if ((mWantDeviceSurface) && (GenerateDeviceSurface()))
		{                        
			delete [] mBits;
			mBits = NULL;

			delete [] mColorIndices;
			mColorIndices = NULL;

			delete [] mColorTable;
			mColorTable = NULL;                        

			return;
		}	
	}
	else // Accelerated
	{
		if (mSurface != NULL)
		{
			GetBits();
			DeleteDeviceSurface();
		}
	}
	
	MemoryImage::PurgeBits();
}

void DeviceImage::DeleteAllNonSurfaceData() //284-302
{
	delete [] mBits;
	mBits = NULL;

	delete [] mNativeAlphaData;
	mNativeAlphaData = NULL;

	delete [] mRLAdditiveData;
	mRLAdditiveData = NULL;

	delete [] mRLAlphaData;
	mRLAlphaData = NULL;

	delete [] mColorTable;
	mColorTable = NULL;

	delete [] mColorIndices;
	mColorIndices = NULL;
}

void DeviceImage::DeleteNativeData() //307-313
{
	if (mSurfaceSet)
		return;

	MemoryImage::DeleteNativeData();
	DeleteDeviceSurface();
}

void DeviceImage::DeleteExtraBuffers() //318-324
{
	if (mSurfaceSet)
		return;

	MemoryImage::DeleteExtraBuffers();
	DeleteDeviceSurface();
}

void DeviceImage::FillRect(const Rect& theRect, const Color& theColor, int theDrawMode) //Double check, not in XNA | 329-348
{
	if ((mDrawToBits) || (mHasAlpha) || ((mHasTrans) && (!mFirstPixelTrans)))
	{
		MemoryImage::FillRect(theRect, theColor, theDrawMode);
		return;
	}	

	switch (theDrawMode)
	{
	case Graphics::DRAWMODE_NORMAL:
		NormalFillRect(theRect, theColor);
		break;
	case Graphics::DRAWMODE_ADDITIVE:
		AdditiveFillRect(theRect, theColor);
		break;
	}

	DeleteAllNonSurfaceData();
}

void DeviceImage::NormalDrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor) //TODO, prob identical, not in XNA | 353-919
{
	if (mNoLock)
		return;

	double aMinX = min(theStartX, theEndX);
	double aMinY = min(theStartY, theEndY);
	double aMaxX = max(theStartX, theEndX);
	double aMaxY = max(theStartY, theEndY);

	if (!LockSurface())
		return;	

	ulong aRMask = mLockedSurfaceDesc.ddpfPixelFormat.dwRBitMask;
	ulong aGMask = mLockedSurfaceDesc.ddpfPixelFormat.dwGBitMask;
	ulong aBMask = mLockedSurfaceDesc.ddpfPixelFormat.dwBBitMask;

	ulong aRRoundAdd = aRMask >> 1;
	ulong aGRoundAdd = aGMask >> 1;
	ulong aBRoundAdd = aBMask >> 1;
	
	if (mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 16)
	{
		if (theColor.mAlpha == 255)
		{
			ushort aColor = (ushort)
				(((((theColor.mRed * aRMask) + aRRoundAdd) >> 8) & aRMask) |
				((((theColor.mGreen * aGMask) + aGRoundAdd) >> 8) & aGMask) |
				((((theColor.mBlue * aBMask) + aBRoundAdd) >> 8) & aBMask));

			double dv = theEndY - theStartY;
			double dh = theEndX - theStartX;
			int minG, maxG, G, DeltaG1, DeltaG2;
			double swap;
			int inc = 1;
			int aCurX;
			int aCurY;
			int aRowWidth = mLockedSurfaceDesc.lPitch/2;
			int aRowAdd = aRowWidth;

			if (fabs(dv) < fabs(dh))
			{
				// Mostly horizontal
				if (dh < 0)
				{
					dh = -dh;
					dv = -dv;
					swap = theEndY;
					theEndY = theStartY;
					theStartY = swap;
					swap = theEndX;
					theEndX = theStartX;
					theStartX = swap;
				}
				if (dv < 0)
				{
					dv = -dv;
					inc = -1;
					aRowAdd = -aRowAdd;
				}

				ushort* aDestPixels = ((ushort*) mLockedSurfaceDesc.lpSurface) + ((int) theStartY * aRowWidth) + (int) theStartX;
				*aDestPixels = aColor;
				aDestPixels++;

				aCurY = (int) theStartY;
				aCurX = (int) theStartX + 1;

				G = 2 * dv - dh;
				DeltaG1 = 2 * (dv - dh);
				DeltaG2 = 2 * dv;
				
				G += DeltaG2 * (theStartY - (int) theStartY);

				while (aCurX <= theEndX)
				{
					if (G > 0)
					{
						G += DeltaG1;
						aCurY += inc;
						aDestPixels += aRowAdd;

						if (aCurX<aMinX || aCurY<aMinY || aCurX>aMaxX || aCurY>aMaxY)
							break;
					}
					else
						G += DeltaG2;
					
					*aDestPixels = aColor;

					aCurX++;
					aDestPixels++;
				}
			}
			else
			{
				// Mostly vertical
				if ( dv < 0 )
				{
					dh = -dh;
					dv = -dv;
					swap = theEndY;
					theEndY = theStartY;
					theStartY = swap;
					swap = theEndX;
					theEndX = theStartX;
					theStartX = swap;
				}

				if (dh < 0)
				{
					dh = -dh;
					inc = -1;
				}

				ushort* aDestPixels = ((ushort*) mLockedSurfaceDesc.lpSurface) + ((int) theStartY * mLockedSurfaceDesc.lPitch/2) + (int) theStartX;
				*aDestPixels = aColor;
				aDestPixels += aRowAdd;

				aCurX = theStartX;
				aCurY = theStartY + 1;

				G = 2 * dh - dv;
				minG = maxG = G;
				DeltaG1 = 2 * ( dh - dv );
				DeltaG2 = 2 * dh;

				G += DeltaG2 * (theStartX - (int) theStartX);

				while (aCurY <= theEndY)
				{
					if ( G > 0 )
					{						
						G += DeltaG1;
						aCurX += inc;
						aDestPixels += inc;

						if (aCurX<aMinX || aCurY<aMinY || aCurX>aMaxX || aCurY>aMaxY)
							break;
					}
					else
						G += DeltaG2;
					
					*aDestPixels = aColor;

					aCurY++;
					aDestPixels += aRowAdd;
				}
			}
		}
		else
		{
			ushort src = 
				((((((theColor.mRed * theColor.mAlpha + 0x80) >> 8) * aRMask) + aRRoundAdd) >> 8) & aRMask) +
				((((((theColor.mGreen * theColor.mAlpha + 0x80) >> 8) * aGMask) + aGRoundAdd) >> 8) & aGMask) +
				((((((theColor.mBlue * theColor.mAlpha + 0x80) >> 8) * aBMask) + aBRoundAdd) >> 8) & aBMask);
			int oma = 256 - theColor.mAlpha;

			double dv = theEndY - theStartY;
			double dh = theEndX - theStartX;
			int minG, maxG, G, DeltaG1, DeltaG2;
			double swap;
			int inc = 1;
			int aCurX;
			int aCurY;
			int aRowWidth = mLockedSurfaceDesc.lPitch/2;
			int aRowAdd = aRowWidth;

			if (abs(dv) < abs(dh))
			{
				// Mostly horizontal
				if (dh < 0)
				{
					dh = -dh;
					dv = -dv;
					swap = theEndY;
					theEndY = theStartY;
					theStartY = swap;
					swap = theEndX;
					theEndX = theStartX;
					theStartX = swap;
				}
				if (dv < 0)
				{
					dv = -dv;
					inc = -1;
					aRowAdd = -aRowAdd;
				}

				ushort* aDestPixels = ((ushort*) mLockedSurfaceDesc.lpSurface) + ((int) theStartY * aRowWidth) + (int) theStartX;
				ushort dest = *aDestPixels;
				*(aDestPixels++) = src + 
					(((((dest & aRMask) * oma) + aRRoundAdd) >> 8) & aRMask) +
					(((((dest & aGMask) * oma) + aGRoundAdd) >> 8) & aGMask) +
					(((((dest & aBMask) * oma) + aBRoundAdd) >> 8) & aBMask);				

				aCurY = theStartY;
				aCurX = theStartX + 1;

				G = 2 * dv - dh;
				DeltaG1 = 2 * (dv - dh);
				DeltaG2 = 2 * dv;

				G += DeltaG2 * (theStartY - (int) theStartY);

				while (aCurX <= theEndX)
				{
					if (G > 0)
					{
						G += DeltaG1;
						aCurY += inc;
						aDestPixels += aRowAdd;

						if (aCurX<aMinX || aCurY<aMinY || aCurX>aMaxX || aCurY>aMaxY)
							break;
					}
					else
						G += DeltaG2;
					
					dest = *aDestPixels;
					*(aDestPixels++) = src + 
						(((((dest & aRMask) * oma) + aRRoundAdd) >> 8) & aRMask) +
						(((((dest & aGMask) * oma) + aGRoundAdd) >> 8) & aGMask) +
						(((((dest & aBMask) * oma) + aBRoundAdd) >> 8) & aBMask);					

					aCurX++;					
				}
			}
			else
			{
				// Mostly vertical
				if ( dv < 0 )
				{
					dh = -dh;
					dv = -dv;
					swap = theEndY;
					theEndY = theStartY;
					theStartY = swap;
					swap = theEndX;
					theEndX = theStartX;
					theStartX = swap;
				}

				if (dh < 0)
				{
					dh = -dh;
					inc = -1;
				}

				ushort* aDestPixels = ((ushort*) mLockedSurfaceDesc.lpSurface) + ((int) theStartY * mLockedSurfaceDesc.lPitch/2) + (int) theStartX;
				ushort dest = *aDestPixels;
				*aDestPixels = src + 
					(((((dest & aRMask) * oma) + aRRoundAdd) >> 8) & aRMask) +
					(((((dest & aGMask) * oma) + aGRoundAdd) >> 8) & aGMask) +
					(((((dest & aBMask) * oma) + aBRoundAdd) >> 8) & aBMask);
				aDestPixels += aRowAdd;

				aCurX = theStartX;
				aCurY = theStartY + 1;

				G = 2 * dh - dv;
				minG = maxG = G;
				DeltaG1 = 2 * ( dh - dv );
				DeltaG2 = 2 * dh;

				G += DeltaG2 * (theStartX - (int) theStartX);

				while (aCurY <= theEndY)
				{
					if ( G > 0 )
					{
						G += DeltaG1;
						aCurX += inc;
						aDestPixels += inc;

						if (aCurX<aMinX || aCurY<aMinY || aCurX>aMaxX || aCurY>aMaxY)
							break;
					}
					else
						G += DeltaG2;
					
					dest = *aDestPixels;
					*aDestPixels = src + 
						(((((dest & aRMask) * oma) + aRRoundAdd) >> 8) & aRMask) +
						(((((dest & aGMask) * oma) + aGRoundAdd) >> 8) & aGMask) +
						(((((dest & aBMask) * oma) + aBRoundAdd) >> 8) & aBMask);

					aCurY++;
					aDestPixels += aRowAdd;
				}
			}
		}
	}
	else if (mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 32)
	{
		if (theColor.mAlpha == 255)
		{
			ulong aColor = 
				((((theColor.mRed * aRMask) + aRRoundAdd) >> 8) & aRMask) |
				((((theColor.mGreen * aGMask) + aGRoundAdd) >> 8) & aGMask) |
				((((theColor.mBlue * aBMask) + aBRoundAdd) >> 8) & aBMask);

			double dv = theEndY - theStartY;
			double dh = theEndX - theStartX;
			int minG, maxG, G, DeltaG1, DeltaG2;
			double swap;
			int inc = 1;
			int aCurX;
			int aCurY;
			int aRowWidth = mLockedSurfaceDesc.lPitch/4;
			int aRowAdd = aRowWidth;;

			if (abs(dv) < abs(dh))
			{
				// Mostly horizontal
				if (dh < 0)
				{
					dh = -dh;
					dv = -dv;
					swap = theEndY;
					theEndY = theStartY;
					theStartY = swap;
					swap = theEndX;
					theEndX = theStartX;
					theStartX = swap;
				}
				if (dv < 0)
				{
					dv = -dv;
					inc = -1;
					aRowAdd = -aRowAdd;
				}

				ulong* aDestPixels = ((ulong*) mLockedSurfaceDesc.lpSurface) + ((int) theStartY * aRowWidth) + (int) theStartX;
				*aDestPixels = aColor;
				aDestPixels++;

				aCurY = theStartY;
				aCurX = theStartX + 1;

				G = 2 * dv - dh;
				DeltaG1 = 2 * (dv - dh);
				DeltaG2 = 2 * dv;

				G += DeltaG2 * (theStartY - (int) theStartY);

				while (aCurX <= theEndX)
				{
					if (G > 0)
					{
						G += DeltaG1;
						aCurY += inc;
						aDestPixels += aRowAdd;

						if (aCurX<aMinX || aCurY<aMinY || aCurX>aMaxX || aCurY>aMaxY)
							break;
					}
					else
						G += DeltaG2;
					
					*aDestPixels = aColor;

					aCurX++;
					aDestPixels++;
				}
			}
			else
			{
				// Mostly vertical
				if ( dv < 0 )
				{
					dh = -dh;
					dv = -dv;
					swap = theEndY;
					theEndY = theStartY;
					theStartY = swap;
					swap = theEndX;
					theEndX = theStartX;
					theStartX = swap;
				}

				if (dh < 0)
				{
					dh = -dh;
					inc = -1;
				}

				ulong* aDestPixels = ((ulong*) mLockedSurfaceDesc.lpSurface) + ((int) theStartY * aRowWidth) + (int) theStartX;
				*aDestPixels = aColor;
				aDestPixels += aRowAdd;

				aCurX = theStartX;
				aCurY = theStartY + 1;

				G = 2 * dh - dv;
				minG = maxG = G;
				DeltaG1 = 2 * ( dh - dv );
				DeltaG2 = 2 * dh;

				G += DeltaG2 * (theStartX - (int) theStartX);

				while (aCurY <= theEndY)
				{
					if ( G > 0 )
					{
						G += DeltaG1;
						aCurX += inc;
						aDestPixels += inc;

						if (aCurX<aMinX || aCurY<aMinY || aCurX>aMaxX || aCurY>aMaxY)
							break;
					}
					else
						G += DeltaG2;
					
					*aDestPixels = aColor;

					aCurY++;
					aDestPixels += aRowAdd;
				}
			}
		}
		else
		{
			ulong src = 
				((((((theColor.mRed * theColor.mAlpha + 0x80) >> 8) * aRMask) + aRRoundAdd) >> 8) & aRMask) +
				((((((theColor.mGreen * theColor.mAlpha + 0x80) >> 8) * aGMask) + aGRoundAdd) >> 8) & aGMask) +
				((((((theColor.mBlue * theColor.mAlpha + 0x80) >> 8) * aBMask) + aBRoundAdd) >> 8) & aBMask);
			int oma = 256 - theColor.mAlpha;

			double dv = theEndY - theStartY;
			double dh = theEndX - theStartX;
			int minG, maxG, G, DeltaG1, DeltaG2;
			double swap;
			int inc = 1;
			int aCurX;
			int aCurY;
			int aRowWidth = mLockedSurfaceDesc.lPitch/4;
			int aRowAdd = aRowWidth;

			if (abs(dv) < abs(dh))
			{
				// Mostly horizontal
				if (dh < 0)
				{
					dh = -dh;
					dv = -dv;
					swap = theEndY;
					theEndY = theStartY;
					theStartY = swap;
					swap = theEndX;
					theEndX = theStartX;
					theStartX = swap;
				}
				if (dv < 0)
				{
					dv = -dv;
					inc = -1;
					aRowAdd = -aRowAdd;
				}

				ulong* aDestPixels = ((ulong*) mLockedSurfaceDesc.lpSurface) + ((int) theStartY * aRowWidth) + (int) theStartX;
				ulong dest = *aDestPixels;
				*(aDestPixels++) = src + 
					(((((dest & aRMask) * oma) + aRRoundAdd) >> 8) & aRMask) +
					(((((dest & aGMask) * oma) + aGRoundAdd) >> 8) & aGMask) +
					(((((dest & aBMask) * oma) + aBRoundAdd) >> 8) & aBMask);				

				aCurY = theStartY;
				aCurX = theStartX + 1;

				G = 2 * dv - dh;
				DeltaG1 = 2 * (dv - dh);
				DeltaG2 = 2 * dv;

				G += DeltaG2 * (theStartX - (int) theStartX);

				while (aCurX <= theEndX)
				{
					if (G > 0)
					{
						G += DeltaG1;
						aCurY += inc;
						aDestPixels += aRowAdd;

						if (aCurX<aMinX || aCurY<aMinY || aCurX>aMaxX || aCurY>aMaxY)
							break;
					}
					else
						G += DeltaG2;
					
					dest = *aDestPixels;
					*(aDestPixels++) = src + 
						(((((dest & aRMask) * oma) + aRRoundAdd) >> 8) & aRMask) +
						(((((dest & aGMask) * oma) + aGRoundAdd) >> 8) & aGMask) +
						(((((dest & aBMask) * oma) + aBRoundAdd) >> 8) & aBMask);					

					aCurX++;					
				}
			}
			else
			{
				// Mostly vertical
				if ( dv < 0 )
				{
					dh = -dh;
					dv = -dv;
					swap = theEndY;
					theEndY = theStartY;
					theStartY = swap;
					swap = theEndX;
					theEndX = theStartX;
					theStartX = swap;
				}

				if (dh < 0)
				{
					dh = -dh;
					inc = -1;
				}

				ulong* aDestPixels = ((ulong*) mLockedSurfaceDesc.lpSurface) + ((int) theStartY * aRowWidth) + (int) theStartX;
				ulong dest = *aDestPixels;
				*aDestPixels = src + 
					(((((dest & aRMask) * oma) + aRRoundAdd) >> 8) & aRMask) +
					(((((dest & aGMask) * oma) + aGRoundAdd) >> 8) & aGMask) +
					(((((dest & aBMask) * oma) + aBRoundAdd) >> 8) & aBMask);
				aDestPixels += aRowAdd;

				aCurX = theStartX;
				aCurY = theStartY + 1;

				G = 2 * dh - dv;
				minG = maxG = G;
				DeltaG1 = 2 * ( dh - dv );
				DeltaG2 = 2 * dh;

				G += DeltaG2 * (theStartX - (int) theStartX);

				while (aCurY <= theEndY)
				{
					if ( G > 0 )
					{
						G += DeltaG1;
						aCurX += inc;
						aDestPixels += inc;

						if (aCurX<aMinX || aCurY<aMinY || aCurX>aMaxX || aCurY>aMaxY)
							break;
					}
					else
						G += DeltaG2;
					
					dest = *aDestPixels;
					*aDestPixels = src + 
						(((((dest & aRMask) * oma) + aRRoundAdd) >> 8) & aRMask) +
						(((((dest & aGMask) * oma) + aGRoundAdd) >> 8) & aGMask) +
						(((((dest & aBMask) * oma) + aBRoundAdd) >> 8) & aBMask);

					aCurY++;
					aDestPixels += aRowAdd;
				}
			}
		}
	}

	UnlockSurface();
}

void DeviceImage::AdditiveDrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor) //TODO prob identical, not in XNA | 924-1274
{
	if (mNoLock)
		return;

	double aMinX = min(theStartX, theEndX);
	double aMinY = min(theStartY, theEndY);
	double aMaxX = max(theStartX, theEndX);
	double aMaxY = max(theStartY, theEndY);

	if (!LockSurface())
		return;

	ulong aRMask = mLockedSurfaceDesc.ddpfPixelFormat.dwRBitMask;
	ulong aGMask = mLockedSurfaceDesc.ddpfPixelFormat.dwGBitMask;
	ulong aBMask = mLockedSurfaceDesc.ddpfPixelFormat.dwBBitMask;

	ulong aRRoundAdd = aRMask >> 1;
	ulong aGRoundAdd = aGMask >> 1;
	ulong aBRoundAdd = aBMask >> 1;

	int aRedShift = mDriver->GetNativeDisplayInfo()->mRedShift;
	int aGreenShift = mDriver->GetNativeDisplayInfo()->mGreenShift;
	int aBlueShift = mDriver->GetNativeDisplayInfo()->mBlueShift;

	int* aMaxRedTable = mDriver->GetNativeDisplayInfo()->mRedAddTable;
	int* aMaxGreenTable = mDriver->GetNativeDisplayInfo()->mGreenAddTable;
	int* aMaxBlueTable = mDriver->GetNativeDisplayInfo()->mBlueAddTable;
	
	if (mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 16)
	{
		ushort rc = ((theColor.mRed * theColor.mAlpha) / 255) >> (8-mDriver->GetNativeDisplayInfo()->mRedBits);
		ushort gc = ((theColor.mGreen * theColor.mAlpha) / 255) >> (8-mDriver->GetNativeDisplayInfo()->mGreenBits);
		ushort bc = ((theColor.mBlue * theColor.mAlpha) / 255) >> (8-mDriver->GetNativeDisplayInfo()->mBlueBits);

		double dv = theEndY - theStartY;
		double dh = theEndX - theStartX;
		int minG, maxG, G, DeltaG1, DeltaG2;
		double swap;
		int inc = 1;
		int aCurX;
		int aCurY;
		int aRowWidth = mLockedSurfaceDesc.lPitch/2;
		int aRowAdd = aRowWidth;

		if (abs(dv) < abs(dh))
		{
			// Mostly horizontal
			if (dh < 0)
			{
				dh = -dh;
				dv = -dv;
				swap = theEndY;
				theEndY = theStartY;
				theStartY = swap;
				swap = theEndX;
				theEndX = theStartX;
				theStartX = swap;
			}
			if (dv < 0)
			{
				dv = -dv;
				inc = -1;
				aRowAdd = -aRowAdd;
			}

			ushort* aDestPixels = ((ushort*) mLockedSurfaceDesc.lpSurface) + ((int) theStartY * aRowWidth) + (int) theStartX;
			ushort dest = *aDestPixels;

			int r = aMaxRedTable[((dest & aRMask) >> aRedShift) + rc];
			int g = aMaxGreenTable[((dest & aGMask) >> aGreenShift) + gc];
			int b = aMaxBlueTable[((dest & aBMask) >> aBlueShift) + bc];

			*(aDestPixels++) = 
				(r << aRedShift) |
				(g << aGreenShift) |
				(b << aBlueShift);

			aCurY = theStartY;
			aCurX = theStartX + 1;

			G = 2 * dv - dh;
			DeltaG1 = 2 * (dv - dh);
			DeltaG2 = 2 * dv;

			G += DeltaG2 * (theStartY - (int) theStartY);

			while (aCurX <= theEndX)
			{
				if (G > 0)
				{					
					G += DeltaG1;
					aCurY += inc;
					aDestPixels += aRowAdd;

					if (aCurX<aMinX || aCurY<aMinY || aCurX>aMaxX || aCurY>aMaxY)
						break;
				}
				else
					G += DeltaG2;
				
				dest = *aDestPixels;

				r = aMaxRedTable[((dest & aRMask) >> aRedShift) + rc];
				g = aMaxGreenTable[((dest & aGMask) >> aGreenShift) + gc];
				b = aMaxBlueTable[((dest & aBMask) >> aBlueShift) + bc];

				*(aDestPixels++) = 
					(r << aRedShift) |
					(g << aGreenShift) |
					(b << aBlueShift);

				aCurX++;				
			}
		}
		else
		{
			// Mostly vertical
			if ( dv < 0 )
			{
				dh = -dh;
				dv = -dv;
				swap = theEndY;
				theEndY = theStartY;
				theStartY = swap;
				swap = theEndX;
				theEndX = theStartX;
				theStartX = swap;
			}

			if (dh < 0)
			{
				dh = -dh;
				inc = -1;
			}

			ushort* aDestPixels = ((ushort*) mLockedSurfaceDesc.lpSurface) + ((int) theStartY * mLockedSurfaceDesc.lPitch/2) + (int) theStartX;
			
			ushort dest = *aDestPixels;

			int r = aMaxRedTable[((dest & aRMask) >> aRedShift) + rc];
			int g = aMaxGreenTable[((dest & aGMask) >> aGreenShift) + gc];
			int b = aMaxBlueTable[((dest & aBMask) >> aBlueShift) + bc];

			*aDestPixels = 
				(r << aRedShift) |
				(g << aGreenShift) |
				(b << aBlueShift);

			aDestPixels += aRowAdd;

			aCurX = theStartX;
			aCurY = theStartY + 1;

			G = 2 * dh - dv;
			minG = maxG = G;
			DeltaG1 = 2 * ( dh - dv );
			DeltaG2 = 2 * dh;	
			
			G += DeltaG2 * (theStartX - (int) theStartX);

			while (aCurY <= theEndY)
			{
				if ( G > 0 )
				{
					G += DeltaG1;
					aCurX += inc;
					aDestPixels += inc;

					if (aCurX<aMinX || aCurY<aMinY || aCurX>aMaxX || aCurY>aMaxY)
						break;
				}
				else
					G += DeltaG2;
				
				dest = *aDestPixels;

				r = aMaxRedTable[((dest & aRMask) >> aRedShift) + rc];
				g = aMaxGreenTable[((dest & aGMask) >> aGreenShift) + gc];
				b = aMaxBlueTable[((dest & aBMask) >> aBlueShift) + bc];

				*aDestPixels = 
					(r << aRedShift) |
					(g << aGreenShift) |
					(b << aBlueShift);

				aCurY++;
				aDestPixels += aRowAdd;
			}
		}
	}
	else if (mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 32)
	{
		ulong rc = ((theColor.mRed * theColor.mAlpha) / 255) >> (8-mDriver->GetNativeDisplayInfo()->mRedBits);
		ulong gc = ((theColor.mGreen * theColor.mAlpha) / 255) >> (8-mDriver->GetNativeDisplayInfo()->mGreenBits);
		ulong bc = ((theColor.mBlue * theColor.mAlpha) / 255) >> (8-mDriver->GetNativeDisplayInfo()->mBlueBits);

		double dv = theEndY - theStartY;
		double dh = theEndX - theStartX;
		int minG, maxG, G, DeltaG1, DeltaG2;
		double swap;
		int inc = 1;
		int aCurX;
		int aCurY;
		int aRowWidth = mLockedSurfaceDesc.lPitch/4;
		int aRowAdd = aRowWidth;

		if (abs(dv) < abs(dh))
		{
			// Mostly horizontal
			if (dh < 0)
			{
				dh = -dh;
				dv = -dv;
				swap = theEndY;
				theEndY = theStartY;
				theStartY = swap;
				swap = theEndX;
				theEndX = theStartX;
				theStartX = swap;
			}

			if (dv < 0)
			{
				dv = -dv;
				inc = -1;
				aRowAdd = -aRowAdd;
			}

			ulong* aDestPixels = ((ulong*) mLockedSurfaceDesc.lpSurface) + ((int) theStartY * aRowWidth) + (int) theStartX;
			ulong dest = *aDestPixels;

			int r = aMaxRedTable[((dest & aRMask) >> aRedShift) + rc];
			int g = aMaxGreenTable[((dest & aGMask) >> aGreenShift) + gc];
			int b = aMaxBlueTable[((dest & aBMask) >> aBlueShift) + bc];

			*(aDestPixels++) = 
				(r << aRedShift) |
				(g << aGreenShift) |
				(b << aBlueShift);

			aCurY = theStartY;
			aCurX = theStartX + 1;

			G = 2 * dv - dh;
			DeltaG1 = 2 * (dv - dh);
			DeltaG2 = 2 * dv;			

			while (aCurX <= theEndX)
			{
				if (G > 0)
				{
					G += DeltaG1;
					aCurY += inc;
					aDestPixels += aRowAdd;

					if (aCurX<aMinX || aCurY<aMinY || aCurX>aMaxX || aCurY>aMaxY)
						break;
				}
				else
					G += DeltaG2;
				
				dest = *aDestPixels;

				r = aMaxRedTable[((dest & aRMask) >> aRedShift) + rc];
				g = aMaxGreenTable[((dest & aGMask) >> aGreenShift) + gc];
				b = aMaxBlueTable[((dest & aBMask) >> aBlueShift) + bc];

				*(aDestPixels++) = 
					(r << aRedShift) |
					(g << aGreenShift) |
					(b << aBlueShift);

				aCurX++;				
			}
		}
		else
		{
			// Mostly vertical
			if ( dv < 0 )
			{
				dh = -dh;
				dv = -dv;
				swap = theEndY;
				theEndY = theStartY;
				theStartY = swap;
				swap = theEndX;
				theEndX = theStartX;
				theStartX = swap;
			}

			if (dh < 0)
			{
				dh = -dh;
				inc = -1;
			}

			ulong* aDestPixels = ((ulong*) mLockedSurfaceDesc.lpSurface) + ((int) theStartY * mLockedSurfaceDesc.lPitch/4) + (int) theStartX;
			
			ulong dest = *aDestPixels;

			int r = aMaxRedTable[((dest & aRMask) >> aRedShift) + rc];
			int g = aMaxGreenTable[((dest & aGMask) >> aGreenShift) + gc];
			int b = aMaxBlueTable[((dest & aBMask) >> aBlueShift) + bc];

			*aDestPixels = 
				(r << aRedShift) |
				(g << aGreenShift) |
				(b << aBlueShift);

			aDestPixels += aRowAdd;

			aCurX = theStartX;
			aCurY = theStartY + 1;

			G = 2 * dh - dv;
			minG = maxG = G;
			DeltaG1 = 2 * ( dh - dv );
			DeltaG2 = 2 * dh;
			while (aCurY <= theEndY)
			{
				if ( G > 0 )
				{
					G += DeltaG1;
					aCurX += inc;
					aDestPixels += inc;

					if (aCurX<aMinX || aCurY<aMinY || aCurX>aMaxX || aCurY>aMaxY)
						break;
				}
				else
					G += DeltaG2;
				
				dest = *aDestPixels;

				r = aMaxRedTable[((dest & aRMask) >> aRedShift) + rc];
				g = aMaxGreenTable[((dest & aGMask) >> aGreenShift) + gc];
				b = aMaxBlueTable[((dest & aBMask) >> aBlueShift) + bc];

				*aDestPixels = 
					(r << aRedShift) |
					(g << aGreenShift) |
					(b << aBlueShift);

				aCurY++;
				aDestPixels += aRowAdd;
			}
		}
	}

	UnlockSurface();
}

void DeviceImage::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode, bool antiAlias) //TODO, not in XNA | 1279-1332
{	
	if ((mDrawToBits) || (mHasAlpha) || (mHasTrans))
	{
		MemoryImage::DrawLine(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode);
		return;
	}

	if (theStartY == theEndY)
	{
		int aStartX = min(theStartX, theEndX);
		int aEndX = max(theStartX, theEndX);

		FillRect(Rect(aStartX, theStartY, aEndX-aStartX+1, theEndY-theStartY+1), theColor, theDrawMode);
		return;
	}
	else if (theStartX == theEndX)
	{
		int aStartY = min(theStartY, theEndY);
		int aEndY = max(theStartY, theEndY);

		FillRect(Rect(theStartX, aStartY, theEndX-theStartX+1, aEndY-aStartY+1), theColor, theDrawMode);
		return;
	}

	CommitBits();	

	switch (theDrawMode)
	{
	case Graphics::DRAWMODE_NORMAL:
		NormalDrawLine(theStartX, theStartY, theEndX, theEndY, theColor);
		break;
	case Graphics::DRAWMODE_ADDITIVE:
		AdditiveDrawLine(theStartX, theStartY, theEndX, theEndY, theColor);
		break;
	}

	DeleteAllNonSurfaceData();
}

void DeviceImage::NormalDrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor) //TODO identical? | 1337-1549
{
	if (mNoLock)
		return;

	if (!LockSurface())
		return;

	ulong aRMask = mLockedSurfaceDesc.ddpfPixelFormat.dwRBitMask;
	ulong aGMask = mLockedSurfaceDesc.ddpfPixelFormat.dwGBitMask;
	ulong aBMask = mLockedSurfaceDesc.ddpfPixelFormat.dwBBitMask;
	ulong color = (((theColor.mRed * aRMask) >> 8) & aRMask) |
					(((theColor.mGreen * aGMask) >> 8) & aGMask) |
					(((theColor.mBlue * aBMask) >> 8) & aBMask);

	int aX0 = (int)theStartX, aX1 = (int)theEndX;
	int aY0 = (int)theStartY, aY1 = (int)theEndY;
	int aXinc = 1;
	if (aY0 > aY1)
	{
		int aTempX = aX0, aTempY = aY0;
		aX0 = aX1; aY0 = aY1;
		aX1 = aTempX; aY1 = aTempY;
		double aTempXd = theStartX, aTempYd = theStartY;
		theStartX = theEndX; theStartY = theEndY;
		theEndX = aTempXd; theEndY = aTempYd;
	}

	int dx = aX1 - aX0;
	int dy = aY1 - aY0;
	double dxd = theEndX - theStartX;
	double dyd = theEndY - theStartY;
	if (dx < 0)
	{
		dx = -dx;
		aXinc = -1;
		dxd = -dxd;
	}

	if (mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 32)
	{
		ulong* aBits = (ulong*)mLockedSurfaceDesc.lpSurface;
#ifdef OPTIMIZE_SOFTWARE_DRAWING
		if (theColor.mAlpha != 255)
		{
			#define PIXEL_TYPE			ulong
			#define CALC_WEIGHT_A(w)	(((w) * (theColor.mAlpha+1)) >> 8)
			#define BLEND_PIXEL(p)		\
						*(p) =			\
								((((color & 0xFF00FF) * a + (dest & 0xFF00FF) * oma) >> 8) & 0xFF00FF) |\
								((((color & 0x00FF00) * a + (dest & 0x00FF00) * oma) >> 8) & 0x00FF00);
			const int STRIDE = mLockedSurfaceDesc.lPitch / sizeof(PIXEL_TYPE);

			#include "GENERIC_DrawLineAA.inc"

			#undef PIXEL_TYPE
			#undef CALC_WEIGHT_A
			#undef BLEND_PIXEL
		}
		else
		{
			#define PIXEL_TYPE			ulong
			#define CALC_WEIGHT_A(w)	(w)
			#define BLEND_PIXEL(p)		\
						*(p) =			\
								((((color & 0xFF00FF) * a + (dest & 0xFF00FF) * oma) >> 8) & 0xFF00FF) |\
								((((color & 0x00FF00) * a + (dest & 0x00FF00) * oma) >> 8) & 0x00FF00);
			const int STRIDE = mLockedSurfaceDesc.lPitch / sizeof(PIXEL_TYPE);

			#include "GENERIC_DrawLineAA.inc"

			#undef PIXEL_TYPE
			#undef CALC_WEIGHT_A
			#undef BLEND_PIXEL
		}
#else
		if (theColor.mAlpha != 255)
		{
			#define PIXEL_TYPE			ulong
			#define CALC_WEIGHT_A(w)	(((w) * (theColor.mAlpha+1)) >> 8)
			#define BLEND_PIXEL(p)		\
						*(p) =			\
								((((color & aRMask) * a + (dest & aRMask) * oma) >> 8) & aRMask) |\
								((((color & aGMask) * a + (dest & aGMask) * oma) >> 8) & aGMask) |\
								((((color & aBMask) * a + (dest & aBMask) * oma) >> 8) & aBMask);
			const int STRIDE = mLockedSurfaceDesc.lPitch / sizeof(PIXEL_TYPE);

			#include "GENERIC_DrawLineAA.inc"

			#undef PIXEL_TYPE
			#undef CALC_WEIGHT_A
			#undef BLEND_PIXEL
		}
		else
		{
			#define PIXEL_TYPE			ulong
			#define CALC_WEIGHT_A(w)	(w)
			#define BLEND_PIXEL(p)		\
						*(p) =			\
								((((color & aRMask) * a + (dest & aRMask) * oma) >> 8) & aRMask) |\
								((((color & aGMask) * a + (dest & aGMask) * oma) >> 8) & aGMask) |\
								((((color & aBMask) * a + (dest & aBMask) * oma) >> 8) & aBMask);
			const int STRIDE = mLockedSurfaceDesc.lPitch / sizeof(PIXEL_TYPE);

			#include "GENERIC_DrawLineAA.inc"

			#undef PIXEL_TYPE
			#undef CALC_WEIGHT_A
			#undef BLEND_PIXEL
		}
#endif
	}
	else if (mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 16)
	{
		ushort* aBits = (ushort*)mLockedSurfaceDesc.lpSurface;
#ifdef OPTIMIZE_SOFTWARE_DRAWING
		if (aGMask == 0x3E0) // 5-5-5
		{
			#define PIXEL_TYPE			ushort
			#define BLEND_PIXEL(p)		\
			{\
				a >>= 3;\
				oma >>= 3;\
				ulong _src = (((color | (color << 16)) & 0x3E07C1F) * a >> 5) & 0x3E07C1F;\
				ulong _dest = (((dest | (dest << 16)) & 0x3E07C1F) * oma >> 5) & 0x3E07C1F;\
				*(p) = (_src | (_src>>16)) + (_dest | (_dest>>16));\
			}
			const int STRIDE = mLockedSurfaceDesc.lPitch / sizeof(PIXEL_TYPE);
			if (theColor.mAlpha != 255)
			{
				#define CALC_WEIGHT_A(w)	(((w) * (theColor.mAlpha+1)) >> 8)
				#include "GENERIC_DrawLineAA.inc"
				#undef CALC_WEIGHT_A
			}
			else
			{
				#define CALC_WEIGHT_A(w)	(w)
				#include "GENERIC_DrawLineAA.inc"
				#undef CALC_WEIGHT_A
			}
			#undef PIXEL_TYPE
			#undef BLEND_PIXEL
		}
		else if (aGMask == 0x7E0) // 5-6-5
		{
			#define PIXEL_TYPE			ushort
			#define BLEND_PIXEL(p)		\
			{\
				a >>= 3;\
				oma >>= 3;\
				ulong _src = (((color | (color << 16)) & 0x7E0F81F) * a >> 5) & 0x7E0F81F;\
				ulong _dest = (((dest | (dest << 16)) & 0x7E0F81F) * oma >> 5) & 0x7E0F81F;\
				*(p) = (_src | (_src>>16)) + (_dest | (_dest>>16));\
			}
			const int STRIDE = mLockedSurfaceDesc.lPitch / sizeof(PIXEL_TYPE);
			if (theColor.mAlpha != 255)
			{
				#define CALC_WEIGHT_A(w)	(((w) * (theColor.mAlpha+1)) >> 8)
				#include "GENERIC_DrawLineAA.inc"
				#undef CALC_WEIGHT_A
			}
			else
			{
				#define CALC_WEIGHT_A(w)	(w)
				#include "GENERIC_DrawLineAA.inc"
				#undef CALC_WEIGHT_A
			}
			#undef PIXEL_TYPE
			#undef BLEND_PIXEL
		}
		else
		{
#endif
			if (theColor.mAlpha != 255)
			{
				#define PIXEL_TYPE			ushort
				#define CALC_WEIGHT_A(w)	(((w) * (theColor.mAlpha+1)) >> 8)
				#define BLEND_PIXEL(p)		\
							*(p) =			\
									((((color & aRMask) * a + (dest & aRMask) * oma) >> 8) & aRMask) |\
									((((color & aGMask) * a + (dest & aGMask) * oma) >> 8) & aGMask) |\
									((((color & aBMask) * a + (dest & aBMask) * oma) >> 8) & aBMask);
				const int STRIDE = mLockedSurfaceDesc.lPitch / sizeof(PIXEL_TYPE);

				#include "GENERIC_DrawLineAA.inc"

				#undef PIXEL_TYPE
				#undef CALC_WEIGHT_A
				#undef BLEND_PIXEL
			}
			else
			{
				#define PIXEL_TYPE			ushort
				#define CALC_WEIGHT_A(w)	(w)
				#define BLEND_PIXEL(p)		\
							*(p) =			\
									((((color & aRMask) * a + (dest & aRMask) * oma) >> 8) & aRMask) |\
									((((color & aGMask) * a + (dest & aGMask) * oma) >> 8) & aGMask) |\
									((((color & aBMask) * a + (dest & aBMask) * oma) >> 8) & aBMask);
				const int STRIDE = mLockedSurfaceDesc.lPitch / sizeof(PIXEL_TYPE);

				#include "GENERIC_DrawLineAA.inc"

				#undef PIXEL_TYPE
				#undef CALC_WEIGHT_A
				#undef BLEND_PIXEL
			}
#ifdef OPTIMIZE_SOFTWARE_DRAWING
		}
#endif
	}

	UnlockSurface();
}

void DeviceImage::AdditiveDrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor) //Nothing | 1554-1557
{


}

void DeviceImage::CommitBits() //1562-1568
{
	if (mSurface == NULL)
	{
		MemoryImage::CommitBits();
		return;
	}
}

void DeviceImage::Create(int theWidth, int theHeight) //1573-1582
{
	delete [] mBits;

	mWidth = theWidth;
	mHeight = theHeight;

	mBits = NULL;

	BitsChanged();	
}

void DeviceImage::BitsChanged() //1587-1592
{
	MemoryImage::BitsChanged();

	if (mSurface != NULL)
		delete mSurface;
	mSurface = NULL;
}

ulong* DeviceImage::GetBits() //TODO: Changed | 1597-1610
{
	if (mBits == NULL)
	{
		if (mSurface == NULL)
			return MemoryImage::GetBits();

		if (mNoLock)
			return NULL;
		
		mBits = mSurface->GetBits(this);	
	}

	return mBits;
}

void DeviceImage::NormalFillRect(const Rect& theRect, const Color& theColor) //Prob correct | 1615-1730
{
	if (mNoLock)
		return;

	if (!LockSurface())
		return;	

	ulong aRMask = mLockedSurfaceDesc.ddpfPixelFormat.dwRBitMask;
	ulong aGMask = mLockedSurfaceDesc.ddpfPixelFormat.dwGBitMask;
	ulong aBMask = mLockedSurfaceDesc.ddpfPixelFormat.dwBBitMask;

	ulong aRRoundAdd = aRMask;
	ulong aGRoundAdd = aGMask;
	ulong aBRoundAdd = aBMask;
	
	if (mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 16)
	{
		if (theColor.mAlpha == 255)
		{
			ushort aColor = 
				((((theColor.mRed * aRMask) + aRRoundAdd) >> 8) & aRMask) |
				((((theColor.mGreen * aGMask) + aGRoundAdd) >> 8) & aGMask) |
				((((theColor.mBlue * aBMask) + aBRoundAdd) >> 8) & aBMask);

			ushort* aDestPixelsRow = ((ushort*) mLockedSurfaceDesc.lpSurface) + (theRect.mY * mLockedSurfaceDesc.lPitch/2) + theRect.mX;
			
			for (int y = 0; y < theRect.mHeight; y++)
			{
				ushort* aDestPixels = aDestPixelsRow;			

				for (int x = 0; x < theRect.mWidth; x++)			
					*(aDestPixels++) = aColor;

				aDestPixelsRow += mLockedSurfaceDesc.lPitch/2;			
			}
		}
		else
		{
			ushort src = 
				((((((theColor.mRed * theColor.mAlpha + 0x80) >> 8) * aRMask) + aRRoundAdd) >> 8) & aRMask) +
				((((((theColor.mGreen * theColor.mAlpha + 0x80) >> 8) * aGMask) + aGRoundAdd) >> 8) & aGMask) +
				((((((theColor.mBlue * theColor.mAlpha + 0x80) >> 8) * aBMask) + aBRoundAdd) >> 8) & aBMask);
			int oma = 256 - theColor.mAlpha;

			ushort* aDestPixelsRow = ((ushort*) mLockedSurfaceDesc.lpSurface) + (theRect.mY * mLockedSurfaceDesc.lPitch/2) + theRect.mX;
					
			for (int y = 0; y < theRect.mHeight; y++)
			{
				ushort* aDestPixels = aDestPixelsRow;			

				for (int x = 0; x < theRect.mWidth; x++)			
				{
					ushort dest = *aDestPixels;

					*(aDestPixels++) = src + 
						(((((dest & aRMask) * oma) + aRRoundAdd) >> 8) & aRMask) +
						(((((dest & aGMask) * oma) + aGRoundAdd) >> 8) & aGMask) +
						(((((dest & aBMask) * oma) + aBRoundAdd) >> 8) & aBMask);
				}

				aDestPixelsRow += mLockedSurfaceDesc.lPitch/2;			
			}
		}
	}
	else if (mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 32)
	{
		if (theColor.mAlpha == 255)
		{
			ulong aColor = 
				((((theColor.mRed * aRMask)) >> 8) & aRMask) |
				((((theColor.mGreen * aGMask)) >> 8) & aGMask) |
				((((theColor.mBlue * aBMask)) >> 8) & aBMask);

			ulong* aDestPixelsRow = ((ulong*) mLockedSurfaceDesc.lpSurface) + (theRect.mY * mLockedSurfaceDesc.lPitch/4) + theRect.mX;
			
			for (int y = 0; y < theRect.mHeight; y++)
			{
				ulong* aDestPixels = aDestPixelsRow;			

				for (int x = 0; x < theRect.mWidth; x++)			
					*(aDestPixels++) = aColor;

				aDestPixelsRow += mLockedSurfaceDesc.lPitch/4;
			}
		}
		else
		{
			ulong src = 
				((((((theColor.mRed * theColor.mAlpha + 0x7F) >> 8) * aRMask) + aRRoundAdd) >> 8) & aRMask) +
				((((((theColor.mGreen * theColor.mAlpha + 0x7F) >> 8) * aGMask) + aGRoundAdd) >> 8) & aGMask) +
				((((((theColor.mBlue * theColor.mAlpha + 0x7F) >> 8) * aBMask) + aBRoundAdd) >> 8) & aBMask);
			int oma = 256 - theColor.mAlpha;

			ulong* aDestPixelsRow = ((ulong*) mLockedSurfaceDesc.lpSurface) + (theRect.mY * mLockedSurfaceDesc.lPitch/4) + theRect.mX;
					
			for (int y = 0; y < theRect.mHeight; y++)
			{
				ulong* aDestPixels = aDestPixelsRow;			

				for (int x = 0; x < theRect.mWidth; x++)			
				{
					ulong dest = *aDestPixels;

					*(aDestPixels++) = src + 
						(((((dest & aRMask) * oma)) >> 8) & aRMask) +
						(((((dest & aGMask) * oma)) >> 8) & aGMask) +
						(((((dest & aBMask) * oma)) >> 8) & aBMask);
				}

				aDestPixelsRow += mLockedSurfaceDesc.lPitch/4;
			}
		}
	}

	UnlockSurface();
}

void DeviceImage::AdditiveFillRect(const Rect& theRect, const Color& theColor) //1735-1818
{
	if (mNoLock)
		return;

	if (!LockSurface())
		return;	

	ulong aRMask = mLockedSurfaceDesc.ddpfPixelFormat.dwRBitMask;
	ulong aGMask = mLockedSurfaceDesc.ddpfPixelFormat.dwGBitMask;
	ulong aBMask = mLockedSurfaceDesc.ddpfPixelFormat.dwBBitMask;

	ulong aRRoundAdd = aRMask >> 1;
	ulong aGRoundAdd = aGMask >> 1;
	ulong aBRoundAdd = aBMask >> 1;
	
	int aRedShift = mDriver->GetNativeDisplayInfo()->mRedShift;
	int aGreenShift = mDriver->GetNativeDisplayInfo()->mGreenShift;
	int aBlueShift = mDriver->GetNativeDisplayInfo()->mBlueShift;

	int* aMaxRedTable = mDriver->GetNativeDisplayInfo()->mRedAddTable;
	int* aMaxGreenTable = mDriver->GetNativeDisplayInfo()->mGreenAddTable;
	int* aMaxBlueTable = mDriver->GetNativeDisplayInfo()->mBlueAddTable;	

	if (mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 16)
	{
		ushort rc = ((theColor.mRed * theColor.mAlpha) / 255) >> (8-mDriver->GetNativeDisplayInfo()->mRedBits);
		ushort gc = ((theColor.mGreen * theColor.mAlpha) / 255) >> (8-mDriver->GetNativeDisplayInfo()->mGreenBits);
		ushort bc = ((theColor.mBlue * theColor.mAlpha) / 255) >> (8-mDriver->GetNativeDisplayInfo()->mBlueBits);

		ushort* aDestPixelsRow = ((ushort*) mLockedSurfaceDesc.lpSurface) + (theRect.mY * mLockedSurfaceDesc.lPitch/2) + theRect.mX;
				
		for (int y = 0; y < theRect.mHeight; y++)
		{
			ushort* aDestPixels = aDestPixelsRow;			

			for (int x = 0; x < theRect.mWidth; x++)			
			{
				ushort dest = *aDestPixels;

				int r = aMaxRedTable[((dest & aRMask) >> aRedShift) + rc];
				int g = aMaxGreenTable[((dest & aGMask) >> aGreenShift) + gc];
				int b = aMaxBlueTable[((dest & aBMask) >> aBlueShift) + bc];				

				*(aDestPixels++) = 
					(r << aRedShift) |
					(g << aGreenShift) |
					(b << aBlueShift);
			}				

			aDestPixelsRow += mLockedSurfaceDesc.lPitch/2;			
		}
	}
	else if (mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 32)
	{
		ulong rc = ((theColor.mRed * theColor.mAlpha) / 255) >> (8-mDriver->GetNativeDisplayInfo()->mRedBits);
		ulong gc = ((theColor.mGreen * theColor.mAlpha) / 255) >> (8-mDriver->GetNativeDisplayInfo()->mGreenBits);
		ulong bc = ((theColor.mBlue * theColor.mAlpha) / 255) >> (8-mDriver->GetNativeDisplayInfo()->mBlueBits);

		ulong* aDestPixelsRow = ((ulong*) mLockedSurfaceDesc.lpSurface) + (theRect.mY * mLockedSurfaceDesc.lPitch/4) + theRect.mX;
		
		for (int y = 0; y < theRect.mHeight; y++)
		{
			ulong* aDestPixels = aDestPixelsRow;			

			for (int x = 0; x < theRect.mWidth; x++)			
			{
				ulong dest = *aDestPixels;

				int r = aMaxRedTable[((dest & aRMask) >> aRedShift) + rc];
				int g = aMaxGreenTable[((dest & aGMask) >> aGreenShift) + gc];
				int b = aMaxBlueTable[((dest & aBMask) >> aBlueShift) + bc];				

				*(aDestPixels++) = 
					(r << aRedShift) |
					(g << aGreenShift) |
					(b << aBlueShift);
			}				

			aDestPixelsRow += mLockedSurfaceDesc.lPitch/4;
		}
	}

	UnlockSurface();
}

void DeviceImage::NormalBlt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor) //TODO changed | 1823-1980
{
	theImage->mDrawn = true;
	MemoryImage* aMemoryImage;
	
	if (theImage)
		aMemoryImage = theImage->AsMemoryImage();
	else
		aMemoryImage = NULL;

	DeviceImage* aDDImage = dynamic_cast<DeviceImage*>(theImage);	

	if (aMemoryImage != NULL)
	{
		aMemoryImage->CommitBits();		

		RECT aDestRect = {theX, theY, theX + theSrcRect.mWidth, theY + theSrcRect.mHeight};
		RECT aSrcRect = {theSrcRect.mX, theSrcRect.mY, theSrcRect.mX + theSrcRect.mWidth, theSrcRect.mY + theSrcRect.mHeight};	

		//TODO:
		if ((aMemoryImage->mIsVolatile) && ((aDDImage == NULL) || (aDDImage->mSurface == NULL)) && 
			(!mNoLock) && (theColor == Color::White))
		{
			if (aMemoryImage->mColorTable == NULL)
			{
				ulong* aSrcBits = aMemoryImage->GetBits();			

				#define SRC_TYPE ulong
				#define NEXT_SRC_COLOR (*(aSrcPixels++))
				#include "DDI_NormalBlt_Volatile.inc"
				#undef	SRC_TYPE
				#undef NEXT_SRC_COLOR
			}
			else
			{			
				ulong* aColorTable = aMemoryImage->mColorTable;
				uchar* aSrcBits = aMemoryImage->mColorIndices;

				#define SRC_TYPE uchar
				#define NEXT_SRC_COLOR (aColorTable[*(aSrcPixels++)])
				
				#include "DDI_NormalBlt_Volatile.inc"

				#undef SRC_TYPE
				#undef NEXT_SRC_COLOR
			}
		}
		else if ((aMemoryImage->mHasAlpha) || (theColor != Color::White))
		{
			if (mNoLock)
				return;			

 			if (!LockSurface())
				return;

			// Ensure NativeAlphaData is calculated
			void *aNativeData = aMemoryImage->GetNativeAlphaData(mDriver->GetNativeDisplayInfo());

			// Ensure RunLength data is calculated
			uchar* aSrcRLAlphaData = aMemoryImage->GetRLAlphaData();

			#define _PLUSPLUS ++
			#define _PLUSEQUALS +=
			if (aMemoryImage->mColorTable == NULL)
			{
				ulong* aSrcPixelsRow = ((ulong*) aNativeData) + (theSrcRect.mY * theImage->mWidth) + theSrcRect.mX;
				ulong* aSrcPixels;
				
				#define NEXT_SRC_COLOR (*(aSrcPixels++))
				#define PEEK_SRC_COLOR (*aSrcPixels)

				#include "DDI_AlphaBlt.inc"

				#undef NEXT_SRC_COLOR
				#undef PEEK_SRC_COLOR
			}
			else
			{
				ulong* aNativeColorTable = (ulong*) aNativeData;

				uchar* aSrcPixelsRow = aMemoryImage->mColorIndices + (theSrcRect.mY * theImage->mWidth) + theSrcRect.mX;
				uchar* aSrcPixels;

				#define NEXT_SRC_COLOR (aNativeColorTable[*(aSrcPixels++)])
				#define PEEK_SRC_COLOR (aNativeColorTable[*aSrcPixels])

				#include "DDI_AlphaBlt.inc"

				#undef NEXT_SRC_COLOR
				#undef PEEK_SRC_COLOR
			}		

			#undef _PLUSPLUS
			#undef _PLUSEQUALS 
			UnlockSurface();
		}		
		else if ((aDDImage == NULL) || (aDDImage->mSurface == NULL) || ((!mVideoMemory) && (aDDImage->mVideoMemory)))
		{
			if (mNoLock)
				return;

			//TODO: Have some sort of cool thing here
			LPDIRECTDRAWSURFACE aSurface = GetSurface();

			if (!LockSurface())
				return;

			void* aNativeAlphaData = aMemoryImage->GetNativeAlphaData(mDriver->GetNativeDisplayInfo());

			if (aMemoryImage->mColorTable == NULL)
			{
				ulong* aSrcPixelsRow = ((ulong*) aNativeAlphaData) + (theSrcRect.mY * theImage->mWidth) + theSrcRect.mX;
				ulong* aSrcPixels;
				
				#define NEXT_SRC_COLOR (*(aSrcPixels++))
				#define PEEK_SRC_COLOR (*aSrcPixels)

				#include "DDI_FastBlt_NoAlpha.inc"

				#undef NEXT_SRC_COLOR
				#undef PEEK_SRC_COLOR
			}
			else
			{
				ulong* aNativeAlphaColorTable = (ulong*) aNativeAlphaData;

				uchar* aSrcPixelsRow = aMemoryImage->mColorIndices + (theSrcRect.mY * theImage->mWidth) + theSrcRect.mX;
				uchar* aSrcPixels;

				#define NEXT_SRC_COLOR (aNativeAlphaColorTable[*(aSrcPixels++)])
				#define PEEK_SRC_COLOR (aNativeAlphaColorTable[*aSrcPixels])

				#include "DDI_FastBlt_NoAlpha.inc"

				#undef NEXT_SRC_COLOR
				#undef PEEK_SRC_COLOR
			}		
		}
		else
		{
			if (mLockCount > 0)
				mSurface->Unlock(NULL);

			DDBLTFX aBltFX;
			ZeroMemory(&aBltFX, sizeof(aBltFX));
			aBltFX.dwSize = sizeof(aBltFX);
    
			DWORD aFlags = DDBLT_WAIT;

			if (aDDImage->mHasTrans)
				aFlags |= DDBLT_KEYSRC;

			HRESULT aResult = GetSurface()->Blt(&aDestRect, aDDImage->GetSurface(), &aSrcRect, aFlags, &aBltFX);
		
			if (mLockCount > 0)
			{
				if (mSurface->Lock(NULL, mLockedSurfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL) != DD_OK)
					return;
			}
		}
	}
}

void DeviceImage::NormalBltMirror(Image* theImage, int theX, int theY, const Rect& theSrcRectOrig, const Color& theColor) //TODO changed? | 1985-2046
{
	theImage->mDrawn = true;
	MemoryImage* aMemoryImage;

	Rect theSrcRect = theSrcRectOrig;
//	theSrcRect.mX = (theSrcRect.mX+theSrcRect.mWidth)*-1 + theImage->mWidth;
	theX += theSrcRect.mWidth-1;
	
	if (theImage)
		aMemoryImage = theImage->AsMemoryImage();
	else
		aMemoryImage = NULL;

	DeviceImage* aDDImage = dynamic_cast<DeviceImage*>(theImage);	

	if (aMemoryImage != NULL)
	{
		aMemoryImage->CommitBits();		

		if (mNoLock)
			return;

 		if (!LockSurface())
			return;

		// Ensure NativeAlphaData is calculated
		void *aNativeData = aMemoryImage->GetNativeAlphaData(mDriver->GetNativeDisplayInfo());

		// Ensure RunLength data is calculated
		uchar* aSrcRLAlphaData = aMemoryImage->GetRLAlphaData();

		#define _PLUSPLUS --
		#define _PLUSEQUALS -=
		if (aMemoryImage->mColorTable == NULL)
		{
			ulong* aSrcPixelsRow = ((ulong*) aNativeData) + (theSrcRect.mY * theImage->mWidth) + theSrcRect.mX;
			ulong* aSrcPixels;
			
			#define NEXT_SRC_COLOR (*(aSrcPixels++))
			#define PEEK_SRC_COLOR (*aSrcPixels)

			#include "DDI_AlphaBlt.inc"

			#undef NEXT_SRC_COLOR
			#undef PEEK_SRC_COLOR
		}
		else
		{
			ulong* aNativeColorTable = (ulong*) aNativeData;

			uchar* aSrcPixelsRow = aMemoryImage->mColorIndices + (theSrcRect.mY * theImage->mWidth) + theSrcRect.mX;
			uchar* aSrcPixels;

			#define NEXT_SRC_COLOR (aNativeColorTable[*(aSrcPixels++)])
			#define PEEK_SRC_COLOR (aNativeColorTable[*aSrcPixels])

			#include "DDI_AlphaBlt.inc"

			#undef NEXT_SRC_COLOR
			#undef PEEK_SRC_COLOR
		}		

		#undef _PLUSPLUS
		#undef _PLUSEQUALS 
		UnlockSurface();
	}
}

void DeviceImage::AdditiveBlt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor) //Todo changed? | 2052-2104
{
	theImage->mDrawn = true;
	MemoryImage* aMemoryImage;

	if (mNoLock)
		return;
	
	if (theImage)
		aMemoryImage = theImage->AsMemoryImage();
	else
		aMemoryImage = NULL;

	DeviceImage* aDDImage = dynamic_cast<DeviceImage*>(theImage);

	if (aMemoryImage != NULL)
	{
		if (!LockSurface())
			return;

		// Ensure NativeAlphaData is calculated
		void* aNativeAlphaData = aMemoryImage->GetNativeAlphaData(mDriver->GetNativeDisplayInfo());

		#define _PLUSPLUS ++
		#define _PLUSEQUALS +=
		if (aMemoryImage->mColorTable == NULL)
		{
			ulong* aSrcPixelsRow = ((ulong*) aNativeAlphaData) + (theSrcRect.mY * theImage->mWidth) + theSrcRect.mX;
			ulong* aSrcPixels;
			
			#define NEXT_SRC_COLOR (*(aSrcPixels++))
			#define PEEK_SRC_COLOR (*aSrcPixels)

			#include "DDI_Additive.inc"

			#undef NEXT_SRC_COLOR
			#undef PEEK_SRC_COLOR
		}
		else
		{
			ulong* aNativeAlphaColorTable = (ulong*) aNativeAlphaData;

			uchar* aSrcPixelsRow = aMemoryImage->mColorIndices + (theSrcRect.mY * theImage->mWidth) + theSrcRect.mX;
			uchar* aSrcPixels;

			#define NEXT_SRC_COLOR (aNativeAlphaColorTable[*(aSrcPixels++)])
			#define PEEK_SRC_COLOR (aNativeAlphaColorTable[*aSrcPixels])

			#include "DDI_Additive.inc"

			#undef NEXT_SRC_COLOR
			#undef PEEK_SRC_COLOR
		}
		#undef _PLUSPLUS
		#undef _PLUSEQUALS

		UnlockSurface();
		UnlockSurface(); //Happens twice?
	}
}

void DeviceImage::AdditiveBltMirror(Image* theImage, int theX, int theY, const Rect& theSrcRectOrig, const Color& theColor) //TODO changed | 2109-2165
{
	theImage->mDrawn = true;
	MemoryImage* aMemoryImage;

	if (mNoLock)
		return;
	
	if (theImage)
	{
		aMemoryImage = theImage->AsMemoryImage();
	}
	else
	{
		aMemoryImage = NULL;
	}
	
	Rect theSrcRect = theSrcRectOrig;
//	theSrcRect.mX = (theSrcRect.mX+theSrcRect.mWidth)*-1 + theImage->mWidth;
	theX += theSrcRect.mWidth-1;

	MemoryImage* aMemoryImage = dynamic_cast<MemoryImage*>(theImage);
	DeviceImage* aDDImage = dynamic_cast<DeviceImage*>(theImage);

	if (aMemoryImage != NULL)
	{
		if (!LockSurface())
			return;

		// Ensure NativeAlphaData is calculated
		void* aNativeAlphaData = aMemoryImage->GetNativeAlphaData(mDriver->GetNativeDisplayInfo());

		#define _PLUSPLUS --
		#define _PLUSEQUALS -=
		if (aMemoryImage->mColorTable == NULL)
		{
			ulong* aSrcPixelsRow = ((ulong*) aNativeAlphaData) + (theSrcRect.mY * theImage->mWidth) + theSrcRect.mX;
			ulong* aSrcPixels;
			
			#define NEXT_SRC_COLOR (*(aSrcPixels++))
			#define PEEK_SRC_COLOR (*aSrcPixels)

			#include "DDI_Additive.inc"

			#undef NEXT_SRC_COLOR
			#undef PEEK_SRC_COLOR
		}
		else
		{
			ulong* aNativeAlphaColorTable = (ulong*) aNativeAlphaData;

			uchar* aSrcPixelsRow = aMemoryImage->mColorIndices + (theSrcRect.mY * theImage->mWidth) + theSrcRect.mX;
			uchar* aSrcPixels;

			#define NEXT_SRC_COLOR (aNativeAlphaColorTable[*(aSrcPixels++)])
			#define PEEK_SRC_COLOR (aNativeAlphaColorTable[*aSrcPixels])

			#include "DDI_Additive.inc"

			#undef NEXT_SRC_COLOR
			#undef PEEK_SRC_COLOR
		}
		#undef _PLUSPLUS
		#undef _PLUSEQUALS

		UnlockSurface();
	}
}

void DeviceImage::Blt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode) //TODO changed | 2170-2203
{
	theImage->mDrawn = true;

	//if (gDebug)
	//	mApp->CopyToClipboard("+DDImage::Blt");	

	DBG_ASSERTE((theColor.mRed >= 0) && (theColor.mRed <= 255)); //2176
	DBG_ASSERTE((theColor.mGreen >= 0) && (theColor.mGreen <= 255)); //2177
	DBG_ASSERTE((theColor.mBlue >= 0) && (theColor.mBlue <= 255)); //2178
	DBG_ASSERTE((theColor.mAlpha >= 0) && (theColor.mAlpha <= 255)); //2179
	

	//Check3D is deprecated
	
	if ((mDrawToBits) || (mHasAlpha) || ((mHasTrans) && (!mFirstPixelTrans))) //?
	{
		MemoryImage::Blt(theImage, theX, theY, theSrcRect, theColor, theDrawMode);
		return;
	}

	switch (theDrawMode)
	{
	case Graphics::DRAWMODE_NORMAL:		
		NormalBlt(theImage, theX, theY, theSrcRect, theColor);
		break;
	case Graphics::DRAWMODE_ADDITIVE:
		AdditiveBlt(theImage, theX, theY, theSrcRect, theColor);
		break;
	}

	DeleteAllNonSurfaceData();
}

void DeviceImage::BltMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode) //2208-2209
{
	DBG_ASSERTE((theColor.mRed >= 0) && (theColor.mRed <= 255)); //2209
	DBG_ASSERTE((theColor.mGreen >= 0) && (theColor.mGreen <= 255)); //2210
	DBG_ASSERTE((theColor.mBlue >= 0) && (theColor.mBlue <= 255)); //2211
	DBG_ASSERTE((theColor.mAlpha >= 0) && (theColor.mAlpha <= 255)); //2212

	//No more Check3D
	
	if ((mDrawToBits) || (mHasAlpha) || ((mHasTrans) && (!mFirstPixelTrans))) //?
	{
		MemoryImage::Blt(theImage, theX, theY, theSrcRect, theColor, theDrawMode);
		return;
	}

	switch (theDrawMode)
	{
	case Graphics::DRAWMODE_NORMAL:		
		NormalBltMirror(theImage, theX, theY, theSrcRect, theColor);
		break;
	case Graphics::DRAWMODE_ADDITIVE:
		AdditiveBltMirror(theImage, theX, theY, theSrcRect, theColor);
		break;
	}

	DeleteAllNonSurfaceData();
}

void DeviceImage::BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect &theClipRect, const Color& theColor, int theDrawMode) //Changed? | 2232-2238
{
	theImage->mDrawn = true;
	BltRotated(theImage,theX,theY,theSrcRect,theClipRect,theColor,theDrawMode,0,0,0);
}

void DeviceImage::BltRotated(Image* theImage, float theX, float theY, const Rect &theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY) //TODO changed | 2243-2330
{
	theImage->mDrawn = true;

	if (mNoLock)
		return;	

	if ((mDrawToBits) || (mHasAlpha) || ((mHasTrans) && (!mFirstPixelTrans)))
	{
		MemoryImage::BltRotated(theImage, theX, theY, theSrcRect, theClipRect, theColor, theDrawMode, theRot, theRotCenterX, theRotCenterY);
		return;
	}	

	// This BltRotatedClipHelper clipping used to happen in Graphics::DrawImageRotated
	FRect aDestRect;
	if (!BltRotatedClipHelper(theX, theY, theSrcRect, theClipRect, theRot, aDestRect,theRotCenterX,theRotCenterY))
		return;

	MemoryImage* aMemoryImage = dynamic_cast<MemoryImage*>(theImage);
	DeviceImage* aDDImage = dynamic_cast<DeviceImage*>(theImage);

	if (aMemoryImage != NULL)
	{
		aMemoryImage->CommitBits();

		if (theDrawMode == Graphics::DRAWMODE_NORMAL)
		{
			if (aMemoryImage->mColorTable == NULL)
			{
				ulong* aSrcBits = aMemoryImage->GetBits() + theSrcRect.mX + theSrcRect.mY*aMemoryImage->GetWidth();			

				#define SRC_TYPE ulong
				#define READ_COLOR(ptr) (*(ptr))

				#include "DDI_BltRotated.inc" //Lines 2-34

				#undef SRC_TYPE
				#undef READ_COLOR

			}
			else
			{			
				ulong* aColorTable = aMemoryImage->mColorTable;
				uchar* aSrcBits = aMemoryImage->mColorIndices + theSrcRect.mX + theSrcRect.mY*aMemoryImage->GetWidth();

				#define SRC_TYPE uchar
				#define READ_COLOR(ptr) (aColorTable[*(ptr)])

				#include "DDI_BltRotated.inc"

				#undef SRC_TYPE
				#undef READ_COLOR
			}
		}
		else
		{
			if (aMemoryImage->mColorTable == NULL)
			{
				ulong* aSrcBits = aMemoryImage->GetBits() + theSrcRect.mX + theSrcRect.mY*aMemoryImage->GetWidth();			

				#define SRC_TYPE ulong
				#define READ_COLOR(ptr) (*(ptr))

				#include "DDI_BltRotated_Additive.inc"

				#undef SRC_TYPE
				#undef READ_COLOR

			}
			else
			{			
				ulong* aColorTable = aMemoryImage->mColorTable;
				uchar* aSrcBits = aMemoryImage->mColorIndices + theSrcRect.mX + theSrcRect.mY*aMemoryImage->GetWidth();

				#define SRC_TYPE uchar
				#define READ_COLOR(ptr) (aColorTable[*(ptr)])

				#include "DDI_BltRotated_Additive.inc"

				#undef SRC_TYPE
				#undef READ_COLOR
			}
		}
	}

	DeleteAllNonSurfaceData();
}

void DeviceImage::BltStretched(Image* theImage, const Rect& theDestRectOrig, const Rect& theSrcRectOrig, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch, bool mirror) //Todo changed | 2335-2555
{
	theImage->mDrawn = true;

	DeviceImage* aSrcDDImage = dynamic_cast<DeviceImage*>(theImage);
	MemoryImage* aSrcMemoryImage;

	if (theImage)
		aSrcMemoryImage = theImage->AsMemoryImage();
	else
		aSrcMemoryImage = NULL;

	Rect theDestRect;
	FRect theSrcRect;
	if (mirror)
	{
		if (!StretchBltMirrorClipHelper(theSrcRectOrig,theClipRect,theDestRectOrig,theSrcRect,theDestRect))
			return;
	}

	if (fastStretch)
	{
		if ((aSrcDDImage != NULL) && (theColor == Color::White) && (theDrawMode == Graphics::DRAWMODE_NORMAL) && 
			(!aSrcDDImage->mHasAlpha) && (aSrcDDImage->GetSurface() != NULL))
		{
			LPDIRECTDRAWSURFACE aSrcSurface = aSrcDDImage->GetSurface();
			LPDIRECTDRAWSURFACE aDestSurface = GetSurface();

			DDBLTFX aBltFX;
			ZeroMemory(&aBltFX, sizeof(aBltFX));
			aBltFX.dwSize = sizeof(aBltFX);
    
			DWORD aFlags = DDBLT_WAIT;

			if (aSrcDDImage->mHasTrans)
				aFlags |= DDBLT_KEYSRC;

			RECT aDestRect = {theDestRect.mX, theDestRect.mY, theDestRect.mX + theDestRect.mWidth, theDestRect.mY + theDestRect.mHeight};
			RECT aSrcRect = {theSrcRect.mX, theSrcRect.mY, theSrcRect.mX + theSrcRect.mWidth, theSrcRect.mY + theSrcRect.mHeight};	
			
			HRESULT aResult = GetSurface()->Blt(&aDestRect, aSrcDDImage->GetSurface(), &aSrcRect, aFlags, &aBltFX);	
		}
		else
		{
			if (aSrcMemoryImage != NULL)
			{
				aSrcMemoryImage->CommitBits();

			// Ensure NativeAlphaData is calculated
				void *aNativeAlphaData = aSrcMemoryImage->GetNativeAlphaData(mDDInterface);

				#define _PLUSPLUS ++
				if (theDrawMode == Graphics::DRAWMODE_NORMAL)
				{
					if (aSrcMemoryImage->mColorTable == NULL)
					{
						//ulong* aSrcBits = aSrcMemoryImage->GetBits();		
						ulong* aSrcBits = ((ulong*) aNativeAlphaData);	

						#define SRC_TYPE ulong
						#define READ_COLOR(ptr) (*(ptr))

						#include "DDI_FastStretch.inc"

						#undef SRC_TYPE
						#undef READ_COLOR
					}
					else
					{
						ulong* aColorTable = (ulong*) aNativeAlphaData;
						uchar* aSrcBits = aSrcMemoryImage->mColorIndices;

						#define SRC_TYPE uchar
						#define READ_COLOR(ptr) (aColorTable[*(ptr)])

						#include "DDI_FastStretch.inc"

						#undef SRC_TYPE
						#undef READ_COLOR
					}
				}
				else
				{
					if (aSrcMemoryImage->mColorTable == NULL)
					{
						//ulong* aSrcBits = aSrcMemoryImage->GetBits();		
						ulong* aSrcBits = ((ulong*) aNativeAlphaData);	

						#define SRC_TYPE ulong
						#define READ_COLOR(ptr) (*(ptr))

						#include "DDI_FastStretch_Additive.inc"

						#undef SRC_TYPE
						#undef READ_COLOR
					}
					else
					{
						ulong* aColorTable = (ulong*) aNativeAlphaData;
						uchar* aSrcBits = aSrcMemoryImage->mColorIndices;

						#define SRC_TYPE uchar
						#define READ_COLOR(ptr) (aColorTable[*(ptr)])

						#include "DDI_FastStretch_Additive.inc"

						#undef SRC_TYPE
						#undef READ_COLOR
					}
				}
				#undef _PLUSPLUS
			}
		}
	}
	else
	{
		if ((mDrawToBits) || (mHasAlpha) || (mHasTrans) || (mDDInterface->mIs3D))
		{
			MemoryImage::BltStretched(theImage, theDestRectOrig, theSrcRectOrig, theClipRect, theColor, theDrawMode, fastStretch, mirror);
			return;
		}

		// Stretch it to a temporary image
		MemoryImage aTempImage(mApp);
		Rect aTempRect(0, 0, theDestRect.mWidth, theDestRect.mHeight);

		aTempImage.Create(theDestRect.mWidth, theDestRect.mHeight);			
		if (fastStretch)
			aTempImage.FastStretchBlt(theImage, aTempRect, theSrcRect, theColor, 0);
		else
			aTempImage.SlowStretchBlt(theImage, aTempRect, theSrcRect, theColor, 0);

		Blt(&aTempImage, theDestRect.mX, theDestRect.mY, aTempRect, theColor, theDrawMode);
	}

	DeleteAllNonSurfaceData();
}

void DeviceImage::BltMatrix(Image* theImage, float x, float y, const SexyMatrix3 &theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect &theSrcRect, bool blend) //Changed? | 2560-2585
{
	theImage->mDrawn = true;

 	if (!LockSurface())
		return;

	int aPixelFormat;
	if (mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 32)
		aPixelFormat = 0x888;
	else if (mLockedSurfaceDesc.ddpfPixelFormat.dwRBitMask == 0xf800 && mLockedSurfaceDesc.ddpfPixelFormat.dwGBitMask == 0x07e0 && mLockedSurfaceDesc.ddpfPixelFormat.dwBBitMask == 0x001f)
		aPixelFormat = 0x565;
	else if (mLockedSurfaceDesc.ddpfPixelFormat.dwRBitMask == 0x7c00 && mLockedSurfaceDesc.ddpfPixelFormat.dwGBitMask == 0x03e0 && mLockedSurfaceDesc.ddpfPixelFormat.dwBBitMask == 0x001f)
		aPixelFormat = 0x555;
	else
		DBG_ASSERTE(false); //2577

	BltMatrixHelper(theImage,x,y,theMatrix,theClipRect,theColor,theDrawMode,theSrcRect,mLockedSurfaceDesc.lpSurface,mLockedSurfaceDesc.lPitch,aPixelFormat,blend);

	UnlockSurface();

	DeleteAllNonSurfaceData();

}

void DeviceImage::BltTriangles(Image* theTexture, const SexyVertex2D theVertices[][3], int theNumTriangles, const Color& theColor, int theDrawMode, float tx, float ty, bool blend, const Rect* theClipRect) //Renamed to BltTriangles, changed? | 2590-2610
{
	theTexture->mDrawn = true;

	int aPixelFormat;
	//?
	if (mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 32)
		aPixelFormat = 0x888;
	else if (mLockedSurfaceDesc.ddpfPixelFormat.dwRBitMask == 0xf800 && mLockedSurfaceDesc.ddpfPixelFormat.dwGBitMask == 0x07e0 && mLockedSurfaceDesc.ddpfPixelFormat.dwBBitMask == 0x001f)
		aPixelFormat = 0x565;
	else if (mLockedSurfaceDesc.ddpfPixelFormat.dwRBitMask == 0x7c00 && mLockedSurfaceDesc.ddpfPixelFormat.dwGBitMask == 0x03e0 && mLockedSurfaceDesc.ddpfPixelFormat.dwBBitMask == 0x001f)
		aPixelFormat = 0x555;
	else
		DBG_ASSERTE(false);

	BltTrianglesTexHelper(theTexture,theVertices,theNumTriangles,theClipRect,theColor,theDrawMode, mLockedSurfaceDesc.lpSurface, mLockedSurfaceDesc.lPitch,aPixelFormat,tx,ty,blend);
	UnlockSurface(); //2754

	DeleteAllNonSurfaceData();
}

bool DeviceImage::Palletize() //Uhh what | 2615-2630
{
	return false;
}

void DeviceImage::FillScanLinesWithCoverage(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode, const BYTE* theCoverage, int theCoverX, int theCoverY, int theCoverWidth, int theCoverHeight) //Changed? | 2635-2721
{
	if (theSpanCount == 0) return;
	
	if (!LockSurface())
		return;

	ulong aRMask = mLockedSurfaceDesc.ddpfPixelFormat.dwRBitMask;
	ulong aGMask = mLockedSurfaceDesc.ddpfPixelFormat.dwGBitMask;
	ulong aBMask = mLockedSurfaceDesc.ddpfPixelFormat.dwBBitMask;

	ulong aRRoundAdd = aRMask >> 1;
	ulong aGRoundAdd = aGMask >> 1;
	ulong aBRoundAdd = aBMask >> 1;
	
	if (mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 16)
	{
		//ushort src_red		= (((theColor.mRed * (theColor.mAlpha+1)) >> 8) * aRMask) & aRMask;
		//ushort src_green	= (((theColor.mGreen * (theColor.mAlpha+1)) >> 8) * aGMask) & aGMask;
		//ushort src_blue		= (((theColor.mBlue * (theColor.mAlpha+1)) >> 8) * aBMask) & aBMask;
		ushort src = 
			(((theColor.mRed * aRMask) >> 8) & aRMask) |
			(((theColor.mGreen * aGMask) >> 8) & aGMask) |
			(((theColor.mBlue * aBMask) >> 8) & aBMask);
		ushort* theBits = (ushort*)mLockedSurfaceDesc.lpSurface;
		
		for (int i = 0; i < theSpanCount; ++i)
		{
			Span* aSpan = &theSpans[i];
			int x = aSpan->mX - theCoverX;
			int y = aSpan->mY - theCoverY;

			ushort* aDestPixels = &theBits[aSpan->mY*mWidth + aSpan->mX];
			const BYTE* aCoverBits = &theCoverage[y*theCoverWidth+x];
			for (int w = 0; w < aSpan->mWidth; ++w)
			{
				int cover = *aCoverBits++;
				int a = ((cover+1) * theColor.mAlpha) >> 8;
				int oma = 256 - a;
				ushort dest = *aDestPixels;
				
				*(aDestPixels++) = 
					((((dest & aRMask) * oma + (src & aRMask) * a) >> 8) & aRMask) |
					((((dest & aGMask) * oma + (src & aGMask) * a) >> 8) & aGMask) |
					((((dest & aBMask) * oma + (src & aBMask) * a) >> 8) & aBMask);
			}
		}
	}
	else if (mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 32)
	{
		//ulong src_red		= (((theColor.mRed * (theColor.mAlpha+1)) >> 8) * aRMask) & aRMask;
		//ulong src_green		= (((theColor.mGreen * (theColor.mAlpha+1)) >> 8) * aGMask) & aGMask;
		//ulong src_blue		= (((theColor.mBlue * (theColor.mAlpha+1)) >> 8) * aBMask) & aBMask;
		ulong src = 
			(((theColor.mRed * aRMask) >> 8) & aRMask) |
			(((theColor.mGreen * aGMask) >> 8) & aGMask) |
			(((theColor.mBlue * aBMask) >> 8) & aBMask);
		ulong* theBits = (ulong*)mLockedSurfaceDesc.lpSurface;
		
		for (int i = 0; i < theSpanCount; ++i)
		{
			Span* aSpan = &theSpans[i];
			int x = aSpan->mX - theCoverX;
			int y = aSpan->mY - theCoverY;

			ulong* aDestPixels = &theBits[aSpan->mY*mWidth + aSpan->mX];
			const BYTE* aCoverBits = &theCoverage[y*theCoverWidth+x];
			for (int w = 0; w < aSpan->mWidth; ++w)
			{
				int cover = *aCoverBits++;
				int a = ((cover+1) * theColor.mAlpha) >> 8;
				int oma = 256 - a;
				ulong dest = *aDestPixels;
				
				*(aDestPixels++) = 
					((((dest & aRMask) * oma + (src & aRMask) * a) >> 8) & aRMask) |
					((((dest & aGMask) * oma + (src & aGMask) * a) >> 8) & aGMask) |
					((((dest & aBMask) * oma + (src & aBMask) * a) >> 8) & aBMask);
			}
		}
	}

	UnlockSurface();
	DeleteAllNonSurfaceData();
}

bool DeviceImage::CheckCache(const std::string& theSrcFile, const std::string& theAltData) //? | 2727-2729
{
	gSexyCache.CheckData(theSrcFile, "DDImage5" + theAltData);
}

bool DeviceImage::SetCacheUpToDate(const std::string& theSrcFile, const std::string& theAltData) //2732-2734
{
	gSexyCache.SetUpToDate(theSrcFile, "DDImage5:" + theAltData);
}

DeviceImage* DeviceImage::ReadFromCache(const std::string& theSrcFile, const std::string& theAltData) //TODO | 2737-2838
{
	HANDLE thePtr;
	int theSize;
	if (!gSexyCache.GetData(theSrcFile, "DDImage5:" + theAltData, &thePtr, &theSize))
		return 0;
	DeviceImage* anImage = new DeviceImage();
	void* p = thePtr;
	SMemR(&p, &anImage->mWidth, 4);
	SMemR(&p, &anImage->mHeight, 4);
	int aFlags;
	SMemR(&p, &aFlags, 4);
	anImage->ReplaceImageFlags(aFlags >> ImageFlag_RenderTarget);
	if ((aFlags & ImageFlag_MinimizeNumSubdivisions) != 0)
	{
		int aSize = anImage->mHeight * anImage->mWidth;
		anImage->mColorTable = new ulong[0x400];
		anImage->mColorIndices = new uchar[aSize];
		SMemR(&p, &anImage->mColorTable, sizeof anImage->mColorTable);
		SMemR(&p, &anImage->mColorIndices, aSize);
	}
	else
	{
		anImage->mBits = new ulong [4 * anImage->mHeight * anImage->mWidth + 1];
		anImage->mBits[(anImage->mHeight * anImage->mWidth + 1)] = MEMORYCHECK_ID;
		SMemR(&p, anImage->mBits, 4 * anImage->mHeight * anImage->mWidth);
	}
	SMemR(&p, &anImage->mNumCols, 4);
	SMemR(&p, &anImage->mNumRows, 4);
	SMemR(&p, &anImage->mForcedMode, 1);
	SMemR(&p, &anImage->mHasTrans, 1);
	SMemR(&p, &anImage->mHasAlpha, 1);
	SMemR(&p, &anImage->mIsVolatile, 1);
	SMemR(&p, &anImage->mPurgeBits, 1);
	SMemR(&p, &anImage->mWantPal, 1);
	SMemR(&p, &anImage->mBitsChanged, 1);
	SMemR(&p, &anImage->mDither16, 1);
	TriRep* aTriReps[2];
	//TriRep* aTriReps2[2]{ &anImage->mNormalTriRep, &anImage->mAdditiveTriRep }; //?
	aTriReps[0] = &anImage->mNormalTriRep; //?
	aTriReps[1] = &anImage->mAdditiveTriRep;
	for (int aRepIdx = 0; aRepIdx < 2; ++aRepIdx)
	{
		TriRep* aRep = aTriReps[aRepIdx];
		int aLevels;
		SMemR(&p, &aLevels, 4);
		for (int aLevelIdx = 0; aLevelIdx < aLevels; aLevelIdx++)
		{
			aRep->mLevels.push_back(TriRep::Level()); //?
			TriRep::Level* aLevel = &aRep->mLevels.back();
			SMemR(&p, aLevel, 4);
			SMemR(&p, &aLevel->mDetailY, 4);
			SMemR(&p, &aLevel->mRegionWidth, 4);
			SMemR(&p, &aLevel->mRegionHeight, 4);
			int aNumRegions = aLevel->mRegionHeight * aLevel->mRegionWidth;
			for (int iRegion = 0; iRegion < aNumRegions; iRegion++)
			{
				aLevel->mRegions.push_back(TriRep::Level::Region()); //?
				TriRep::Level::Region* aRegion = &aLevel->mRegions.back();
				SMemR(&p, aRegion, 16);
				int aNumTris;
				SMemR(&p, &aNumTris, 4);
				for (int aTriNum = 0; aTriNum < aNumTris; ++aTriNum)
				{
					TriRep::Tri aTri;
					aRegion->mTris.push_back(aTri);
				}
			}
		}
	}
	gSexyCache.FreeGetData(thePtr);
	std::string aSrcPath = theSrcFile;
	if (strncmp(gSexyAppBase->mChangeDirTo.c_str(), aSrcPath.c_str(), gSexyAppBase->mChangeDirTo.length()))
		aSrcPath.erase(); //?
	if (aSrcPath.length() > 0 && aSrcPath[0] == '\\' || aSrcPath[0] == '/')
		aSrcPath.erase(); //?
	anImage->mFilePath = aSrcPath;
	return anImage;
}

void DeviceImage::WriteToCache(const std::string& theSrcFile, const std::string& theAltData) //TODO | 2843-2977
{
	if (!gSexyCache.Connected())
		return;

	TriRep* aTriReps[2];
	aTriReps[0] = &mNormalTriRep;
	aTriReps[1] = &mAdditiveTriRep;
	int aFlags = GetImageFlags() << ImageFlag_RenderTarget;
	int aSize = 12;
	aSize += mColorTable ? mHeight * mWidth + 1024 : 4 * mHeight * mWidth;
	aSize += 16;
	for (int aRepIdx = 0; aRepIdx < 2; aRepIdx++)
	{
		TriRep* aRep = aTriReps[aRepIdx];
		int aLevels = aRep->mLevels.size();
		aSize += 4;
		for (int aLevelIdx = 0; aLevelIdx < aLevels; aLevelIdx++)
		{
			TriRep::Level* aLevel = &aRep->mLevels[aLevelIdx];
			aSize += 16;
			assert(aLevel->mRegions.size() == aLevel->mRegionWidth * aLevel->mRegionHeight); //2872
			for (int iRegion = 0; iRegion < aLevel->mRegions.size(); iRegion++)
			{
				TriRep::Level::Region* aRegion = &aLevel->mRegions[iRegion];
				aSize += 16;
				int aNumTris = aRegion->mTris.size();
				aSize += 4;
				for (int aTriNum = 0; aTriNum < aNumTris; ++aTriNum)
					aSize += 24;
			}
		}
	}
	void* thePtr = gSexyCache.AllocSetData(theSrcFile, "DDImage5:" + theAltData, aSize);

	if (thePtr == NULL)
		return;

	void* p = thePtr;
	SMemW(&p, &mWidth, 4);
	SMemW(&p, &mHeight, 4);
	ulong* aFlagsPtr = (ulong*)p;
	SMemW(&p, &aFlags, 4);
	if (mColorTable)
	{
		*aFlagsPtr |= ImageFlag_MinimizeNumSubdivisions; //?
		SMemW(&p, mColorTable, 1024);
		SMemW(&p, mColorIndices, mHeight * mWidth);
	}
	else
	{
		MemoryImage::GetBits();
		SMemW(&p, mBits, 4 * mHeight * mWidth);
	}
	SMemW(&p, &mNumCols, 4);
	SMemW(&p, &mNumRows, 4);
	if (mAnimInfo)
		*aFlagsPtr |= ImageFlag_Use64By64Subdivisions;
	SMemW(&p, &mForcedMode, 1);
	SMemW(&p, &mHasTrans, 1);
	SMemW(&p, &mHasAlpha, 1);
	SMemW(&p, &mIsVolatile, 1);
	SMemW(&p, &mPurgeBits, 1);
	SMemW(&p, &mWantPal, 1);
	SMemW(&p, &mBitsChanged, 1);
	SMemW(&p, &mDither16, 1);
	for (int i = 0; i < 2; ++i)
	{
		SMemW(&p, &aTriReps[i]->mLevels.size(), 4);
		for (int i = 0; i <= aTriReps[i]->mLevels.size(); i++)
		{
			SMemW(&p, &aTriReps[i]->mLevels[i], 4);
			SMemW(&p, (char*)&aTriReps[i]->mLevels[i] + 4, 4);
			SMemW(&p, (char*)&aTriReps[i]->mLevels[i] + 8, 4);
			SMemW(&p, (char*)&aTriReps[i]->mLevels[i] + 12, 4);
			int aNumRegions = &aTriReps[i]->mLevels[i] + 3 * aTriReps[i]->mLevels[i] + 2;
			for (int iRegion = 0; iRegion < aNumRegions; iRegion++)
			{
				SMemW(&p, &aTriReps[i]->mLevels[i], 4);
				SMemW(&p, (char*)&aTriReps[i]->mLevels[i] + 4, 4);
				SMemW(&p, (char*)&aTriReps[i]->mLevels[i] + 8, 4);
				SMemW(&p, (char*)&aTriReps[i]->mLevels[i] + 12, 4);
				for (int i = 0; i < aTriReps[i]->mLevels.size(); i++)
				{
					SMemW(&p, &aTriReps[i]->mLevels[i] + 16, 16);
					SMemW(&p, &aTriReps[i]->mLevels[i] + 16 + 16, 4);
					for (int i = 0; i < aTriReps[i]->mLevels.size(); i++)
						SMemW(&p, &aTriReps[i]->mLevels[i] + 16, 24);
				}
			}
		}
	}
	int aWriteSize = &p - thePtr;
	DBG_ASSERTE(aWriteSize == aSize); //2961
	gSexyCache.SetData(thePtr);
	gSexyCache.FreeSetData(thePtr);
	int aLastDotPos = theSrcFile.rfind('.');
	if (aLastDotPos == -1)
	{
		int aLastSlashPos = theSrcFile.rfind('\\') ? theSrcFile.rfind('/') : theSrcFile.rfind('.');
		gSexyCache.SetFileDeps(theSrcFile, "DDImage5:" + theAltData, theSrcFile + ".*\n" + theSrcFile + "_.*\n" + theSrcFile.substr(aLastSlashPos + 1) + "_" + theSrcFile.substr(theSrcFile.length() - aLastSlashPos - 1) + ".*"); //?
	}
	else
		gSexyCache.SetFileDeps(theSrcFile, "DDImage5:" + theAltData, theSrcFile);
}