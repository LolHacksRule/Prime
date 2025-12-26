#include "Image.h"
#include "Graphics.h"

using namespace Sexy;

Image::Image() : //12-21
	mImageFlags(ImageFlags(NULL)),
	mRenderData(NULL)
{
	mWidth = 0;
	mHeight = 0;

	mNumRows = 1;
	mNumCols = 1;

	mAnimInfo = NULL;
	mDrawn = false;
}

Image::Image(const Image& theImage) : //30-36
	mImageFlags(theImage.mImageFlags),
	mRenderData(NULL),
	mWidth(theImage.mWidth),
	mHeight(theImage.mHeight),
	mNumRows(theImage.mNumRows),
	mNumCols(theImage.mNumCols)
{
	mDrawn = false;
	if (theImage.mAnimInfo != NULL)
		mAnimInfo = new AnimInfo(*theImage.mAnimInfo);
	else
		mAnimInfo = NULL;
}

Image::~Image() //39-41
{
	delete mAnimInfo;
}

int Image::GetWidth() //44-46
{
	return mWidth;
}

int	Image::GetHeight() //49-51
{
	return mHeight;
}

int Image::GetCelHeight() //54-56
{
	return mHeight / mNumRows;
}

int Image::GetCelWidth() //59-61
{
	return mWidth / mNumCols;
}

int Image::GetCelCount() //64-66
{
	return mNumRows * mNumCols;
}

Rect Image::GetCelRect(int theCel) //69-76
{
	int h = GetCelHeight();
	int w = GetCelWidth();
	int x = (theCel % mNumCols) * w;
	int y = (theCel / mNumCols) * h;

	return Rect(x, y, w, h);
}

Rect Image::GetCelRect(int theCol, int theRow) //79-86
{
	int h = GetCelHeight();
	int w = GetCelWidth();
	int x = theCol * w;
	int y = theRow * h;

	return Rect(x, y, w, h);
}

void Image::CreateRenderData() //Bool in XNA? | 89-93
{
	MemoryImage* aMemoryImage = AsMemoryImage();
	if (aMemoryImage != NULL && gSexyAppBase->mGraphicsDriver != NULL && gSexyAppBase->mGraphicsDriver->GetRenderDevice3D() != NULL)
		gSexyAppBase->mGraphicsDriver->GetRenderDevice3D()->CreateImageRenderData(aMemoryImage);
}

AnimInfo::AnimInfo() //96-100
{
	mAnimType = AnimType_None;
	mFrameDelay = 1;
	mNumCels = 1;
}

void AnimInfo::SetPerFrameDelay(int theFrame, int theTime) //103-108
{
	if ((int)mPerFrameDelay.size()<=theFrame)
		mPerFrameDelay.resize(theFrame+1);

	mPerFrameDelay[theFrame] = theTime;
}

void AnimInfo::Compute(int theNumCels, int theBeginFrameTime, int theEndFrameTime) //111-158
{
	int i;

	mNumCels = theNumCels;
	if (mNumCels<=0)
		mNumCels = 1;

	if (mFrameDelay<=0)
		mFrameDelay = 1;

	if (mAnimType==AnimType_PingPong && mNumCels>1)
	{
		mFrameMap.resize(theNumCels*2-2);
		int index = 0;
		for (i=0; i<theNumCels; i++)
			mFrameMap[index++] = i;
		for (i=theNumCels-2; i>=1; i--)
			mFrameMap[index++] = i;
	}

	if (!mFrameMap.empty())
		mNumCels = (int)mFrameMap.size();

	if (theBeginFrameTime>0) 
		SetPerFrameDelay(0,theBeginFrameTime);

	if (theEndFrameTime>0)
		SetPerFrameDelay(mNumCels-1,theEndFrameTime);

	if (!mPerFrameDelay.empty())
	{
		mTotalAnimTime = 0;
		mPerFrameDelay.resize(mNumCels);

		for (i=0; i<mNumCels; i++)
		{
			if (mPerFrameDelay[i]<=0)
				mPerFrameDelay[i] = mFrameDelay;
				
			mTotalAnimTime += mPerFrameDelay[i];
		}
	}
	else
		mTotalAnimTime = mFrameDelay*mNumCels;

	if (!mFrameMap.empty())
		mFrameMap.resize(mNumCels);
}
	
int AnimInfo::GetPerFrameCel(int theTime) //161-170
{
	for (int i=0; i<mNumCels; i++)
	{
		theTime -= mPerFrameDelay[i];
		if (theTime<0)
			return i;
	}

	return mNumCels-1;
}



int AnimInfo::GetCel(int theTime) //175-196
{
	if (mAnimType==AnimType_Once && theTime>=mTotalAnimTime)
	{
		if (!mFrameMap.empty())
			return mFrameMap[mFrameMap.size()-1];
		else
			return mNumCels-1;
	}

	theTime = theTime%mTotalAnimTime;

	int aFrame;
	if (!mPerFrameDelay.empty())
		aFrame = GetPerFrameCel(theTime);
	else
		aFrame = (theTime/mFrameDelay)%mNumCels;

	if (mFrameMap.empty())
		return aFrame;
	else
		return mFrameMap[aFrame];	
}

int	Image::GetAnimCel(int theTime) //199-204
{
	if (mAnimInfo==NULL)
		return 0;
	else
		return mAnimInfo->GetCel(theTime);
}

Rect Image::GetAnimCelRect(int theTime) //207-216
{
	Rect aRect;
	int aCel = GetAnimCel(theTime);
	int aCelWidth = GetCelWidth();
	int aCelHeight = GetCelHeight();
	if (mNumCols>1)
		return Rect(aCel*aCelWidth,0,aCelWidth,mHeight);
	else
		return Rect(0,aCel*aCelHeight,mWidth,aCelHeight);
}

void Image::CopyAttributes(Image *from) //219-226
{
	mNumCols = from->mNumCols;
	mNumRows = from->mNumRows;
	delete mAnimInfo;
	mAnimInfo = NULL;
	if (from->mAnimInfo != NULL)
		mAnimInfo = new AnimInfo(*from->mAnimInfo);
}

//GetGraphics is gone, it's in Transmension's fork of Prime but not the original Prime

//FillRect, ClearRect, DrawRect, DrawLine are moved to MemoryImage and DeviceImage

//FillScanlines was moved to RenderDevice