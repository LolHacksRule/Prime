
#include "ResourceManager.h"
#include "ResStreamsManager.h"
#include "XMLParser.h"
#include "SoundManager.h"
#include "DeviceImage.h"
#include "ImageFont.h"
#include "SysFont.h"
#include "ImageLib/ImageLib.h"

//#define SEXY_PERF_ENABLED
#include "PerfTimer.h"
#include "Debug.h"
#include "AutoCrit.h"

using namespace Sexy;

AutoInitResourceGen2::AutoInitResourceGen2(const std::string theXMLFileName, const std::string theResGen2ExePath, const std::string theResPropsUsed, int theResGen2Version) //C++ only, not present in EAMT or iOS | 30-51
{
	if (gResourceManagerInfo == NULL)
		gResourceManagerInfo = new ResourceManagerInfo();
	ResGenInfo aResGenInfo;
	aResGenInfo.mResGenExePath = theResGen2ExePath;
	aResGenInfo.mResPropsUsed = theResPropsUsed;
	aResGenInfo.mResGenMajorVersion = 2;
	aResGenInfo.mResGenMinorVersion = theResGen2Version;
	aResGenInfo.mResWatchFileUsed = "";
	aResGenInfo.mResGenPlatformName = "";
	std::string xmlPath = theXMLFileName;
	int xmlPathLen = xmlPath.length();
	for (int i = 0; i < xmlPathLen; ++i)
	{
		if (xmlPath[i] == '\\')
			xmlPath[i] = '/';
	}
	gResourceManagerInfo->mResGenInfoMap[xmlPath] = aResGenInfo;
}

AutoInitResourceGen3::AutoInitResourceGen3(const std::string theXMLFileName, const std::string theResGen3ExePath, const std::string theResPropsUsed, const std::string theResWatchFileUsed, int theResGen3MinorVersion, const char* thePlatformName) //54-75
{
	if (gResourceManagerInfo == NULL)
		gResourceManagerInfo = new ResourceManagerInfo();
	ResGenInfo aResGenInfo;
	aResGenInfo.mResGenExePath = theResGen3ExePath;
	aResGenInfo.mResPropsUsed = theResPropsUsed;
	aResGenInfo.mResGenMajorVersion = 3;
	aResGenInfo.mResGenMinorVersion = theResGen3MinorVersion;
	aResGenInfo.mResWatchFileUsed = "";
	aResGenInfo.mResGenPlatformName = "";
	aResGenInfo.mResWatchFileUsed = theResWatchFileUsed;
	aResGenInfo.mResGenPlatformName = thePlatformName ? thePlatformName : "windows"; //Not on iOS or Android?
	std::string xmlPath = theXMLFileName;
	int xmlPathLen = xmlPath.length();
	for (int i = 0; i < xmlPathLen; ++i)
	{
		if (xmlPath[i] == '\\')
			xmlPath[i] = '/';
	}
	gResourceManagerInfo->mResGenInfoMap[xmlPath] = aResGenInfo;
}

ResourceRef::ResourceRef() //80-82
{
	mBaseResP = NULL;
}

ResourceRef::ResourceRef(const ResourceRef& theResourceRef) //85-89
{
	mBaseResP = theResourceRef.mBaseResP;
	if (mBaseResP != NULL)
		((ResourceManager::BaseRes*)mBaseResP)->mRefCount++;
}

ResourceRef::~ResourceRef() //92-94
{
	Release();
}

ResourceRef& ResourceRef::operator=(const ResourceRef& theResourceRef) //97-103
{
	Release();
	mBaseResP = theResourceRef.mBaseResP;
	if (mBaseResP != NULL)
		((ResourceManager::BaseRes*)mBaseResP)->mRefCount++;
}

bool ResourceRef::HasResource() //106-108
{
	mBaseResP != NULL;
}

void ResourceRef::Release() //111-117
{
	if (mBaseResP)
	{
		((ResourceManager::BaseRes*)mBaseResP)->mParent->Deref((ResourceManager::BaseRes*)mBaseResP);
	}
	mBaseResP = NULL;
}

const std::string ResourceRef::GetId() //120-124
{
	if (mBaseResP == NULL)
		return "";
	return  ((ResourceManager::BaseRes*)mBaseResP)->mId;
}

ResourceRef::operator SharedImageRef() //127-131
{
	if (mBaseResP != NULL && ((ResourceManager::BaseRes*)mBaseResP)->mType != ResourceManager::ResType_Image)
		return NULL;
	return ((ResourceManager::ImageRes*)mBaseResP)->mImage;
}

ResourceRef::operator Image* () //134-138
{
	if (mBaseResP != NULL && ((ResourceManager::BaseRes*)mBaseResP)->mType != ResourceManager::ResType_Image)
		return NULL;
	return ((ResourceManager::ImageRes*)mBaseResP)->mImage;
}

ResourceRef::operator MemoryImage* () //141-145
{
	if (mBaseResP != NULL && ((ResourceManager::BaseRes*)mBaseResP)->mType != ResourceManager::ResType_Image)
		return NULL;
	return ((ResourceManager::ImageRes*)mBaseResP)->mImage;
}

ResourceRef::operator DeviceImage* () //148-152
{
	if (mBaseResP != NULL && ((ResourceManager::BaseRes*)mBaseResP)->mType != ResourceManager::ResType_Image)
		return NULL;
	return ((ResourceManager::ImageRes*)mBaseResP)->mImage;
}

ResourceRef::operator int () //155-159
{
	if (mBaseResP != NULL && ((ResourceManager::BaseRes*)mBaseResP)->mType != ResourceManager::ResType_Sound)
		return NULL;
	return ((ResourceManager::SoundRes*)mBaseResP)->mSoundId;
}

ResourceRef::operator Font* () //162-166
{
	if (mBaseResP != NULL && ((ResourceManager::BaseRes*)mBaseResP)->mType != ResourceManager::ResType_Font)
		return NULL;
	return ((ResourceManager::FontRes*)mBaseResP)->mFont;
}

ResourceRef::operator ImageFont* () //169-173
{
	if (mBaseResP != NULL && ((ResourceManager::BaseRes*)mBaseResP)->mType != ResourceManager::ResType_Font)
		return NULL;
	return (ImageFont*)((ResourceManager::FontRes*)mBaseResP)->mFont;
}

ResourceRef::operator PopAnim* () //176-180
{
	if (mBaseResP != NULL && ((ResourceManager::BaseRes*)mBaseResP)->mType != ResourceManager::ResType_PopAnim)
		return NULL;
	return ((ResourceManager::PopAnimRes*)mBaseResP)->mPopAnim;
}

ResourceRef::operator PIEffect* () //183-187
{
	if (mBaseResP != NULL && ((ResourceManager::BaseRes*)mBaseResP)->mType != ResourceManager::ResType_PIEffect)
		return NULL;
	return ((ResourceManager::PIEffectRes*)mBaseResP)->mPIEffect;
}

ResourceRef::operator RenderEffectDefinition* () //190-194
{
	if (mBaseResP != NULL && ((ResourceManager::BaseRes*)mBaseResP)->mType != ResourceManager::ResType_RenderEffect)
		return NULL;
	return ((ResourceManager::RenderEffectRes*)mBaseResP)->mRenderEffectDefinition;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ResourceManager::ImageRes::DeleteResource() //197-203
{	
	if (mResourceRef.HasResource())
		mResourceRef.Release();
	if (mGlobalPtr)
		mGlobalPtr = NULL;
	mImage.Release();
}

void ResourceManager::ImageRes::ApplyConfig() //206-266
{
	if (mResourceRef.HasResource())
		return;
	DeviceImage* aDDImage = (DeviceImage*)&mImage;
	if (aDDImage == NULL)
		return;
	aDDImage->ReplaceImageFlags(0);
	if (mNoTriRep)
		aDDImage->AddImageFlags(ImageFlag_NoTriRep);
	aDDImage->mNumRows = mRows;
	aDDImage->mNumCols = mCols;
	if (mDither16)
		aDDImage->mDither16 = true;
	if (mA4R4G4B4)
		aDDImage->AddImageFlags(ImageFlag_UseA4R4G4B4);
	if (mA8R8G8B8)
		aDDImage->AddImageFlags(ImageFlag_UseA8R8G8B8);
	if (mMinimizeSubdivisions)
		aDDImage->AddImageFlags(ImageFlag_MinimizeNumSubdivisions);
	if (mCubeMap)
		aDDImage->AddImageFlags(ImageFlag_CubeMap);
	else if (mVolumeMap)
		aDDImage->AddImageFlags(ImageFlag_VolumeMap);
	if (mAnimInfo.mAnimType != AnimType_None)
		aDDImage->mAnimInfo = new AnimInfo(mAnimInfo);
#ifdef SEXYDECOMP_USE_LATEST_CODE
	if (mIsAtlas)
		aDDImage->AddImageFlags(513); //?
	if (mAtlasName != null)
	{
		aDDImage.mAtlasImage = gSexyAppBase->mResourceManager->LoadImage(mAtlasName).GetImage();
		aDDImage.mAtlasStartX = mAtlasX;
		aDDImage.mAtlasStartY = mAtlasY;
		aDDImage.mAtlasEndX = mAtlasX + mAtlasY;
		aDDImage.mAtlasEndY = mAtlasY + mAtlasH;
		aDDImage->MemoryImage::CommitBits();
		aDDImage->mPurgeBits = mPurgeBits;
	}
#endif
	aDDImage->CommitBits();
	aDDImage->mPurgeBits = this->mPurgeBits;
	if (mDDSurface) //Similar to line 787 of 1.30 ResourceManager::DoLoadImage
	{
		SEXY_PERF_BEGIN("ResourceManager:DDSurface"); //Present in Bej3JP and BejLiveWin8

		aDDImage->CommitBits();

		if (!aDDImage->mHasAlpha)
		{
			aDDImage->mWantDeviceSurface = true;
			aDDImage->mPurgeBits = true;
		}

		SEXY_PERF_END("ResourceManager:DDSurface");

		if (aDDImage->mPurgeBits)
		{
			AutoCrit anAutoCrit(gSexyAppBase->mImageSetCritSect);
			aDDImage->PurgeBits();
		}
	}
}

void ResourceManager::SoundRes::DeleteResource() //269-278
{
	if (mResourceRef.HasResource())
		mResourceRef.Release();
	else if (mSoundId >= 0)
		gSexyAppBase->mSoundManager->ReleaseSound(mSoundId);

	mSoundId = -1;
	if (mGlobalPtr != NULL)
		mGlobalPtr = (void*)-1;
}

void ResourceManager::SoundRes::ApplyConfig() //281-294
{
	if (mSoundId >= -1 && !mResourceRef.HasResource())
	{
		if (mVolume >= 0.0)
			gSexyAppBase->mSoundManager->SetBaseVolume(mSoundId, mVolume);
		if (mPanning != 0)
			gSexyAppBase->mSoundManager->SetBasePan(mSoundId, mPanning);
	}
}

void ResourceManager::FontRes::DeleteResource() //297-309
{
	if (mResourceRef.HasResource())
		mResourceRef.Release();

	delete mFont;
	mFont = NULL;

	delete mImage;
	mImage = NULL;

	if (mGlobalPtr)
		mGlobalPtr = NULL;
}

void ResourceManager::FontRes::ApplyConfig() //312-334
{
	if (mFont == NULL|| mSysFont)
		return;

	ImageFont* anImageFont = (ImageFont*)mFont;
	if (!mTags.empty())
	{
		char aBuf[1024];
		strcpy(aBuf, mTags.c_str());
		const char* aPtr = strtok(aBuf, ", \r\n\t");
		while (aPtr != NULL)
			anImageFont->AddTag(aPtr);
		anImageFont->Prepare();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ResourceManager::PopAnimRes::DeleteResource() //339-347
{
	if (mResourceRef.HasResource())
		mResourceRef.Release();
	else if (mPopAnim)
		delete mPopAnim;
	mPopAnim = NULL;
	if (mGlobalPtr)
		mGlobalPtr = NULL;
}

void ResourceManager::PIEffectRes::DeleteResource() //350-358
{
	if (mResourceRef.HasResource())
		mResourceRef.Release();
	else if (mPIEffect)
		delete mPIEffect; //?
	mPIEffect = NULL;
	if (mGlobalPtr)
		mGlobalPtr = NULL;
}

void ResourceManager::RenderEffectRes::DeleteResource() //361-369
{
	if (mResourceRef.HasResource())
		mResourceRef.Release();
	else if (mRenderEffectDefinition)
		delete mRenderEffectDefinition; //?
	mRenderEffectDefinition = NULL;
	if (mGlobalPtr)
		mGlobalPtr = NULL;
}

ResourceManager::ResourceManager(SexyAppBase *theApp) //372-385
{
	mBaseArtRes = 0; //Defined first in C++
	mCurArtRes = 0; //Defined first in C++
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	mCurLocSet = 1162761555; //ENUS
#endif
	mApp = theApp;
	mHasFailed = false;
	mXMLParser = NULL;
	mResGenMajorVersion = 0;
	mResGenMinorVersion = 0;
	mAllowMissingProgramResources = false;
	mAllowAlreadyDefinedResources = false;
	mCurResGroupList = NULL;
	mReloadIdx = 0;
}

ResourceManager::~ResourceManager() //388-391
{
	for (int i = 0; i < Num_ResTypes; i++) //7 in XNA why idk
		DeleteMap(mResMaps[i]);
	delete(&mLoadCrit);
}

bool ResourceManager::IsGroupLoaded(const std::string& theGroup) //394-396
{
	return mLoadedGroups.find(theGroup) != mLoadedGroups.end();
}

///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::IsResourceLoaded(const std::string& theId) //400-408
{
	if ((Image*)GetImage(theId) != NULL)
		return true;
	if (GetFont(theId) != NULL)
		return true;
	if (GetSound(theId) != -1)
		return true;
	return false;
}

void ResourceManager::DeleteMap(ResMap &theMap) //411-418
{
	for (ResMap::iterator anItr = theMap.begin(); anItr != theMap.end(); anItr++)
		anItr->second->DeleteResource();
	for (ResMap::iterator anItr = theMap.begin(); anItr != theMap.end(); anItr++)
		delete anItr->second;

	theMap.clear();
}

void ResourceManager::DeleteResources(ResMap &theMap, const std::string &theGroup) //421-433
{
	for (ResMap::iterator anItr = theMap.begin(); anItr != theMap.end(); ++anItr)
	{
		if (theGroup.empty() || anItr->second->mResGroup == theGroup)
		{
			if (anItr->second->mDirectLoaded)
				Deref(anItr->second);
		}
	}
}

ResourceManager::BaseRes* ResourceManager::GetBaseRes(int theType, const std::string& theId) //Different in XNA | 436-441
{
	ResMap::iterator anItr = mResMaps[theType].find(theId);
	if (anItr != mResMaps[theType].end())
		return anItr->second;
	return NULL;
}

void ResourceManager::DeleteResources(const std::string &theGroup) //444-448
{
	for (int i = 0; i < Num_ResTypes; i++)
		DeleteResources(mResMaps[i], theGroup); //?
	mLoadedGroups.erase(theGroup);
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	if (mApp->mResStreamsManager != NULL && mApp->mResStreamsManager->IsInitialized())
		mApp->mResStreamsManager->DeleteGroup(theGroup);
#endif
}

void ResourceManager::DeleteExtraImageBuffers(const std::string &theGroup) //451-462
{
	for (ResMap::iterator anItr = mResMaps[ResType_Image].begin(); anItr != mResMaps[ResType_Image].end(); ++anItr)
	{
		if (theGroup.empty() || anItr->second->mResGroup==theGroup)
		{
			ImageRes *aRes = (ImageRes*)anItr->second;
			MemoryImage *anImage = (MemoryImage*)aRes->mImage;
			if (anImage != NULL)
				anImage->DeleteExtraBuffers();
		}
	}
}

std::string ResourceManager::GetErrorText() //465-467
{
	return mError;
}

bool ResourceManager::HadError() //470-472
{
	return mHasFailed;
}

bool ResourceManager::Fail(const std::string& theErrorText) //475-502
{
	if (!mHasFailed)
	{
		mHasFailed = true;
#if (!defined(_IPHONEOS) && !defined(_ANDROID)) || (defined(_DEBUG) || (defined(_TRANSMENSION))) //On mobile, it's hidden on retail (excluding debug [at least for Android]), as Transmension, Xbox 360 and PS4 still have this, it won't be locked under debug.
		if (mXMLParser==NULL)
		{
			mError = theErrorText;
			return false;
		}

		int aLineNum = mXMLParser->GetCurrentLineNum();

		char aLineNumStr[16];
		sprintf(aLineNumStr, "%d", aLineNum);	

		mError = theErrorText;

		if (aLineNum > 0)
			mError += std::string(" on Line ") + aLineNumStr; //SexyString beyond Bej3?

		if (mXMLParser->GetFileName().length() > 0)
			mError += " in File '" + mXMLParser->GetFileName() + "'";
#endif
		assert(false); //498 (only on non-mobile? Present in BejLiveWin8 [571])
	}

	return false;
}

bool ResourceManager::ParseCommonResource(XMLElement &theElement, BaseRes *theRes, ResMap &theMap) //505-548
{
	mHadAlreadyDefinedError = false;
	theRes->mParent = this;
	theRes->mGlobalPtr = NULL;

	const SexyString &aPath = theElement.mAttributes[_S("path")];
	if (aPath.empty())
		return Fail("No path specified.");

	theRes->mXMLAttributes = theElement.mAttributes;
	theRes->mFromProgram = false;
	if (aPath[0]==_S('!'))
	{
		theRes->mPath = SexyStringToStringFast(aPath);
		if (aPath==_S("!program"))
			theRes->mFromProgram = true;
	}
	else
	{
		theRes->mPath = mDefaultPath + SexyStringToStringFast(aPath);
		mResFromPathMap[Upper(theRes->mPath)] = theRes;
	}

	
	std::string anId;
	XMLParamMap::iterator anItr = theElement.mAttributes.find(_S("id"));
	if (anItr == theElement.mAttributes.end())
		anId = mDefaultIdPrefix + GetFileName(theRes->mPath,true);
	else
		anId = mDefaultIdPrefix + SexyStringToStringFast(anItr->second);

#ifdef _SEXYDECOMP_USE_LATEST_CODE
	if (mCurResGroupArtRes != 0)
		anId = StrFormat("|", mCurResGroupArtRes);
	if (mCurResGroupLocSet != 0)
		anId = StrFormat("%s||%8x", anId, mCurResGroupLocSet);
#endif

	theRes->mResGroup = mCurResGroup;
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	theRes->mCompositeResGroup = mCurCompositeResGroup;
#endif
	theRes->mId = anId;
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	theRes->mArtRes = mCurResGroupArtRes;
	theRes->mLocSet = mCurResGroupLocSet;
#endif

	std::pair<ResMap::iterator,bool> aRet = theMap.insert(ResMap::value_type(anId,theRes));
	if (!aRet.second)
	{
		mHadAlreadyDefinedError = true;
		return Fail("Resource already defined.");
	}

	mCurResGroupList->push_back(theRes);
	return true;
}

bool ResourceManager::ParseSoundResource(XMLElement &theElement) //551-590
{
	SoundRes *aRes = new SoundRes;
	aRes->mSoundId = -1;
	aRes->mVolume = -1;
	aRes->mPanning = 0;

	if (!ParseCommonResource(theElement, aRes, mResMaps[ResType_Sound]))
	{
		if (mHadAlreadyDefinedError && mAllowAlreadyDefinedResources)
		{
			mError = "";
			mHasFailed = false;
			SoundRes *oldRes = aRes;
			aRes = (SoundRes*)mResMaps[ResType_Sound][oldRes->mId];
			aRes->mPath = oldRes->mPath;
			aRes->mXMLAttributes = oldRes->mXMLAttributes;
			delete oldRes;
		}
		else			
		{
			delete aRes;
			return false;
		}
	}
	
	XMLParamMap::iterator anItr;

	anItr = theElement.mAttributes.find(_S("volume"));
	if (anItr != theElement.mAttributes.end())
		sexysscanf(anItr->second.c_str(),_S("%lf"),&aRes->mVolume);

	anItr = theElement.mAttributes.find(_S("pan"));
	if (anItr != theElement.mAttributes.end())
		sexysscanf(anItr->second.c_str(),_S("%d"),&aRes->mPanning);
	aRes->ApplyConfig();
	aRes->mReloadIdx = mReloadIdx;

	return true;
}

static void ReadIntVector(const SexyString &theVal, std::vector<int> &theVector) //593-606 (Not in Blitz)
{
	theVector.clear();

	SexyString::size_type aPos = 0; //SexyString (or widestring?) now
	while (true)
	{
		theVector.push_back(sexyatoi(theVal.c_str()+aPos));
		aPos = theVal.find_first_of(_S(','),aPos);
		if (aPos==std::string::npos)
			break;

		aPos++;
	}	
}

bool ResourceManager::ParseImageResource(XMLElement &theElement) //609-757
{
#ifdef _SEXYDECOMP_USE_LATEST_CODE //theElement.mAttributes.find for bools is Sexy::XMLParser::GetAttributeBool
	SexyString imageID = theElement.mAttributes.find(_S("id"));
	if (imageID)
		return true;
	SexyString imagePath = theElement.mAttributes.find(_S("path"));
	if (imagePath)
		return true;
#endif
	ImageRes *aRes = new ImageRes;
	if (!ParseCommonResource(theElement, aRes, mResMaps[ResType_Image]))
	{
		if (mHadAlreadyDefinedError && mAllowAlreadyDefinedResources)
		{
			mError = "";
			mHasFailed = false;
			ImageRes *oldRes = aRes;
			aRes = (ImageRes*)mResMaps[ResType_Image][oldRes->mId];
			aRes->mPath = oldRes->mPath;
			aRes->mXMLAttributes = oldRes->mXMLAttributes;
			delete oldRes;
		}
		else			
		{
			delete aRes;
			return false;
		}
	}
	
	aRes->mPalletize = theElement.mAttributes.find(_S("nopal")) == theElement.mAttributes.end();
	aRes->mA4R4G4B4 = theElement.mAttributes.find(_S("a4r4g4b4")) != theElement.mAttributes.end();
	aRes->mDDSurface = theElement.mAttributes.find(_S("ddsurface")) != theElement.mAttributes.end();
	//A bool is here in the new revision
	aRes->mPurgeBits = (theElement.mAttributes.find(_S("nobits")) != theElement.mAttributes.end()) ||
		((mApp->Is3DAccelerated()) && (theElement.mAttributes.find(_S("nobits3d")) != theElement.mAttributes.end())) ||
		((!mApp->Is3DAccelerated()) && (theElement.mAttributes.find(_S("nobits2d")) != theElement.mAttributes.end()));
	aRes->mA8R8G8B8 = theElement.mAttributes.find(_S("a8r8g8b8")) != theElement.mAttributes.end();
	aRes->mDither16 = theElement.mAttributes.find(_S("dither16")) != theElement.mAttributes.end();
	aRes->mMinimizeSubdivisions = theElement.mAttributes.find(_S("minsubdivide")) != theElement.mAttributes.end();
	aRes->mAutoFindAlpha = theElement.mAttributes.find(_S("noalpha")) == theElement.mAttributes.end();	
	aRes->mCubeMap = theElement.mAttributes.find(_S("cubemap")) != theElement.mAttributes.end();
	aRes->mVolumeMap = theElement.mAttributes.find(_S("volumemap")) != theElement.mAttributes.end();
	aRes->mNoTriRep = (theElement.mAttributes.find(_S("notrirep")) != theElement.mAttributes.end()) || (theElement.mAttributes.find(_S("noquadrep")) != theElement.mAttributes.end());
	aRes->m2DBig = (theElement.mAttributes.find(_S("2dbig")) == theElement.mAttributes.end());
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	aRes->mIsAtlas = (theElement.mAttributes.find(_S("atlas")) == theElement.mAttributes.end());
#endif

	XMLParamMap::iterator anItr;
	anItr = theElement.mAttributes.find(_S("alphaimage"));
	if (anItr != theElement.mAttributes.end())
		aRes->mAlphaImage = mDefaultPath + SexyStringToStringFast(anItr->second);

	aRes->mAlphaColor = 0xFFFFFF;
	anItr = theElement.mAttributes.find(_S("alphacolor"));
	if (anItr != theElement.mAttributes.end())
		sexysscanf(anItr->second.c_str(),_S("%x"),&aRes->mAlphaColor);

	aRes->mOffset = Point(0,0);
	anItr = theElement.mAttributes.find(_S("x"));
	if (anItr != theElement.mAttributes.end())
		aRes->mOffset.mX = sexysscanf(anItr->second.c_str(), _S("x")); //?

	anItr = theElement.mAttributes.find(_S("y"));
	if (anItr != theElement.mAttributes.end())
		aRes->mOffset.mY = sexysscanf(anItr->second.c_str(), _S("y")); //?

	anItr = theElement.mAttributes.find(_S("variant"));
	if (anItr != theElement.mAttributes.end())
		aRes->mVariant = SexyStringToStringFast(anItr->second);

	anItr = theElement.mAttributes.find(_S("alphagrid"));
	if (anItr != theElement.mAttributes.end())
		aRes->mAlphaGridImage = mDefaultPath + SexyStringToStringFast(anItr->second);

	anItr = theElement.mAttributes.find(_S("rows"));
	if (anItr != theElement.mAttributes.end())
		aRes->mRows = sexyatoi(anItr->second.c_str());
	else
		aRes->mRows = 1;

	anItr = theElement.mAttributes.find(_S("cols"));
	if (anItr != theElement.mAttributes.end())
		aRes->mCols = sexyatoi(anItr->second.c_str());
	else
		aRes->mCols = 1;

#ifdef _SEXYDECOMP_USE_LATEST_CODE
	anItr = theElement.mAttributes.find(_S("parent"));
	if (anItr != theElement.mAttributes.end())
	{
		aRes->mAtlasName = theElement.mAttributes.find(_S("parent"));
		aRes->mAtlasX = sexysscanf(anItr->second.c_str(), _S("ax")); //?
		aRes->mAtlasY = sexysscanf(anItr->second.c_str(), _S("ay")); //?
		aRes->mAtlasW = sexysscanf(anItr->second.c_str(), _S("aw")); //?
		aRes->mAtlasH = sexysscanf(anItr->second.c_str(), _S("ah")); //?
	}
#endif

	if (aRes->mCubeMap)
	{
		if (aRes->mRows * aRes->mCols != 6)
		{
			Fail("Invalid CubeMap definition; must have 6 cells (check rows & cols values).");
			return false;
		}
	}

	if (aRes->mVolumeMap)
	{
		int cellCount = aRes->mRows * aRes->mCols;
		bool isPow2 = cellCount && (cellCount & (cellCount - 1)) != 0;
		if (!isPow2)
		{
			Fail("Invalid VolumeMap definition; must have a pow2 cell count (check rows & cols values).");
			return false;
		}
	}

	anItr = theElement.mAttributes.find(_S("anim"));
	AnimType anAnimType = AnimType_None;
	if (anItr != theElement.mAttributes.end())
	{
		const SexyChar *aType = anItr->second.c_str();

		if (sexystricmp(aType,_S("none"))==0) anAnimType = AnimType_None;
		else if (sexystricmp(aType,_S("once"))==0) anAnimType = AnimType_Once;
		else if (sexystricmp(aType,_S("loop"))==0) anAnimType = AnimType_Loop;
		else if (sexystricmp(aType,_S("pingpong"))==0) anAnimType = AnimType_PingPong;
		else 
		{
			Fail("Invalid animation type.");
			return false;
		}
	}
	aRes->mAnimInfo.mAnimType = anAnimType;
	if (anAnimType != AnimType_None)
	{
		int aNumCels = max(aRes->mRows,aRes->mCols);
		int aBeginDelay = 0, anEndDelay = 0;

		anItr = theElement.mAttributes.find(_S("framedelay"));
		if (anItr != theElement.mAttributes.end())
			aRes->mAnimInfo.mFrameDelay = sexyatoi(anItr->second.c_str());

		anItr = theElement.mAttributes.find(_S("begindelay"));
		if (anItr != theElement.mAttributes.end())
			aBeginDelay = sexyatoi(anItr->second.c_str());

		anItr = theElement.mAttributes.find(_S("enddelay"));
		if (anItr != theElement.mAttributes.end())
			anEndDelay = sexyatoi(anItr->second.c_str());

		anItr = theElement.mAttributes.find(_S("perframedelay"));
		if (anItr != theElement.mAttributes.end())
			ReadIntVector(anItr->second,aRes->mAnimInfo.mPerFrameDelay);

		anItr = theElement.mAttributes.find(_S("framemap"));
		if (anItr != theElement.mAttributes.end())
			ReadIntVector(anItr->second,aRes->mAnimInfo.mFrameMap);

		aRes->mAnimInfo.Compute(aNumCels,aBeginDelay,anEndDelay);
	}

	aRes->ApplyConfig();
	aRes->mReloadIdx = mReloadIdx;
	return true;
}

bool ResourceManager::ParseFontResource(XMLElement &theElement) //760-819
{
	FontRes *aRes = new FontRes;
	aRes->mFont = NULL;
	aRes->mImage = NULL;

	if (!ParseCommonResource(theElement, aRes, mResMaps[ResType_Font]))
	{
		if (mHadAlreadyDefinedError && mAllowAlreadyDefinedResources)
		{
			mError = "";
			mHasFailed = false;
			FontRes *oldRes = aRes;
			aRes = (FontRes*)mResMaps[ResType_Font][oldRes->mPath];
			aRes->mPath = oldRes->mPath;
			aRes->mXMLAttributes = oldRes->mXMLAttributes;
			delete oldRes;
		}
		else			
		{
			delete aRes;
			return false;
		}
	}


	XMLParamMap::iterator anItr;
	anItr = theElement.mAttributes.find(_S("image"));
	if (anItr != theElement.mAttributes.end())
		aRes->mImagePath = SexyStringToStringFast(anItr->second);

	anItr = theElement.mAttributes.find(_S("tags"));
	if (anItr != theElement.mAttributes.end())
		aRes->mTags = SexyStringToStringFast(anItr->second);

	if (strncmp(aRes->mPath.c_str(),"!sys:",5)==0)
	{
		aRes->mSysFont = true;
		aRes->mPath = aRes->mPath.substr(5);

		anItr = theElement.mAttributes.find(_S("size"));
		if (anItr==theElement.mAttributes.end())
			return Fail("SysFont needs point size");

		aRes->mSize = sexyatoi(anItr->second.c_str());
		if (aRes->mSize<=0)
			return Fail("SysFont needs point size");
			
		aRes->mBold = theElement.mAttributes.find(_S("bold"))!=theElement.mAttributes.end();
		aRes->mItalic = theElement.mAttributes.find(_S("italic"))!=theElement.mAttributes.end();
		aRes->mShadow = theElement.mAttributes.find(_S("shadow"))!=theElement.mAttributes.end();
		aRes->mUnderline = theElement.mAttributes.find(_S("underline"))!=theElement.mAttributes.end();
	}
	else
		aRes->mSysFont = false;
	aRes->ApplyConfig();
	aRes->mReloadIdx = mReloadIdx;
	return true;
}

bool ResourceManager::ParsePopAnimResource(XMLElement &theElement) //822-847
{
	PopAnimRes *aRes = new PopAnimRes();
	if (!ParseCommonResource(theElement, aRes, mResMaps[ResType_PopAnim]))
	{
		if (!mHadAlreadyDefinedError || !mAllowAlreadyDefinedResources)
		{
			mError = "";
			mHasFailed = false;
			PopAnimRes* oldRes = aRes;
			aRes = (PopAnimRes*)mResMaps[ResType_PopAnim][oldRes->mId];
			aRes->mPath = oldRes->mPath;
			aRes->mXMLAttributes = oldRes->mXMLAttributes;
			delete oldRes;
		}
		else
		{
			delete aRes;
			return false;
		}
	}

	aRes->ApplyConfig();

	aRes->mReloadIdx = mReloadIdx;
    return true;
}

bool ResourceManager::ParsePIEffectResource(XMLElement &theElement) //850-875
{
	PIEffectRes* aRes = new PIEffectRes();
	if (!ParseCommonResource(theElement, aRes, mResMaps[ResType_PIEffect]))
	{
		if (!mHadAlreadyDefinedError || !mAllowAlreadyDefinedResources)
		{
			mError = "";
			mHasFailed = false;
			PIEffectRes* oldRes = aRes;
			aRes = (PIEffectRes*)mResMaps[ResType_PIEffect][oldRes->mId];
			aRes->mPath = oldRes->mPath;
			aRes->mXMLAttributes = oldRes->mXMLAttributes;
			delete oldRes;
		}
		else
		{
			delete aRes;
			return false;
		}
	}

	aRes->ApplyConfig();

	aRes->mReloadIdx = mReloadIdx;
	return true;
}

bool ResourceManager::ParseRenderEffectResource(XMLElement& theElement) //878-908
{
	RenderEffectRes* aRes = new RenderEffectRes();
	if (!ParseCommonResource(theElement, aRes, mResMaps[ResType_RenderEffect]))
	{
		if (mHadAlreadyDefinedError && mAllowAlreadyDefinedResources)
		{
			mError = "";
			mHasFailed = false;
			RenderEffectRes* oldRes = aRes;
			aRes = (RenderEffectRes*)mResMaps[ResType_RenderEffect][oldRes->mId];
			aRes->mPath = oldRes->mPath;
			aRes->mXMLAttributes = oldRes->mXMLAttributes;
			delete oldRes;
		}
		else
		{
			delete aRes;
			return false;
		}
		
	}

	aRes->mSrcFilePath.clear();
	const SexyString& aSrcFilePath = theElement.mAttributes[_S("srcpath")];

	if (!aSrcFilePath.empty())
		aRes->mSrcFilePath = SexyStringToStringFast(aSrcFilePath, 0) + mDefaultPath;

	aRes->ApplyConfig();

	aRes->mReloadIdx = mReloadIdx;
	return true;
}

bool ResourceManager::ParseSetDefaults(XMLElement &theElement) //911-922
{
	XMLParamMap::iterator anItr;
	anItr = theElement.mAttributes.find(_S("path"));
	if (anItr != theElement.mAttributes.end())
		mDefaultPath = RemoveTrailingSlash(SexyStringToStringFast(anItr->second)) + '/';

	anItr = theElement.mAttributes.find(_S("idprefix"));
	if (anItr != theElement.mAttributes.end())
		mDefaultIdPrefix = RemoveTrailingSlash(SexyStringToStringFast(anItr->second));	

	return true;
}

bool ResourceManager::ParseResources() //925-1027
{
	for (;;)
	{
		XMLElement aXMLElement;
		if (!mXMLParser->NextElement(&aXMLElement))
			return false;
		
		if (aXMLElement.mType == XMLElement::TYPE_START)
		{
			if (aXMLElement.mValue == _S("Image"))
			{
				if (!ParseImageResource(aXMLElement))
					return false;

				if (!mXMLParser->NextElement(&aXMLElement))
					return false;

				if (aXMLElement.mType != XMLElement::TYPE_END)
					return Fail("Unexpected element found.");
			}
			else if (aXMLElement.mValue == _S("Sound"))
			{
				if (!ParseSoundResource(aXMLElement))
					return false;

				if (!mXMLParser->NextElement(&aXMLElement))
					return false;

				if (aXMLElement.mType != XMLElement::TYPE_END)
					return Fail("Unexpected element found.");
			}
			else if (aXMLElement.mValue == _S("Font"))
			{
				if (!ParseFontResource(aXMLElement))
					return false;

				if (!mXMLParser->NextElement(&aXMLElement))
					return false;

				if (aXMLElement.mType != XMLElement::TYPE_END)
					return Fail("Unexpected element found.");
			}
            else if (aXMLElement.mValue == _S("PopAnim"))
            {
                if (!ParsePopAnimResource(aXMLElement))
                    return false;

                if (!mXMLParser->NextElement(&aXMLElement))
                    return false;

                if (aXMLElement.mType != XMLElement::TYPE_END)
                    return Fail("Unexpected element found.");
            }
            else if (aXMLElement.mValue == _S("PIEffect"))
            {
                if (!ParsePIEffectResource(aXMLElement))
                    return false;

                if (!mXMLParser->NextElement(&aXMLElement))
                    return false;

                if (aXMLElement.mType != XMLElement::TYPE_END)
                    return Fail("Unexpected element found.");
            }
			else if (aXMLElement.mValue == _S("RenderEffect"))
			{
				if (!ParseRenderEffectResource(aXMLElement))
					return false;

				if (!mXMLParser->NextElement(&aXMLElement))
					return false;

				if (aXMLElement.mType != XMLElement::TYPE_END)
					return Fail("Unexpected element found.");
			}
#ifdef _SEXYDECOMP_USE_LATEST_CODE //Seen in BejLiveWin8 and beyond
			else if (aXMLElement.mValue == _S("File"))
			{
				if (!ParseGenericResFileResource(aXMLElement))
					return false;

				if (!mXMLParser->NextElement(&aXMLElement))
					return false;

				if (aXMLElement.mType != XMLElement::TYPE_END)
					return Fail("Unexpected element found.");
			}
#endif
			else if (aXMLElement.mValue == _S("SetDefaults"))
			{
				if (!ParseSetDefaults(aXMLElement))
					return false;

				if (!mXMLParser->NextElement(&aXMLElement))
					return false;

				if (aXMLElement.mType != XMLElement::TYPE_END)
					return Fail("Unexpected element found.");		
			}
			else
			{
				Fail("Invalid Section '" + SexyStringToStringFast(aXMLElement.mValue) + "'");
				return false;
			}
		}
		else if (aXMLElement.mType == XMLElement::TYPE_ELEMENT)
		{
			Fail("Element Not Expected '" + SexyStringToStringFast(aXMLElement.mValue) + "'");
			return false;
		}		
		else if (aXMLElement.mType == XMLElement::TYPE_END)
		{
			return true;
		}
	}
}

bool ResourceManager::DoParseResources() //1030-1076
{
	if (!mXMLParser->HasFailed())
	{
		for (;;)
		{
			XMLElement aXMLElement;
			if (!mXMLParser->NextElement(&aXMLElement))
				break;

			if (aXMLElement.mType == XMLElement::TYPE_START)
			{
				if (aXMLElement.mValue == _S("Resources"))
				{
					mCurResGroup = SexyStringToStringFast(aXMLElement.mAttributes[_S("id")]);
					mCurResGroupList = &mResGroupMap[mCurResGroup];

					if (mCurResGroup.empty())
					{
						Fail("No id specified.");
						break;
					}

#ifdef _SEXYDECOMP_USE_LATEST_CODE //TODO
					else
					{
						mCurResGroupList = new ResList();
						mResGroupMap[mCurResGroup] = mCurResGroupList;
					}
					//mCurCompositeResGroup = SexyStringToStringFast(aXMLElement.mAttributes[_S("parent")]);
					int mCurResGroupArtRes = sexyatoi(SexyStringToStringFast(aXMLElement.mAttributes[_S("res")]));
					mCurResGroupLocSet = SexyStringToStringFast(aXMLElement.mAttributes[_S("loc")]);
#endif

					if (!ParseResources())
						break;
				}
				else 
				{
#ifdef _SEXYDECOMP_USE_LATEST_CODE //TODO
					if (!(aXMLElement.mValue == _S("CompositeResources")))
						break;

					mCurResGroup = SexyStringToStringFast(aXMLElement.mAttributes[_S("id")]);
					mCurResGroupList = &mResGroupMap[mCurResGroup];

					if (mCurResGroup.empty())
					{
						Fail("No id specified on CompositeGroup.");
						break;
					}

					for (;;)
					{
						XMLElement aXMLElement;
						if (!mXMLParser->NextElement(&aXMLElement))
							break;

						if (!(aXMLElement.mValue == _S("Group")))
						{
							Fail("Invalid Section '" + SexyStringToStringFast(aXMLElement.mValue) + "' within CompositeGroup");
						}

					}
#endif
					Fail("Invalid Section '" + SexyStringToStringFast(aXMLElement.mValue) + "'");
					break;
				}
			}
			else if (aXMLElement.mType == XMLElement::TYPE_ELEMENT)
			{
				Fail("Element Not Expected '" + SexyStringToStringFast(aXMLElement.mValue) + "'");
				break;
			}
		}
	}

	if (mXMLParser->HasFailed())
		Fail(SexyStringToStringFast(mXMLParser->GetErrorText()));

	delete mXMLParser;
	mXMLParser = NULL;

	return !mHasFailed;
}

bool ResourceManager::ParseResourcesFile(const std::string& theFilename) //1079-1111
{
	if (gResourceManagerInfo)
	{
		ResGenInfoMap::iterator anItr = gResourceManagerInfo->mResGenInfoMap.find(theFilename);
		if (anItr != gResourceManagerInfo->mResGenInfoMap.end())
		{
			mResGenPlatformName = anItr->second.mResGenPlatformName;
			mResGenMinorVersion = anItr->second.mResGenMinorVersion;
			mResGenMajorVersion = anItr->second.mResGenMajorVersion;
			mResWatchFileUsed = anItr->second.mResWatchFileUsed;
			mResPropsUsed = anItr->second.mResPropsUsed;
			InitResourceGen(anItr->second.mResGenExePath, mResPropsUsed, mResWatchFileUsed, mResGenMajorVersion, mResGenMinorVersion, mResGenPlatformName); //Useless
		}
	}

	mLastXMLFileName = theFilename;

#ifdef _SEXYDECOMP_USE_LATEST_CODE
	if (mApp->mResStreamsManager && mApp->mResStreamsManager->IsInitialized())
		return mApp->mResStreamsManager->LoadResourcesManifest(this);
#endif

	mXMLParser = new XMLParser();
	if (!mXMLParser->OpenFile(theFilename))
		Fail("Resource file not found: " + theFilename);

	XMLElement aXMLElement;
	while (!mXMLParser->HasFailed())
	{
		if (!mXMLParser->NextElement(&aXMLElement))
			Fail(SexyStringToStringFast(mXMLParser->GetErrorText()));

		if (aXMLElement.mType == XMLElement::TYPE_START)
		{
			if (aXMLElement.mValue != _S("ResourceManifest"))
				break;
#ifdef _SEXYDECOMP_USE_LATEST_CODE //TODO
			XMLParamMap::iterator anItr = aXMLElement.mAttributes.find(_S("version"));
			if (anItr != aXMLElement.mAttributes.end())
			{
				if (sexyatoi(anItr->second.c_str()) != 2) //Version mismatch
					Fail("Expecting ResourceManifest tag with version 2; please make sure your version of ResourceGen3 is current"); //C++ only.
			}
#endif
			else
				return DoParseResources();
		}
	}
		
	Fail("Expecting ResourceManifest tag");

	return DoParseResources();	
}

bool ResourceManager::ReparseResourcesFile(const std::string& theFilename) //1114-1135
{
	bool oldDefined = mAllowAlreadyDefinedResources;
	mAllowAlreadyDefinedResources = true;
	mReloadIdx++;

	bool aResult = ParseResourcesFile(theFilename);

	for (int aResType = 0; aResType < Num_ResTypes; aResType++) //7 in XNA
	{
		for (ResMap::iterator anItr = mResMaps[aResType].begin(); anItr != mResMaps[aResType].end(); ++anItr)
		{
			BaseRes* aBaseRes = anItr->second;
			if (aBaseRes->mReloadIdx != mReloadIdx)
				aBaseRes->DeleteResource();
		}
	}

	mAllowAlreadyDefinedResources = oldDefined;

	return aResult;
}

void ResourceManager::RegisterGlobalPtr(const std::string& theId, void* theGlobalPtr) //1138-1148
{
	for (int aType = 0; aType < Num_ResTypes; aType++)
	{
		BaseRes* aBaseRes = GetBaseRes(aType, theId);
		if (aBaseRes != NULL)
		{
			aBaseRes->mGlobalPtr = theGlobalPtr;
			mGlobalPtrToResMap[theGlobalPtr] = aBaseRes; //?
		}
	}
}

void ResourceManager::InitResourceGen(const std::string& theResGenExePath, const std::string& theResPropsUsed, const std::string& theResWatchFileUsed, int theResGenMajorVersion, int theResGenMinorVersion, const std::string& thePlatformName) //C++ only | 1151-1188
{
	bool isFirstRun = mResGenExePath.empty();
	mResGenExePath = theResGenExePath;
	mResGenMajorVersion = theResGenMajorVersion;
	mResGenMinorVersion = theResGenMinorVersion;
	mResPropsUsed = theResPropsUsed;
	mResWatchFileUsed = theResWatchFileUsed;
	mResGenPlatformName = thePlatformName;
	if (isFirstRun)
	{
		if (GetFileDate(theResPropsUsed))
		{
			if (mResGenMajorVersion == 2)
			{
				ulong aWatchDate = GetFileDate(theResPropsUsed + ".watch");
				ulong aTraceDate = GetFileDate(theResPropsUsed + ".trace");
				if (aWatchDate > aTraceDate || !aTraceDate)
					gSexyAppBase->ReloadAllResources();
			}
			if (mResGenMajorVersion == 3)
			{
				ulong aTraceDate = GetFileDate(theResPropsUsed + ".trace");
				if (GetFileDate(theResWatchFileUsed) > aTraceDate || !aTraceDate)
					gSexyAppBase->ReloadAllResources();
			}
		}
	}
}

void ResourceManager::ReapplyConfigs() //1191-1197
{
	for (int aResType = 0; aResType < Num_ResTypes; aResType++)
	{
		for (ResMap::iterator anItr = mResMaps[aResType].begin(); anItr != mResMaps[aResType].end(); anItr++)
			anItr->second->ApplyConfig();
	}
}

bool ResourceManager::LoadAlphaGridImage(ImageRes *theRes, DeviceImage *theImage) //Unimplemented in XNA | 1200-1244
{	
	ImageLib::Image* anAlphaImage = ImageLib::GetImage(theRes->mAlphaGridImage,true);	
	if (anAlphaImage==NULL)
		return Fail(StrFormat("Failed to load image: %s",theRes->mAlphaGridImage.c_str()));

	std::auto_ptr<ImageLib::Image> aDelAlphaImage(anAlphaImage);

	int aNumRows = theRes->mRows;
	int aNumCols = theRes->mCols;

	int aCelWidth = theImage->mWidth/aNumCols;
	int aCelHeight = theImage->mHeight/aNumRows;


	if (anAlphaImage->mWidth!=aCelWidth || anAlphaImage->mHeight!=aCelHeight)
		return Fail(StrFormat("GridAlphaImage size mismatch between %s and %s",theRes->mPath.c_str(),theRes->mAlphaGridImage.c_str()));

	unsigned long *aMasterRowPtr = theImage->mBits;
	for (int i=0; i < aNumRows; i++)
	{
		unsigned long *aMasterColPtr = aMasterRowPtr;
		for (int j=0; j < aNumCols; j++)
		{
			unsigned long* aRowPtr = aMasterColPtr;
			unsigned long* anAlphaBits = anAlphaImage->mBits;
			for (int y=0; y<aCelHeight; y++)
			{
				unsigned long *aDestPtr = aRowPtr;
				for (int x=0; x<aCelWidth; x++)
				{
					*aDestPtr = (*aDestPtr & 0x00FFFFFF) | ((*anAlphaBits & 0xFF) << 24);
					++anAlphaBits;
					++aDestPtr;
				}
				aRowPtr += theImage->mWidth;
			}

			aMasterColPtr += aCelWidth;
		}
		aMasterRowPtr += aCelHeight*theImage->mWidth;
	}

	theImage->BitsChanged();
	return true;
}

bool ResourceManager::LoadAlphaImage(ImageRes *theRes, DeviceImage *theImage) //Unimplemented in XNA | 1247-1271
{
	SEXY_PERF_BEGIN("ResourceManager::GetImage"); //Present in Bej3JP and BejLiveWin8
	ImageLib::Image* anAlphaImage = ImageLib::GetImage(theRes->mAlphaImage,true, -1);
	SEXY_PERF_END("ResourceManager::GetImage");

	if (anAlphaImage==NULL)
		return Fail(StrFormat("Failed to load image: %s",theRes->mAlphaImage.c_str()));

	std::auto_ptr<ImageLib::Image> aDelAlphaImage(anAlphaImage);

	if (anAlphaImage->mWidth!=theImage->mWidth || anAlphaImage->mHeight!=theImage->mHeight)
		return Fail(StrFormat("AlphaImage size mismatch between %s and %s",theRes->mPath.c_str(),theRes->mAlphaImage.c_str()));

	unsigned long* aBits1 = theImage->mBits;
	unsigned long* aBits2 = anAlphaImage->mBits;
	int aSize = theImage->mWidth*theImage->mHeight;

	for (int i = 0; i < aSize; i++)
	{
		*aBits1 = (*aBits1 & 0x00FFFFFF) | ((*aBits2 & 0xFF) << 24);
		++aBits1;
		++aBits2;
	}

	theImage->BitsChanged();
	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool ResourceManager::DoLoadImage(ImageRes *theRes) //1275-1376
{
	AutoCrit aCrit(mLoadCrit);
	std::string &aPath = theRes->mPath;
	std::string aResizePath = StrFormat("images\\%d\\", mBaseArtRes);
	DeviceImage* anImage;
	bool gotFromCache;

	if (strnicmp(theRes->mPath.c_str(), aResizePath.c_str(), aResizePath.length()) && (!theRes->m2DBig || mApp->Is3DAccelerated()) == 0)
		aPath = StrFormat("images\\%d\\%s", mCurArtRes, aPath.substr(aResizePath.length()).c_str());

	if (strnicmp(aPath.c_str(), "!ref:", 5) == 0)
	{
		std::string aRefName = aPath.substr(5);
		theRes->mResourceRef = GetImageRef(aRefName);
		SharedImageRef aRefImage(theRes->mResourceRef); //?
		
		if (&aRefImage == NULL)
			return Fail(StrFormat("Ref Image not found:", aRefName));

		theRes->mImage = aRefImage;
#ifdef _SEXYDECOMP_USE_LATEST_CODE
		theRes->mGlobalPtr = RegisterGlobalPtr(aRefName);
#endif
	}
	else
	{
#ifdef _SEXYDECOMP_USE_LATEST_CODE
		bool isAtlas = theRes->mAtlasName != NULL;
#endif
		gotFromCache = false;
		SharedImageRef aSharedImageRef = gSexyAppBase->CheckSharedImage(aPath, theRes->mVariant);
		if (&aSharedImageRef)
			gotFromCache = true;
#ifdef _SEXYDECOMP_USE_LATEST_CODE
		else if (!isAtlas)
#else
		else
#endif
		{
			anImage = DeviceImage::ReadFromCache(GetFullPath(aPath), "ResMan");
			if (anImage != NULL)
			{
				SharedImageRef aSharedImageRef = gSexyAppBase->SetSharedImage(aPath, theRes->mVariant, anImage, false);
				theRes->mImage = aSharedImageRef;
				gotFromCache = true;
			}
		}

		SEXY_PERF_BEGIN("ResourceManager:GetImage"); //Present in Bej3JP and BejLiveWin8, Not debug only

		bool isNew;
		ImageLib::gAlphaComposeColor = theRes->mAlphaColor; //C++ only
		bool oldWriteToSexyCache = gSexyAppBase->mWriteToSexyCache;
		gSexyAppBase->mWriteToSexyCache = false;
		if (!gotFromCache)
			aSharedImageRef = gSexyAppBase->GetSharedImage(theRes->mPath, theRes->mVariant, &isNew, !theRes->mNoTriRep);

		gSexyAppBase->mWriteToSexyCache = oldWriteToSexyCache;
		ImageLib::gAlphaComposeColor = 0xFFFFFF;

		DeviceImage* aDDImage = (DeviceImage*)aSharedImageRef;

		if (aDDImage == NULL)
			return Fail(StrFormat("Failed to load image: %s", aPath.c_str()));

		if (isNew)
		{
#ifdef _SEXYDECOMP_USE_LATEST_CODE
			if (isAtlas)
			{
				aDDImage->mWidth = theRes->mAtlasW;
				aDDImage->mHeight = theRes->mAtlasH;
			}
#endif
			if (!theRes->mAlphaImage.empty())
			{
				if (!LoadAlphaImage(theRes, aSharedImageRef))
					return false;
			}

			if (!theRes->mAlphaGridImage.empty())
			{
				if (!LoadAlphaGridImage(theRes, aSharedImageRef))
					return false;
			}
		}

		if (theRes->mPalletize && !gotFromCache)
		{
			SEXY_PERF_BEGIN("ResourceManager:Palletize"); //Not debug only
			if (aDDImage->mSurface == NULL)
				aDDImage->Palletize();
			else
				aDDImage->mWantPal = true;
			SEXY_PERF_END("ResourceManager:Palletize"); //Not debug only
		}

		theRes->mImage = aSharedImageRef;

		theRes->ApplyConfig();
		if (theRes->mGlobalPtr)
			theRes->mGlobalPtr = aDDImage;

		if (!gotFromCache)
			aDDImage->WriteToCache(GetFullPath(aPath), "ResMan");

		ResourceLoadedHook(theRes);
		return true;
	}
}

void ResourceManager::DeleteImage(const std::string& theName) //1279-1386
{
	BaseRes* aBaseRes = GetBaseRes(ResType_Image, theName);
	if (aBaseRes && aBaseRes->mDirectLoaded)
		aBaseRes->mDirectLoaded = 0;
	Deref(aBaseRes);
}

SharedImageRef ResourceManager::LoadImage(const std::string& theName) //Correct | 1489-1412
{
	AutoCrit aCrit(mLoadCrit);
	ImageRes* aRes = new ImageRes;
	aRes = (ImageRes*)GetBaseRes(ResType_Image, theName);
	if (aRes)
		return NULL;

	if (!aRes->mDirectLoaded)
	{
		aRes->mRefCount++;
		aRes->mDirectLoaded = true;
	}

	if (aRes->mFromProgram)
		return NULL;

	if (!DoLoadImage(aRes))
		return NULL;

	return aRes->mImage;
}

Point ResourceManager::GetImageOffset(const std::string& theName) //1415-1421
{
	static Point aPoint;
	ImageRes* aRes = (ImageRes*)GetBaseRes(ResType_Image, theName);
	if (aRes)
		return aRes->mOffset;
	else
		return aPoint;
}

bool ResourceManager::DoLoadSound(SoundRes* theRes) //1424-1458
{
	AutoCrit aCrit(mLoadCrit);
	SoundRes* aRes = theRes;

	if (strncmp(theRes->mPath.c_str(), "!ref:", 5) == 0)
	{
		std::string aRefName = theRes->mPath.substr(5);
		theRes->mResourceRef = GetSoundRef(aRefName);
		int aRefSound = theRes->mResourceRef.operator int();
		if (aRefSound == -1)
			return Fail(StrFormat("Ref sound not found: ", aRefName));
		else
			theRes->mSoundId = aRefSound;
	}

	SEXY_PERF_BEGIN("ResourceManager:LoadSound"); //Present in Bej3JP and BejLiveWin8
	int aSoundId = mApp->mSoundManager->GetFreeSoundId();
	if (aSoundId < 0)
		return Fail("Out of free sound ids");

	if (!mApp->mSoundManager->LoadSound(aSoundId, aRes->mPath))
		return Fail(StrFormat("Failed to load sound: %s", aRes->mPath.c_str()));
	SEXY_PERF_END("ResourceManager:LoadSound");

	aRes->mSoundId = aSoundId;
	if (theRes->mGlobalPtr != NULL)
		*(DWORD*)theRes->mGlobalPtr = aSoundId;
	aRes->ApplyConfig();

	ResourceLoadedHook(theRes);

	return true;
}

bool ResourceManager::DoLoadPopAnim(PopAnimRes* theRes) //1461-1487
{
	AutoCrit aCrit(mLoadCrit);
	PopAnim* aPopAnim = new PopAnim(0, NULL);

	std::string aResizePath = StrFormat("images\\%d\\" + mBaseArtRes);

	if (!strnicmp(theRes->mPath.c_str(), aResizePath.c_str(), aResizePath.length()))
	{
		aPopAnim->mImgScale = (float)mCurArtRes / (float)mBaseArtRes;
		aPopAnim->mDrawScale = (float)mCurArtRes / (float)mBaseArtRes;
	}

	aPopAnim->LoadFile(theRes->mPath);

	if (!aPopAnim->mError.empty())
	{
		Fail("PopAnim loading error: " + aPopAnim->mError + " on file " + theRes->mPath);
		delete aPopAnim;
		return false;
	}

	if (theRes->mGlobalPtr)
		theRes->mGlobalPtr = aPopAnim;
	theRes->mPopAnim = aPopAnim;

	return true;
}

bool ResourceManager::DoLoadPIEffect(PIEffectRes* theRes) //1490-1508
{
	AutoCrit aCrit(mLoadCrit);
	PIEffect* aPIEffect = new PIEffect;
	aPIEffect->LoadEffect(theRes->mPath);
	if (!aPIEffect->mError.empty())
	{
		Fail("PIEffect loading error: " + aPIEffect->mError + " on file " + theRes->mPath);
		delete aPIEffect;
		return false;
	}

	if (theRes->mGlobalPtr)
		theRes->mGlobalPtr = aPIEffect;
	theRes->mPIEffect = aPIEffect;
	return true;
}

bool ResourceManager::DoLoadRenderEffect(RenderEffectRes* theRes) //Dummied in XNA | 1511-1529
{
	AutoCrit aCrit(mLoadCrit);
	RenderEffectDefinition* aDefinition = new RenderEffectDefinition();
	aDefinition->LoadFromFile(theRes->mPath.c_str(), theRes->mSrcFilePath.c_str());
	if (aDefinition == NULL)
	{
		Fail("RenderEffect loading error on file " + theRes->mPath);
		delete aDefinition;
		return false;
	}
	else
	{
		theRes->mRenderEffectDefinition = aDefinition;
		if (theRes->mGlobalPtr)
			theRes->mGlobalPtr = aDefinition;
		return true;
	}
}

bool ResourceManager::DoLoadFont(FontRes* theRes) //1532-1640
{
	AutoCrit aCrit(mLoadCrit);
	Font* aFont = NULL;
	SEXY_PERF_BEGIN("ResourceManager:DoLoadFont"); //Keeping this so latest Prime doesn't lose this perf
#ifndef _SEXYDECOMP_USE_LATEST_CODE //Not in current Prime
	std::string aPath = theRes->mPath;
	std::string aResizePath = StrFormat("fonts\\%d\\", mBaseArtRes);

	if (strncmp(theRes->mPath.c_str(), aResizePath.c_str(), aResizePath.length() == 0))
		aPath = StrFormat("fonts\\%d\\%s", mCurArtRes, aPath.substr(aResizePath.length()));
#endif
	SexyString aCustomPathOpt = _S("path%d", mCurArtRes); //Definitely a SexyString

	if (theRes->mXMLAttributes.find(aCustomPathOpt) != theRes->mXMLAttributes.end())
		aPath = ToString(theRes->mXMLAttributes[aCustomPathOpt]);

	if (theRes->mSysFont)
	{
#ifndef _SEXYDECOMP_USE_LATEST_CODE //Not in current Prime
		bool bold = theRes->mBold, simulateBold = false;
		if (Sexy::CheckFor98Mill()) //Only on Win
		{
			simulateBold = bold;
			bold = false;
		}
		aFont = new SysFont(aPath, theRes->mSize, bold, theRes->mItalic, theRes->mUnderline);
		SysFont* aSysFont = (SysFont*)aFont;
		aSysFont->mDrawShadow = theRes->mShadow;
		aSysFont->mSimulateBold = simulateBold;
#endif
	}
	else if (theRes->mImagePath.empty())
	{
		if (strncmp(aPath.c_str(), "!ref:", 5) == 0)
		{
			std::string aRefName = aPath.substr(5);
			theRes->mResourceRef = GetFontRef(aRefName);
			Font* aRefFont = theRes->mResourceRef.operator Font *(); //?
			if (aRefFont == NULL)
				return Fail("Ref font not found: " + aRefName);

			aFont = aRefFont->Duplicate();
			aFont = theRes->mFont;
		}
		else
		{
			ImageFont* anImageFont = ImageFont::ReadFromCache(GetFullPath(aPath), "ResMan");
			if (anImageFont != NULL)
				aFont = anImageFont;
			else
			{
				anImageFont = new ImageFont(mApp, aPath);
				aFont = anImageFont;
				anImageFont->WriteToCache(GetFullPath(aPath), "ResMan"); //C++ only, calls "ResMan", why
			}
		}
	}
	else
	{
		Image* anImage = mApp->GetImage(theRes->mImagePath);
		if (anImage == NULL)
			return Fail(StrFormat("Failed to load image: %s", theRes->mImagePath.c_str()));

		theRes->mImage = anImage;
		aFont = new ImageFont(anImage, aPath);
	}

	ImageFont* anImageFont = dynamic_cast<ImageFont*>(aFont);
	if (anImageFont != NULL)
	{
		if (anImageFont->mFontData == NULL || !anImageFont->mFontData->mInitialized)
		{
			delete aFont;
			return Fail(StrFormat("Failed to load font: %s", theRes->mPath.c_str()));
		}

		anImageFont->mTagVector.clear();
		anImageFont->mActiveListValid = false;

		if (!theRes->mTags.empty())
		{
			char aBuf[1024];
			strcpy(aBuf, theRes->mTags.c_str());
			const char* aPtr = strtok(aBuf, ", \r\n\t");
			while (aPtr != NULL)
			{
				anImageFont->AddTag(aPtr);
				aPtr = strtok(NULL, ", \r\n\t");
			}
			anImageFont->Prepare();
		}
	}

	theRes->mFont = aFont;
	if (theRes->mGlobalPtr)
		theRes->mGlobalPtr = aFont;
	theRes->ApplyConfig();

	SEXY_PERF_END("ResourceManager:DoLoadFont"); //Keeping this so latest Prime doesn't lose this perf

	ResourceLoadedHook(theRes);
	return true;
}

Font* ResourceManager::LoadFont(const std::string& theName) //1643-1666
{
	AutoCrit aCrit(mLoadCrit);

	FontRes* aRes = (FontRes*)GetBaseRes(ResType_Font, theName);
	if (aRes == NULL)
		return NULL;

	if (!aRes->mDirectLoaded)
	{
		aRes->mRefCount++;
		aRes->mDirectLoaded = true;
	}

	if (aRes->mFont != NULL)
		return aRes->mFont;

	if (aRes->mFromProgram)
		return NULL;

	if (!DoLoadFont(aRes))
		return NULL;

	return aRes->mFont;
}

void ResourceManager::DeleteFont(const std::string& theName) //Unimplemented in XNA | 1669-1671
{
	ReplaceFont(theName,NULL);
}

PopAnim* ResourceManager::LoadPopAnim(const std::string& theName) //1674-1697
{
	AutoCrit aCrit(mLoadCrit);

	PopAnimRes* aRes = (PopAnimRes*)GetBaseRes(ResType_PopAnim, theName);
	if (aRes == NULL)
		return NULL;

	if (!aRes->mDirectLoaded)
	{
		aRes->mRefCount++;
		aRes->mDirectLoaded = true;
	}

	if (aRes->mPopAnim == NULL)
		return NULL;

	if (aRes->mFromProgram)
		return NULL;

	if (!DoLoadPopAnim(aRes))
		return NULL;

	return aRes->mPopAnim;
}

void ResourceManager::DeletePopAnim(const std::string& theName) //Unimplemented in XNA | 1700-1702
{
	ReplacePopAnim(theName,NULL);
}

PIEffect* ResourceManager::LoadPIEffect(const std::string& theName) //1705-1728
{
	AutoCrit aCrit(mLoadCrit);

	PIEffectRes* aRes = (PIEffectRes*)GetBaseRes(ResType_PIEffect, theName);
	if (aRes == NULL)
		return NULL;

	if (!aRes->mDirectLoaded)
	{
		aRes->mRefCount++;
		aRes->mDirectLoaded = true;
	}

	if (aRes->mPIEffect == NULL)
		return NULL;

	if (aRes->mFromProgram)
		return NULL;

	if (!DoLoadPIEffect(aRes))
		return NULL;

	return aRes->mPIEffect;
}

void ResourceManager::DeletePIEffect(const std::string& theName) //Unimplemented in XNA | 1731-1733
{
	ReplacePIEffect(theName,NULL);
}

RenderEffectDefinition* ResourceManager::LoadRenderEffect(const std::string& theName) //Unimplemented in XNA | 1736-1759
{
	AutoCrit aCrit(mLoadCrit);
	
	RenderEffectRes* aRes = (RenderEffectRes*)GetBaseRes(ResType_RenderEffect, theName);
	if (aRes == NULL)
		return NULL;

	if (!aRes->mDirectLoaded)
	{
		aRes->mRefCount++;
		aRes->mDirectLoaded = true;
	}

	if (aRes->mRenderEffectDefinition != NULL)
		return aRes->mRenderEffectDefinition;

	if (aRes->mFromProgram)
		return NULL;

	if (!DoLoadRenderEffect(aRes))
		return NULL;

	return aRes->mRenderEffectDefinition;
}

void ResourceManager::DeleteRenderEffect(const std::string& theName) //Unimplemented in XNA | 1762-1764
{
	ReplaceRenderEffect(theName,NULL);
}

int ResourceManager::LoadSound(const std::string& theName) //1767-1790
{
	AutoCrit aCrit(mLoadCrit);

	SoundRes* aRes = (SoundRes*)GetBaseRes(ResType_Sound, theName);
	if (aRes != NULL)
		return -1;

	if (!aRes->mDirectLoaded)
	{
		aRes->mRefCount++;
		aRes->mDirectLoaded = true;
	}

	if (aRes->mSoundId == -1)
		return -1;

	if (aRes->mFromProgram)
		return -1;

	if (!DoLoadSound(aRes))
		return -1;

	return aRes->mSoundId;
}

void ResourceManager::DeleteSound(const std::string& theName) //Unimplemented in XNA | 1793-1795
{
	ReplaceSound(theName,-1);
}

bool ResourceManager::DoLoadResource(BaseRes* theRes, bool* skipped) //ALMOST | 1798-1854
{
	*skipped = false;
    if (theRes->mFromProgram)
    {
        *skipped = true;
        return true;
    }

    switch (theRes->mType) //No clue how these perfs are visible
    {
        case ResType_Image:
			//SexyAutoPerf aPerf("ResourceManager::DoLoadResource(ResType_Image)";
			ImageRes *anImageRes = (ImageRes*)theRes;
			DBG_ASSERTE((DeviceImage*) anImageRes->mImage == 0); //1813 | 2029 in BejLiveWin8
            return DoLoadImage((ImageRes*)theRes);
        case ResType_Sound:
			//SexyAutoPerf aPerf = "ResourceManager::DoLoadResource(ResType_Sound)";
			SoundRes* aSoundRes = (SoundRes*)theRes;
            return DoLoadSound((SoundRes*)theRes);
        case ResType_Font:
			//SexyAutoPerf aPerf = "ResourceManager::DoLoadResource(ResType_Font)";
			FontRes* aFontRes = (FontRes*)theRes;
			return DoLoadFont((FontRes*)theRes);
		case ResType_PopAnim:
			//SexyAutoPerf aPerf = "ResourceManager::DoLoadResource(ResType_PopAnim)";
			PopAnimRes* aPopAnimRes = (PopAnimRes*)theRes;
			return DoLoadPopAnim((PopAnimRes*)theRes);
            break;
		case ResType_PIEffect:
			//SexyAutoPerf aPerf = "ResourceManager::DoLoadResource(ResType_PIEffect)";
			//SEXY_AUTO_PERF(aPerf);
			PIEffectRes* aPIEffectRes = (PIEffectRes*)theRes;
			return DoLoadPIEffect((PIEffectRes*)theRes);
		case ResType_RenderEffect:
			//SexyAutoPerf aPerf = "ResourceManager::DoLoadResource(ResType_RenderEffect)";
			RenderEffectRes* aRenderEffectRes = (RenderEffectRes*)theRes;
			return DoLoadRenderEffect((RenderEffectRes*)theRes);
#ifdef _SEXYDECOMP_USE_LATEST_CODE
        case ResType_GenericResFile: //Comment, not in BejLiveWin8 either
            //SexyAutoPerf aPerf("ResourceManager::DoLoadResource(ResType_GenericResFile)");
            return DoLoadGenericResFile((GenericResFileRes*)theRes);
            break;
#endif
		default: return false;
    }
}

bool ResourceManager::LoadNextResource() //1857-1880
{
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	if (SexyAppBase::gAppSuspended)
	{
		printf("\n CPU usage , Loading, LoadNextResource, Returning and thus sleeping coz app is suspended");
		return true;
	}
#endif

	if (HadError())
		return false;

	if (mCurResGroupList == NULL)
		return false;

	while (mCurResGroupListItr != mCurResGroupList->end())
	{
		bool result = true;
		bool skipped = true;
		BaseRes* aRes = *mCurResGroupListItr++;
		if (!aRes->mRefCount)
			result = DoLoadResource(aRes, &skipped);
		aRes->mDirectLoaded = true;
		aRes->mRefCount++;
		if (!skipped)
			return result;
	}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
	if (mCurCompositeResGroup.size > 0 && this.mCompositeResGroupMap.ContainsKey(mCurCompositeResGroup))
	{
		CompositeResGroup compositeResGroup = mCompositeResGroupMap[mCurCompositeResGroup];
		int count = compositeResGroup->mSubGroups.size();
		SubGroup subGroup = compositeResGroup->mSubGroups[i];
		for (int i = this.mCurCompositeSubGroupIndex + 1; i < count; i++)
		{
			if (subGroup->mGroupName.size() > 0 && (subGroup->mArtRes == 0 || subGroup->mArtRes == mCurArtRes) && (subGroup->mLocSet == 0U || subGroup->mLocSet == mCurLocSet))
			{
				mCurCompositeSubGroupIndex = i;
				StartLoadResources(subGroup.mGroupName, true);
				return LoadNextResource();
			}
		}
	}
#endif

	return false;
}

void ResourceManager::ResourceLoadedHook(BaseRes *theRes) //1883-1884
{
}

void ResourceManager::StartLoadResources(const std::string &theGroup) //TODO: Changed in latest Prime | 1887-1894
{
	mError = "";
	mHasFailed = false;

	mCurResGroup = theGroup;
	mCurResGroupList = &mResGroupMap[theGroup];
	mCurResGroupListItr = mCurResGroupList->begin();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void ResourceManager::DumpCurResGroup(std::string& theDestStr) //1899-1931
{
	const ResList* rl = &mResGroupMap.find(mCurResGroup)->second;
	ResList::const_iterator it = rl->begin();
	theDestStr = StrFormat("About to dump %d elements from current res group name %s\r\n", rl->size(), mCurResGroup.c_str());
	
	ResList::const_iterator rl_end = rl->end();
	while (it != rl_end)
	{
		BaseRes* br = *it++;
		std::string prefix = StrFormat("%s: %s\r\n", br->mId.c_str(), br->mPath.c_str());
		theDestStr += prefix;
		if (br->mFromProgram)
			theDestStr += std::string("     res is from program\r\n");
		else if (br->mType == ResType_Image)
			theDestStr += std::string("     res is an image\r\n");
		else if (br->mType == ResType_Sound)
			theDestStr += std::string("     res is a sound\r\n");
		else if (br->mType == ResType_Font)
			theDestStr += std::string("     res is a font\r\n");
		else if (br->mType == ResType_PopAnim)
			theDestStr += std::string("     res is a popanim\r\n");
		else if (br->mType == ResType_PIEffect)
			theDestStr += std::string("     res is a popanim\r\n");
		else if (br->mType == ResType_RenderEffect)
			theDestStr += std::string("     res is a rendereffectdefinition\r\n");
#ifdef _SEXYDECOM_USE_LATEST_CODE
		else if (br->mType == ResType_GenericResFile)
			theDestStr += std::string("     res is a genericresfile\r\n");
#endif

		if (it == mCurResGroupListItr)
			theDestStr += std::string("iterator has reached mCurResGroupItr\r\n");

	}

	theDestStr += std::string("Done dumping resources\r\n");
}

bool ResourceManager::LoadResources(const std::string &theGroup) //1934-1954
{
	if (mApp->mResStreamsManager != NULL && mApp->mResStreamsManager->IsInitialized())
		mApp->mResStreamsManager->ForceLoadGroup(theGroup);

	mError = "";
	mHasFailed = false;
	StartLoadResources(theGroup);
	while (LoadNextResource())
	{
	}

	if (!HadError())
	{
		mLoadedGroups.insert(theGroup);
		return true;
	}
	else
		return false;
}

int	ResourceManager::GetNumResources(const std::string &theGroup, ResMap &theMap) //1957-1970
{
	if (theGroup.empty())
		return theMap.size();

	int aCount = 0;
	for (ResMap::iterator anItr = theMap.begin(); anItr != theMap.end(); ++anItr)
	{
		BaseRes *aRes = anItr->second;
		if (aRes->mResGroup==theGroup && !aRes->mFromProgram)
			++aCount;
	}

	return aCount;
}

int	ResourceManager::GetNumImages(const std::string &theGroup) //1973-1975
{
	return GetNumResources(theGroup, mResMaps[ResType_Image]);
}

int	ResourceManager::GetNumSounds(const std::string &theGroup) //1978-1980
{
	return GetNumResources(theGroup, mResMaps[ResType_Sound]);
}

int ResourceManager::GetNumFonts(const std::string &theGroup) //1983-1985
{
	return GetNumResources(theGroup, mResMaps[ResType_Font]);
}

int	ResourceManager::GetNumResources(const std::string &theGroup) //1988-1993
{
	int aCount = 0;
	for (int i = 0; i < Num_ResTypes; ++i)
		aCount += GetNumResources(theGroup, mResMaps[i]);
	return aCount;
}

SharedImageRef ResourceManager::GetImage(const std::string &theId) //1996-2001
{
	ImageRes* aRes = (ImageRes*)GetBaseRes(ResType_Image, theId);
	if (aRes == NULL)
		return NULL;
	return aRes->mImage;
}
	
int	ResourceManager::GetSound(const std::string &theId) //2004-2009
{
	SoundRes* aRes = (SoundRes*)GetBaseRes(ResType_Sound, theId);
	if (!aRes)
		return 0;		
	return aRes->mSoundId;
}

Font* ResourceManager::GetFont(const std::string &theId) //2012-2017
{
	FontRes* aRes = (FontRes*)GetBaseRes(ResType_Font, theId);
	if (aRes == NULL)
		return NULL;		
	return aRes->mFont;
}

PopAnim* ResourceManager::GetPopAnim(const std::string& theId) //2020-2025
{
	PopAnimRes* aRes = (PopAnimRes*)GetBaseRes(ResType_PopAnim, theId);
	if (aRes == NULL)
		return NULL;
	return aRes->mPopAnim;
}

PIEffect* ResourceManager::GetPIEffect(const std::string& theId) //2028-2033
{
	PIEffectRes* aRes = (PIEffectRes*)GetBaseRes(ResType_PIEffect, theId);
	if (aRes == NULL)
		return NULL;
	return aRes->mPIEffect;
}

RenderEffectDefinition* ResourceManager::GetRenderEffect(const std::string& theId) //2036-2041
{
	RenderEffectRes* aRes = (RenderEffectRes*)GetBaseRes(ResType_RenderEffect, theId);
	if (aRes == NULL)
		return NULL;
	return aRes->mRenderEffectDefinition;
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
GenericResFile* ResourceManager::GetGenericResFile(const std::string& theId)
{
	GenericResFile* aRes = (GenericResFile*)GetBaseRes(ResType_GenericResFile, theId);
	if (aRes == NULL)
		return NULL;
	return aRes->mGenericResFile;
}
#endif

std::string ResourceManager::GetIdByPath(const std::string& thePath) //2044-2062
{
	std::string aCheckStr = Upper(thePath);
	for (int i = 0; i < aCheckStr.length(); i++)
	{
		if (aCheckStr[i] == '/')
			aCheckStr[i] == '\\';
	}
	for (int aMapIdx = 0; aMapIdx < Num_ResTypes; aMapIdx++)
	{
		for (ResMap::iterator anItr = mResMaps[aMapIdx].begin(); anItr != mResMaps[aMapIdx].end(); ++anItr)
		{
			if (Upper(anItr->second->mPath) == aCheckStr)
				return anItr->second->mId;
		}
	}
	return "";
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
SharedImageRef ResourceManager::GetImageThrow(const std::string& theId, int artRes, bool optional)
#else
SharedImageRef ResourceManager::GetImageThrow(const std::string& theId, bool optional) //2065-2080
#endif
{
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	if (gSexyAppBase->mShutdown) //mApp in XNA
		return NULL;
	if (artRes != 0 && artRes != mCurArtRes)
	{
		Fail(StrFormat("Attempted to load image of incorrect art resolution %d (expected %d): %s", artRes, theId.c_str()));
		throw ResourceManagerException(GetErrorText());
	}
#endif
	ImageRes* aRes = (ImageRes*)GetBaseRes(ResType_Image, theId);
	if (aRes != NULL)
	{
		if ((MemoryImage*)aRes->mImage != NULL)
			return aRes->mImage;

		if (mAllowMissingProgramResources && aRes->mFromProgram)
			return NULL;
	}

	else if (optional)
		return NULL;
	

	Fail(StrFormat("Image resource not found: %s", theId.c_str()));
	throw ResourceManagerException(GetErrorText());
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
int ResourceManager::GetSoundThrow(const std::string& theId, int artRes)
#else
int ResourceManager::GetSoundThrow(const std::string& theId) //2083-2097
#endif
{
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	if (gSexyAppBase->mShutdown) //mApp in XNA
		return NULL;
#endif
	SoundRes* aRes = (SoundRes*)GetBaseRes(ResType_Sound, theId);
	if (aRes != NULL)
	{
		if (aRes->mSoundId != -1)
			return aRes->mSoundId;

		if (mAllowMissingProgramResources && aRes->mFromProgram)
			return -1;
	}

	Fail(StrFormat("Sound resource not found: %s", theId.c_str()));
	throw ResourceManagerException(GetErrorText());
	return -1;
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
Font* ResourceManager::GetFontThrow(const std::string& theId, int artRes)
#else
Font* ResourceManager::GetFontThrow(const std::string& theId) //2100-2113
#endif
{
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	if (gSexyAppBase->mShutdown) //mApp in XNA
		return NULL;
	if (artRes != 0 && artRes != mCurArtRes)
	{
		Fail(StrFormat("Attempted to load font of incorrect art resolution %d (expected %d): %s", artRes, theId.c_str()));
		throw ResourceManagerException(GetErrorText());
	}
#endif
	FontRes* aRes = (FontRes*)GetBaseRes(ResType_Font, theId);
	if (aRes != NULL)
	{
		if (aRes->mFont != NULL)
			return aRes->mFont;

		if (mAllowMissingProgramResources && aRes->mFromProgram)
			return NULL;
	}

	Fail(StrFormat("Font resource not found: %s", theId.c_str()));
	throw ResourceManagerException(GetErrorText());
}

PopAnim* ResourceManager::GetPopAnimThrow(const std::string& theId) //NULL in XNA | 2116-2129
{
	PopAnimRes* aRes = (PopAnimRes*)GetBaseRes(ResType_PopAnim, theId);
	if (aRes != NULL)
	{
		if (aRes->mPopAnim != NULL)
			return aRes->mPopAnim;

		if (mAllowMissingProgramResources && aRes->mFromProgram)
			return NULL;
	}

	Fail(StrFormat("PopAnim resource not found: %s", theId.c_str()));
	throw ResourceManagerException(GetErrorText());
}

PIEffect* ResourceManager::GetPIEffectThrow(const std::string& theId) //2132-2145
{
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	if (gSexyAppBase->mShutdown) //mApp in XNA
		return NULL;
#endif
	PIEffectRes* aRes = (PIEffectRes*)GetBaseRes(ResType_PIEffect, theId);
	if (aRes != NULL)
	{
		if (aRes->mPIEffect != NULL)
			return aRes->mPIEffect;

		if (mAllowMissingProgramResources && aRes->mFromProgram)
			return NULL;
	}

	Fail(StrFormat("PIEffect resource not found: %s", theId.c_str()));
	throw ResourceManagerException(GetErrorText());
}

RenderEffectDefinition* ResourceManager::GetRenderEffectThrow(const std::string& theId) //NULL in XNA | 2148-2161
{
	RenderEffectRes* aRes = (RenderEffectRes*)GetBaseRes(ResType_RenderEffect, theId);
	if (aRes != NULL)
	{
		if (aRes->mRenderEffectDefinition != NULL)
			return aRes->mRenderEffectDefinition;

		if (mAllowMissingProgramResources && aRes->mFromProgram)
			return NULL;
	}

	Fail(StrFormat("RenderEffectDefinition resource not found: %s", theId.c_str()));
	throw ResourceManagerException(GetErrorText());
}

ResourceRef ResourceManager::GetResourceRef(BaseRes* theBaseRes) //2164-2174
{
	ResourceRef aResourceRef;

	bool skipped;
	aResourceRef.mBaseResP = theBaseRes;
	if (theBaseRes->mRefCount == 0)
		DoLoadResource(theBaseRes, &skipped);

	theBaseRes->mRefCount++;
	return aResourceRef;
}

ResourceRef ResourceManager::GetResourceRef(int type, const std::string& theId) //2177-2182
{
	BaseRes* aBaseRes = GetBaseRes(type, theId);
	if (aBaseRes != NULL)
		return GetResourceRef(aBaseRes);
	return ResourceRef();
}

ResourceRef ResourceManager::GetResourceRef(void* theGlobalPtr) //C++ only | 2185-2190
{
	GlobalPtrToResMap::iterator anItr = mGlobalPtrToResMap.find(theGlobalPtr); //?
	if (anItr != mGlobalPtrToResMap.end())
		return GetResourceRef(anItr->second);
	return ResourceRef();
}

ResourceRef ResourceManager::GetResourceRefFromPath(const std::string& theFileName) //2193-2201
{
	std::string anUpperPath = Upper(theFileName);
	ResMap::iterator anItr = mResFromPathMap.find(anUpperPath); //?
	if (anItr != mResFromPathMap.end())
		return GetResourceRef(anItr->second);
	return ResourceRef();
}

ResourceRef ResourceManager::GetImageRef(const std::string& theId) //2204-2206
{
	return GetResourceRef(ResType_Image, theId);
}

ResourceRef ResourceManager::GetImageRef(void* theGlobalPtrRef) //2209-2212
{
	void* aGlobalPtr = (Image*)theGlobalPtrRef; //?
	return GetResourceRef(aGlobalPtr); //Different in XNA
}

ResourceRef ResourceManager::GetSoundRef(const std::string& theId) //2215-2217
{
	return GetResourceRef(ResType_Sound, theId);
}

ResourceRef ResourceManager::GetSoundRef(void* theGlobalPtrRef) //Unimplemented in XNA | 2220-2222
{
	return GetResourceRef(theGlobalPtrRef);
}

ResourceRef ResourceManager::GetFontRef(const std::string& theId) //2225-2227
{
	return GetResourceRef(ResType_Font, theId);
}

ResourceRef ResourceManager::GetFontRef(void* theGlobalPtrRef) //C++ only | 2230-2232
{
	return GetResourceRef(theGlobalPtrRef);
}

ResourceRef ResourceManager::GetPopAnimRef(const std::string& theId) //2235-2237
{
	return GetResourceRef(ResType_PopAnim, theId);
}

ResourceRef ResourceManager::GetPopAnimRef(void* theGlobalPtrRef) //C++ only | 2240-2242
{
	return GetResourceRef(theGlobalPtrRef);
}

ResourceRef ResourceManager::GetPIEffectRef(const std::string& theId) //2245-2247
{
	return GetResourceRef(ResType_PIEffect, theId);
}

ResourceRef ResourceManager::GetPIEffectRef(void* theGlobalPtrRef) //Unimplemented in XNA | 2250-2252
{
	return GetResourceRef(theGlobalPtrRef);
}

ResourceRef ResourceManager::GetRenderEffectRef(const std::string& theId) //2255-2257
{
	return GetResourceRef(ResType_RenderEffect, theId);
}

ResourceRef ResourceManager::GetRenderEffectRef(void* theGlobalPtrRef) //Unimplemented in XNA | 2260-2262
{
	return GetResourceRef(theGlobalPtrRef);
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
ResourceRef ResourceManager::GetGenericResFileRef(const std::string& theId)
{
	return GetResourceRef(ResType_GenericResFile, theId);
}

ResourceRef ResourceManager::GetGenericResFileRef(void* theGlobalPtrRef) //Unimplemented in XNA
{
	return GetResourceRef(theGlobalPtrRef);
}
#endif

void ResourceManager::SetAllowMissingProgramImages(bool allow) //2265-2267
{
	mAllowMissingProgramResources = allow;
}

bool ResourceManager::ReplaceImage(const std::string &theId, Image *theImage) //Unimplemented in XNA | 2270-2283
{
	ImageRes* aRes = (ImageRes*)GetBaseRes(ResType_Image, theId);
	if (aRes != NULL)
	{
		aRes->DeleteResource();
		aRes->mImage = (MemoryImage*) theImage;
		aRes->mImage.mOwnsUnshared = true;
		if (aRes->mGlobalPtr)
			aRes->mImage = (MemoryImage*)theImage; //?
		return true;
	}
	else
		return false;
}

bool ResourceManager::ReplaceSound(const std::string &theId, int theSound) //Unimplemented in XNA | 2286-2298
{
	SoundRes* aRes = (SoundRes*)GetBaseRes(ResType_Sound, theId);
	if (aRes != NULL)
	{
		aRes->DeleteResource();
		aRes->mSoundId = theSound; //?
		if (aRes->mGlobalPtr)
			aRes->mSoundId = theSound; //?
		return true;
	}
	else
		return false;
}

bool ResourceManager::ReplaceFont(const std::string &theId, Font *theFont) //Unimplemented in XNA | 2301-2313
{
	FontRes* aRes = (FontRes*)GetBaseRes(ResType_Font, theId);
	if (aRes != NULL)
	{
		aRes->DeleteResource();
		aRes->mFont = theFont; //?
		if (aRes->mGlobalPtr)
			aRes->mFont = theFont; //?
		return true;
	}
	else
		return false;
}

bool ResourceManager::ReplacePopAnim(const std::string& theId, PopAnim* thePopAnim) //Unimplemented in XNA | 2316-2328
{
	PopAnimRes* aRes = (PopAnimRes*)GetBaseRes(ResType_PopAnim, theId);
	if (aRes != NULL)
	{
		aRes->DeleteResource();
		aRes->mPopAnim = thePopAnim; //?
		if (aRes->mGlobalPtr)
			aRes->mPopAnim = thePopAnim; //?
		return true;
	}
	else
		return false;
}

bool ResourceManager::ReplacePIEffect(const std::string& theId, PIEffect* thePIEffect) //Unimplemented in XNA | 2331-2343
{
	PIEffectRes* aRes = (PIEffectRes*)GetBaseRes(ResType_PIEffect, theId);
	if (aRes != NULL)
	{
		aRes->DeleteResource();
		aRes->mPIEffect = thePIEffect; //?
		if (aRes->mGlobalPtr)
			aRes->mPIEffect = thePIEffect; //?
		return true;
	}
	else
		return false;
}

bool ResourceManager::ReplaceRenderEffect(const std::string& theId, RenderEffectDefinition* theDefinition) //Unimplemented in XNA | 2346-2358
{
	RenderEffectRes* aRes = (RenderEffectRes*)GetBaseRes(ResType_RenderEffect, theId);
	if (aRes != NULL)
	{
		aRes->DeleteResource();
		aRes->mRenderEffectDefinition = theDefinition; //?
		if (aRes->mGlobalPtr)
			aRes->mRenderEffectDefinition = theDefinition; //?
		return true;
	}
	else
		return false;
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
bool ResourceManager::ReplaceGenericResFile(const std::string& theId, GenericResFile* theFile) //Unimplemented in XNA
{
	GenericResFileRes* aRes = (GenericResFileRes*)GetBaseRes(ResType_GenericResFile, theId);
	if (aRes != NULL)
	{
		aRes->DeleteResource();
		aRes->mFilePath = theId; //?
		if (aRes->mGlobalPtr)
			aRes->mFilePath = theId; //?
		return true;
	}
	else
		return false;
}
#endif

const XMLParamMap& ResourceManager::GetImageAttributes(const std::string &theId) //2361-2369
{
	static XMLParamMap aStrMap; ///Not defined in XNA
	ImageRes* aRes = new ImageRes;
	aRes = (ImageRes*)GetBaseRes(ResType_Image, theId);

	if (aRes != NULL)
		return aRes->mXMLAttributes;
	else
		return aStrMap;
}

void ResourceManager::Deref(BaseRes *theRes) //2372-2380
{
	theRes->mRefCount--;
	DBG_ASSERTE(theRes->mRefCount >= 0); //2374 | 2705 BejLiveWin8
	if (theRes->mRefCount == 0)
	{
		DBG_ASSERTE(!theRes->mDirectLoaded); //2377 | 2708 BejLiveWin8
		theRes->DeleteResource();
	}
}

/*ResourceManagerInfo::ResourceManagerInfo()
{
}

ResGenInfo::ResGenInfo()
{
	mResPropsUsed = "";
	mResWatchFileUsed = "";
	mResGenPlatformName = "";
	//TODO: Windows is set on iOS?
}*/