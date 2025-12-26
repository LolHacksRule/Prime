#ifndef __SYSFONT_H__
#define __SYSFONT_H__

#include "Font.h"

namespace Sexy
{

class ImageFont;
class SexyAppBase;

class SysFont : public Font //HFONT and Init removed
{
private:
	ImageFont*				mImageFont;

	void					InitFromImageFont(); //C++ only

public:	
	SexyAppBase*			mApp;
	bool					mDrawShadow;
	bool					mSimulateBold;	

public:
	SysFont(const std::string& theFace, int thePointSize, bool bold = false, bool italics = false, bool underline = false);
	SysFont(SexyAppBase* theApp, const std::string& theFace, int thePointSize, int theScript = ANSI_CHARSET, bool bold = false, bool italics = false, bool underline = false);
	SysFont(const SysFont& theSysFont);

	~SysFont();

	ImageFont*				CreateImageFont();
	int				StringWidth(const SexyString& theString);
	void			DrawString(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, const Rect& theClipRect);

	Font*			Duplicate();
};

}

#endif //__SYSFONT_H__