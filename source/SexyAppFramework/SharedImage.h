#ifndef __SHARED_IMAGE_H__
#define __SHARED_IMAGE_H__

#include "Common.h"

namespace Sexy
{

class Image;
class DeviceImage;
class MemoryImage;

class SharedImage
{
public:
	DeviceImage*				mImage;
	int							mRefCount;		
	bool						mLoading;		

	SharedImage();
	static const char*			REFLECT_ATTR$CLASS$ToStringMethod() { return "ToStringProxy"; } void ToStringProxy(char* theBuffer, int theBufferLen) { std::string s = ToString(); strncpy_s(theBuffer, theBufferLen, s.c_str(), theBufferLen); } //22 (Both on same line) | C++ only (Autogen?)
	std::string					ToString();
};

typedef std::map<std::pair<std::string, std::string>, SharedImage> SharedImageMap;

class SharedImageRef
{
public:
	SharedImage*			mSharedImage;
	MemoryImage*			mUnsharedImage;
	bool					mOwnsUnshared;

public:
	SharedImageRef();
	SharedImageRef(const SharedImageRef& theSharedImageRef);
	SharedImageRef(SharedImage* theSharedImage);
	~SharedImageRef();

	void					Release();

	SharedImageRef&			operator=(const SharedImageRef& theSharedImageRef);
	SharedImageRef&			operator=(SharedImage* theSharedImage);
	SharedImageRef&			operator=(MemoryImage* theUnsharedImage);
	MemoryImage*			operator->();
	operator				Image*();
	operator				MemoryImage*();
	operator				DeviceImage*();
	static const char* REFLECT_ATTR$CLASS$ToStringMethod() { return "ToStringProxy"; } void ToStringProxy(char* theBuffer, int theBufferLen) { std::string s = ToString(); strncpy_s(theBuffer, theBufferLen, s.c_str(), theBufferLen); } //50 (Both on same line) | C++ only (Autogen?)
	std::string				ToString();
};

typedef std::vector<SharedImageRef> SharedImageRefVector;

}

#endif //__SHARED_IMAGE_H__