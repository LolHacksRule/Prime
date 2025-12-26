#ifndef __IMAGE_H__
#define __IMAGE_H__

#include "Common.h"
#include "Rect.h"
#include "MemoryImage.h"

namespace Sexy
{
//Span moved to RenderDevice

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
enum ImageFlags
{
	ImageFlag_MinimizeNumSubdivisions = 0x0001,		// subdivide image into fewest possible textures (may use more memory)
	ImageFlag_Use64By64Subdivisions = 0x0002,		// good to use with image strips so the entire texture isn't pulled in when drawing just a piece
	ImageFlag_UseA4R4G4B4 = 0x0004,		// images with not too many color gradients work well in this format
	ImageFlag_UseA8R8G8B8 = 0x0008,		// non-alpha images will be stored as R5G6B5 by default so use this option if you want a 32-bit non-alpha image
	ImageFlag_RenderTarget = 0x0010,
	ImageFlag_CubeMap = 0x0020,
	ImageFlag_VolumeMap = 0x0040,
	ImageFlag_NoTriRep = 0x0080,
	ImageFlag_NoQuadRep = 0x0080,
	ImageFlag_RTUseDefaultRenderMode = 0x0001,
	REFLECT_ATTR$ENUM$FLAGS = -1 //?
};

enum ImageFlags
{
    //ImageFlag_NONE=0, //XNA
    ImageFlag_MinimizeNumSubdivisions=1,
    ImageFlag_Use64By64Subdivisions=2,
    ImageFlag_UseA4R4G4B4=4,
    ImageFlag_UseA8R8G8B8=8,
    ImageFlag_RenderTarget=16,
    ImageFlag_CubeMap=32,
    ImageFlag_VolumeMap=64,
    ImageFlag_NoTriRep=128,
    ImageFlag_NoQuadRep=128,
    ImageFlag_RTUseDefaultRenderMode=256,
#ifdef _SEXYDECOMP_USE_LATEST_CODE
    ImageFlag_Atlas=512,
#endif
    REFLECT_ATTR$ENUM$FLAGS=-1 //? 513 on XNA except with underscores
};
enum AnimType
{
	AnimType_None,
	AnimType_Once,
	AnimType_PingPong,
	AnimType_Loop
};

struct AnimInfo
{
	AnimType				mAnimType;
	int						mFrameDelay; // 1/100s
	int						mNumCels;
	IntVector				mPerFrameDelay;
	IntVector				mFrameMap;
	int						mTotalAnimTime;

	AnimInfo();
	void SetPerFrameDelay(int theFrame, int theTime);
	void Compute(int theNumCels, int theBeginFrameTime = 0, int theEndFrameTime = 0);

	int GetPerFrameCel(int theTime);
	int GetCel(int theTime);
};

class Graphics;
class SexyMatrix3;
class SysFont;
class SexyVertex;

class Image
{
	friend class			Sexy::SysFont; //?

protected:
	ImageFlags				mImageFlags;
	void*					mRenderData;

public:
	bool					mDrawn;
	std::string				mFilePath;
	int						mWidth;
	int						mHeight;

	// for image strips
	int						mNumRows; 
	int						mNumCols;

#ifdef _SEXYDECOMP_USE_LATEST_CODE
	SexyString				mFileName;
	SexyString				mNameForRes;
	Rect					mCelRect;
	Rect					mRect;
	Image*					mAtlasImage;
	int						mAtlasStartX;
	int						mAtlasStartX;
	int						mAtlasEndX;
	int						mAtlasEndX;
	bool					mAtlasValidate;
	//mVector from XNA here?
#endif

	// for animations
	AnimInfo				*mAnimInfo;

public:
	Image();
	Image(const Image& theImage);
	virtual ~Image();

	virtual	MemoryImage*	AsMemoryImage() { return 0; } //84

	int						GetWidth();
	int						GetHeight();
	int						GetCelWidth();		// returns the width of just 1 cel in a strip of images
	int						GetCelHeight();	// like above but for vertical strips
	int						GetCelCount();
	int						GetAnimCel(int theTime); // use animinfo to return appropriate cel to draw at the time
	Rect					GetAnimCelRect(int theTime);
	Rect					GetCelRect(int theCel);				// Gets the rectangle for the given cel at the specified row/col 
	Rect					GetCelRect(int theCol, int theRow);	// Same as above, but for an image with both multiple rows and cols
	void					CopyAttributes(Image *from);
	ImageFlags				GetImageFlags() { return mImageFlags; } //98
	void					ReplaceImageFlags(DWORD inFlags) { mImageFlags = (ImageFlags)inFlags; } //? | 99
	void					AddImageFlags(DWORD inFlags) { inFlags |= mImageFlags; } //Is this right | 100
	void					RemoveImageFlags(DWORD inFlags); //? Not a function
	bool					HasImageFlag(DWORD inFlag) { return (inFlag & mImageFlags) != 0; } //102
	
	void*					GetRenderData() { return mRenderData; } //104
	void					SetRenderData(void* inRenderData) { mRenderData = inRenderData; } //105
	void					CreateRenderData();
	static const char*		REFLECT_ATTR$CLASS$ToStringMethod() { return "ToStringProxy"; } void ToStringProxy(char* theBuffer, int theBufferLen) { std::string s = ToString(); strncpy_s(theBuffer, theBufferLen, s.c_str(), theBufferLen); } //131
	std::string				ToString() //132-134
	{
		return StrFormat("%s", mFilePath.c_str());
	}
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	void					InitAtalasState(); //XNA only code, putting it here for now.
	virtual DeviceImage*	AsDeviceImage() { return NULL; }
#endif
};

}

#endif //__IMAGE_H__