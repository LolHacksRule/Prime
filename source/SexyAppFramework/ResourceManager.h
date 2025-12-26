#ifndef __SEXY_RESOURCEMANAGER_H__
#define __SEXY_RESOURCEMANAGER_H__

#include "Common.h"
#include "Image.h"
#include "SexyAppBase.h"
#include <string>
#include <map>

namespace ImageLib
{
class Image;
};

namespace Sexy
{

class XMLParser;
class XMLElement;
class Image;
class SoundInstance;
class SexyAppBase;
class Font;

typedef std::map<std::string, std::string>	StringToStringMap;
typedef std::map<SexyString, SexyString>	XMLParamMap;

class AutoInitResourceGen2 //Why
{
	AutoInitResourceGen2(const std::string theXMLFileName, const std::string theResGen2ExePath, const std::string theResPropsUsed, int theResGen2Version);
};

class AutoInitResourceGen3 //Why
{
	AutoInitResourceGen3(const std::string theXMLFileName, const std::string theResGen3ExePath, const std::string theResPropsUsed, const std::string theResWatchFileUsed, int theResGen3MinorVersion, const char* thePlatformName);
};

//GetAutoInitResourceGen3Info ?

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class ResourceManager
{
public: //Public now
	enum ResType
	{
		ResType_Image,
		ResType_Sound,
		ResType_Font,
        ResType_PopAnim,
        ResType_PIEffect,
        ResType_RenderEffect,
#ifdef _SEXYDECOMP_USE_LATEST_CODE
        ResType_GenericResFile, //Mobile / XNA
#endif
        Num_ResTypes,
	};


	struct BaseRes
	{
		ResourceManager* mParent;
		void*			mGlobalPtr; //?
		ResourceRef		mResourceRef;
		int				mRefCount;
		bool			mDirectLoaded;
		int				mReloadIdx;
		ResType 		mType;
		std::string 	mId;
		std::string 	mResGroup;
		std::string 	mPath;
		XMLParamMap 	mXMLAttributes;
		bool 			mFromProgram;

		BaseRes() { mGlobalPtr = NULL; mRefCount = 0; mDirectLoaded = false; mReloadIdx = 0; } //96
		virtual ~BaseRes() {} //97
		virtual void DeleteResource() { } //98
		virtual void ApplyConfig() { } //99
	};

    struct ImageRes : public BaseRes
	{
		SharedImageRef mImage;
		std::string mAlphaImage;
		std::string mAlphaGridImage;
		std::string mVariant;
		Point mOffset;
		bool mAutoFindAlpha;
		bool mPalletize;
		bool mA4R4G4B4;
		bool mA8R8G8B8;
		bool mDither16;
		bool mDDSurface;
		bool mPurgeBits;
		bool mMinimizeSubdivisions;
		bool mCubeMap;
	    bool mVolumeMap;
	    bool mNoTriRep;
	    bool m2DBig;
		int mRows;
		int mCols;	
		DWORD mAlphaColor;
		AnimInfo mAnimInfo;
#ifdef _SEXYDECOMP_USE_LATEST_CODE
		int mAtlasX;
		int mAtlasY;
		int mAtlasW;
		int mAtlasH;
#endif

		ImageRes() { mType = ResType_Image; } //126
		virtual void DeleteResource();
		virtual void ApplyConfig();
	};

	struct SoundRes : public BaseRes
	{
		int mSoundId;
		double mVolume;
		int mPanning;

		SoundRes() { mType = ResType_Sound; } //137
		virtual void DeleteResource();
		virtual void ApplyConfig();
	};

	struct FontRes : public BaseRes
	{
		Font *mFont;
		Image *mImage;
		std::string mImagePath;
		std::string mTags;

		// For SysFonts
		bool mSysFont;
		bool mBold;
		bool mItalic;
		bool mUnderline;
		bool mShadow;
		int mSize;


		FontRes() { mType = ResType_Font; } //157
		virtual void DeleteResource();
		virtual void ApplyConfig();
	};
	
	struct PopAnimRes : public BaseRes
	{
		PopAnim *mPopAnim;

		PopAnimRes() { mType = ResType_PopAnim; } //166
		virtual void DeleteResource();
	};
	
	struct PIEffectRes : public BaseRes
	{
		PIEffect *mPIEffect;

		PIEffectRes() { mType = ResType_PIEffect; } //174
		virtual void DeleteResource();
	};
	
	struct RenderEffectRes : public BaseRes
	{
		RenderEffectDefinition *mRenderEffectDefinition;
		std::string	  mSrcFilePath;

		RenderEffectRes() { mType = ResType_RenderEffect; } //183
		virtual void DeleteResource();
	};

#ifdef _SEXYDECOMP_USE_LATEST_CODE //Present in XNA / Current
	struct GenericResFileRes : public BaseRes
	{
		GenericResFile *mGenericResFile;

		GenericResFileRes() { mType = ResType_GenericResFile; }
		virtual void DeleteResource();
	};
#endif
	

	typedef std::list<BaseRes*> ResList;
	typedef std::map<std::string, BaseRes*> ResMap;
	typedef std::map<HANDLE, BaseRes*> GlobalPtrToResMap; //Official
	typedef std::map<std::string, ResList, StringLessNoCase> ResGroupMap;


	std::set<std::string,StringLessNoCase> 				   mLoadedGroups;
	ResMap					mResMaps[Num_ResTypes]; //Has to be set by num of restypes, XNA has it as 7 (uses 7 types)
	ResMap					mResFromPathMap;
	GlobalPtrToResMap		mGlobalPtrToResMap;
	
	std::string				mLastXMLFileName;
	std::string				mResGenExePath;
	int						mResGenMajorVersion;
	int						mResGenMinorVersion;
	std::string				mResPropsUsed;
	std::string				mResWatchFileUsed;
	std::string				mResGenPlatformName;

	XMLParser*				mXMLParser;
	std::string				mError;
	bool					mHasFailed;
	SexyAppBase*			mApp;
	std::string				mCurResGroup;
	std::string				mDefaultPath;
	std::string				mDefaultIdPrefix;
	bool					mAllowMissingProgramResources;
	bool					mAllowAlreadyDefinedResources; // for reparsing file while running
	bool					mHadAlreadyDefinedError;
	int						mReloadIdx;

	ResGroupMap				mResGroupMap;
	ResList*				mCurResGroupList;
	ResList::iterator		mCurResGroupListItr;
	int						mBaseArtRes;
	int						mCurArtRes;
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	int						mCurLocSet; //Recover from XNA
#endif
	CritSect				mLoadCrit;


	bool							Fail(const std::string& theErrorText);

	virtual bool					ParseCommonResource(XMLElement &theElement, BaseRes *theRes, ResMap &theMap);
	virtual bool					ParseSoundResource(XMLElement &theElement);
	virtual bool					ParseImageResource(XMLElement &theElement);
	virtual bool					ParseFontResource(XMLElement &theElement);
    virtual bool					ParsePopAnimResource(XMLElement &theElement);
    virtual bool					ParsePIEffectResource(XMLElement &theElement);
    virtual bool					ParseRenderEffectResource(XMLElement &theElement);
	virtual bool					ParseSetDefaults(XMLElement &theElement);
	virtual bool					ParseResources();

	bool							DoParseResources();
	void							DeleteMap(ResMap &theMap);
	virtual void					DeleteResources(const std::string &theGroup);
	virtual void					DeleteResources(ResMap &theMap, const std::string &theGroup);

	bool							LoadAlphaGridImage(ImageRes *theRes, DeviceImage *theImage);
	bool							LoadAlphaImage(ImageRes *theRes, DeviceImage *theImage);
	virtual bool					DoLoadImage(ImageRes *theRes);
	virtual bool					DoLoadFont(FontRes* theRes);
	virtual bool					DoLoadSound(SoundRes* theRes);
	virtual bool					DoLoadPopAnim(PopAnimRes* theRes);
	virtual bool					DoLoadPIEffect(PIEffectRes* theRes);
	virtual bool					DoLoadRenderEffect(RenderEffectRes* theRes);
    virtual bool					DoLoadResource(BaseRes* theRes, bool* skipped);
	
	int								GetNumResources(const std::string &theGroup); //TODO: This changed past Win
	int								GetNumResources(const std::string &theGroup, ResMap &theMap);

public:
	ResourceManager(SexyAppBase *theApp);
	virtual ~ResourceManager();

	///////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////
	bool							IsGroupLoaded(const std::string& theGroup);
	bool							ParseResourcesFile(const std::string& theFilename);
	bool							ReparseResourcesFile(const std::string& theFilename);
	void							RegisterGlobalPtr(const std::string &theId, void *theGlobalPtr);
	void							InitResourceGen(const std::string &theResGenExePath, const std::string &theResPropsUsed, const std::string &theResWatchFileUsed, int theResGenMajorVersion, int theResGenMinorVersion, const std::string &thePlatformName);
	void							ReapplyConfigs();

	std::string						GetErrorText();
	bool							HadError();
	bool							IsResourceLoaded(const std::string &theId);

	int								GetNumImages(const std::string &theGroup);
	int								GetNumSounds(const std::string &theGroup);
	int								GetNumFonts(const std::string &theGroup);
	int								GetNumResources(const std::string &theGroup);

	virtual bool					LoadNextResource();
	virtual void					ResourceLoadedHook(BaseRes *theRes);

	virtual void					StartLoadResources(const std::string &theGroup);
	virtual bool					LoadResources(const std::string &theGroup);

	bool							ReplaceImage(const std::string &theId, Image *theImage);
	bool							ReplaceSound(const std::string &theId, int theSound);
	bool							ReplaceFont(const std::string &theId, Font *theFont);
	bool							ReplacePopAnim(const std::string &theId, PopAnim *thePopAnim);
	bool							ReplacePIEffect(const std::string &theId, PIEffect *thePIEffect);
	bool							ReplaceRenderEffect(const std::string &theId, RenderEffectDefinition *theDefinition);
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	bool							ReplaceGenericResFile(const std::string &theId, GenericResFile* theFile);
#endif

	Point							GetImageOffset(const std::string &theName);
	void							DeleteImage(const std::string &theName);
	void							DeleteSound(const std::string &theName);
	void							DeleteFont(const std::string &theName);
	void							DeletePopAnim(const std::string &theName);
	void							DeletePIEffect(const std::string &theName);
	void							DeleteRenderEffect(const std::string &theName);
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	void							DeleteGenericResFile(const std::string &theName);
#endif
	
	SharedImageRef					LoadImage(const std::string &theName); //Keeping LoadImage so it won't break outside Win
	int								LoadSound(const std::string &theName);
	Font*							LoadFont(const std::string &theName);
	PopAnim*						LoadPopAnim(const std::string &theName);
	PIEffect*						LoadPIEffect(const std::string &theName);
	RenderEffectDefinition*			LoadRenderEffect(const std::string &theName);
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	GenericResFile*					LoadGenericResFile(const std::string &theName);
#endif

	SharedImageRef					GetImage(const std::string &theId);
	int								GetSound(const std::string &theId);
	Font*							GetFont(const std::string &theId);
	PopAnim*						GetPopAnim(const std::string &theId);
	PIEffect*						GetPIEffect(const std::string &theId);
	RenderEffectDefinition*			GetRenderEffect(const std::string &theId);
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	GenericResFile*					GetGenericResFile(const std::string &theId);
#endif
	
	std::string						GetIdByPath(const std::string& thePath);

    BaseRes*						GetBaseRes(int type, const std::string &theId);
    void 							Deref(BaseRes *theRes);
	ResourceRef						GetResourceRef(BaseRes* theBaseRes); //Changed param
    ResourceRef						GetResourceRef(int type, const std::string &theId);
    ResourceRef						GetResourceRef(void* theGlobalPtr);
    ResourceRef						GetResourceRefFromPath(const std::string &theFileName);
	ResourceRef						GetImageRef(const std::string& theId);
	ResourceRef						GetImageRef(void* theGlobalPtrRef);
	ResourceRef						GetSoundRef(const std::string& theId);
	ResourceRef						GetSoundRef(void* theGlobalPtrRef);
	ResourceRef						GetFontRef(const std::string& theId);
	ResourceRef						GetFontRef(void* theGlobalPtrRef);
	ResourceRef						GetPopAnimRef(const std::string& theId);
	ResourceRef						GetPopAnimRef(void* theGlobalPtrRef);
	ResourceRef						GetPIEffectRef(const std::string& theId);
	ResourceRef						GetPIEffectRef(void* theGlobalPtrRef);
	ResourceRef						GetRenderEffectRef(const std::string& theId);
	ResourceRef						GetRenderEffectRef(void* theGlobalPtrRef);
	
	// Returns all the XML attributes associated with the image
	const XMLParamMap&				GetImageAttributes(const std::string &theId);

	// These throw a ResourceManagerException if the resource is not found
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	virtual SharedImageRef			GetImageThrow(const std::string& theId, int artRes, bool optional);
	virtual int						GetSoundThrow(const std::string& theId, int artRes);
	virtual Font*					GetFontThrow(const std::string& theId, int artRes);
#else
	virtual SharedImageRef			GetImageThrow(const std::string &theId, bool optional);
	virtual int						GetSoundThrow(const std::string& theId);
	virtual Font*					GetFontThrow(const std::string& theId);
#endif
	virtual PopAnim*				GetPopAnimThrow(const std::string &theId);
	virtual PIEffect*				GetPIEffectThrow(const std::string &theId);
	virtual RenderEffectDefinition*	GetRenderEffectThrow(const std::string &theId);

	void							SetAllowMissingProgramImages(bool allow);

	virtual void					DeleteResources(const std::string &theGroup);
	void							DeleteExtraImageBuffers(const std::string &theGroup);

	const ResList*					GetCurResGroupList()	{return mCurResGroupList;}
	std::string						GetCurResGroup()		{return mCurResGroup;}
	void							DumpCurResGroup(std::string& theDestStr);
};

class ResourceRef
{
public:
	void* mBaseResP; //Is this supposed to be baseres?
	ResourceRef();
	ResourceRef(const ResourceRef& theResourceRef);
	~ResourceRef();
	ResourceRef& operator=(const ResourceRef& theResourceRef);
	bool HasResource();
	void Release();
	const std::string GetId();
	operator SharedImageRef();
	operator Image*();
	operator MemoryImage*();
	operator DeviceImage*();
	operator int(); //Sound ID
	operator Font*();
	operator ImageFont*();
	operator PopAnim*();
	operator PIEffect*();
	operator RenderEffectDefinition*();
	//operator GenericResFile*();
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
struct ResourceManagerException : public std::exception
{
	std::string what;
	ResourceManagerException(const std::string &theWhat) : what(theWhat) { } //366
	~ResourceManagerException() { } //367
};

class ResourceManagerInfo
{
public:
	ResGenInfoMap mResGenInfoMap;
	ResourceManagerInfo();
};

class ResGenInfo //assuming it's here useless but for accuracy
{
public:
	std::string mResGenExePath;
	std::string mResPropsUsed;
	std::string mResWatchFileUsed;
	int mResGenMajorVersion;
	int mResGenMinorVersion;
	std::string mResGenPlatformName;

	ResGenInfo();
	~ResGenInfo();
};

typedef std::map<std::string, ResGenInfo>	ResGenInfoMap; //Official

static ResourceManagerInfo* gResourceManagerInfo;

}

#endif //__PROPERTIESPARSER_H__


