#include "SharedRenderTarget.h"
#include "Debug.h"
#include "Image.h"

using namespace Sexy;

SharedRenderTarget::SharedRenderTarget() : mImage(NULL), mScreenSurface(NULL), mLockHandle(0) {} //14
SharedRenderTarget::~SharedRenderTarget() //16-18
{
    DBG_ASSERTE((mImage == 0) && "SharedRenderTarget deleted while locked; please call Unlock() first"); //17 | 19 BejLiveWin8
}

DeviceImage* SharedRenderTarget::Lock(int theWidth, int theHeight, ulong additionalD3DFlags, const char* debugTag) //21-28, Correct?
{
	Unlock();

	ulong aImpliedFlags = ImageFlag_RenderTarget; //?

	gSexyAppBase->GetSharedRenderTargetPool()->Acquire(*this, theWidth, theHeight, additionalD3DFlags | aImpliedFlags, debugTag);

	return mImage;
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
DeviceImage* SharedRenderTarget::LockScreenImage(const char* debugTag, ulong flags)
#else
DeviceImage* SharedRenderTarget::LockScreenImage(const char* debugTag) //30-46
#endif
{
	IGraphicsDriver* aDriver = gSexyAppBase->mGraphicsDriver;
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	if (aDriver->GetRenderDevice3D()->GetCapsFlags() & CAPF_LastLockScreenImage || flags & 1 && Lock(aDriver->GetScreenImage()->mWidth, aDriver->GetScreenImage()->mHeight, 0, debugTag) == NULL)
#else
	if (!Lock(aDriver->GetScreenImage()->mWidth, aDriver->GetScreenImage()->mHeight, 0, debugTag))
#endif
		return NULL;
	if (aDriver->GetRenderDevice3D() != NULL)
	{
#ifdef _SEXYDECOMP_USE_LATEST_CODE
		if ((aDriver->GetRenderDevice3D().GetCapsFlags() & CAPF_CopyScreenImage)
		{
			mGraphicsDriver.GetRenderDevice3D().CopyScreenImage(this.mImage, flags);
		}
		else
		{
#endif
		Image* aResult = aDriver->GetRenderDevice3D()->SwapScreenImage(mImage, mScreenSurface);
		DBG_ASSERTE(aResult == mImage); //40
#ifdef _SEXYDECOMP_USE_LATEST_CODE
		}
#endif
	}
	gSexyAppBase->GetSharedRenderTargetPool()->UpdateEntry(*this);
	return mImage;
}

bool SharedRenderTarget::Unlock() //49-56 | Correct?
{
	if (!mLockHandle)
		return false;

	gSexyAppBase->GetSharedRenderTargetPool()->Unacquire(*this);

	return true;
}

DeviceImage* SharedRenderTarget::GetCurrentLockImage() //59-64
{
	if (mLockHandle == 0)
		return NULL;

	return mImage;
}

SharedRenderTarget::Pool::Pool() //67-68
{
}
SharedRenderTarget::Pool::~Pool() //70-85
{
	int anEntryCount = mEntries.size();
	for (int iEntry = 0; iEntry < anEntryCount; ++iEntry)
	{
		Entry* aEntry = &mEntries[iEntry];
		DBG_ASSERTE((aEntry->mLockOwner == 0) && "SharedRenderTarget::Pool deleted while an image was still locked"); //76
		delete aEntry->mImage; //Do we need an if?
		if (aEntry->mScreenSurface != NULL)
			aEntry->mScreenSurface->Release();
	}
	mEntries.clear();
}

void SharedRenderTarget::Pool::Acquire(SharedRenderTarget& outTarget, int theWidth, int theHeight, ulong theD3DFlags, const char* debugTag) //88-138
{
	int anEntryCount = mEntries.size();
	for (int iEntry = 0; iEntry < anEntryCount; iEntry++)
	{
		Entry* aEntry = &mEntries[iEntry];
		if (!aEntry->mLockOwner && aEntry->mImage->mWidth == theWidth && aEntry->mImage->mHeight == theHeight && aEntry->mImage->GetImageFlags() == theD3DFlags)
		{
			outTarget.mImage = aEntry->mImage;
			outTarget.mScreenSurface = aEntry->mScreenSurface;
			outTarget.mLockHandle = iEntry + 1;
			aEntry->mLockOwner = &outTarget;
			aEntry->mLockDebugTag = debugTag ? debugTag : "NULL";
		}
	}
	mEntries.push_back(Entry());
	Entry* aEntry = &mEntries.back();
	int iEntry = anEntryCount++;
	aEntry->mImage = new DeviceImage();
	aEntry->mImage->AddImageFlags(theD3DFlags);
	aEntry->mImage->SetImageMode(false, false);
	aEntry->mImage->CreateRenderData();
	aEntry->mScreenSurface = NULL;
	Graphics g(aEntry->mImage);
	Graphics3D* g3D = g.Get3D();
	if (g3D)
		g3D->ClearColorBuffer(Color(0, 0, 0, 0));
	outTarget.mImage = aEntry->mImage;
	outTarget.mScreenSurface = aEntry->mScreenSurface;
	outTarget.mLockHandle = iEntry + 1;
	aEntry->mLockOwner = &outTarget;
	aEntry->mLockDebugTag = debugTag ? debugTag : "NULL";
}
void SharedRenderTarget::Pool::UpdateEntry(SharedRenderTarget& inTarget) //140-152
{
	if (inTarget.mLockHandle == 0)
		return;

	int iEntry = inTarget.mLockHandle - 1;
	DBG_ASSERTE((iEntry < (int)mEntries.size()) && "SharedRenderTarget::Pool UpdateEntry lock handle is out of bounds"); //145
	Entry* aEntry = &mEntries[iEntry];
	
	DBG_ASSERTE((aEntry->mLockOwner != NULL) && "SharedRenderTarget::Pool UpdateEntry lock handle refers to an image that is not currently locked"); //148
	DBG_ASSERTE((inTarget.mImage == aEntry->mImage) && "SharedRenderTarget::Pool UpdateEntry image mismatch"); //149

	aEntry->mScreenSurface = inTarget.mScreenSurface;
}
void SharedRenderTarget::Pool::Unacquire(SharedRenderTarget& ioTarget) //154-170
{
	if (ioTarget.mLockHandle == 0)
		return;

	int iEntry = ioTarget.mLockHandle - 1;
	DBG_ASSERTE((iEntry < (int)mEntries.size()) && "SharedRenderTarget::Pool Unacquire lock handle is out of bounds"); //159
	Entry* aEntry = &mEntries[iEntry];

	DBG_ASSERTE((aEntry->mLockOwner != 0) && "SharedRenderTarget::Pool Unacquire lock handle refers to an image that is not currently locked"); //162
	DBG_ASSERTE((ioTarget.mImage == aEntry->mImage) && "SharedRenderTarget::Pool Unacquire image mismatch"); //163

	ioTarget.mImage = NULL;
	ioTarget.mScreenSurface = NULL;
	ioTarget.mLockHandle = 0;
	aEntry->mLockOwner = NULL;
	aEntry->mLockDebugTag = "";
}
void SharedRenderTarget::Pool::InvalidateSurfaces() //172-187
{
	int anEntryCount = mEntries.size();
	for (int iEntry = 0; iEntry < anEntryCount; iEntry++)
	{
		Entry* aEntry = &mEntries[iEntry];
		if (aEntry->mScreenSurface)
		{
			aEntry->mScreenSurface->Release();
			aEntry->mScreenSurface = NULL;
		}
		if (aEntry->mLockOwner)
			aEntry->mLockOwner->mScreenSurface = NULL;
	}
}
void SharedRenderTarget::Pool::InvalidateDevice() //189-208
{
	std::vector<SharedRenderTarget*> aLockedSRTs;
	int anEntryCount = mEntries.size();
	for (int iEntry = 0; iEntry < anEntryCount; iEntry++)
	{
		Entry* aEntry = &mEntries[iEntry];
		if (aEntry->mImage)
			gSexyAppBase->Remove3DData(aEntry->mImage);
		if (aEntry->mImage)
			aLockedSRTs.push_back(aEntry->mLockOwner);
	}
	for (int i = 0; i < aLockedSRTs.size(); i++)
		aLockedSRTs[i]->Unlock();
	InvalidateSurfaces();
}
std::string SharedRenderTarget::Pool::GetInfoString() //210-246
{
	int aLockedCount = 0;
	int aFullSizeCount = 0;
	int aHalfSizeCount = 0;
	int aQuarterSizeCount = 0;
	int aOtherSizeCount = 0;
	int anEntryCount = mEntries.size();
	for (int iEntry = 0; iEntry < anEntryCount; iEntry++)
	{
		Entry* aEntry = &mEntries[iEntry];
		if (aEntry->mLockOwner)
			++aLockedCount;
		int aImgWidth = aEntry->mImage->mWidth;
		int aImgHeight = aEntry->mImage->mHeight;
		int aAppWidth = gSexyAppBase->mWidth;
		int aAppHeight = gSexyAppBase->mHeight;
		if (aImgWidth == aAppWidth && aImgHeight == aAppHeight)
			aFullSizeCount++;
		else if (aImgWidth == aAppWidth / 2 && aImgHeight == aAppHeight / 2)
			aHalfSizeCount++;
		else if (aImgWidth == aAppWidth / 4 && aImgHeight == aAppHeight / 4)
			aQuarterSizeCount++;
		else
			aOtherSizeCount++;
	}
	return StrFormat("Total:%d (%d Full, %d Half, %d Quarter, %d Other); Locked:%d", anEntryCount, aFullSizeCount, aHalfSizeCount, aQuarterSizeCount, aOtherSizeCount, aLockedCount);
}