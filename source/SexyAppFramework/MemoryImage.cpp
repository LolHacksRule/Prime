#include "MemoryImage.h"

#include "SexyAppBase.h"
#include "Graphics.h"
#include "NativeDisplay.h"
#include "Debug.h"
#include "Quantize.h"
#include "PerfTimer.h"
#include "SWTri.h"

#include <math.h>

using namespace Sexy;

#ifdef OPTIMIZE_SOFTWARE_DRAWING
bool gOptimizeSoftwareDrawing = false;
#endif


// Disable macro redefinition warning
#pragma warning(disable:4005)

MemoryImage::MemoryImage() //Was this changed | 32-36
{	
	mApp = gSexyAppBase;
	
	Init();
}

MemoryImage::MemoryImage(SexyAppBase* theApp) //39-42
{
	mApp = theApp;
	Init();
}

MemoryImage::MemoryImage(const MemoryImage& theMemoryImage) : //54-131
	Image(theMemoryImage),
	mApp(theMemoryImage.mApp),
	mHasAlpha(theMemoryImage.mHasAlpha),
	mHasTrans(theMemoryImage.mHasTrans),
	mBitsChanged(theMemoryImage.mBitsChanged),
	mIsVolatile(theMemoryImage.mIsVolatile),
	mPurgeBits(theMemoryImage.mPurgeBits),
	mWantPal(theMemoryImage.mWantPal),
	mBitsChangedCount(theMemoryImage.mBitsChangedCount)
{
	bool deleteBits = false;

	MemoryImage* aNonConstMemoryImage = (MemoryImage*) &theMemoryImage;

	if ((theMemoryImage.mBits == NULL) && (theMemoryImage.mColorTable == NULL))
	{
		// Must be a DDImage with only a DDSurface
		aNonConstMemoryImage->GetBits();
		deleteBits = true;
	}

	if (theMemoryImage.mBits != NULL)
	{
		mBits = new ulong[mWidth*mHeight + 1];
		mBits[mWidth*mHeight] = MEMORYCHECK_ID;
		memcpy(mBits, theMemoryImage.mBits, (mWidth*mHeight + 1)*sizeof(ulong));
	}
	else
		mBits = NULL;

	if (deleteBits)
	{
		// Remove the temporary source bits
		delete [] aNonConstMemoryImage->mBits;
		aNonConstMemoryImage->mBits = NULL;
	}

	if (theMemoryImage.mColorTable != NULL)
	{
		mColorTable = new ulong[256];
		memcpy(mColorTable, theMemoryImage.mColorTable, 256*sizeof(ulong));
	}
	else
		mColorTable = NULL;

	if (theMemoryImage.mColorIndices != NULL)
	{
		mColorIndices = new uchar[mWidth*mHeight];
		memcpy(mColorIndices, theMemoryImage.mColorIndices, mWidth*mHeight*sizeof(uchar));
	}
	else
		mColorIndices = NULL;

	if (theMemoryImage.mNativeAlphaData != NULL)
	{
		if (theMemoryImage.mColorTable == NULL)
		{
			mNativeAlphaData = new ulong[mWidth*mHeight];
			memcpy(mNativeAlphaData, theMemoryImage.mNativeAlphaData, mWidth*mHeight*sizeof(ulong));
		}
		else
		{
			mNativeAlphaData = new ulong[256];
			memcpy(mNativeAlphaData, theMemoryImage.mNativeAlphaData, 256*sizeof(ulong));
		}
	}
	else
		mNativeAlphaData = NULL;

	if (theMemoryImage.mRLAlphaData != NULL)
	{
		mRLAlphaData = new uchar[mWidth*mHeight];
		memcpy(mRLAlphaData, theMemoryImage.mRLAlphaData, mWidth*mHeight);
	}
	else
		mRLAlphaData = NULL;

	if (theMemoryImage.mRLAdditiveData != NULL)
	{
		mRLAdditiveData = new uchar[mWidth*mHeight];
		memcpy(mRLAdditiveData, theMemoryImage.mRLAdditiveData, mWidth*mHeight);
	}
	else
		mRLAdditiveData = NULL;	

	mApp->AddMemoryImage(this);
}

MemoryImage::~MemoryImage() //134-135
{	
	mApp->RemoveMemoryImage(this);
	
	delete [] mBits;
	delete [] mNativeAlphaData;	
	delete [] mRLAlphaData;
	delete [] mRLAdditiveData;
	delete [] mColorIndices;
	delete [] mColorTable;
}

void MemoryImage::Init() //146-167
{
	mBits = NULL;
	mColorTable = NULL;
	mColorIndices = NULL;

	mNativeAlphaData = NULL;
	mRLAlphaData = NULL;
	mRLAdditiveData = NULL;
	mHasTrans = false;
	mHasAlpha = false;	
	mBitsChanged = false;
	mForcedMode = false;
	mIsVolatile = false;

	mBitsChangedCount = 0;

	mPurgeBits = false;
	mWantPal = false;
	mDither16 = false;

	mApp->AddMemoryImage(this);
}

void MemoryImage::BitsChanged() //Correct? (C++ only) | 170-191
{
	mBitsChanged = true;
	mBitsChangedCount++;

	delete [] mNativeAlphaData;
	mNativeAlphaData = NULL;

	delete [] mRLAlphaData;
	mRLAlphaData = NULL;

	delete [] mRLAdditiveData;
	mRLAdditiveData = NULL;

	// Verify secret value at end to protect against overwrite
	if (mBits != NULL)
	{
		DBG_ASSERTE(mBits[mWidth*mHeight] == MEMORYCHECK_ID); //186 | 190 BejLiveWin8
	}

	mNormalTriRep.mLevels.clear();
	mAdditiveTriRep.mLevels.clear();
}

void MemoryImage::NormalDrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor) //C++ only (Correct?) | 194-481
{
	double aMinX = min(theStartX, theEndX);
	double aMinY = min(theStartY, theEndY);
	double aMaxX = max(theStartX, theEndX);
	double aMaxY = max(theStartY, theEndY);

	ulong aRMask = 0xFF0000;
	ulong aGMask = 0x00FF00;
	ulong aBMask = 0x0000FF;
	ulong aRRoundAdd = aRMask >> 1;
	ulong aGRoundAdd = aGMask >> 1;
	ulong aBRoundAdd = aBMask >> 1;
	
	DWORD *aSurface = GetBits();

	if (true)//(mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 32)
	{
		if (theColor.mAlpha == 255)
		{
			ulong aColor = 0xFF000000 | 
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
			int aRowWidth = mWidth;
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

				ulong* aDestPixels = ((ulong*) aSurface) + ((int) theStartY * aRowWidth) + (int) theStartX;
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

				ulong* aDestPixels = ((ulong*) aSurface) + ((int) theStartY * aRowWidth) + (int) theStartX;
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
			ulong src = 0xFF000000 | 
				((((((theColor.mRed * theColor.mAlpha + 0x80) >> 8) * aRMask) + aRRoundAdd) >> 8) & aRMask) |
				((((((theColor.mGreen * theColor.mAlpha + 0x80) >> 8) * aGMask) + aGRoundAdd) >> 8) & aGMask) |
				((((((theColor.mBlue * theColor.mAlpha + 0x80) >> 8) * aBMask) + aBRoundAdd) >> 8) & aBMask);
			int oma = 256 - theColor.mAlpha;

			double dv = theEndY - theStartY;
			double dh = theEndX - theStartX;
			int minG, maxG, G, DeltaG1, DeltaG2;
			double swap;
			int inc = 1;
			int aCurX;
			int aCurY;
			int aRowWidth = mWidth;
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

				ulong* aDestPixels = ((ulong*) aSurface) + ((int) theStartY * aRowWidth) + (int) theStartX;
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

				ulong* aDestPixels = ((ulong*) aSurface) + ((int) theStartY * aRowWidth) + (int) theStartX;
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
}

void MemoryImage::AdditiveDrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor) //484-666
{
	double aMinX = min(theStartX, theEndX);
	double aMinY = min(theStartY, theEndY);
	double aMaxX = max(theStartX, theEndX);
	double aMaxY = max(theStartY, theEndY);

	ulong aRMask = 0xFF0000;
	ulong aGMask = 0x00FF00;
	ulong aBMask = 0x0000FF;
	int aRedShift = 16;
	int aGreenShift = 8;
	int aBlueShift = 0;

	ulong aRRoundAdd = aRMask >> 1;
	ulong aGRoundAdd = aGMask >> 1;
	ulong aBRoundAdd = aBMask >> 1;

	uchar* aMaxTable = mApp->mAdd8BitMaxTable;
	DWORD *aSurface = GetBits();
	
	if (true)//(mLockedSurfaceDesc.ddpfPixelFormat.dwRGBBitCount == 32)
	{
		ulong rc = ((theColor.mRed * theColor.mAlpha) / 255);
		ulong gc = ((theColor.mGreen * theColor.mAlpha) / 255);
		ulong bc = ((theColor.mBlue * theColor.mAlpha) / 255);

		double dv = theEndY - theStartY;
		double dh = theEndX - theStartX;
		int minG, maxG, G, DeltaG1, DeltaG2;
		double swap;
		int inc = 1;
		int aCurX;
		int aCurY;
		int aRowWidth = mWidth;
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

			ulong* aDestPixels = ((ulong*) aSurface) + ((int) theStartY * aRowWidth) + (int) theStartX;
			ulong dest = *aDestPixels;

			int r = aMaxTable[((dest & aRMask) >> aRedShift) + rc];
			int g = aMaxTable[((dest & aGMask) >> aGreenShift) + gc];
			int b = aMaxTable[((dest & aBMask) >> aBlueShift) + bc];

			*(aDestPixels++) = 
				0xFF000000 | 
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

				r = aMaxTable[((dest & aRMask) >> aRedShift) + rc];
				g = aMaxTable[((dest & aGMask) >> aGreenShift) + gc];
				b = aMaxTable[((dest & aBMask) >> aBlueShift) + bc];

				*(aDestPixels++) = 
					0xFF000000 | 
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

			ulong* aDestPixels = ((ulong*) aSurface) + ((int) theStartY * mWidth) + (int) theStartX;
			
			ulong dest = *aDestPixels;

			int r = aMaxTable[((dest & aRMask) >> aRedShift) + rc];
			int g = aMaxTable[((dest & aGMask) >> aGreenShift) + gc];
			int b = aMaxTable[((dest & aBMask) >> aBlueShift) + bc];

			*aDestPixels = 
				0xFF000000 | 
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

				r = aMaxTable[((dest & aRMask) >> aRedShift) + rc];
				g = aMaxTable[((dest & aGMask) >> aGreenShift) + gc];
				b = aMaxTable[((dest & aBMask) >> aBlueShift) + bc];

				*aDestPixels = 
					0xFF000000 | 
					(r << aRedShift) |
					(g << aGreenShift) |
					(b << aBlueShift);

				aCurY++;
				aDestPixels += aRowAdd;
			}
		}
	}
}


void MemoryImage::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode, bool antiAlias) //C++ only | 670-714
{	
	if (theStartY == theEndY)
	{
		int aStartX = min(theStartX, theEndX);
		int aEndX = max(theStartX, theEndX);

		FillRect(Rect(aStartX, theStartY, aEndX-aStartX+1, theEndY-theStartY+1), theColor, theDrawMode);
		ReInit(); //?
		return;
	}
	else if (theStartX == theEndX)
	{
		int aStartY = min(theStartY, theEndY);
		int aEndY = max(theStartY, theEndY);

		FillRect(Rect(theStartX, aStartY, theEndX-theStartX+1, aEndY-aStartY+1), theColor, theDrawMode);
		ReInit(); //?
		return;
	}

	switch (theDrawMode)
	{
	case Graphics::DRAWMODE_NORMAL:
		NormalDrawLine(theStartX, theStartY, theEndX, theEndY, theColor);
		break;
	case Graphics::DRAWMODE_ADDITIVE:
		AdditiveDrawLine(theStartX, theStartY, theEndX, theEndY, theColor);
		break;
	}

	BitsChanged();
}

void MemoryImage::NormalDrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor) //TODO: 717-794
{
	ulong* aBits = GetBits();
	ulong color = theColor.ToInt();

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

	if (theColor.mAlpha != 255)
	{
		#define PIXEL_TYPE				ulong
		#define CALC_WEIGHT_A(w)		(((w) * (theColor.mAlpha+1)) >> 8)
		#define BLEND_PIXEL(p) \
		{\
			int aDestAlpha = dest >> 24;\
			int aNewDestAlpha = aDestAlpha + ((255 - aDestAlpha) * a) / 255;\
			a = 255 * a / aNewDestAlpha;\
			oma = 256 - a;\
			*(p) = (aNewDestAlpha << 24) |\
					((((color & 0xFF0000) * a + (dest & 0xFF0000) * oma) >> 8) & 0xFF0000) |\
					((((color & 0x00FF00) * a + (dest & 0x00FF00) * oma) >> 8) & 0x00FF00) |\
					((((color & 0x0000FF) * a + (dest & 0x0000FF) * oma) >> 8) & 0x0000FF);\
		}
		const int STRIDE = mWidth;

		#include "GENERIC_DrawLineAA.inc" //2-100

		#undef PIXEL_TYPE
		#undef CALC_WEIGHT_A
		#undef BLEND_PIXEL
	}
	else
	{
		#define PIXEL_TYPE				ulong
		#define CALC_WEIGHT_A(w)		(w)
		#define BLEND_PIXEL(p) \
		{\
			int aDestAlpha = dest >> 24;\
			int aNewDestAlpha = aDestAlpha + ((255 - aDestAlpha) * a) / 255;\
			a = 255 * a / aNewDestAlpha;\
			oma = 256 - a;\
			*(p) = (aNewDestAlpha << 24) |\
					((((color & 0xFF0000) * a + (dest & 0xFF0000) * oma) >> 8) & 0xFF0000) |\
					((((color & 0x00FF00) * a + (dest & 0x00FF00) * oma) >> 8) & 0x00FF00) |\
					((((color & 0x0000FF) * a + (dest & 0x0000FF) * oma) >> 8) & 0x0000FF);\
		}
		const int STRIDE = mWidth;

		#include "GENERIC_DrawLineAA.inc" //2-100

		#undef PIXEL_TYPE
		#undef CALC_WEIGHT_A
		#undef BLEND_PIXEL
	}


	BitsChanged();
}

void MemoryImage::AdditiveDrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor) //797-798
{
}


void MemoryImage::CommitBits() //C++ only | 803-863
{
	//if (gDebug)
	//	mApp->CopyToClipboard("+MemoryImage::CommitBits");
	
	if ((mBitsChanged) && (!mForcedMode))
	{			
		// Analyze 
		if (mBits != NULL)
		{
			mHasTrans = false;
			mHasAlpha = false;
			
			int aSize = mWidth*mHeight;
			ulong* ptr = mBits;
			
			for (int i = 0; i < aSize; i++)
			{
				uchar anAlpha = (uchar) (*ptr++ >> 24);

				if (anAlpha == 0)
					mHasTrans = true;
				else if (anAlpha != 255)
					mHasAlpha = true;
			}
		}
		else if (mColorTable != NULL)
		{
			mHasTrans = false;
			mHasAlpha = false;
			
			int aSize = 256;
			ulong* ptr = mColorTable;
			
			for (int i = 0; i < aSize; i++)
			{
				uchar anAlpha = (uchar) (*ptr++ >> 24);

				if (anAlpha == 0)
					mHasTrans = true;
				else if (anAlpha != 255)
					mHasAlpha = true;
			}
		}
		else
		{
			mHasTrans = true;
			mHasAlpha = false;
		}

		mBitsChanged = false;
	}

	//if (gDebug)
	//	mApp->CopyToClipboard("-MemoryImage::CommitBits");
}

void MemoryImage::SetImageMode(bool hasTrans, bool hasAlpha) //866-870 (Matched)
{
	mForcedMode = true;	
	mHasTrans = hasTrans;
	mHasAlpha = hasAlpha;	
}

void MemoryImage::SetVolatile(bool isVolatile) //873-875 (Matched)
{
	mIsVolatile = isVolatile;
}

void* MemoryImage::GetNativeAlphaData(NativeDisplay *theDisplay) //C++ only | 878-955 (Matched)
{
	if (mNativeAlphaData != NULL)
		return mNativeAlphaData;

	CommitBits();

	const int rRightShift = 16 + (8-theDisplay->mRedBits);
	const int gRightShift = 8 + (8-theDisplay->mGreenBits);
	const int bRightShift = 0 + (8-theDisplay->mBlueBits);

	const int rLeftShift = theDisplay->mRedShift;
	const int gLeftShift = theDisplay->mGreenShift;
	const int bLeftShift = theDisplay->mBlueShift;

	const int rMask = theDisplay->mRedMask;
	const int gMask = theDisplay->mGreenMask;
	const int bMask = theDisplay->mBlueMask;

	if (mColorTable == NULL)
	{
		ulong* aSrcPtr = GetBits();

		ulong* anAlphaData = new ulong[mWidth*mHeight];	

		ulong* aDestPtr = anAlphaData;
		int aSize = mWidth*mHeight;
		for (int i = 0; i < aSize; i++)
		{
			ulong val = *(aSrcPtr++);

			int anAlpha = val >> 24;			

			ulong r = ((val & 0xFF0000) * (anAlpha+1)) >> 8;
			ulong g = ((val & 0x00FF00) * (anAlpha+1)) >> 8;
			ulong b = ((val & 0x0000FF) * (anAlpha+1)) >> 8;

			*(aDestPtr++) =
				(((r >> rRightShift) << rLeftShift) & rMask) |
				(((g >> gRightShift) << gLeftShift) & gMask) |
				(((b >> bRightShift) << bLeftShift) & bMask) |
				(anAlpha << 24);
		}
		
		mNativeAlphaData = anAlphaData;	
	}
	else
	{
		ulong* aSrcPtr = mColorTable;		

		ulong* anAlphaData = new ulong[256];
		
		for (int i = 0; i < 256; i++)
		{
			ulong val = *(aSrcPtr++);

			int anAlpha = val >> 24;

			ulong r = ((val & 0xFF0000) * (anAlpha+1)) >> 8;
			ulong g = ((val & 0x00FF00) * (anAlpha+1)) >> 8;
			ulong b = ((val & 0x0000FF) * (anAlpha+1)) >> 8;

			anAlphaData[i] =
				(((r >> rRightShift) << rLeftShift) & rMask) |
				(((g >> gRightShift) << gLeftShift) & gMask) |
				(((b >> bRightShift) << bLeftShift) & bMask) |
				(anAlpha << 24);
		}
		
		
		mNativeAlphaData = anAlphaData;	
	}

	return mNativeAlphaData;
}


uchar* MemoryImage::GetRLAlphaData() //C++ only | 959-992
{
	CommitBits();

	if (mRLAlphaData == NULL)
	{		
		mRLAlphaData = new uchar[mWidth*mHeight];

		if (mColorTable == NULL)
		{
			ulong* aSrcPtr;
			if (mNativeAlphaData != NULL)
				aSrcPtr = (ulong*) mNativeAlphaData;
			else
				aSrcPtr = GetBits();

			#define NEXT_SRC_COLOR (*(aSrcPtr++))

			#include "MI_GetRLAlphaData.inc" //3-50

			#undef NEXT_SRC_COLOR
		}
		else
		{
			uchar* aSrcPtr = mColorIndices;
			ulong* aColorTable = mColorTable;

			#define NEXT_SRC_COLOR (aColorTable[*(aSrcPtr++)])

			#include "MI_GetRLAlphaData.inc"

			#undef NEXT_SRC_COLOR
		}
	}

	return mRLAlphaData;
}

uchar* MemoryImage::GetRLAdditiveData(NativeDisplay *theNative) //997-1113
{
	if (mRLAdditiveData == NULL)
	{
		if (mColorTable == NULL)
		{
			ulong* aBits = (ulong*) GetNativeAlphaData(theNative);

			mRLAdditiveData = new uchar[mWidth*mHeight];

			uchar* aWPtr = mRLAdditiveData;
			ulong* aRPtr = aBits;

			if (mWidth==1)
			{
				memset(aWPtr,1,mHeight);
			}
			else
			{
				for (int aRow = 0; aRow < mHeight; aRow++)			
				{
					int aRCount = 1;
					int aRLCount = 1;
					
					int aLastAClass = (((*aRPtr++) & 0xFFFFFF) != 0) ? 1 : 0;

					while (aRCount < mWidth)
					{
						aRCount++;				

						int aThisAClass = (((*aRPtr++) & 0xFFFFFF) != 0) ? 1 : 0;				

						if ((aThisAClass != aLastAClass) || (aRCount == mWidth))
						{
							if (aThisAClass == aLastAClass)
								aRLCount++;

							for (int i = aRLCount; i > 0; i--)
							{
								if (i >= 255)
									*aWPtr++ = 255;
								else
									*aWPtr++ = i;
							}					

							if ((aRCount == mWidth) && (aThisAClass != aLastAClass))
								*aWPtr++ = 1;

							aLastAClass = aThisAClass;
							aRLCount = 1;
						}
						else
						{
							aRLCount++;
						}
					}
				}
			}
		}
		else
		{
			ulong* aNativeColorTable = (ulong*) GetNativeAlphaData(theNative);

			mRLAdditiveData = new uchar[mWidth*mHeight];

			uchar* aWPtr = mRLAdditiveData;
			uchar* aRPtr = mColorIndices;

			if (mWidth==1)
			{
				memset(aWPtr,1,mHeight);
			}
			else
			{
				for (int aRow = 0; aRow < mHeight; aRow++)			
				{
					int aRCount = 1;
					int aRLCount = 1;
					
					int aLastAClass = (((aNativeColorTable[*aRPtr++]) & 0xFFFFFF) != 0) ? 1 : 0;

					while (aRCount < mWidth)
					{
						aRCount++;				

						int aThisAClass = (((aNativeColorTable[*aRPtr++]) & 0xFFFFFF) != 0) ? 1 : 0;				

						if ((aThisAClass != aLastAClass) || (aRCount == mWidth))
						{
							if (aThisAClass == aLastAClass)
								aRLCount++;

							for (int i = aRLCount; i > 0; i--)
							{
								if (i >= 255)
									*aWPtr++ = 255;
								else
									*aWPtr++ = i;
							}					

							if ((aRCount == mWidth) && (aThisAClass != aLastAClass))
								*aWPtr++ = 1;

							aLastAClass = aThisAClass;
							aRLCount = 1;
						}
						else
						{
							aRLCount++;
						}
					}
				}
			}
		}
	}

	return mRLAdditiveData;
}

void MemoryImage::PurgeBits() //1116-1145 (Matched)
{
	mPurgeBits = true;

	if (mApp->Is3DAccelerated())
	{
		// Due to potential D3D threading issues we have to defer the texture creation
		//  and therefore the actual purging
		if (GetRenderData() == NULL) //No longer D3D
			return;
	}
	else
	{
		if ((mBits == NULL) && (mColorIndices == NULL))
			return;
		
		GetNativeAlphaData(gSexyAppBase->mGraphicsDriver->GetNativeDisplayInfo());
	}		
	
	delete [] mBits;
	mBits = NULL;
	
	if (GetRenderData() != NULL)
	{
		delete [] mColorIndices;
		mColorIndices = NULL;

		delete [] mColorTable;
		mColorTable = NULL;
	}	
}

void MemoryImage::DeleteSWBuffers() //1148-1161 (Matched)
{
	if ((mNativeAlphaData != NULL) && (mRLAdditiveData != NULL) && (mRLAlphaData != NULL)) return;
	if ((mBits == NULL) && (mColorIndices == NULL))
		GetBits();
	
	delete[] mNativeAlphaData;
	mNativeAlphaData = NULL;
	
	delete[] mRLAdditiveData;
	mRLAdditiveData = NULL;
	
	delete[] mRLAlphaData;
	mRLAlphaData = NULL;
}

void MemoryImage::Delete3DBuffers() //C++ only | 1164-1166 (Matched)
{
	mApp->Remove3DData(this);
}

void MemoryImage::DeleteExtraBuffers() //C++ only | 1169-1172 (Matched)
{
	DeleteSWBuffers();
	Delete3DBuffers();
}

void MemoryImage::ReInit() //C++ only | 1175-1182 (Matched)
{
	// Fix any un-palletizing
	if (mWantPal)
		Palletize();
			
	if (mPurgeBits)
		PurgeBits();
}

void MemoryImage::DeleteNativeData() //C++ only | 1185-1194 (Matched)
{
	if ((mBits == NULL) && (mColorIndices == NULL))
		GetBits(); // We need to keep the bits around
	
	delete [] mNativeAlphaData;
	mNativeAlphaData = NULL;

	delete [] mRLAdditiveData;
	mRLAdditiveData = NULL;	
}

void MemoryImage::SetBits(ulong* theBits, int theWidth, int theHeight, bool commitBits) //1197-1220 (Matched)
{	
	if (theBits != mBits)
	{
		delete [] mColorIndices;
		mColorIndices = NULL;

		delete [] mColorTable;
		mColorTable = NULL;

		if (theWidth != mWidth || theHeight != mHeight)
		{
			delete [] mBits;
			mBits = new ulong[theWidth*theHeight + 1];
			mWidth = theWidth;
			mHeight = theHeight;
		}
		memcpy(mBits, theBits, mWidth*mHeight*sizeof(ulong));
		mBits[mWidth*mHeight] = MEMORYCHECK_ID;

		BitsChanged();
		if (commitBits) //C++ only
			CommitBits();
	}
}

void MemoryImage::Create(int theWidth, int theHeight) //1223-1235
{
	delete [] mBits;
	mBits = NULL;

	mWidth = theWidth;
	mHeight = theHeight;	

	// All zeros --> trans + alpha
	mHasTrans = true;
	mHasAlpha = true;

	BitsChanged();	
}

ulong* MemoryImage::GetBits() //1238-1304 (Prob correct)
{
	if (mBits == NULL)
	{
		int aSize = mWidth*mHeight;

		mBits = new ulong[aSize+1];		
		mBits[aSize] = MEMORYCHECK_ID;		

		if (mColorTable != NULL)
		{
			for (int i = 0; i < aSize; i++)
				mBits[i] = mColorTable[mColorIndices[i]];

			delete [] mColorIndices;
			mColorIndices = NULL;

			delete [] mColorTable;
			mColorTable = NULL;

			delete [] mNativeAlphaData;
			mNativeAlphaData = NULL;
		}
		else if (mNativeAlphaData != NULL)
		{
			NativeDisplay* aDisplay = gSexyAppBase->mGraphicsDriver->GetNativeDisplayInfo();

			const int rMask = aDisplay->mRedMask;
			const int gMask = aDisplay->mGreenMask;
			const int bMask = aDisplay->mBlueMask;

			const int rLeftShift = aDisplay->mRedShift + (aDisplay->mRedBits);
			const int gLeftShift = aDisplay->mGreenShift + (aDisplay->mGreenBits);
			const int bLeftShift = aDisplay->mBlueShift + (aDisplay->mBlueBits);			

			ulong* aDestPtr = mBits;
			ulong* aSrcPtr = mNativeAlphaData;

			int aSize = mWidth*mHeight;
			for (int i = 0; i < aSize; i++)
			{
				ulong val = *(aSrcPtr++);

				int anAlpha = val >> 24;			

				ulong r = (((((val & rMask) << 8) / (anAlpha+1)) & rMask) << 8) >> rLeftShift;
				ulong g = (((((val & gMask) << 8) / (anAlpha+1)) & gMask) << 8) >> gLeftShift;
				ulong b = (((((val & bMask) << 8) / (anAlpha+1)) & bMask) << 8) >> bLeftShift;

				*(aDestPtr++) = (r << 16) | (g << 8) | (b) | (anAlpha << 24);
			}
		}
		else if ((GetRenderData() == NULL) || mApp->mGraphicsDriver->GetRenderDevice3D() == NULL || (!mApp->mGraphicsDriver->GetRenderDevice3D()->RecoverImageBitsFromRenderData(this)))
		{
			ZeroMemory(mBits, aSize*sizeof(ulong));
		}
	}	

	return mBits;
}

void MemoryImage::FillRect(const Rect& theRect, const Color& theColor, int theDrawMode) //1307-1356 (Prob correct)
{
	ulong src = theColor.ToInt();

	ulong* aBits = GetBits();

	int oldAlpha = src >> 24;

	if (oldAlpha == 0xFF)
	{
		for (int aRow = theRect.mY; aRow < theRect.mY+theRect.mHeight; aRow++)
		{
			ulong* aDestPixels = &aBits[aRow*mWidth+theRect.mX];

			for (int i = 0; i < theRect.mWidth; i++)
				*aDestPixels++ = src;
		}
	}
	else
	{
		for (int aRow = theRect.mY; aRow < theRect.mY+theRect.mHeight; aRow++)
		{
			ulong* aDestPixels = &aBits[aRow*mWidth+theRect.mX];

			for (int i = 0; i < theRect.mWidth; i++)
			{				
				ulong dest = *aDestPixels;
								
				int aDestAlpha = dest >> 24;
				int aNewDestAlpha = aDestAlpha + ((255 - aDestAlpha) * oldAlpha) / 255;
									
				int newAlpha = 255 * oldAlpha / aNewDestAlpha;

				int oma = 256 - newAlpha;

#ifdef OPTIMIZE_SOFTWARE_DRAWING
				*(aDestPixels++) = (aNewDestAlpha << 24) |
					((((dest & 0xFF00FF) * oma + (src & 0xFF00FF) * newAlpha) >> 8) & 0xFF00FF) |
					((((dest & 0x00FF00) * oma + (src & 0x00FF00) * newAlpha) >> 8) & 0x00FF00);
#else
				*(aDestPixels++) = (aNewDestAlpha << 24) |
					((((dest & 0x0000FF) * oma) >> 8) + (((src & 0x0000FF) * newAlpha) >> 8) & 0x0000FF) |
					((((dest & 0x00FF00) * oma) >> 8) + (((src & 0x00FF00) * newAlpha) >> 8) & 0x00FF00) |
					((((dest & 0xFF0000) * oma) >> 8) + (((src & 0xFF0000) * newAlpha) >> 8) & 0xFF0000);
#endif
			}
		}
	}

	BitsChanged();
}

void MemoryImage::ClearRect(const Rect& theRect) //1359-1371 (Matched)
{
	ulong* aBits = GetBits();
	
	for (int aRow = theRect.mY; aRow < theRect.mY+theRect.mHeight; aRow++)
	{
		ulong* aDestPixels = &aBits[aRow*mWidth+theRect.mX];

		for (int i = 0; i < theRect.mWidth; i++)
			*aDestPixels++ = 0;
	}	
	
	BitsChanged();
}

void MemoryImage::Clear() //1374-1383 (Match)
{
	ulong* ptr = GetBits();
	if (ptr != NULL)
	{
		for (int i = 0; i < mWidth*mHeight; i++)
			*ptr++ = 0;

		BitsChanged();
	}
}

void MemoryImage::AdditiveBlt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor) //C++ only (TODO INC) | 1386-1423
{
	theImage->mDrawn = true;

	MemoryImage* aSrcMemoryImage = theImage ? theImage->AsMemoryImage() : NULL;

	uchar* aMaxTable = mApp->mAdd8BitMaxTable;

	if (aSrcMemoryImage != NULL)
	{
		if (aSrcMemoryImage->mColorTable == NULL)
		{			
			ulong* aSrcBits = aSrcMemoryImage->GetBits();

			#define NEXT_SRC_COLOR		(*(aSrcPtr++))
			#define SRC_TYPE			ulong			

			#include "MI_AdditiveBlt.inc"

			#undef NEXT_SRC_COLOR
			#undef SRC_TYPE		
		}
		else
		{			
			ulong* aColorTable = aSrcMemoryImage->mColorTable;
			uchar* aSrcBits = aSrcMemoryImage->mColorIndices;

			#define NEXT_SRC_COLOR		(aColorTable[*(aSrcPtr++)])
			#define SRC_TYPE uchar

			#include "MI_AdditiveBlt.inc"

			#undef NEXT_SRC_COLOR
			#undef SRC_TYPE		
		}

		BitsChanged();
	}	
}

void MemoryImage::NormalBlt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor) //C++ only | 1426-1465 (TODO INC)
{
	theImage->mDrawn = true;

	MemoryImage* aSrcMemoryImage = theImage ? theImage->AsMemoryImage() : NULL;

	if (aSrcMemoryImage != NULL)
	{
		if (aSrcMemoryImage->mColorTable == NULL)
		{			
			ulong* aSrcPixelsRow = ((ulong*) aSrcMemoryImage->GetBits()) + (theSrcRect.mY * theImage->mWidth) + theSrcRect.mX;

			#define NEXT_SRC_COLOR		(*(aSrcPtr++))
			#define READ_SRC_COLOR		(*(aSrcPtr))
			#define EACH_ROW			ulong* aSrcPtr = aSrcPixelsRow

			#include "MI_NormalBlt.inc" //2-220

			#undef NEXT_SRC_COLOR	
			#undef READ_SRC_COLOR	
			#undef EACH_ROW			
		}
		else
		{			
			ulong* aColorTable = aSrcMemoryImage->mColorTable;
			uchar* aSrcPixelsRow = aSrcMemoryImage->mColorIndices + (theSrcRect.mY * theImage->mWidth) + theSrcRect.mX;

			#define NEXT_SRC_COLOR		(aColorTable[*(aSrcPtr++)])
			#define READ_SRC_COLOR		(aColorTable[*(aSrcPtr)])
			#define EACH_ROW			uchar* aSrcPtr = aSrcPixelsRow

			#include "MI_NormalBlt.inc"

			#undef NEXT_SRC_COLOR	
			#undef READ_SRC_COLOR	
			#undef EACH_ROW			
		}

		BitsChanged();
	}
}

void MemoryImage::Blt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode) //C++ only | 1468-1485 (Matched)
{
	theImage->mDrawn = true;

	DBG_ASSERTE((theColor.mRed >= 0) && (theColor.mRed <= 255)); //1471 | 1493 BejLiveWin8
	DBG_ASSERTE((theColor.mGreen >= 0) && (theColor.mGreen <= 255)); //1472 | 1494 BejLiveWin8
	DBG_ASSERTE((theColor.mBlue >= 0) && (theColor.mBlue <= 255)); //1473 | 1495 BejLiveWin8
	DBG_ASSERTE((theColor.mAlpha >= 0) && (theColor.mAlpha <= 255)); //1474 | 1496 BejLiveWin8

	switch (theDrawMode)
	{
	case Graphics::DRAWMODE_NORMAL:
		NormalBlt(theImage, theX, theY, theSrcRect, theColor);
		break;
	case Graphics::DRAWMODE_ADDITIVE:
		AdditiveBlt(theImage, theX, theY, theSrcRect, theColor);
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void MemoryImage::BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect &theClipRect, const Color& theColor, int theDrawMode) //C++ only | 1491-1494 (Matched)
{
	theImage->mDrawn = true;

	BltRotated(theImage,theX,theY,theSrcRect,theClipRect,theColor,theDrawMode,0,0,0);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool MemoryImage::BltRotatedClipHelper(float &theX, float &theY, const Rect &theSrcRect, const Rect &theClipRect, double theRot, FRect &theDestRect, float theRotCenterX, float theRotCenterY) //C++ only | 1499-1540 (Matched?)
{
	// Clipping Code (this used to be in Graphics::DrawImageRotated)
	float aCos = cosf(theRot);
	float aSin = sinf(theRot);

	// Map the four corners and find the bounding rectangle
	float px[4] = { 0, theSrcRect.mWidth, theSrcRect.mWidth, 0 };
	float py[4] = { 0, 0, theSrcRect.mHeight, theSrcRect.mHeight };
	float aMinX = 10000000;
	float aMaxX = -10000000;
	float aMinY = 10000000;
	float aMaxY = -10000000;

	for (int i=0; i<4; i++)
	{
		float ox = px[i] - theRotCenterX;
		float oy = py[i] - theRotCenterY;

		px[i] = (theRotCenterX + ox*aCos + oy*aSin) + theX;
		py[i] = (theRotCenterY + oy*aCos - ox*aSin) + theY;

		if (px[i] < aMinX)
			aMinX = px[i];
		if (px[i] > aMaxX)
			aMaxX = px[i];
		if (py[i] < aMinY)
			aMinY = py[i];
		if (py[i] > aMaxY)
			aMaxY = py[i];
	}



	FRect aClipRect(theClipRect.mX,theClipRect.mY,theClipRect.mWidth,theClipRect.mHeight);

	FRect aDestRect = FRect(aMinX, aMinY, aMaxX-aMinX, aMaxY-aMinY).Intersection(aClipRect);	
	if ((aDestRect.mWidth <= 0) || (aDestRect.mHeight <= 0)) // nothing to draw
		return false;

	theDestRect = aDestRect;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool MemoryImage::StretchBltClipHelper(const Rect &theSrcRect, const Rect &theClipRect, const Rect &theDestRect, FRect &theSrcRectOut, Rect &theDestRectOut) //C++ only | 1545-1557 (Matched)
{
	theDestRectOut = Rect(theDestRect.mX , theDestRect.mY, theDestRect.mWidth, theDestRect.mHeight).Intersection(theClipRect);	

	double aXFactor = theSrcRect.mWidth / (double) theDestRect.mWidth;
	double aYFactor = theSrcRect.mHeight / (double) theDestRect.mHeight;

	theSrcRectOut = FRect(theSrcRect.mX + (theDestRectOut.mX - theDestRect.mX)*aXFactor, 
				   theSrcRect.mY + (theDestRectOut.mY - theDestRect.mY)*aYFactor, 
				   theSrcRect.mWidth + (theDestRectOut.mWidth - theDestRect.mWidth)*aXFactor, 
				   theSrcRect.mHeight + (theDestRectOut.mHeight - theDestRect.mHeight)*aYFactor);

	return theSrcRectOut.mWidth>0 && theSrcRectOut.mHeight>0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool MemoryImage::StretchBltMirrorClipHelper(const Rect &theSrcRect, const Rect &theClipRect, const Rect &theDestRect, FRect &theSrcRectOut, Rect &theDestRectOut) //C++ only | 1562-1378 (Matched)
{
	theDestRectOut = Rect(theDestRect.mX, theDestRect.mY, theDestRect.mWidth, theDestRect.mHeight).Intersection(theClipRect);	

	double aXFactor = theSrcRect.mWidth / (double) theDestRect.mWidth;
	double aYFactor = theSrcRect.mHeight / (double) theDestRect.mHeight;

	int aTotalClip = theDestRect.mWidth - theDestRectOut.mWidth;
	int aLeftClip = theDestRectOut.mX - theDestRect.mX;
	int aRightClip = aTotalClip-aLeftClip;

	theSrcRectOut = FRect(theSrcRect.mX + (aRightClip)*aXFactor, 
				   theSrcRect.mY + (theDestRectOut.mY - theDestRect.mY)*aYFactor, 
				   theSrcRect.mWidth + (theDestRectOut.mWidth - theDestRect.mWidth)*aXFactor, 
				   theSrcRect.mHeight + (theDestRectOut.mHeight - theDestRect.mHeight)*aYFactor);

	return theSrcRectOut.mWidth>0 && theSrcRectOut.mHeight>0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void MemoryImage::BltRotated(Image* theImage, float theX, float theY, const Rect &theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY) //C++ only (TODO INC?) | 1583-1638
{
	theImage->mDrawn = true;

	// This BltRotatedClipHelper clipping used to happen in Graphics::DrawImageRotated
	FRect aDestRect;
	if (!BltRotatedClipHelper(theX, theY, theSrcRect, theClipRect, theRot, aDestRect,theRotCenterX,theRotCenterY))
		return;

	MemoryImage* aMemoryImage = theImage ? theImage->AsMemoryImage() : NULL;
	uchar* aMaxTable = mApp->mAdd8BitMaxTable;

	if (aMemoryImage != NULL)
	{	
		if (aMemoryImage->mColorTable == NULL)
		{			
			ulong* aSrcBits = aMemoryImage->GetBits() + theSrcRect.mX + theSrcRect.mY*theSrcRect.mWidth;			

			#define SRC_TYPE ulong
			#define READ_COLOR(ptr) (*(ptr))

			if (theDrawMode == Graphics::DRAWMODE_NORMAL)
			{
				#include "MI_BltRotated.inc"
			}
			else
			{
				#include "MI_BltRotated_Additive.inc"
			}

			#undef SRC_TYPE
			#undef READ_COLOR
		}
		else
		{			
			ulong* aColorTable = aMemoryImage->mColorTable;
			uchar* aSrcBits = aMemoryImage->mColorIndices + theSrcRect.mX + theSrcRect.mY*theSrcRect.mWidth;

			#define SRC_TYPE uchar
			#define READ_COLOR(ptr) (aColorTable[*(ptr)])

			if (theDrawMode == Graphics::DRAWMODE_NORMAL)
			{
				#include "MI_BltRotated.inc"
			}
			else
			{
				#include "MI_BltRotated_Additive.inc"
			}

			#undef SRC_TYPE
			#undef READ_COLOR
		}

		BitsChanged();
	}
}

void MemoryImage::SlowStretchBlt(Image* theImage, const Rect& theDestRect, const FRect& theSrcRect, const Color& theColor, int theDrawMode) //C++ only | TODO INC | 1641-1681
{
	theImage->mDrawn = true;

	// This thing was a pain to write.  I bet i could have gotten something just as good
	// from some Graphics Gems book.	
	
	ulong* aDestEnd = GetBits() + (mWidth * mHeight);

	MemoryImage* aSrcMemoryImage = theImage ? theImage->AsMemoryImage() : NULL;

	if (aSrcMemoryImage != NULL)
	{
		if (aSrcMemoryImage->mColorTable == NULL)
		{			
			ulong* aSrcBits = aSrcMemoryImage->GetBits();

			#define SRC_TYPE ulong
			#define READ_COLOR(ptr) (*(ptr))

			#include "MI_SlowStretchBlt.inc"

			#undef SRC_TYPE
			#undef READ_COLOR
		}
		else
		{
			ulong* aColorTable = aSrcMemoryImage->mColorTable;
			uchar* aSrcBits = aSrcMemoryImage->mColorIndices;

			#define SRC_TYPE uchar
			#define READ_COLOR(ptr) (aColorTable[*(ptr)])

			#include "MI_SlowStretchBlt.inc"

			#undef SRC_TYPE
			#undef READ_COLOR
		}

		BitsChanged();
	}	
}

//TODO: Make the special version
void MemoryImage::FastStretchBlt(Image* theImage, const Rect& theDestRect, const FRect& theSrcRect, const Color& theColor, int theDrawMode) //C++ only | 1685-1747 (Match)
{
	theImage->mDrawn = true;

	MemoryImage* aSrcMemoryImage = theImage ? theImage->AsMemoryImage() : NULL;

	if (aSrcMemoryImage != NULL)
	{
		ulong* aDestPixelsRow = ((ulong*) GetBits()) + (theDestRect.mY * mWidth) + theDestRect.mX;
		ulong* aSrcPixelsRow = (ulong*) aSrcMemoryImage->GetBits();;
		
		double aSrcY = theSrcRect.mY;

		double anAddX = theSrcRect.mWidth / theDestRect.mWidth;
		double anAddY = theSrcRect.mHeight / theDestRect.mHeight;

		if (theColor == Color::White)
		{
			for (int y = 0; y < theDestRect.mHeight; y++)
			{
				double aSrcX = theSrcRect.mX;

				ulong* aDestPixels = aDestPixelsRow;								

				for (int x = 0; x < theDestRect.mWidth; x++)
				{
					aSrcX += anAddX;

					ulong* aSrcPixels = aSrcPixelsRow + ((int) aSrcX) + (aSrcMemoryImage->mWidth * ((int) aSrcY));
					ulong src = *aSrcPixels;

					ulong dest = *aDestPixels;
					
					int a = src >> 24;	
					
					if (a != 0)
					{
						int aDestAlpha = dest >> 24;
						int aNewDestAlpha = aDestAlpha + ((255 - aDestAlpha) * a) / 255;
											
						a = 255 * a / aNewDestAlpha;

						int oma = 256 - a;
						
						*(aDestPixels++) = (aNewDestAlpha << 24) |
							((((dest & 0x0000FF) * oma) >> 8) + (((src & 0x0000FF) * a) >> 8) & 0x0000FF) |
							((((dest & 0x00FF00) * oma) >> 8) + (((src & 0x00FF00) * a) >> 8) & 0x00FF00) |
							((((dest & 0xFF0000) * oma) >> 8) + (((src & 0xFF0000) * a) >> 8) & 0xFF0000);
					}
					else
						aDestPixels++;
				}

				aDestPixelsRow += mWidth;				
				aSrcY += anAddY;
			}
		}
		else
		{
		}
	}

	BitsChanged();
}

void MemoryImage::BltStretched(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch, bool mirror) //C++ only (Changed from StretchBlt) | 1750-1766 (Matched)
{
	if (!mirror)
		return;

	theImage->mDrawn = true;

	Rect aDestRect;
	FRect aSrcRect;

	if (!StretchBltClipHelper(theSrcRect, theClipRect, theDestRect, aSrcRect, aDestRect))
		return;

	if (fastStretch)
		FastStretchBlt(theImage, aDestRect, aSrcRect, theColor, theDrawMode);
	else
		SlowStretchBlt(theImage, aDestRect, aSrcRect, theColor, theDrawMode);
}

void MemoryImage::BltMatrixHelper(Image* theImage, float x, float y, const SexyMatrix3 &theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect &theSrcRect, void *theSurface, int theBytePitch, int thePixelFormat, bool blend) //C++ only | 1769-1799
{
	MemoryImage *anImage = theImage ? theImage->AsMemoryImage() : NULL;
	if (anImage==NULL)
		return;
 
	float w2 = theSrcRect.mWidth/2.0f;
	float h2 = theSrcRect.mHeight/2.0f;

	float u0 = (float)theSrcRect.mX/theImage->mWidth;
	float u1 = (float)(theSrcRect.mX + theSrcRect.mWidth)/theImage->mWidth;
	float v0 = (float)theSrcRect.mY/theImage->mHeight;
	float v1 = (float)(theSrcRect.mY + theSrcRect.mHeight)/theImage->mHeight;

	SWHelper::XYZStruct aVerts[4] =
	{
		{ -w2,	-h2,	u0, v0, 0xFFFFFFFF },
		{ w2,	-h2,	u1,	v0,	0xFFFFFFFF },
		{ -w2,	h2,		u0,	v1,	0xFFFFFFFF },
		{ w2,	h2,		u1,	v1,	0xFFFFFFFF }
	};

	for (int i=0; i<4; i++)
	{
		SexyVector3 v(aVerts[i].mX, aVerts[i].mY, 1);
		v = theMatrix*v;
		aVerts[i].mX = v.x + x - 0.5f;
		aVerts[i].mY = v.y + y - 0.5f;
	}

	SWHelper::SWDrawShape(aVerts, 4, anImage, theColor, theDrawMode, theClipRect, theSurface, theBytePitch, thePixelFormat, blend,false);
}

void MemoryImage::BltMatrix(Image* theImage, float x, float y, const SexyMatrix3 &theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect &theSrcRect, bool blend) //C++ only | 1802-1813 (Match)
{
	theImage->mDrawn = true;

	DWORD *aSurface = GetBits();
	int aPitch = mWidth*4;
	int aFormat = 0x8888;
	if (mForcedMode && !mHasAlpha && !mHasTrans)
		aFormat = 0x888;

	BltMatrixHelper(theImage,x,y,theMatrix,theClipRect,theColor,theDrawMode,theSrcRect,aSurface,aPitch,aFormat,blend);
	BitsChanged();
}

void MemoryImage::BltTrianglesTexHelper(Image *theTexture, const SexyVertex2D theVertices[][3], int theNumTriangles, const Rect &theClipRect, const Color &theColor, int theDrawMode, void *theSurface, int theBytePitch, int thePixelFormat, float tx, float ty, bool blend) //C++ only | 1816-1842 (Correct?)
{
	MemoryImage *anImage = theTexture ? theTexture->AsMemoryImage() : NULL;
//	if (anImage==NULL)
//		return;

//	int aColor = theColor.ToInt();
	for (int i=0; i<theNumTriangles; i++)
	{
		bool vertexColor = false;

		SWHelper::XYZStruct aVerts[3];
		for (int j=0; j<3; j++)
		{
			aVerts[j].mX = theVertices[i][j].x + tx;
			aVerts[j].mY = theVertices[i][j].y + ty;
			aVerts[j].mU = theVertices[i][j].u;
			aVerts[j].mV = theVertices[i][j].v;
			aVerts[j].mDiffuse = theVertices[i][j].color;

			if (aVerts[j].mDiffuse!=0) 
				vertexColor = true;
		}

		SWHelper::SWDrawShape(aVerts, 3, anImage, theColor, theDrawMode, theClipRect, theSurface, theBytePitch, thePixelFormat, blend, vertexColor);
	}

}

void MemoryImage::FillScanLinesWithCoverage(RenderDevice::Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode, const BYTE* theCoverage, int theCoverX, int theCoverY, int theCoverWidth, int theCoverHeight) //C++ only | 1845-1878 (Matched)  
{
	ulong* theBits = GetBits();
	ulong src = theColor.ToInt();
	for (int i = 0; i < theSpanCount; ++i)
	{
		Span* aSpan = &theSpans[i];
		int x = aSpan->mX - theCoverX;
		int y = aSpan->mY - theCoverY;

		ulong* aDestPixels = &theBits[aSpan->mY*mWidth + aSpan->mX];
		const BYTE* aCoverBits = &theCoverage[y*theCoverWidth+x];
		for (int w = 0; w < aSpan->mWidth; ++w)
		{
			int cover = *aCoverBits++ + 1;
			int a = (cover * theColor.mAlpha) >> 8;
			int oma;
			ulong dest = *aDestPixels;
							
			if (a > 0)
			{
				int aDestAlpha = dest >> 24;
				int aNewDestAlpha = aDestAlpha + ((255 - aDestAlpha) * a) / 255;
				
				a = 255 * a / aNewDestAlpha;
				oma = 256 - a;
				*(aDestPixels++) = (aNewDestAlpha << 24) |
					((((dest & 0x0000FF) * oma + (src & 0x0000FF) * a) >> 8) & 0x0000FF) |
					((((dest & 0x00FF00) * oma + (src & 0x00FF00) * a) >> 8) & 0x00FF00) |
					((((dest & 0xFF0000) * oma + (src & 0xFF0000) * a) >> 8) & 0xFF0000);
			}
		}
	}
	BitsChanged();
}

void MemoryImage::BltTriangles(Image* theImage, const SexyVertex2D theVertices[][3], int theNumTriangles, const Color& theColor, int theDrawMode, float tx, float ty, bool blend, const Rect* theClipRect) //C++ only | 1881-1893 (Matched)  
{
	theImage->mDrawn = true;

	DWORD *aSurface = GetBits();

	int aPitch = mWidth*4;
	int aFormat = 0x8888;
	if (mForcedMode && !mHasAlpha && !mHasTrans)
		aFormat = 0x888;

	BltTrianglesTexHelper(theImage,theVertices,theNumTriangles,*theClipRect,theColor,theDrawMode,aSurface,aPitch,aFormat,tx,ty,blend); //?
	BitsChanged();
}

bool MemoryImage::Palletize() //C++ only (returns true on XNA) | 1896-1932 (Matched)
{
	CommitBits();
	
	if (mColorTable != NULL)
		return true;

	GetBits();

	if (mBits == NULL)
		return false;

	mColorIndices = new uchar[mWidth*mHeight];
	mColorTable = new ulong[256];

	if (!Quantize8Bit(mBits, mWidth, mHeight, mColorIndices, mColorTable))
	{
		delete [] mColorIndices;
		mColorIndices = NULL;

		delete [] mColorTable;
		mColorTable = NULL;

		mWantPal = false;

		return false;
	}
	
	delete [] mBits;
	mBits = NULL;

	delete [] mNativeAlphaData;
	mNativeAlphaData = NULL;

	mWantPal = true;

	return true;
}

TriRepGenerator::CoverageGrid::CoverageGrid() { mGrid = NULL; mGridWidth = mGridHeight = 0; } //1948

TriRepGenerator::CoverageGrid::~CoverageGrid() //1950-1953
{
	if (mGrid)
		delete mGrid;
}

void TriRepGenerator::CoverageGrid::InitFromImage(MemoryImage* inImage, Rect* inSrcRect, int inGridWidth, int inGridHeight, bool inAdditive, float inMaxFillPct) //1956-2083 (Correct?)
{
	if (inImage == NULL)
		return;

	int aImageWidth = inImage->mWidth;
	int aCelWidth = inSrcRect->mWidth;
	int aCelHeight = inSrcRect->mHeight;
	float aWidthStep = inGridWidth / aCelWidth;
	float aHeightStep = inGridHeight / aCelHeight;
	int aGridSize = inGridHeight * inGridWidth;
	uchar* aGrid = new uchar[inGridHeight * inGridWidth];
	ZeroMemory(aGrid, inGridHeight * inGridWidth);
	ulong* ptr = &inImage->mBits[inSrcRect->mX + aImageWidth * inSrcRect->mY];
	int aExtraPitch = aImageWidth - aCelWidth;
	if (inAdditive)
	{
		float yAccum = 0.0;
		for (int y = 0; y < aCelHeight; y++)
		{
			int yBase = inGridWidth * (int)yAccum;
			float xAccum = 0.0;
			for (int x = 0; x < aCelWidth; x++)
			{
				if ((*ptr & 0xFFFFFF) != 0 && *ptr >> 24 & 0xFF)
				{
					aGrid[yBase + (int)xAccum] = 1;
					if (x > 0)
						aGrid[yBase + (int)(xAccum - aWidthStep)] = 1;
					if (x < aCelWidth - 1)
						aGrid[yBase + (int)(xAccum + aWidthStep)] = 1;
					if (y > 0)
						aGrid[inGridWidth * (int)(yAccum - aHeightStep) + (int)xAccum] = 1;
					if (y < aCelHeight - 1)
						aGrid[inGridWidth * (int)(yAccum + aHeightStep) + (int)xAccum] = 1;
				}
				xAccum = xAccum + aWidthStep;
				ptr++;
			}
			ptr += aExtraPitch;
			yAccum += aHeightStep;
		}
	}
	else
	{
		float yAccum = 0.0;
		for (int y = 0; y < aCelHeight; y++)
		{
			int yBase = inGridWidth * (int)yAccum;
			float xAccum = 0.0;
			for (int x = 0; x < aCelWidth; x++)
			{
				if (*ptr >> 24 != 0)
				{
					aGrid[yBase + (int)xAccum] = 1;
					if (x > 0)
						aGrid[yBase + (int)(xAccum - aWidthStep)] = 1;
					if (x < aCelWidth - 1)
						aGrid[yBase + (int)(xAccum + aWidthStep)] = 1;
					if (y > 0)
						aGrid[inGridWidth * (int)(yAccum - aHeightStep) + (int)xAccum] = 1;
					if (y < aCelHeight - 1)
						aGrid[inGridWidth * (int)(yAccum + aHeightStep) + (int)xAccum] = 1;
				}
				xAccum = xAccum + aWidthStep;
				ptr++;
			}
			ptr += aExtraPitch;
			yAccum += aHeightStep;
		}
	}
	if (inGridHeight >= 3)
	{
		uchar* aGridPtrAbove = aGrid;
		uchar* aGridPtr = &aGrid[inGridWidth];
		uchar* aGridPtrBelow = &aGrid[inGridWidth + inGridWidth];
		for (int i = 1; i < inGridHeight - 1; i++)
		{
			aGridSize = 0;
			while (aGridSize < inGridWidth)
			{
				if (!*aGridPtr && *aGridPtrAbove && *aGridPtrBelow)
					*aGridPtr = 1;
				aGridSize++;
				aGridPtr++;
				aGridPtrAbove++;
				aGridPtrBelow++;
			}
		}
	}
	int aCheckFilledCount = 0;
	for (int i = 0; i < aGridSize; ++i)
		aCheckFilledCount += aGrid[i];
	float aCheckFilledPct = (double)aCheckFilledCount / (double)aGridSize * 100.0;
	if (inMaxFillPct >= (double)aCheckFilledPct)
	{
		mGrid = aGrid;
		mGridWidth = inGridWidth;
		mGridHeight = inGridHeight;
	}
	else
		delete aGrid;
}

TriRepGenerator::SpanRow::Span::Span() { mGroup = -1; mMonotoneOpen = 1; } //2100

TriRepGenerator::PointGroup::BarGroup::Bar::Bar(int inStartX, int inEndX, int inY) { mStartX = inStartX; mEndX = inEndX; mY = inY; } //2116

TriRepGenerator::PointGroup::BarGroup::BarGroup() { mLastSpan = NULL; } //2124

void TriRepGenerator::PointGroup::InitChainHead(Point* inChainHead) //2150-2154 (Matched)
{
	inChainHead->mType = Point::EPointType::PT_ChainHead;
	inChainHead->mChainPrev = inChainHead;
	inChainHead->mChainNext = inChainHead;
	inChainHead->mChainHead = NULL;
}

TriRepGenerator::PointGroup::PointGroup() {} //2156

TriRepGenerator::PointGroup::~PointGroup() //2159-2163 (Matched)
{
	int aPointCount = mPoints.size();
	for (int i = 0; i < aPointCount; i++)
		delete mPoints[i];
}

void TriRepGenerator::PointGroup::Init() //2166-2169 (Matched)
{
	TriRepGenerator::PointGroup::InitChainHead(&mLeftChain);
	TriRepGenerator::PointGroup::InitChainHead(&mRightChain);
}

void TriRepGenerator::PointGroup::AddChainPoint(PointGroup::Point* inChainHead, int inX, int inY, Point::EPointType inType) //2172-2184 (Matched)
{
	PointGroup::Point* p = new PointGroup::Point;

	p->x = inX;
	p->y = inY;
	p->mType = inType;
	p->mChainNext = inChainHead;
	p->mChainPrev = inChainHead->mChainPrev;
	p->mChainPrev->mChainNext = p->mChainNext->mChainPrev = p;
	p->mChainHead = inChainHead;

	mPoints.push_back(p);
}
void TriRepGenerator::PointGroup::AddLeftChainPoint(int inX, int inY, Point::EPointType inType) { AddChainPoint(&mLeftChain, inX, inY, inType); } //2185
void TriRepGenerator::PointGroup::AddRightChainPoint(int inX, int inY, Point::EPointType inType) { AddChainPoint(&mRightChain, inX, inY, inType); } //2186

void TriRepGenerator::PointGroup::RemoveRedundantPoints() //TODO | 2189-2320
{
	/*int aRemovedCount;
	do
	{
		PointGroup::Point* aChains[2];
		aRemovedCount = 0;
		aChains[0] = &mLeftChain;
		aChains[1] = &mRightChain;
		PointGroup::Point* next;
		for (int iChain = 0; iChain < 2; ++iChain)
		{
			PointGroup::Point* aChainHead = aChains[iChain];
			for (PointGroup::Point* p = aChainHead->mChainNext; p != aChainHead; p = next)
			{
				next = p->mChainNext;
				if (p->mType == Point::EPointType::PT_Default && p->mChainPrev->mType != Point::EPointType::PT_ChainHead && p->mChainNext->mType != Point::EPointType::PT_ChainHead)
				{
					int nextDeltaX = p->mChainNext->x - p->x;
					int prevDeltaX = p->x - p->mChainPrev->x;
					bool canRemove = false;
					if (nextDeltaX)
					{
						if (prevDeltaX)
						{
							float nextSlope = ((double)p->mChainNext->y - p->y) / (double)nextDeltaX;
							float prevSlope = ((double)p->y - p->mChainPrev->y) / (double)prevDeltaX;
							if (fabs(nextSlope - prevSlope) < 0.0000009999999974752427)
								canRemove = true;
						}
					}
					else if (!prevDeltaX)
						canRemove = 1;
					if (canRemove)
					{
						p->mChainNext->mChainPrev = p->mChainPrev;
						p->mChainPrev->mChainNext = p->mChainNext;
						p->mChainPrev = p;
						p->mChainNext = p;
						p->mType = Point::EPointType::PT_Removed;
						++aRemovedCount;
					}
				}
			}
		}
		for (int iChain = 0; iChain < 2; ++iChain)
		{
			PointGroup::Point* aChainHead = aChains[iChain];
			for (PointGroup::Point* p = aChains[iChain]->mChainNext; p != aChains[iChain]; p = next)
			{
				next = aChains[iChain]->mChainNext;
				if (aChains[iChain]->mType == Point::EPointType::PT_Default && next->mType == Point::EPointType::PT_Default && next->mChainNext->mType == PT_Default)
				{
					int firstDeltaX = next->x - j->x;
					int firstDeltaY = next->y - j->y;
					int secondDeltaX = next->mChainNext->x - next->x;
					int secondDeltaY = next->mChainNext->y - next->y;
					if (iChain)
					{
						if (firstDeltaX >= 0 || firstDeltaY != aChains[iChain]->x - next->x || secondDeltaX || secondDeltaY > 4)
						{
							if (secondDeltaX > 0 && secondDeltaY == secondDeltaX && !firstDeltaX && firstDeltaY <= 4)
								next->mType = Point::EPointType::PT_PendingRemove;
						}
						else
							next->mType = Point::EPointType::PT_PendingRemove;
					}
					else if (firstDeltaX <= 0 || firstDeltaY != firstDeltaX || secondDeltaX || secondDeltaY > 4)
					{
						if (secondDeltaX < 0 && secondDeltaY == next->x - next->mChainNext->x && !firstDeltaX && firstDeltaY <= 4)
							next->mType = Point::EPointType::PT_PendingRemove;
					}
					else
						next->mType = Point::EPointType::PT_PendingRemove;
				}
			}
		}
		//for (? = ?)
	} while (aRemovedCount);*/
}

TriRepGenerator::SpanSet::SpanSet() { mGroupCount = 0; } //2330

void TriRepGenerator::SpanSet::InitFromCoverageGrid(TriRepGenerator::CoverageGrid* inGrid) //2237-2381
{
	if (inGrid == NULL)
		return;

	uchar* aGridBits = inGrid->mGrid;
	int aGridWidth = inGrid->mGridWidth;
	int aGridHeight = inGrid->mGridHeight;
	mRows.resize(aGridHeight);
	for (int y = 0; y < aGridHeight; ++y)
	{
		uchar* gridSpan = &aGridBits[aGridWidth * y];
		SpanRow* row = &mRows[y];
		float vStart = (double)y / (double)aGridHeight;
		float vEnd = (double)(y + 1) / (double)aGridHeight;
		int spanStart = -1;
		int spanEnd = -1;
		for (int x = 0; x < aGridWidth + 1; x++)
		{
			if (x < aGridWidth && gridSpan[x])
			{
				if (spanStart < 0)
					spanStart = x;
				spanEnd = x;
			}
			else if (spanStart >= 0)
			{
				float uStart = (double)spanStart / (double)aGridWidth;
				TriRepGenerator::SpanRow::Span s;
				s.mStartX = spanStart;
				s.mEndX = spanEnd;
				s.mEndY = y;
				s.mStartY = y;
				row->mSpans.push_back(s);
				spanStart = -1;
				spanEnd = -1;
			}
		}
	}
}

void TriRepGenerator::SpanSet::GroupMonotoneSpans() //2384-2429
{
	int aRowCount = mRows.size();
	int aCurGroup = 0;
	if (aRowCount > 0)
	{
		int aSpanCount = mRows[0].mSpans.size();
		for (int iSpan = 0; iSpan < aSpanCount; ++iSpan)
			mRows[0].mSpans[iSpan].mGroup = aCurGroup++;
	}
	for (int iRow = 1; iRow < aRowCount; ++iRow)
	{
		SpanRow* aPrevRow = &mRows[iRow - 1];
		SpanRow* aCurRow = &mRows[iRow];
		int aPrevSpanCount = aPrevRow->mSpans.size();
		int aCurSpanCount = aPrevRow->mSpans.size();
		for (int iCurSpan = 0; iCurSpan < aCurSpanCount; ++iCurSpan)
		{
			SpanRow::Span* r = &aCurRow->mSpans[iCurSpan];
			for (int iPrevSpan = 0; iPrevSpan < aPrevSpanCount; ++iPrevSpan)
			{
				SpanRow::Span* p = &aPrevRow->mSpans[iPrevSpan];
				if (p->mMonotoneOpen && r->mEndX >= p->mStartX && r->mStartX <= p->mEndX)
				{
					r->mGroup = p->mGroup;
					p->mMonotoneOpen = false;
					break;
				}
			}
			if (r->mGroup < 0)
				r->mGroup = aCurGroup++;
		}
	}
	mGroupCount = aCurGroup;
}

bool TriRepGenerator::SpanSet::IsRangeOpen(int inStartX, int inEndX, int inY) //2432-2444
{
	SpanRow* aRow = &mRows[inY];
	int aSpanCount = aRow->mSpans.size();
	for (int iSpan = 0; iSpan < aSpanCount; iSpan++)
	{
		SpanRow::Span* s = &aRow->mSpans[iSpan];
		if (s->mEndX >= inStartX && s->mStartX <= inEndX)
			return false;
	}
	return true;
}

void TriRepGenerator::SpanSet::GeneratePointGroups(std::vector<PointGroup>& outPointGroups) //TODO | 2447-2532
{
//	std::vector<PointGroup::BarGroup> aBarGroups;
//	aBarGroups.resize(mGroupCount);
//	int aRowCount = mRows.size();
//	for (int iRow = 0; iRow < aRowCount; iRow++)
//	{
//		SpanRow* aRow = &mRows[iRow];
//		int aSpanCount = aRow->mSpans.size();
//		for (int iSpan = 0; iSpan < aSpanCount; ++iSpan)
//		{
//			SpanRow::Span* r = &aRow->mSpans[iSpan];
//			PointGroup::BarGroup* bg = &aBarGroups[r->mGroup];
//			if (bg->mLastSpan)
//			{
//				/*if (r->mEndX <= bg->mLastSpan->mEndX)
//					inEndX = bg->mLastSpan->mEndX;
//				else
//					inEndX = r->mEndX;(/
//				/*if (r->mStartX >= bg->mLastSpan->mStartX)
//					PointGroup::BarGroup::Bar(bg->mLastSpan->mStartX, bg->mLastSpan->mEndX, r->mStartY);
//				else
//					PointGroup::BarGroup::Bar(r->mStartX, r->mEndX, r->mStartY);*/
//				//PointGroup::BarGroup::Bar(max(r->mStartX, bg->mLastSpan->mStartX), inEndX, r->mStartY);
//				//bg->mBars.push_back();
//				bg->mBars.push_back(PointGroup::BarGroup::Bar::Bar(min(r->mStartX, bg->mLastSpan->mStartX), max(r->mEndX, bg->mLastSpan->mEndX), r->mStartY));
//			}
//			else
//			{
//				//PointGroup::BarGroup::Bar(r->mStartX, r->mEndX, r->mStartY);
//				//bg->mBars.push_back();
//				bg->mBars.push_back(PointGroup::BarGroup::Bar::Bar(r->mStartX, r->mEndX, r->mStartY));
//			}
//			bg->mLastSpan = r;
//		}
//	}
//	std::vector<PointGroup&> outPointGroups;
//	outPointGroups.resize(mGroupCount);
//	for (int iGroup = 0; iGroup < this->mGroupCount; ++iGroup)
//	{
//		aBarGroups[iGroup].mBars.push_back();
//		PointGroup* pg = &outPointGroups[iGroup];
//		pg->Init();
//		//?
//		//if (&aBarGroups[iGroup])
//		//{
//			//aBarGroups[iGroup].
//		//}
//		int aBarCount = aBarGroups[iGroup].mBars.size();
//	}
}

int TriRepGenerator::SpanSet::GetWinding(int p0x, int p0y, int p1x, int p1y, int p2x, int p2y) //2536-2544
{
	return (p2y - p0y) * (p1x - p0x) - (p2x - p0x) * (p1y - p0y);
}

void TriRepGenerator::SpanSet::AddTri(std::vector<MemoryImage::TriRep::Tri&> outTris, PointGroup::Point** inTri, int inWidth, int inHeight, int inGroup) //2547-2582
{
	PointGroup::Point* tri[3];
	if (GetWinding((*inTri)->x, (*inTri)->y, inTri[1]->x, inTri[1]->y, inTri[2]->x, inTri[2]->y) >= 0)
	{
		tri[0] = *inTri;
		tri[1] = inTri[1];
		tri[2] = inTri[2];
	}
	else
	{
		tri[0] = *inTri;
		tri[1] = inTri[2];
		tri[2] = inTri[1];
	}
	MemoryImage::TriRep::Tri(tri[0]->x / inWidth, tri[0]->y / inHeight, tri[1]->x / inWidth, tri[1]->y / inHeight, tri[2]->x / inWidth, tri[2]->y / inHeight);
}

void TriRepGenerator::SpanSet::TriangulatePointGroups(std::vector<PointGroup>& inPointGroups, std::vector<MemoryImage::TriRep::Tri>& outTris) //TODO | 2585-2746
{
	struct Local
	{
		static int PointCompare(PointGroup::Point** a, PointGroup::Point** b) //Idk | 2660-2770
		{
			PointGroup::Point* p1 = *a;
			PointGroup::Point* p2 = *b;
			if ((*a)->y < (*b)->y)
				return -1;
			if (p1->y > p2->y)
				return 1;
			if (p1->x > p2->x)
				return 1;
			return -1;
		};
	};
	/*
	int aQuadResDim = mRows.size();
	for (int iPointGroup = 0; inPointsGroup.size(); iPointGroup++)
	{
		PointGroup* pg = &inPointGroups[iPointGroup];
		std::vector<PointGroup::Point*> aSortedPoints;
        PointGroup::Point* aChains[2];
		aChains[0] = &pg->mLeftChain;
		aChains[1] = &pg->mRightChain;
		for (int iChain = 0; iChain < 2; iChain++)
		{
			PointGroup::Point* aChainHead = aChains[iChain];
			for (PointGroup::Point* p = aChainHead->mChainNext; p != aChainHead; p = p->mChainNext)
			{
				PointGroup::Point* tri[3];
				memset(tri, 0, sizeof(tri));
				if (p->mType == PointGroup::Point::PT_EarStart)
				{
					tri[0] = p;
					tri[1] = p->mChainNext;
					tri[2] = p->mChainNext->mChainNext;
				}
				else if (p->mType == PointGroup::Point::PT_EarEnd)
				{
					tri[0] = p->mChainPrev->mChainPrev;
					tri[1] = p->mChainPrev;
					tri[2] = p;
				}
				if (tri[0])
				{
					AddTri(outTris, tri, aQuadResDim, aQuadResDim, 0);
					PointGroup::Point* r = tri[1];
					tri[1]->mChainNext->mChainPrev = tri[1]->mChainPrev;
					r->mChainPrev->mChainNext = r->mChainNext;
					r->mChainPrev = r;
					r->mChainNext = r;
					r->mType = PointGroup::Point::PT_Removed;
					p->mType = PointGroup::Point::PT_Default;
				}
			}
			int curY = -1;
			for (PointGroup::Point* p = aChainHead->mChainNext; p != aChainHead; p = p->mChainNext)
			{
				assert(p->y > curY); //2646 | 2668 BejLiveWin8
				curY = p->y;
				aSortedPoints.push_back(p);
			}
		}
        
		if (aSortedPoints.size() >= 3)
		{
			qsort(aSortedPoints[0], aSortedPoints.size(), 4, Local::PointCompare);
			int curY = -1;
			for (int i = 0; aSortedPoints.size(); i++)
			{
				assert(aSortedPoints[i]->y >= curY); //2679 | 2701 BejLiveWin8
				curY = aSortedPoints[i]->y;
			}
			std::vector<PointGroup::Point*> aPointStack;
			int aPointCount = aSortedPoints.size();
			aPointStack.push_back(aSortedPoints[0]);
			aPointStack.push_back(aSortedPoints[1]);
			for (int iPoint = 2; iPoint < aPointCount; iPoint++)
			{
				PointGroup::Point* u = aSortedPoints[iPoint];
				PointGroup::Point* vBottom = aPointStack.front();
				PointGroup::Point* vTop = aPointStack.back();
				bool adjBottom = u->mChainNext == vBottom || u->mChainPrev == vBottom;
				bool adjTop = u->mChainNext == vTop || u->mChainPrev == vTop;
				assert(adjBottom ^ adjTop); //2698 | 2720 BejLiveWin8
                
				if (adjBottom || !adjTop)
				{
                    assert(u->mChainHead != vTop->mChainHead); //2703 | 2725 BejLiveWin8
                    int aStackSize = aPointStack.size();
                    for (iStackPoint = 1; iStackPoint < aStackSize; iStackPoint++)
                    {
                        PointGroup::Point* tri[3];
				        memset(tri, 0, sizeof(tri));
                        tri[0] = aPointStack[iStackPoint - 1];
                        tri[1] = aPointStack[iStackPoint];
					    tri[2] = u;
                        AddTri(tri, aQuadResDim, aQuadResDim, 1);
                    }
                    aPointStack.clear();
                    aPointStack.push_back(vTop);
                    aPointStack.push_back(u);
                }
                else
				{
					assert(u->mChainHead == vTop->mChainHead); //2720 | 2742 BejLiveWin8
					while (aPointStack.size() > 1) //aPointStack is only defined once, assuming not a local var
					{
						if (u->mChainHead == &pg->mLeftChain)
						{
							if (TriRepGenerator::SpanSet::GetWinding((aPointStack.back() - 1)->x, (aPointStack.back() - 1)->y, (aPointStack.back()), (aPointStack.back()[1]), u->x, u->y) > 0) //?
								break;
						}
                        
						else
						{
							assert(u->mChainHead == &pg->mRightChain); //2732 | 2754 BejLiveWin8
							if (TriRepGenerator::SpanSet::GetWinding((aPointStack.back() - 1)->x, (aPointStack.back() - 1)->y, (aPointStack.back()), (aPointStack.back()[1]), u->x, u->y) > 0) //?
								break;
							TriRepGenerator::SpanSet::AddTri(outTris, &aPointStack, aQuadResDim, aQuadResDim, 2);
							aPointStack.pop_back();
						}
					}
                    aPointStack.push_back(u);
				}
			}
            delete aPointStack; //?
            delete aSortedPoints; //?
		}
        else
            delete aSortedPoints; //?
	}
    */
}

void TriRepGenerator::SpanSet::ConvertToTris(std::vector<MemoryImage::TriRep::Tri>& outTris) //2749-2753 (Matched)
{
	std::vector<PointGroup> aPointGroups;
	GeneratePointGroups(aPointGroups);
	TriangulatePointGroups(aPointGroups, outTris);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
TriRepGenerator::TriRepGenerator() {} //2757

bool TriRepGenerator::GenerateLevel(MemoryImage::TriRep::Level* inLevel, MemoryImage* inImage, int inGridWidth, int inGridHeight, bool inAdditive) //2761-2817
{
	if (inLevel == NULL)
		return false;

	inLevel->mDetailX = inGridWidth;
	inLevel->mDetailY = inGridHeight;
	inLevel->mRegionWidth = inImage->mNumCols;
	inLevel->mRegionHeight = inImage->mNumRows;
	for (int ry = 0; ry < inLevel->mRegionHeight; ++ry)
	{
		for (int rx = 0; rx < inLevel->mRegionWidth; ++rx)
		{
			Rect aSrcRect;
			aSrcRect.mWidth = inImage->mWidth / inLevel->mRegionWidth;
			aSrcRect.mHeight = inImage->mHeight / inLevel->mRegionHeight;
			aSrcRect.mX = aSrcRect.mWidth * rx;
			aSrcRect.mY = aSrcRect.mHeight * ry;
			MemoryImage::TriRep::Level::Region* aRegion;
			inLevel->mRegions.push_back(*aRegion);
			aRegion = &inLevel->mRegions.back();
			aRegion->mRect = aSrcRect;
			CoverageGrid aCoverageGrid;
			aCoverageGrid.InitFromImage(inImage, &aSrcRect, inGridWidth, inGridHeight, inAdditive, 85.0);
			if (aCoverageGrid.mGrid)
			{
				SpanSet aRegSpanSet;
				aRegSpanSet.InitFromCoverageGrid(&aCoverageGrid);
				aRegSpanSet.GroupMonotoneSpans();
				aRegSpanSet.ConvertToTris(aRegion->mTris);
				if (!aRegion->mTris.empty())
				{
					float uScale = (double)aSrcRect.mWidth / (double)inImage->mWidth;
					float vScale = (double)aSrcRect.mHeight / (double)inImage->mHeight;
					float uOffset = (double)aSrcRect.mX / (double)inImage->mWidth;
					float vOffset = (double)aSrcRect.mY / (double)inImage->mHeight;
					int aTriCount = aRegion->mTris.size();
					MemoryImage::TriRep::Tri* aTri = &aRegion->mTris.front();
					int iTri = 0;
					while (iTri < aTriCount)
					{
						for (int j = 0; j < 3; ++j)
						{
							MemoryImage::TriRep::Tri::Point* p = &aTri->p0 + j;
							p->u = p->u * uScale + uOffset;
							p->v = p->v * vScale + vOffset;
						}
						++iTri;
						++aTri;
					}
				}
			}
		}
	}
	return !inLevel->mRegions.empty();
}

bool MemoryImage::BuildTriRep(TriRep* inTriRep, bool inAdditive, bool inForceRebuild) //C++ only | 2821-2849
{
	if (!inTriRep->mLevels.empty() && !inForceRebuild)
		return true;
	if (HasImageFlag(ImageFlag_NoTriRep | ImageFlag_RTUseDefaultRenderMode))
		return true;
	inTriRep->mLevels.clear();
	if (mWidth < 32 || mHeight < 32)
		return true;
	GetBits();
	if (!this->mBits)
		return false;
	TriRepGenerator trg;
	TriRep::Level* level;
	inTriRep->mLevels.push_back(*level);
	level = &inTriRep->mLevels.back();
	if (!trg.GenerateLevel(level, this, 16, 16, inAdditive))
		inTriRep->mLevels.pop_back();
	return true;
}

bool MemoryImage::BuildTriReps(bool inForceRebuild) //C++ only | 2851-2857
{
	bool builtNormal = !mHasAlpha && !mHasTrans || BuildTriRep(&mNormalTriRep, false, inForceRebuild);
	bool builtAdditive = BuildTriRep(&mAdditiveTriRep, true, inForceRebuild);


	return builtNormal && builtAdditive;
}

void MemoryImage::TriRep::Level::GetRegionTris(std::vector<Tri>& outTris, MemoryImage* inImage, const Rect& inSrcRect, bool inAllowRotation) //C++ only | 2860-2881
{
	if (mRegions.empty())
		return;
	assert(mRegions.size() == mRegionWidth*mRegionHeight); //2863
	if (mRegionWidth != inImage->mNumCols && mRegionHeight != inImage->mNumRows)
		return;

	int aCelWidth = inImage->mWidth / mRegionWidth;
	int aCelHeight = inImage->mHeight / mRegionHeight;
	if (inSrcRect.mWidth != aCelWidth && inSrcRect.mHeight != aCelHeight)
		return;

	int aCelX = inSrcRect.mX / aCelWidth;
	int aCelY = inSrcRect.mY / aCelHeight;
	if (aCelX < this->mRegionWidth && aCelY < this->mRegionHeight)
	{
		Region* aRegion = &mRegions[aCelX + mRegionWidth * aCelY];
		assert(inSrcRect == aRegion->mRect); //2877

		outTris = aRegion->mTris;
	}
}

MemoryImage::TriRep::Level* MemoryImage::TriRep::GetLevelForScreenSpaceUsage(float inUsageFrac, bool inAllowRotation) //C++ only | 2884-2893
{
	if (mLevels.empty())
		return NULL;
	for (MemoryImage::TriRep::Level* level = &mLevels.back(); level >= &mLevels.front(); level--)
	{
		if (inUsageFrac > 0.00100000004749745)
			return level;
	}
	return NULL;
}