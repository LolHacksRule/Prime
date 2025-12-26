#ifndef __IMAGEFONT_H__
#define __IMAGEFONT_H__

#include "Font.h"
#include "DescParser.h"
#include "SharedImage.h"
#include "Widget.h"

struct SortedKern
{
	SexyChar mKey;
	SexyChar mValue;
	int mOffset;
	SortedKern(SexyChar inKey, SexyChar inValue, int inOffset);
	//SortedKern();
	static int Compare(const void* a, const void* b);
};

namespace Sexy
{
	
class CharDataHashEntry
{
public:
	WORD					mChar;
	WORD					mDataIndex;
	DWORD					mNext;
	CharDataHashEntry() { mChar = 0; mDataIndex = -1; mNext = -1; } //28
};

class SexyAppBase;
class Image;

class CharData
{
public:
	Rect					mImageRect;
	Point					mOffset;
	WORD					mKerningFirst;
	WORD					mKerningCount;
	int						mWidth;
	int						mOrder;
	int						mHashEntryIndex;

public:
	CharData();
};

class CharDataHashTable
{
	enum //ECharDataHash //in XNA
	{
		HASH_BITS = 10,
		HASH_BUCKET_COUNT = 1024,
		HASH_BUCKET_MASK = 1023
	};
public:
	bool							mOrderedHash;
	std::vector<CharData>			mCharData;
	std::vector<CharDataHashEntry>	mHashEntries;

protected:
	int GetBucketIndex(SexyChar inChar);

public:

	CharDataHashTable() //68-72
	{
		mOrderedHash = false;
		mHashEntries.resize(1024);
	}
	CharData* GetCharData(SexyChar theChar, bool inAllowAdd = false);
};

class FontData;

class FontLayer
{
public:
	typedef std::vector<std::string> StringVector; //According to pl_d.pdb the typedef is still here
	typedef std::map<SexyString, SexyString> ExInfoMap;
public:
	union KerningValue //Add
    {
        int mInt;
        uint16_t mChar;
        int mOffset;
    };
	FontData*				mFontData;
	ExInfoMap				mExtendedInfo; //Add
	std::string				mLayerName; //Add
	StringVector			mRequiredTags;
	StringVector			mExcludedTags;
	IntVector				mKerningData; //Add
	CharDataHashTable		mCharDataHashTable; //Replace mCharData
	Color					mColorMult;
	Color					mColorAdd;
	SharedImageRef			mImage;	
	bool					mImageIsWhite; //Add
	std::string				mImageFileName; //Add
	int						mDrawMode;
	Point					mOffset;
	int						mSpacing;
	int						mMinPointSize;
	int						mMaxPointSize;
	int						mPointSize;
	int						mAscent;
	int						mAscentPadding; // How much space is above the avg uppercase char
	int						mHeight;		// 	
	int						mDefaultHeight; // Max height of font character image rects	
	int						mLineSpacingOffset; // This plus height should get added between lines
	int						mBaseOrder;
	bool					mUseAlphaCorrection;

public:
	FontLayer(FontData* theFontData);
	FontLayer(const FontLayer& theFontLayer);
	FontLayer();
	CharData* GetCharData(SexyChar theChar); //Different in Transmension
};

typedef std::list<FontLayer> FontLayerList;
typedef std::map<std::string, FontLayer*> FontLayerMap;
typedef std::list<Rect> RectList;
typedef std::vector<int> IntVector;

typedef std::map<SexyChar, SexyChar> SexyCharToSexyCharMap;

class FontData : public DescParser
{
public:
	bool					mInitialized;
	int						mRefCount;
	SexyAppBase*			mApp;		

	int						mDefaultPointSize;
	SexyCharToSexyCharMap	mCharMap; //Changed
	FontLayerList			mFontLayerList;
	FontLayerMap			mFontLayerMap;

	std::string				mSourceFile;
	std::wstring			mFontErrorHeader; //Changed to WSTRING?

public:
	virtual bool			Error(const std::wstring& theError); //Changed to WSTRING?

	bool					GetColorFromDataElement(DataElement *theElement, Color &theColor);
	bool					DataToLayer(DataElement* theSource, FontLayer** theFontLayer);
	virtual bool			HandleCommand(const ListDataElement& theParams);

public:
	FontData();
	virtual ~FontData();

	void					Ref();
	void					DeRef();

	bool					Load(SexyAppBase* theSexyApp, const std::string& theFontDescFileName);
	bool					LoadLegacy(Image* theFontImage, const std::string& theFontDescFileName);
};

typedef std::map<int, Rect> SexyCharToRectMap;
class ActiveFontLayer //Removed mScaledImage,add mColorStack, change mScaledCharImageRects
{
public:
	FontLayer*				mBaseFontLayer;

	SharedImageRef			mScaledImages[8]; //Changed to multiple
	bool					mUseAlphaCorrection;
	bool					mOwnsImage;
	SexyCharToRectMap		mScaledCharImageRects; //Changed from 256 Rect
	ColorVector				mColorStack;

public:
	ActiveFontLayer();
	ActiveFontLayer(const ActiveFontLayer& theActiveFontLayer);
	virtual ~ActiveFontLayer();
	SharedImageRef GenerateAlphaCorrectedImage(int thePalette);
	void PushColor(const Color& theColor);
	void PopColor();
};

typedef std::list<ActiveFontLayer> ActiveFontLayerList;

class RenderCommand //Removed mImage
{
public:
	ActiveFontLayer			mFontLayer;
	int						mDest[2];
	int						mSrc[4];
	int						mMode;
	Color					mColor;
	RenderCommand*			mNext;
};

class ImageFont : public Font //Add mAlphaCorrectionEnabled, mOrderedHash, mActivateAllLayers, mWantAlphaCorrection, EnableAlphaCorrection, SetOrderedHashing
{
public:
	//typedef std::vector<std::string> StringVector;
	static bool				mAlphaCorrectionEnabled;
	static bool				mOrderedHash;
	FontData*				mFontData;
	int						mPointSize;
	StringVector			mTagVector;

	bool					mActivateAllLayers;
	bool					mActiveListValid;
	ActiveFontLayerList		mActiveLayerList;
	double					mScale;
	bool					mForceScaledImagesWhite;
	bool					mWantAlphaCorrection;
	MemoryImage*			mFontImage;

public:
	virtual void			GenerateActiveFontLayers();
	virtual void			DrawStringEx(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, const Rect* theClipRect, RectList* theDrawnAreas, int* theWidth);
	static void				EnableAlphaCorrection(bool alphaCorrect = true);
	static void				SetOrderedHashing(bool orderedHash = true); //Not a function
	SexyChar				GetMappedChar(SexyChar theChar);

public:
	ImageFont();
	ImageFont(SexyAppBase* theSexyApp, const std::string& theFontDescFileName);
	ImageFont(Image *theFontImage); // for constructing your own image font without a file descriptor
	ImageFont(const ImageFont& theImageFont);
	virtual ~ImageFont();

	ImageFont(Image* theFontImage, const std::string& theFontDescFileName);
	
	virtual int				CharWidth(SexyChar theChar);
	virtual int				CharWidthKern(SexyChar theChar, SexyChar thePrevChar);
	virtual int				StringWidth(const SexyString& theString);
	virtual void			DrawString(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, const Rect& theClipRect);

	virtual Font*			Duplicate();

	virtual void			SetPointSize(int thePointSize);
	virtual int				GetPointSize();
	virtual void			SetScale(double theScale);
	virtual int				GetDefaultPointSize();
	virtual bool			AddTag(const std::string& theTagName);	
	virtual bool			RemoveTag(const std::string& theTagName);
	virtual bool			HasTag(const std::string& theTagName);
	virtual SexyString		GetDefine(const SexyString& theName);

	virtual void			Prepare();
	static bool				CheckCache(const std::string& theSrcFile, const std::string& theAltData); //?
	static bool				SetCacheUpToDate(const std::string& theSrcFile, const std::string& theAltData); //?
	static ImageFont*		ReadFromCache(const std::string& theSrcFile, const std::string& theAltData); //?
	virtual void			WriteToCache(const std::string& theSrcFile, const std::string& theAltData); //?
	bool					SerializeRead(void *thePtr, int theSize); //?
	#ifdef _SEXYDECOMP_USE_LATEST_CODE
		void SerializeReadEndian(void *thePtr, std::string* theStr);
	#endif
	int						SerializeWrite(void *thePtr, int theSizeIfKnown); //?
	int						GetLayerCount(); //?
	void					PushLayerColor(const std::string& theLayerName, const Color& theColor); //?
	void					PushLayerColor(int theLayer, const Color& theColor); //?
	void					PopLayerColor(const std::string& theLayerName); //?
	void					PopLayerColor(int theLayer); //?
};

#ifdef _SEXYDECOMP_USE_LATEST_CODE
void SMemRStr(void** theDest, std::string* theStr);
#endif

}

#endif //__IMAGEFONT_H__