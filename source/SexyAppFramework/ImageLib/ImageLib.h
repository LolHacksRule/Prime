#ifndef __IMAGELIB_H__
#define __IMAGELIB_H__

struct PFILE; //?

#include <string>

namespace ImageLib
{

class Image
{
public:
	int						mWidth;
	int						mHeight;
	unsigned long*			mBits;

public:
	Image();
	virtual ~Image();

	int						GetWidth();
	int						GetHeight();
	unsigned long*			GetBits();
};

bool WriteJPEGImage(const std::string& theFileName, Image* theImage, int theQuality = 80); //Init as 80? Not on iOS
bool WritePNGImage(const std::string& theFileName, Image* theImage, int theDPI = 0);
bool WriteTGAImage(const std::string& theFileName, Image* theImage); //Not on iOS
bool WriteBMPImage(const std::string& theFileName, Image* theImage); //Not on iOS
extern int gAlphaComposeColor;
extern bool gJ2KLoadFuncs;
extern bool gAutoLoadAlpha;
extern bool gIgnoreJPEG2000Alpha;  // I've noticed alpha in jpeg2000's that shouldn't have alpha so this defaults to true

extern std::string gAlphaFileName;
extern std::string gColorFileName;

bool GetGIFSize(PFILE *fp, int* width, int* height); //Not on iOS
bool LoadPNGToTextureAlpha(PFILE *fp, int expected_width, int expected_height, unsigned char *pBits, unsigned long Pitch); //Not on iOS
bool LoadGIFToTextureAlpha(PFILE *fp, int expected_width, int expected_height, unsigned char *pBits, unsigned long Pitch); //Not on iOS

Image* GetImage(const std::string& theFileName, bool lookForAlphaImage = true, int thePakLibSearchOrder = -1);

void InitJPEG2000();
void CloseJPEG2000();
void SetJ2KCodecKey(const std::string& theKey);

}

#endif //__IMAGELIB_H__