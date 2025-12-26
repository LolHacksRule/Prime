#include "SharedImage.h"
#include "DeviceImage.h"
#include "SexyAppBase.h"

using namespace Sexy;

SharedImage::SharedImage() //8-12
{
	mImage = NULL;
	mRefCount = 0;
	mLoading = false;
}

std::string SharedImage::ToString() //15-17
{
	return StrFormat("RefCount(%d):%s", mRefCount, mImage ? mImage->ToString().c_str() : "NULL");
}

SharedImageRef::SharedImageRef(const SharedImageRef& theSharedImageRef) //20-26
{
	mSharedImage = theSharedImageRef.mSharedImage;
	if (mSharedImage != NULL)
		mSharedImage->mRefCount++;
	mUnsharedImage = theSharedImageRef.mUnsharedImage;	
	mOwnsUnshared = false;
}

SharedImageRef::SharedImageRef() //29-33
{
	mSharedImage = NULL;
	mUnsharedImage = NULL;
	mOwnsUnshared = false;
}

SharedImageRef::SharedImageRef(SharedImage* theSharedImage) //36-43
{
	mSharedImage = theSharedImage;
	if (theSharedImage != NULL)
		mSharedImage->mRefCount++;

	mUnsharedImage = NULL;
	mOwnsUnshared = false;
}

SharedImageRef::~SharedImageRef() //46-48
{
	Release();
}

void SharedImageRef::Release() //51-61
{	
	if (mOwnsUnshared)
		delete mUnsharedImage;
	mUnsharedImage = NULL;
	if (mSharedImage != NULL)
	{
		if (--mSharedImage->mRefCount == 0)
			gSexyAppBase->mCleanupSharedImages = true;
	}
	mSharedImage = NULL;
}

SharedImageRef& SharedImageRef::operator=(const SharedImageRef& theSharedImageRef) //64-71
{
	Release();
	mSharedImage = theSharedImageRef.mSharedImage;
	mUnsharedImage = theSharedImageRef.mUnsharedImage;
	if (mSharedImage != NULL)
		mSharedImage->mRefCount++;
	return *this;
}

SharedImageRef&	SharedImageRef::operator=(SharedImage* theSharedImage) //74-79
{
	Release();
	mSharedImage = theSharedImage;
	mSharedImage->mRefCount++;
	return *this;
}

SharedImageRef& SharedImageRef::operator=(MemoryImage* theUnsharedImage) //82-86
{
	Release();
	mUnsharedImage = theUnsharedImage;	
	return *this;
}

MemoryImage* SharedImageRef::operator->() //89-91
{
	return (MemoryImage*) *this;
}


SharedImageRef::operator Image*() //95-97
{	
	return (MemoryImage*) *this;
}

SharedImageRef::operator MemoryImage*() //100-105
{
	if (mUnsharedImage != NULL)
		return mUnsharedImage;
	else
		return (DeviceImage*) *this;
}

SharedImageRef::operator DeviceImage*() //108-113
{
	if (mSharedImage != NULL)
		return mSharedImage->mImage;
	else
		return NULL;
}

std::string SharedImageRef::ToString() //C++ only | 116-123
{
	if (mSharedImage != NULL)
		return StrFormat("Shared:%s", mSharedImage->ToString().c_str());
	else if (mUnsharedImage != NULL)
		return StrFormat("Unshared:%s", mUnsharedImage->ToString().c_str());
	else
		return "NULL";
}
