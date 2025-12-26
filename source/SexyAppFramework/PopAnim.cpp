#include "PopAnim.h"
#include "Debug.h"
#include "AutoCrit.h"
#include "SexyAppBase.h"
#include "ResourceManager.h"

using namespace Sexy;

static const std::string& WildcardExpand(const std::string& theValue, int theMatchStart, int theMatchEnd, const std::string& theReplacement) //Not in H5, Correct? | 46-89
{
	static std::string aTempString;
	if (theReplacement.length() == 0)
		return aTempString.erase();
	if (theReplacement[0] == '*')
	{
		if (theReplacement.length() == 1)
			aTempString = theValue.substr(0, theMatchStart) + theValue.substr(theMatchEnd);
		if (theReplacement[theReplacement.length() - 1] == '*')
			aTempString = theValue.substr(0, theMatchStart) + theReplacement.substr(1, theReplacement.length() - 2) + theValue.substr(theMatchEnd);
		else
			aTempString = theValue.substr(0, theMatchStart) + theReplacement.substr(1, theReplacement.length() - 1);
	}
	if (theReplacement[theReplacement.length() - 1] == '*')
		aTempString = theReplacement.substr(0, theReplacement.length() - 1) + theValue.substr(theMatchEnd);
	return aTempString;
}

static bool WildcardReplace(const std::string& theValue, const std::string& theWildcard, const std::string& theReplacement, std::string& theResult) //Not in H5, Correct? | 92-180
{
	static std::string aString;
	if (!theWildcard.length())
		return false;
	if (theWildcard[0] == '*')
	{
		if (theWildcard.length() == 1)
		{
			theResult = WildcardExpand(theValue, 0, theValue.length() - 1, theReplacement);
			return true;
		}
		if (theWildcard[theWildcard.length() - 1] != '*')
		{
			if (theValue.length() < theWildcard.length() - 1)
				return false;
			if (theWildcard.substr(1) == theValue.substr(theValue.length() - theWildcard.length() + 1))
				return false;
			theResult = WildcardExpand(theValue, theValue.length() - theWildcard.length() + 1, theValue.length(), theReplacement);
			return true;
		}
		int aSearchLen = theWildcard.length() - 2;
		int aLastStart = theValue.length() - aSearchLen;
		for (int aStartIdx = 0; aStartIdx <= aLastStart; aStartIdx++)
		{
			bool match = true;
			for (int aLookIdx = 0; aLookIdx < aSearchLen; aLookIdx++)
			{
				if (toupper(theWildcard[aLookIdx + 1]) != toupper(theValue[aStartIdx + aLookIdx]))
				{
					match = false;
					break;
				}
			}
			if (match)
			{
				theResult = WildcardExpand(theValue, aStartIdx, aStartIdx + aSearchLen, theReplacement);
				return true;
			}
		}
	}
	else
	{
		if (theWildcard[theWildcard.length() - 1] == '*')
		{
			if (theValue.length() < theWildcard.length() - 1)
				return false;
			if (theWildcard == theValue.substr(0, theWildcard.length() - 1))
				return false;
			theResult = WildcardExpand(theValue, 0, theWildcard.length() - 1, theReplacement);
			return true;
		}
		if (theWildcard == theValue)
		{
			if (theReplacement.length() > 0)
			{
				if (theReplacement[0] == '*')
					theResult = theValue + theReplacement.substr(1);
				else if (theReplacement[theReplacement.length() - 1] == '*')
					theResult = theReplacement.substr(0, theReplacement.length() - 1) + theValue;
				else
					theResult = theReplacement;
			}
			else
				theResult = theReplacement;
			return true;
		}
	}
	return false;
}

static bool WildcardMatches(const std::string& theValue, const std::string& theWildcard) //Not in H5, Correct? | 183-249
{
	static std::string aString;
	if (!theWildcard.length())
		return false;
	if (theWildcard[0] == '*')
	{
		if (theWildcard.length() == 1)
			return true;
		if (theWildcard[theWildcard.length() - 1] == '*')
		{
			int aSearchLen = theWildcard.length() - 2;
			int aLastStart = theValue.length() - aSearchLen;
			for (int aStartIdx = 0; aStartIdx <= aLastStart; aStartIdx++)
			{
				bool match = true;
				for (int aLookIdx = 0; aLookIdx < aSearchLen; aLookIdx++)
				{
					if (toupper(theWildcard[aLookIdx + 1]) != toupper(theValue[aStartIdx + aLookIdx]))
					{
						match = false;
						break;
					}
				}
				if (match)
					return true;
			}
			return false;
		}
		else
		{
			if (theValue.length() < theWildcard.length() - 1)
				return false;
			return stricmp((theWildcard.c_str() + 1), theValue.length() + theValue.c_str() - theWildcard.length() + 1);
		}
	}
	if (theWildcard[theWildcard.length() - 1] == '*')
	{
		if (theValue.length() < theWildcard.length() - 1)
			return false;
		return strnicmp(theWildcard.c_str(), theValue.c_str(), theWildcard.length() - 1) == NULL;
	}
	return theWildcard == theValue;
}

PATransform::PATransform() //255-257
{
	mMatrix.LoadIdentity();
}

PATransform PATransform::TransformSrc(const PATransform& theSrcTransform) //Correct? | 260-274
{
	PATransform aNewTransform;
	aNewTransform.mMatrix.m00 = mMatrix.m00 * theSrcTransform.mMatrix.m00 + mMatrix.m01 * theSrcTransform.mMatrix.m10;
	aNewTransform.mMatrix.m01 = mMatrix.m00 * theSrcTransform.mMatrix.m01 + mMatrix.m01 * theSrcTransform.mMatrix.m11;
	aNewTransform.mMatrix.m10 = mMatrix.m10 * theSrcTransform.mMatrix.m00 + mMatrix.m11 * theSrcTransform.mMatrix.m10;
	aNewTransform.mMatrix.m11 = mMatrix.m10 * theSrcTransform.mMatrix.m01 + mMatrix.m11 * theSrcTransform.mMatrix.m11;
	aNewTransform.mMatrix.m02 = mMatrix.m00 * theSrcTransform.mMatrix.m02 + mMatrix.m02 + mMatrix.m01 * theSrcTransform.mMatrix.m12;
	aNewTransform.mMatrix.m12 = mMatrix.m10 * theSrcTransform.mMatrix.m02 + mMatrix.m12 + mMatrix.m11 * theSrcTransform.mMatrix.m12;
	memcpy(this, &aNewTransform, sizeof PATransform); //?
	return aNewTransform; //?
}

PATransform PATransform::InterpolateTo(const PATransform& theNextTransform, float thePct) //Correct? 277-289
{
	PATransform aNewTransform;
	aNewTransform.mMatrix.m00 = (1.0 - thePct) * mMatrix.m00 + theNextTransform.mMatrix.m00 * thePct;
	aNewTransform.mMatrix.m01 = (1.0 - thePct) * mMatrix.m01 + theNextTransform.mMatrix.m01 * thePct;
	aNewTransform.mMatrix.m10 = (1.0 - thePct) * mMatrix.m10 + theNextTransform.mMatrix.m10 * thePct;
	aNewTransform.mMatrix.m11 = (1.0 - thePct) * mMatrix.m11 + theNextTransform.mMatrix.m11 * thePct;
	aNewTransform.mMatrix.m02 = (1.0 - thePct) * mMatrix.m02 + theNextTransform.mMatrix.m02 * thePct;
	aNewTransform.mMatrix.m12 = (1.0 - thePct) * mMatrix.m12 + theNextTransform.mMatrix.m12 * thePct;
	memcpy(this, &aNewTransform, sizeof PATransform); //?
	return aNewTransform; //?
}

int PASpriteDef::GetLabelFrame(const std::string& theLabel) //294-300
{
	std::string aLabelUpper = StringToUpper(theLabel);
	StringIntMap::iterator anItr = mLabels.find(aLabelUpper);
	if (anItr != mLabels.end())
		return anItr->second;
	return -1;
}

void PASpriteDef::GetLabelFrameRange(const std::string& theLabel, int& theStart, int& theEnd) //Not in H5, Correct? | 305-325
{
	theStart = GetLabelFrame(theLabel);
	theEnd = -1;
	if (theStart == -1)
		return;

	std::string aLabelUpper = StringToUpper(theLabel);
	StringIntMap::iterator it = mLabels.begin();
	while (it != mLabels.end())
	{
		if (aLabelUpper != it->first && it->second > theStart && (theEnd < 0 || it->second < theEnd)) //?
			theEnd = it->second - 1;
		it++;
	}
	if (theEnd < 0)
		theEnd = mFrames.size() - 1;
}

PAObjectInst* PASpriteInst::GetObjectInst(const std::string& theName) //330-357
{
	std::string aCurName;
	std::string aNextName;
	int aSlashPos = theName.find("\\", 0);
	if (aSlashPos != -1)
	{
		aCurName = theName.substr(0, aSlashPos);
		aNextName = theName.substr(aSlashPos + 1);
	}
	else
		aCurName = theName;
	for (int aChildIdx = 0; aChildIdx < mChildren.size(); aChildIdx++)
	{
		PAObjectInst* anObjectInst = &mChildren[aChildIdx];
		if (strcmp(anObjectInst->mName, aCurName.c_str()) == NULL)
		{
			if (aSlashPos == -1)
				return anObjectInst;
			if (anObjectInst->mSpriteInst == NULL)
				return NULL;
			return anObjectInst->mSpriteInst->GetObjectInst(aNextName);
		}
	}
	return NULL;
}

PASpriteInst::~PASpriteInst() //360-369
{
	for (int aChildIdx = 0; aChildIdx < mChildren.size(); ++aChildIdx)
	{
		delete &mChildren[aChildIdx].mSpriteInst;
		while (mParticleEffectVector.size() > 0)
		{
			delete mParticleEffectVector.back().mEffect;
			mParticleEffectVector.pop_back();
		}
	}
}

PopAnimModParser::PopAnimModParser() //374-376
{
	mCmdSep = CMDSEP_NO_INDENT;
}

bool PopAnimModParser::Error(const std::string& theError) //This looks like a near-identical copy of FontData::Error, not in H5 | 379-393
{
#if (!defined(_IPHONEOS) && !defined(_ANDROID)) || (defined(_DEBUG) || (defined(_TRANSMENSION))) //On mobile, it's hidden on retail (excluding debug [at least for Android]), as Transmension, Xbox 360 and PS4 still have this, it won't be locked under debug.
	if (gSexyAppBase != NULL) //Originally gSexyApp, prob due to it's PopCap's oldest format before they moved *almost everything into SexyAppBase. As the standalone SexyApp class handles DRM while SAB is the same but without it, it will be renamed to gSexyAppBase.
	{
		std::string anErrorString = mErrorHeader + theError;

		if (mCurrentLine.length() > 0)
		{
			anErrorString += " on Line " + StrFormat("%d:\r\n\r\n", mCurrentLineNum) + WStringToString(mCurrentLine, 0);
		}

		gSexyAppBase->Popup(anErrorString);
	}
#endif
	return false;
}

void PopAnimModParser::SetParamHelper(PASpriteDef* theSpriteDef, const std::string& theCmdName, int* theCmdNum, const std::string& theNewParam) //Not in H5 | 396-421
{
	if (!theCmdNum)
		return;

	for (int aFrameNum = 0; aFrameNum < theSpriteDef->mFrames.size(); aFrameNum++)
	{
		PAFrame* aFrame = &theSpriteDef->mFrames[aFrameNum];
		for (int aCmdNum = 0; aCmdNum < aFrame->mCommandVector.size(); aCmdNum++)
		{
			PACommand* aCommand = &aFrame->mCommandVector[aCmdNum];
			if (!WildcardMatches(aCommand->mCommand, theCmdName))
				continue;
			if (*theCmdNum <= 1)
				aCommand->mParam = theNewParam;
			if (theCmdNum >= 0)
			{
				theCmdNum--;
				if (theCmdNum == 0)
					return;
			}
		}
	}
}

bool PopAnimModParser::HandleCommand(const ListDataElement& theParams) //Similar to FontData::HandleCommand, not in H5, do we return error | 429-566
{
	SexyString aCmd = ((SingleDataElement*)theParams.mElementVector[0])->mString;
	int aNumParams = theParams.mElementVector.size() - 1;
	if (aCmd == _S("SetPamFile")) //Switch statements in XNA
	{
		if (mPassNum != 1)
			return true;
		if (aNumParams != 1)
			return Error("Invalid Number of Parameters");
		if (mPopAnim->mModPamFile.length() == 0)
		{
			SexyString aFileName;
			if (DataToString(theParams.mElementVector[1], &aFileName))
				return Error("Invalid Paramater Type"); //Typo
			mPopAnim->mModPamFile = ToString(aFileName);
			return true;
		}
	}
	if (aCmd == _S("Remap"))
	{
		if (mPassNum != 1)
			return true;
		if (aNumParams != 2)
			return Error("Invalid Number of Parameters");
		SexyString aWildcard;
		if (DataToString(theParams.mElementVector[1], &aWildcard))
			return Error("Invalid Paramater Type"); //Typo
		SexyString aReplacement;
		if (DataToString(theParams.mElementVector[2], &aReplacement))
			return Error("Invalid Paramater Type"); //Typo
		mPopAnim->Load_AddRemap(ToString(aWildcard), ToString(aReplacement));
		return true;
	}
	if (aCmd == _S("Colorize") && aCmd != _S("HueShift"))
	{
		if (mPassNum != 2)
			return true;
		SexyString anElementName;
		if (DataToString(theParams.mElementVector[1], &anElementName))
			return Error("Invalid Paramater Type"); //Typo
		bool foundOne = false;
		for (int aSpriteNum = 0; aSpriteNum < mPopAnim->mImageVector.size(); aSpriteNum++)
		{
			PAImage* anImage = &mPopAnim->mImageVector[aSpriteNum];
			for (int i = 0; i < anImage->mImages.size(); i++)
			{
				if (WildcardMatches(ToString(anImage->mImageName), ToString(anElementName)))
				{
					if (aCmd == _S("Colorize"))
					{
						IntVector aColorElements;
						if (DataToIntVector(theParams.mElementVector[2], &aColorElements) || (aColorElements.size() != 3 && aColorElements.size() != 4))
							return Error("Invalid Paramater Type");
						Color aColor;
						if (aColorElements.size() == 3)
							aColor = Color(aColorElements[0], aColorElements[1], aColorElements[2]);
						else
							aColor = Color(aColorElements[0], aColorElements[1], aColorElements[2], aColorElements[3]);
						gSexyAppBase->ColorizeImage(anImage->mImages[i], aColor); //C++
					}
					else
					{
						int aShift = 0;
						if (DataToInt(theParams.mElementVector[2], &aShift))
							return false;
						gSexyAppBase->RotateImageHue(anImage->mImages[i], aShift); //C++
					}
					foundOne = true;
				}
			}
		}
		if (foundOne)
			return Error("Unable to locate specified element");
	}
	if (aCmd == _S("SetParam"))
	{
		if (mPassNum != 2)
			return true;
		if (aNumParams != 2)
			return Error("Invalid Number of Parameters");
		SexyString aCmdName;
		if (DataToString(theParams.mElementVector[1], &aCmdName))
			Error("Invalid Paramater Type");
		SexyString aNewParam;
		if (DataToString(theParams.mElementVector[2], &aNewParam))
			Error("Invalid Paramater Type");
		int aCmdNum = -1;
		int aBracketPos = aCmdName.find('[');
		if (aBracketPos != -1)
		{
			aCmdNum = sexyatoi(aCmdName.c_str() + 2 * aBracketPos + 2);
			aCmdName = aCmdName.substr(0, aBracketPos);
		}
		SetParamHelper(mPopAnim->mMainAnimDef->mMainSpriteDef, ToString(aCmdName), &aCmdNum, ToString(aNewParam));
		for (int i = 0; i < mPopAnim->mMainAnimDef->mSpriteDefVector.size(); i++)
			SetParamHelper(&mPopAnim->mMainAnimDef->mSpriteDefVector[i], ToString(aNewParam), &aCmdNum, ToString(aCmdName));
	}
	else
		return Error("Unknown Command");
	return false;
}

PopAnim::PopAnim(int theId, PopAnimListener* theListener) //571-608
{
	AutoCrit aCrit(gSexyAppBase->mCritSect);

	gSexyAppBase->mPopAnimSet.insert(this);
	mId = theId;
	mListener = theListener;
	mMirror = false;
	mAdditive = false;
	mColor = Color::White;
	mAnimRate = 0;
	mLoaded = false;
	mAnimRunning = false;
	mMainSpriteInst = NULL;
	mMainAnimDef = NULL;
	mInterpolate = true;
	mLoadedImageIsNew = false;
	mRandUsed = false;
	Clear();
	mVersion = 0;
	mPaused = false;
	mColorizeType = false;
	mImgScale = 1.0;
	mDrawScale = 1.0;
	mTransDirty = true;
	mBlendTicksTotal = 0.0;
	mBlendTicksCur = 0.0;
	mBlendDelay = 0.0;
	mImageSearchPathVector.push_back("images\\");
	mImageSearchPathVector.push_back("");
}

PopAnim::PopAnim(const PopAnim& rhs) //611-622
{
	AutoCrit aCrit(gSexyAppBase->mCritSect);

	gSexyAppBase->mPopAnimSet.insert(this);
	*this = rhs;
	mMainSpriteInst = new PASpriteInst;
	mMainSpriteInst->mDef = NULL;
	mMainSpriteInst->mParent = NULL;
	mMainAnimDef->mRefCount++;
}

PopAnim::~PopAnim() //628-635
{
	AutoCrit aCrit(gSexyAppBase->mCritSect);
	gSexyAppBase->mPopAnimSet.erase(gSexyAppBase->mPopAnimSet.find(this)); //gSexyAppBase->mPopAnimSet.erase(this); //?
	Clear();
}

void PopAnim::Clear() //Not in H5 | 641-670
{
	mMirror = false;
	mColor = Color::White;
	mLoaded = false;
	mRandUsed = false;
	mAnimRunning = false;
	mAnimRate = 0;
	mError.erase(0, std::string::npos);
	mImageVector.clear();
	mModPamFile.erase(0, std::string::npos);
	mRemapList.clear();
	mCRCBuffer.Clear();
	if (mMainAnimDef != NULL)
	{
		if (mMainAnimDef->mRefCount == 0)
		{
			mMainAnimDef->mSpriteDefVector.clear();
			delete mMainAnimDef;
		}
		else
			mMainAnimDef->mRefCount--;
	}
	mMainAnimDef = NULL;
	mTransDirty = true;
	delete mMainSpriteInst;
	mMainSpriteInst = NULL;
}

PopAnim* PopAnim::Duplicate() //676-679
{
	PopAnim* aPopAnim = new PopAnim(); //A variable in C++
	return aPopAnim;
}

bool PopAnim::Fail(const std::string& theError) //Not in H5 | 685-688
{
	mError = theError;
	return false;
}

const std::string& PopAnim::Remap(const std::string& theString) //Not in H5 | 691-703
{
	static std::string aString;
	StringPairList::iterator anItr = mRemapList.begin(); //?
	while (anItr != mRemapList.end())
	{
		if (WildcardReplace(theString, anItr->first, anItr->second, aString))
			return aString;
		anItr++;
	}
}

bool PopAnim::LoadSpriteDef(Buffer* theBuffer, PASpriteDef* theSpriteDef) //ALMOST | 709-939
{
	IntToPAObjectPosMap aCurObjectMap;
	if (mVersion >= 4)
	{
		mMainAnimDef->mObjectNamePool.push_back(theBuffer->ReadString());
		theSpriteDef->mName = mMainAnimDef->mObjectNamePool.back().c_str();
		theSpriteDef->mAnimRate = theBuffer->ReadLong() / 65536.0;
		mCRCBuffer.WriteString(theSpriteDef->mName);
	}
	else
	{
		theSpriteDef->mName = 0;
		theSpriteDef->mAnimRate = mAnimRate;
	}
	int aNumFrames = theBuffer->ReadShort();

	if (mVersion >= 5)
	{
		theSpriteDef->mWorkAreaStart = theBuffer->ReadShort();
		theSpriteDef->mWorkAreaDuration = theBuffer->ReadShort();
	}
	else
	{	
		theSpriteDef->mWorkAreaStart = 0;
		theSpriteDef->mWorkAreaDuration = aNumFrames - 1;
	}
	theSpriteDef->mWorkAreaDuration = min(theSpriteDef->mWorkAreaStart + theSpriteDef->mWorkAreaDuration, aNumFrames - 1) - theSpriteDef->mWorkAreaStart;
	mCRCBuffer.WriteShort(aNumFrames);
	theSpriteDef->mFrames.resize(aNumFrames);
	for (int aFrameNum = 0; aFrameNum < aNumFrames; aFrameNum++)
	{
		PAFrame* aFrame = &theSpriteDef->mFrames[aFrameNum];
		uchar aFrameFlags = theBuffer->ReadByte();
		if (aFrameFlags & FRAMEFLAGS_HAS_REMOVES)
		{
			int aNumRemoves = theBuffer->ReadByte();
			if (aNumRemoves == 255)
				aNumRemoves = theBuffer->ReadShort();
			for (int aRemoveNum = 0; aRemoveNum < aNumRemoves; ++aRemoveNum)
			{
				int anObjectId = theBuffer->ReadShort();
				if (anObjectId >= 2047)
					anObjectId = theBuffer->ReadLong();
				aCurObjectMap.erase(aCurObjectMap.find(anObjectId)); //find?
			}
		}
		if (aFrameFlags & FRAMEFLAGS_HAS_ADDS)
		{
			int aNumAdds = theBuffer->ReadByte();
			if (aNumAdds == 255)
				aNumAdds = theBuffer->ReadShort();
			for (int anAddNum = 0; anAddNum < aNumAdds; anAddNum++) {
				PAObjectPos aPAObjectPos;
				ushort anObjectNumAndType = theBuffer->ReadShort();
				aPAObjectPos.mObjectNum = (anObjectNumAndType & 0x7ff);
				if (aPAObjectPos.mObjectNum == 2047)
					aPAObjectPos.mObjectNum = theBuffer->ReadLong();
				aPAObjectPos.mIsSprite = (anObjectNumAndType & 0x8000) != 0;
				aPAObjectPos.mIsAdditive = (anObjectNumAndType & 0x4000) != 0;
				aPAObjectPos.mResNum = theBuffer->ReadByte();
				aPAObjectPos.mHasSrcRect = false;
				aPAObjectPos.mColor = Color::White;
				aPAObjectPos.mAnimFrameNum = 0;
				aPAObjectPos.mTimeScale = 1.0;
				aPAObjectPos.mName = NULL;
				//aPAObjectPos.mTransform = new Matrix(); //H5 only
				if (anObjectNumAndType & 0x2000)
					aPAObjectPos.mPreloadFrames = theBuffer->ReadShort();
				else
					aPAObjectPos.mPreloadFrames = 0;
				if (anObjectNumAndType & 0x1000)
				{
					mMainAnimDef->mObjectNamePool.push_back(theBuffer->ReadString());
					aPAObjectPos.mName = mMainAnimDef->mObjectNamePool.back().c_str();
				}
				if (anObjectNumAndType & 0x800)
					aPAObjectPos.mTimeScale = theBuffer->ReadLong() / 65536.0;
				if (theSpriteDef->mObjectDefVector.size() < aPAObjectPos.mObjectNum + 1)
					theSpriteDef->mObjectDefVector.resize(aPAObjectPos.mObjectNum + 1);
				theSpriteDef->mObjectDefVector[aPAObjectPos.mObjectNum].mName = aPAObjectPos.mName;
				if (aPAObjectPos.mIsSprite)
					theSpriteDef->mObjectDefVector[aPAObjectPos.mObjectNum].mSpriteDef = &mMainAnimDef->mSpriteDefVector[aPAObjectPos.mResNum];
				aCurObjectMap.insert(IntToPAObjectPosMap::value_type(aPAObjectPos.mObjectNum, aPAObjectPos)); //?
			}
		}
		if (aFrameFlags & FRAMEFLAGS_HAS_MOVES)
		{
			int aNumMoves = theBuffer->ReadByte();
			if (aNumMoves == 255)
				aNumMoves = theBuffer->ReadShort();
			for (int aMoveNum = 0; aMoveNum < aNumMoves; aMoveNum++) {
				ushort aFlagsAndObjectNum = theBuffer->ReadShort();
				int anObjectNum = aFlagsAndObjectNum & 0x3ff;
				if (anObjectNum == 1024)
					anObjectNum = theBuffer->ReadLong();
				PAObjectPos* aPAObjectPos = &aCurObjectMap.find(anObjectNum)->second; //?
				aPAObjectPos->mTransform.mMatrix.LoadIdentity();
				if (aFlagsAndObjectNum & MOVEFLAGS_HAS_MATRIX)
				{
					aPAObjectPos->mTransform.mMatrix.m00 = theBuffer->ReadLong() / 65536.0;
					aPAObjectPos->mTransform.mMatrix.m01 = theBuffer->ReadLong() / 65536.0;
					aPAObjectPos->mTransform.mMatrix.m10 = theBuffer->ReadLong() / 65536.0;
					aPAObjectPos->mTransform.mMatrix.m11 = theBuffer->ReadLong() / 65536.0;
				}
				else if (aFlagsAndObjectNum & MOVEFLAGS_HAS_ROTATE)
				{
					float aRot = theBuffer->ReadShort() / 1000.0;
					float sinRot = sinf(aRot); //C++
					float cosRot = cosf(aRot); //C++
					if (mVersion == 2)
						sinRot = -sinRot;
					aPAObjectPos->mTransform.mMatrix.m00 = cosRot;
					aPAObjectPos->mTransform.mMatrix.m01 = -sinRot;
					aPAObjectPos->mTransform.mMatrix.m10 = sinRot;
					aPAObjectPos->mTransform.mMatrix.m11 = cosRot;
				}
				SexyMatrix3 aMatrix; //SexyTransform2D in XNA
				aMatrix.LoadIdentity();
				if (aFlagsAndObjectNum & MOVEFLAGS_HAS_LONGCOORDS)
				{
					aMatrix.m02 = theBuffer->ReadLong() / 20.0;
					aMatrix.m12 = theBuffer->ReadLong() / 20.0;
				}
				else
				{
					aMatrix.m02 = theBuffer->ReadShort() / 20.0;
					aMatrix.m12 = theBuffer->ReadShort() / 20.0;
				}
				aPAObjectPos->mTransform.mMatrix *= aMatrix; //For now
				//memcpy(&aPAObjectPos->mTransform, &aMatrix * aPAObjectPos->mTransform.mMatrix, sizeof(aPAObjectPos->mTransform)); //?
				aPAObjectPos->mHasSrcRect = aFlagsAndObjectNum & MOVEFLAGS_HAS_SRCRECT; //Function var in H5
				if (aFlagsAndObjectNum & MOVEFLAGS_HAS_SRCRECT)
				{
					aPAObjectPos->mSrcRect.mX = theBuffer->ReadShort() / 20;
					aPAObjectPos->mSrcRect.mY = theBuffer->ReadShort() / 20;
					aPAObjectPos->mSrcRect.mWidth = theBuffer->ReadShort() / 20;
					aPAObjectPos->mSrcRect.mHeight = theBuffer->ReadShort() / 20;
				}
				if (aFlagsAndObjectNum & MOVEFLAGS_HAS_COLOR)
				{
					aPAObjectPos->mColor.mRed = theBuffer->ReadByte();
					aPAObjectPos->mColor.mGreen = theBuffer->ReadByte();
					aPAObjectPos->mColor.mBlue = theBuffer->ReadByte();
					aPAObjectPos->mColor.mAlpha = theBuffer->ReadByte();
				}
				if (aFlagsAndObjectNum & MOVEFLAGS_HAS_ANIMFRAMENUM)
					aPAObjectPos->mAnimFrameNum = theBuffer->ReadShort();
			}
		}
		if (aFrameFlags & FRAMEFLAGS_HAS_FRAME_NAME)
		{
			std::string aFrameName = theBuffer->ReadString();
			aFrameName = StringToUpper(Remap(aFrameName));
			theSpriteDef->mLabels.insert(StringIntMap::value_type(aFrameName, aFrameNum));
		}
		if (aFrameFlags & FRAMEFLAGS_HAS_STOP)
			aFrame->mHasStop = true;
		if (aFrameFlags & FRAMEFLAGS_HAS_COMMANDS)
		{
			int aNumCmds = theBuffer->ReadByte();
			aFrame->mCommandVector.resize(aNumCmds);
			for (int aCmdNum = 0; aCmdNum < aNumCmds; aCmdNum++)
			{
				aFrame->mCommandVector[aCmdNum].mCommand = Remap(theBuffer->ReadString());
				aFrame->mCommandVector[aCmdNum].mParam = Remap(theBuffer->ReadString());
			}
		}
		aFrame->mFrameObjectPosVector.resize(aCurObjectMap.size());
		int anObjectNum = 0;
		IntToPAObjectPosMap::iterator anItr = aCurObjectMap.begin();
		while (anItr != aCurObjectMap.end())
		{
			PAObjectPos* anObjectPos = &anItr->second;
			memcpy(&aFrame->mFrameObjectPosVector[anObjectNum], anObjectPos, sizeof PAObjectPos);
			anObjectPos->mPreloadFrames = 0;
			++anObjectNum;
			++anItr;
		}
	}
	if (!aNumFrames)
		theSpriteDef->mFrames.resize(1);
	for (int anObjectNum = 0; anObjectNum < theSpriteDef->mObjectDefVector.size(); anObjectNum++) //Not in H5, possibly correct
	{
		PAObjectDef* anObjectDef = &theSpriteDef->mObjectDefVector[anObjectNum];
		mCRCBuffer.WriteBoolean(anObjectDef->mSpriteDef != NULL);
	}
	return true;
}

void PopAnim::InitSpriteInst(PASpriteInst* theSpriteInst, PASpriteDef* theSpriteDef) //945-976
{
	theSpriteInst->mFrameRepeats = 0;
	theSpriteInst->mDelayFrames = 0;
	theSpriteInst->mDef = theSpriteDef;
	theSpriteInst->mLastUpdated = -1;
	theSpriteInst->mOnNewFrame = true;
	theSpriteInst->mFrameNum = 0.0;
	theSpriteInst->mChildren.resize(theSpriteDef->mObjectDefVector.size());
	for (int anObjectNum = 0; anObjectNum < theSpriteDef->mObjectDefVector.size(); ++anObjectNum)
	{
		PAObjectDef* anObjectDef = &theSpriteDef->mObjectDefVector[anObjectNum];
		PAObjectInst* anObjectInst = &theSpriteInst->mChildren[anObjectNum];
		anObjectInst->mColorMult = Color::White;
		anObjectInst->mName = anObjectDef->mName;
		anObjectInst->mIsBlending = false;
		PASpriteDef* aChildSpriteDef = anObjectDef->mSpriteDef;
		if (aChildSpriteDef != NULL)
		{
			PASpriteInst* aChildSpriteInst = new PASpriteInst();
			aChildSpriteInst->mParent = theSpriteInst;
			InitSpriteInst(aChildSpriteInst, aChildSpriteDef);
			anObjectInst->mSpriteInst = aChildSpriteInst;
		}
	}
	if (theSpriteInst == mMainSpriteInst)
		GetToFirstFrame();
}

void PopAnim::Load_Init() //Not in H5 | 979-981
{
	Clear();
}

void PopAnim::Load_SetModPamFile(const std::string& theFileName) //Not in H5 | 984-986
{
	mModPamFile = theFileName;
}

void PopAnim::Load_AddRemap(const std::string& theWildcard, const std::string& theReplacement) //Not in H5 | 989-991
{
	mRemapList.push_back(StringPairList::value_type(theWildcard, theReplacement));
}

bool PopAnim::Load_LoadPam(const std::string& theFileName) //Not in H5, PopAnimResource::SerializeRead in that one | 994-1187
{
	mLoadedPamFile = theFileName;
	DBG_ASSERTE(mMainAnimDef == 0); //997 | 1001 in BejLiveWin8
	if (mMainAnimDef != 0)
		return false;
	mMainSpriteInst = new PASpriteInst();
	mMainSpriteInst->mParent = NULL;
	mMainSpriteInst->mDef = NULL;
	mMainAnimDef = new PopAnimDef();
	Buffer aBuffer;
	std::string aFileDir = GetFileDir(theFileName);
	if (!gSexyAppBase->ReadBufferFromFile(theFileName, &aBuffer, true))
		return Fail("Unable to load file: " + theFileName);
	if (aBuffer.ReadLong() != PAM_MAGIC)
		return Fail("Invalid header");
	mVersion = aBuffer.ReadLong();
	if (mVersion > PAM_VERSION) //5
		return Fail("Invalid version");
	mAnimRate = aBuffer.ReadByte();
	mAnimRect.mX = aBuffer.ReadShort() / 20;
	mAnimRect.mY = aBuffer.ReadShort() / 20;
	mAnimRect.mWidth = (ushort)aBuffer.ReadShort() / 20;
	mAnimRect.mHeight = (ushort)aBuffer.ReadShort() / 20;
	int aNumImages = aBuffer.ReadShort();
	mImageVector.resize(aNumImages);
	for (int anImageNum = 0; anImageNum < aNumImages; anImageNum++)
	{
		PAImage* anImage = &mImageVector[anImageNum];
		anImage->mDrawMode = Graphics::DRAWMODE_NORMAL;
		std::string anOrigName = aBuffer.ReadString();
		std::string aRemappedName = Remap(anOrigName); //C++
		std::string anAttribs;
		int aLeftPos = aRemappedName.find('(');
		int aRightPos = aRemappedName.find(')');
		if (aLeftPos != -1 && aRightPos != -1 && aLeftPos < aRightPos)
		{
			anAttribs = Lower(aRemappedName.substr(aLeftPos + 1, aRightPos - aLeftPos - 1));
			aRemappedName = Trim(aRemappedName.substr(0, aLeftPos) + aRemappedName.substr(aRightPos + 1));
		}
		else
		{
			aRightPos = aRemappedName.find('$');
			if (aRightPos != -1)
			{
				anAttribs = Lower(aRemappedName.substr(0, aRightPos));
				aRemappedName = Trim(aRemappedName.substr(aRightPos + 1));
			}
		}
		anImage->mCols = 1;
		anImage->mRows = 1;
		aLeftPos = aRemappedName.find('[');
		aRightPos = aRemappedName.find(']');
		if (aLeftPos != -1 && aRightPos != -1 && aLeftPos < aRightPos)
		{
			std::string anAttribsInner = Lower(aRemappedName.substr(aLeftPos + 1, aRightPos - aLeftPos - 1));
			aRemappedName = Trim(aRemappedName.substr(0, aLeftPos) + aRemappedName.substr(aRightPos + 1));
			int aCommaPos = anAttribsInner.find(',');
			if (aCommaPos != -1)
			{
				anImage->mCols = atoi(anAttribsInner.substr(0, aCommaPos).c_str());
				anImage->mRows = atoi(anAttribsInner.substr(aCommaPos + 1).c_str());
			}
			if (anAttribs.find('add') != -1)
				anImage->mDrawMode = Graphics::DRAWMODE_ADDITIVE; //Assuming, add would mean additive
			if (mVersion >= 4)
			{
				anImage->mOrigWidth = aBuffer.ReadShort();
				anImage->mOrigHeight = aBuffer.ReadShort();
			}
			else
			{
				anImage->mOrigWidth = -1;
				anImage->mOrigHeight = -1;
			}
			if (mVersion == 1) //Not in H5
			{
				float aRot = aBuffer.ReadShort() / 1000.0;
				float sinRot = sinf(aRot);
				float cosRot = cosf(aRot);
				anImage->mTransform.mMatrix.m00 = cosRot;
				anImage->mTransform.mMatrix.m01 = -sinRot;
				anImage->mTransform.mMatrix.m10 = sinRot;
				anImage->mTransform.mMatrix.m11 = cosRot;
				anImage->mTransform.mMatrix.m02 = aBuffer.ReadShort() / 20.0;
				anImage->mTransform.mMatrix.m12 = aBuffer.ReadShort() / 20.0;
			}
			else
			{
				anImage->mTransform.mMatrix.m00 = aBuffer.ReadLong() / 1310720.0;
				anImage->mTransform.mMatrix.m01 = aBuffer.ReadLong() / 1310720.0;
				anImage->mTransform.mMatrix.m10 = aBuffer.ReadLong() / 1310720.0;
				anImage->mTransform.mMatrix.m11 = aBuffer.ReadLong() / 1310720.0;
				anImage->mTransform.mMatrix.m02 = aBuffer.ReadShort() / 20.0;
				anImage->mTransform.mMatrix.m12 = aBuffer.ReadShort() / 20.0;
			}
			anImage->mImageName = aRemappedName;
			if (anImage->mImageName.length() == 0)
			{
				bool isNew = false;
				SharedImageRef aDotImage = gSexyAppBase->GetSharedImage("!whitepixel", "", &isNew, true);
				if (isNew) //C++ only
				{
					aDotImage->Create(1, 1);
					aDotImage->SetImageMode(false, false);
					*aDotImage->GetBits() = -1;
					aDotImage->BitsChanged();
				}
				anImage->mImages.push_back(aDotImage);
			}
			else
			{
				int aCurPos = 0;
				while (aCurPos < aRemappedName.length())
				{
					std::string aFileName;
					int aCommaPos = aRemappedName.find(',', aCurPos);
					if (aCommaPos == -1)
						aFileName = aRemappedName.substr(aCurPos);
					else
						aFileName = aRemappedName.substr(aCurPos, aCommaPos - aCurPos);
					Load_GetImage(anImage, aFileDir, aFileName, aFileName);
					if (aCommaPos == -1)
						break;
					aCurPos = aCommaPos + 1;
				}
			}
			if (mError.length() > 0)
				return false;
			if (mMirror && mLoadedImageIsNew)
			{
				for (int i = 0; i < anImage->mImages.size(); i++)
					gSexyAppBase->MirrorImage(anImage->mImages[i]);
			}
			Load_PostImageLoadHook(anImage);
		}
		mMotionFilePos = aBuffer.mReadBitPos / 8;
		int aNumSprites = aBuffer.ReadShort();
		mMainAnimDef->mSpriteDefVector.resize(aNumSprites);
		for (int aSpriteNum = 0; aSpriteNum < aNumSprites; aSpriteNum++)
		{
			if (!LoadSpriteDef(&aBuffer, &mMainAnimDef->mSpriteDefVector[aSpriteNum]))
				return false;
		}
		if (mVersion <= 3 || aBuffer.ReadBoolean())
		{
			mMainAnimDef->mMainSpriteDef = new PASpriteDef();
			if (!LoadSpriteDef(&aBuffer, mMainAnimDef->mMainSpriteDef))
				return false;
		}
		mLoaded = true;
		mRandUsed = false;
		return true;
	}
}

bool PopAnim::Load_LoadMod(const std::string& theFileName) //Not in H5 | 1190-1212
{
	PopAnimModParser aPopAnimModParser;
	aPopAnimModParser.mErrorHeader = "PopAnim Mod File Error in " + theFileName + "\r\n";
	aPopAnimModParser.mPopAnim = this;
	aPopAnimModParser.mPassNum = 1;
	if (!aPopAnimModParser.LoadDescriptor(theFileName))
		return false;
	if (mModPamFile.size() == 0)
		return Fail("No Pam file specified");
	std::string aFileName = GetPathFrom(mModPamFile, GetFileDir(theFileName));
	if (!Load_LoadPam(aFileName))
		return aPopAnimModParser.Error("Failed to load Pam: " + mModPamFile + "\r\n\r\n" + mError);
	aPopAnimModParser.mPassNum = 2;
	if (!aPopAnimModParser.LoadDescriptor(theFileName))
		return false;
	return true;
}

SharedImageRef PopAnim::Load_GetImageHook(const std::string& theFileDir, const std::string& theOrigName, const std::string& theRemappedName) //1215-1239
{
	if (theRemappedName.length() == 0)
	{
		Fail("No image file name specified");
		return NULL;
	}
	for (int aSearchIdx = 0; aSearchIdx < mImageSearchPathVector.size(); aSearchIdx++)
	{
		std::string aFilePath = GetPathFrom(mImageSearchPathVector[aSearchIdx], theFileDir);
		if (aFilePath.length() > 0 && aFilePath[aFilePath.length() - 1] != '\\' && aFilePath[aFilePath.length() - 1] != '/')
			aFilePath += "\\";
		aFilePath += theRemappedName;
		SharedImageRef anImage = gSexyAppBase->GetSharedImage(aFilePath, mMirror ? "MIRRORED" : "", &mLoadedImageIsNew, true);
		if (&anImage != NULL)
		{
			anImage->Palletize(); //C++ only
			return anImage;
		}
	}
	Fail("Unable to load image: " + theRemappedName + " (" + theOrigName + ")");
	return NULL;
}

bool PopAnim::Load_GetImage(PAImage* theImage, const std::string& theFileDir, const std::string& theOrigName, const std::string& theRemappedName) //1242-1264 (Correct?)
{
	SharedImageRef anImage = Load_GetImageHook(theFileDir, theOrigName, theRemappedName);

	if (&anImage == NULL)
		return false;

	anImage->mNumCols = theImage->mCols;
	anImage->mNumRows = theImage->mRows;
	if (theImage->mImages.size() == 0 && theImage->mOrigWidth != -1 && theImage->mOrigHeight != -1)
	{
		theImage->mTransform.mMatrix.m02 += (0.0 - anImage->mWidth - theImage->mOrigWidth * mImgScale) / (anImage->mNumCols + 1); //m20?
		theImage->mTransform.mMatrix.m12 += (0.0 - anImage->mHeight - theImage->mOrigHeight * mImgScale) / (anImage->mNumRows + 1); //m21?
	}
	theImage->mImages.push_back(anImage);
	return true;
}

void PopAnim::Load_PostImageLoadHook(PAImage* theImage) //1267-1268
{
}

bool PopAnim::LoadFile(const std::string& theFileName, bool doMirror) //1274-1307
{
	Load_Init();
	mMirror = doMirror;
	std::string anExt;
	int aDotPos = theFileName.rfind('.');
	if (aDotPos != -1)
		anExt = Lower(theFileName.substr(aDotPos));
	if (anExt == ".pam")
		return Load_LoadPam(theFileName);
	if (anExt == ".txt")
	{
		if (Load_LoadMod(theFileName))
			return true;
		if (mError.length() == 0)
			mError = "Mod file loading error";
		return false;
	}
	if (anExt.length() == 0)
	{
		if (Load_LoadPam(theFileName + ".pam"))
			return true;
		if (Load_LoadMod(theFileName + ".txt"))
			return true;
		return false;
	}
	return false;
}

void PopAnim::ResetAnimHelper(PASpriteInst* theSpriteInst) //1313-1335
{
	theSpriteInst->mFrameNum = 0.0;
	theSpriteInst->mFrameRepeats = 0;
	theSpriteInst->mDelayFrames = 0;
	theSpriteInst->mLastUpdated = -1;
	theSpriteInst->mOnNewFrame = true;
	for (int i = 0; i < theSpriteInst->mParticleEffectVector.size(); i++)
	{
		PAParticleEffect* aParticleEffect = &theSpriteInst->mParticleEffectVector[i];
		aParticleEffect->mEffect->ResetAnim();
	}
	for (int aSpriteIdx = 0; aSpriteIdx < theSpriteInst->mChildren.size(); aSpriteIdx++)
	{
		PASpriteInst* aSpriteInst = theSpriteInst->mChildren[aSpriteIdx].mSpriteInst;
		if (aSpriteInst != NULL)
			ResetAnimHelper(aSpriteInst);
	}
	mTransDirty = true;
}

void PopAnim::DrawParticleEffects(Graphics* g, PASpriteInst* theSpriteInst, PATransform* theTransform, Color& theColor, bool front) //1338-1368
{
	if (theSpriteInst->mParticleEffectVector.size() <= 0)
		return;

	for (int i = 0; i < theSpriteInst->mParticleEffectVector.size(); i++)
	{
		PAParticleEffect* aParticleEffect = &theSpriteInst->mParticleEffectVector[i];
		if (aParticleEffect->mTransform)
		{
			if (!aParticleEffect->mAttachEmitter)
			{
				SexyTransform2D aTransform;
				aTransform.Translate(aParticleEffect->mEffect->mWidth / 2.0, aParticleEffect->mEffect->mHeight / 2.0);
				aTransform = theTransform->mMatrix * aTransform;
				memcpy(&aTransform, &aTransform, sizeof(aTransform));
				memcpy(&aParticleEffect->mEffect->mDrawTransform, &aTransform, sizeof(aParticleEffect->mEffect->mDrawTransform));
			}
			else
			{
				aParticleEffect->mEffect->mDrawTransform.LoadIdentity();
				aParticleEffect->mEffect->mDrawTransform.Translate(-mParticleAttachOffset.mX, -mParticleAttachOffset.mY);
			}
			aParticleEffect->mEffect->mColor = theColor;
		}
		if (aParticleEffect->mBehind == !front)
			aParticleEffect->mEffect->Draw(g);
	}
}

void PopAnim::ResetAnim() //1374-1383
{
	ResetAnimHelper(mMainSpriteInst);
	CleanParticles(mMainSpriteInst, true);
	mAnimRunning = false;
	GetToFirstFrame();
	mBlendTicksTotal = 0.0;
	mBlendTicksCur = 0.0;
	mBlendDelay = 0.0;
}

void PopAnim::GetToFirstFrame() //1386-1399
{
	while (mMainSpriteInst->mDef != NULL && mMainSpriteInst->mFrameNum < mMainSpriteInst->mDef->mWorkAreaStart)
	{
		bool wasAnimRunning = mAnimRunning;
		bool wasPaused = mPaused;
		mAnimRunning = true;
		mPaused = false;
		Update();
		if (gSexyAppBase->mVSyncUpdates) //Not in H5
			UpdateF(1.0); //WidgetContainer in C++? Prob not
		mAnimRunning = wasAnimRunning;
		mPaused = wasPaused;
	}
}

bool PopAnim::Play(int theFrameNum, bool resetAnim) //PopAnimResource::PlayFrom in H5 | 1406-1431
{
	if (!SetupSpriteInst())
		return false;

	if (theFrameNum >= mMainSpriteInst->mDef->mFrames.size())
	{
		mAnimRunning = false;
		return false;
	}
	if (mMainSpriteInst->mFrameNum != theFrameNum && resetAnim)
		ResetAnim();
	mPaused = false;
	mAnimRunning = true;
	mMainSpriteInst->mDelayFrames = 0;
	mMainSpriteInst->mFrameNum = theFrameNum;
	mMainSpriteInst->mFrameRepeats = 0;
	if (resetAnim)
		CleanParticles(mMainSpriteInst, true);
	if (mBlendDelay == 0.0)
		DoFramesHit(mMainSpriteInst, NULL);
	MarkDirty();
	return true;
}

bool PopAnim::Play(const std::string& theFrameLabel, bool resetAnim) //1437-1459
{
	mAnimRunning = false;
	if (mMainAnimDef->mMainSpriteDef != NULL)
	{
		if (!SetupSpriteInst())
			return false;
		int aFrameNum = mMainAnimDef->mMainSpriteDef->GetLabelFrame(theFrameLabel);
		if (aFrameNum == -1)
			return false;
		mLastPlayedFrameLabel = theFrameLabel;
		return Play(aFrameNum, resetAnim);
	}
	SetupSpriteInst(theFrameLabel);
	return Play(mMainSpriteInst->mDef->mWorkAreaStart, resetAnim);
}

bool PopAnim::BlendTo(const std::string& theFrameLabel, int theBlendTicks, int theAnimStartDelay) //Correct? Not in H5 | 1469-1607
{
	if (!SetupSpriteInst())
		return false;

	if (mTransDirty) //Is this supposed to be true in C++?
	{
		UpdateTransforms(mMainSpriteInst, NULL, mColor, false);
		mTransDirty = false;
	}
	StringToBlendSrcMap aNameToBlendSrcMap;
	PAFrame* aFrame = &mMainSpriteInst->mDef->mFrames[mMainSpriteInst->mFrameNum];
	PATransform aCurTransform;
	Color aCurColor;
	for (int anObjectPosIdx = 0; anObjectPosIdx < aFrame->mFrameObjectPosVector.size(); anObjectPosIdx++)
	{
		PAObjectPos* anObjectPos = &aFrame->mFrameObjectPosVector[anObjectPosIdx];
		PAObjectInst* anObjectInst = &mMainSpriteInst->mChildren[anObjectPos->mObjectNum];
		if (anObjectInst->mName != NULL && *anObjectInst->mName != NULL)
		{
			if (anObjectPos->mIsSprite)
			{
				PASpriteInst* aChildSpriteInst = mMainSpriteInst->mChildren[anObjectPos->mObjectNum].mSpriteInst;
				aCurColor = aChildSpriteInst->mCurColor;
				memcpy(&aCurTransform, &aChildSpriteInst->mCurTransform, sizeof(aCurTransform));
			}
			else
				CalcObjectPos(mMainSpriteInst, anObjectPosIdx, false, &aCurTransform, &aCurColor);
			BlendSrcData aBlendSrcData;
			memcpy(&aBlendSrcData.mTransform, &aCurTransform, sizeof aBlendSrcData.mTransform);
			aBlendSrcData.mColor = aCurColor;
			if (anObjectInst->mSpriteInst != NULL)
			{
				aBlendSrcData.mParticleEffectVector = anObjectInst->mSpriteInst->mParticleEffectVector;
				anObjectInst->mSpriteInst->mParticleEffectVector.clear();
			}
			aNameToBlendSrcMap.insert(StringToBlendSrcMap::value_type(anObjectPos->mName, aBlendSrcData));
		}
	}
	PAParticleEffectVector aCopiedParticleEffectVector = mMainSpriteInst->mParticleEffectVector;
	mMainSpriteInst->mParticleEffectVector.clear();
	mBlendTicksTotal = theBlendTicks;
	mBlendTicksCur = 0.0;
	mBlendDelay = theAnimStartDelay;
	if (mMainAnimDef->mMainSpriteDef != NULL)
	{
		if (!SetupSpriteInst())
			return false;
		int aFrameNum = mMainAnimDef->mMainSpriteDef->GetLabelFrame(theFrameLabel);
		if (aFrameNum == -1)
			return false;
		mLastPlayedFrameLabel = theFrameLabel;
		Play(aFrameNum, false);
		mTransDirty = true;
	}
	else
	{
		SetupSpriteInst(theFrameLabel);
		Play(mMainSpriteInst->mDef->mWorkAreaStart, false);
	}
	mMainSpriteInst->mParticleEffectVector = aCopiedParticleEffectVector;
	aCopiedParticleEffectVector.clear();
	for (int anObjectNum = 0; anObjectNum < mMainSpriteInst->mDef->mObjectDefVector.size(); anObjectNum++)
	{
		PAObjectInst* anObjectInst = &mMainSpriteInst->mChildren[anObjectNum];
		if (anObjectInst->mName && *anObjectInst->mName)
		{
			StringToBlendSrcMap::iterator anItr = aNameToBlendSrcMap.find(anObjectInst->mName);
			if (anItr != aNameToBlendSrcMap.end())
			{
				anObjectInst->mIsBlending = true;
				anObjectInst->mBlendSrcColor.mRed = anItr->second.mColor.mRed;
				anObjectInst->mBlendSrcColor.mGreen = anItr->second.mColor.mGreen;
				anObjectInst->mBlendSrcColor.mBlue = anItr->second.mColor.mBlue;
				anObjectInst->mBlendSrcColor.mAlpha = anItr->second.mColor.mAlpha;
				memcpy(&anObjectInst->mBlendSrcTransform, &anItr->second.mTransform, sizeof anObjectInst->mBlendSrcTransform);
				if (anObjectInst->mSpriteInst)
				{
					if (anItr->second.mParticleEffectVector.size() > 0)
					{
						anObjectInst->mSpriteInst->mParticleEffectVector = anItr->second.mParticleEffectVector;
						anItr->second.mParticleEffectVector.clear();
					}
					else
					{
						PAParticleEffectVector* aVector = &anItr->second.mParticleEffectVector;
						while (aVector->size() > 0)
						{
							delete aVector->back().mEffect;
							aVector->pop_back();
						}
					}
					aNameToBlendSrcMap.erase(anObjectInst->mName);
				}
				else
					anObjectInst->mIsBlending = false;
			}
		}
	}
	while (aNameToBlendSrcMap.size() > 0)
	{
		PAParticleEffectVector* aVector = &aNameToBlendSrcMap.begin()->second.mParticleEffectVector;
		while (aVector->size() > 0)
		{
			delete aVector->back().mEffect;
			aVector->pop_back();
		}
		aNameToBlendSrcMap.erase(aNameToBlendSrcMap.begin()); //?
	}
	return true;
}

bool PopAnim::IsActive() //1610-1618
{
	if (mAnimRunning)
		return true;

	if (HasParticles(mMainSpriteInst))
		return true;

	return false;
}

void PopAnim::SaveStateSpriteInst(Buffer& theBuffer, PASpriteInst* theSpriteInst) //Not in H5 | 1621-1645
{
	theBuffer.WriteLong(theSpriteInst->mFrameNum * 65536.0);
	theBuffer.WriteLong(theSpriteInst->mDelayFrames);
	theBuffer.WriteLong(theSpriteInst->mLastUpdated);
	theBuffer.WriteShort(theSpriteInst->mParticleEffectVector.size());
	for (int aParticleIdx = 0; aParticleIdx < theSpriteInst->mParticleEffectVector.size(); aParticleIdx++)
	{
		PAParticleEffect* aParticleEffect = &theSpriteInst->mParticleEffectVector[aParticleIdx];
		aParticleEffect->mEffect->SaveState(theBuffer, false);
		theBuffer.WriteString(aParticleEffect->mName);
		theBuffer.WriteBoolean(aParticleEffect->mBehind);
		theBuffer.WriteBoolean(aParticleEffect->mAttachEmitter);
		theBuffer.WriteBoolean(aParticleEffect->mTransform);
		theBuffer.WriteLong(aParticleEffect->mXOfs * 65536.0);
		theBuffer.WriteLong(aParticleEffect->mYOfs * 65536.0);
	}
	for (int aChildIdx = 0; aChildIdx < theSpriteInst->mChildren.size(); aChildIdx++)
	{
		PAObjectInst* anObjectInst = &theSpriteInst->mChildren[aChildIdx];
		if (anObjectInst->mSpriteInst != NULL)
			SaveStateSpriteInst(theBuffer, anObjectInst->mSpriteInst);
	}
}

bool PopAnim::SaveState(Buffer& theBuffer) //Not in H5 | 1648-1678
{
	theBuffer.mWriteBitPos = (theBuffer.mReadBitPos + 7) & ~7;
	int aSizeWritePos = theBuffer.mWriteBitPos / 8;
	theBuffer.WriteLong(0);
	theBuffer.WriteShort((short)PAM_STATE_VERSION); //According to XNA
	theBuffer.WriteBoolean(mLoaded);
	if (mLoaded)
	{
		theBuffer.WriteString(mLoadedPamFile);
		theBuffer.WriteLong(mCRCBuffer.GetCRC32(0));
		theBuffer.WriteBoolean(mMirror);
		theBuffer.WriteBoolean(mAnimRunning);
		theBuffer.WriteBoolean(mPaused);
		SetupSpriteInst();
		theBuffer.WriteString(mMainSpriteInst->mDef->mName != NULL ? mMainSpriteInst->mDef->mName : "");
		SaveStateSpriteInst(theBuffer, mMainSpriteInst);
		theBuffer.WriteBoolean(mRandUsed);
		if (mRandUsed)
			theBuffer.WriteString(mRand.Serialize());
	}
	int aSize = theBuffer.mWriteBitPos / 8 - aSizeWritePos - 4;
	theBuffer.mData[aSizeWritePos] = aSize;
	return true;
}

void PopAnim::LoadStateSpriteInst(Buffer& theBuffer, PASpriteInst* theSpriteInst) //Not in H5? | 1681-1709
{
	theSpriteInst->mFrameNum = theBuffer.ReadLong() / 65536.0;
	theSpriteInst->mFrameRepeats = 0;
	theSpriteInst->mDelayFrames = theBuffer.ReadLong();
	theSpriteInst->mLastUpdated = theBuffer.ReadLong();
	theSpriteInst->mOnNewFrame = false;
	int aParticleCount = theBuffer.ReadShort();
	for (int aParticleIdx = 0; aParticleIdx < aParticleCount; aParticleIdx++)
	{
		PAParticleEffect aParticleEffect;
		aParticleEffect.mEffect = new PIEffect();
		aParticleEffect.mEffect->LoadState(theBuffer);
		aParticleEffect.mName = theBuffer.ReadString();
		aParticleEffect.mBehind = theBuffer.ReadBoolean();
		aParticleEffect.mAttachEmitter = theBuffer.ReadBoolean();
		aParticleEffect.mTransform = theBuffer.ReadBoolean();
		aParticleEffect.mXOfs = theBuffer.ReadLong() / 65536.0;
		aParticleEffect.mYOfs = theBuffer.ReadLong() / 65536.0;
		theSpriteInst->mParticleEffectVector.push_back(aParticleEffect);
	}
	for (int aChildIdx = 0; aChildIdx < theSpriteInst->mChildren.size(); aChildIdx++)
	{
		PAObjectInst* anObjectInst = &theSpriteInst->mChildren[aChildIdx];
		if (anObjectInst->mSpriteInst != NULL)
			LoadStateSpriteInst(theBuffer, anObjectInst->mSpriteInst);
	}
}

bool PopAnim::LoadState(Buffer& theBuffer) //Not in H5 | 1712-1758
{
	theBuffer.mReadBitPos = (theBuffer.mReadBitPos + 7) & ~7;
	int aSize = theBuffer.ReadLong();
	int anEnd = theBuffer.mReadBitPos / 8 + aSize;
	int aVersion = theBuffer.ReadShort();
	if (theBuffer.ReadBoolean())
	{
		std::string aSrcFileName = theBuffer.ReadString();
		int aChecksum = (int)theBuffer.ReadLong();
		mMirror = theBuffer.ReadBoolean();
		if (!mLoaded)
			Load_LoadPam(aSrcFileName);
		else if (mMainSpriteInst != NULL)
		{
			ResetAnimHelper(mMainSpriteInst);
			CleanParticles(mMainSpriteInst, true);
			mAnimRunning = false;
		}
		mAnimRunning = theBuffer.ReadBoolean();
		mPaused = theBuffer.ReadBoolean();
		if (aChecksum != mCRCBuffer.GetCRC32(0))
		{
			theBuffer.mReadBitPos = anEnd * 8;
			return false;
		}
		std::string aCurInstName = theBuffer.ReadString();
		SetupSpriteInst(aCurInstName);
		LoadStateSpriteInst(theBuffer, mMainSpriteInst);
		if (aVersion >= 1)
		{
			mRandUsed = theBuffer.ReadBoolean();
			if (mRandUsed)
				mRand.SRand(theBuffer.ReadString());
		}
	}
	return true;
}

void PopAnim::SetColor(const Color& theColor) //Not in H5 | 1764-1767
{
	mColor = theColor;
	MarkDirty();
}

PAObjectInst* PopAnim::GetObjectInst(const std::string& theName) //Not in H5 | 1770-1773
{
	SetupSpriteInst();
	return mMainSpriteInst->GetObjectInst(theName);
}

void PopAnim::FrameHit(PASpriteInst* theSpriteInst, PAFrame* theFrame, PAObjectPos* theObjectPos) //ALMOST | 1780-2008
{
	theSpriteInst->mOnNewFrame = false;
	for (int anObjectPosIdx = 0; anObjectPosIdx < theFrame->mFrameObjectPosVector.size(); anObjectPosIdx++)
	{
		PAObjectPos* anObjectPos = &theFrame->mFrameObjectPosVector[anObjectPosIdx];
		if (anObjectPos->mIsSprite) //Also !NULL?
		{
			PASpriteInst* aSpriteInst = theSpriteInst->mChildren[anObjectPos->mObjectNum].mSpriteInst;
			if (aSpriteInst != NULL)
			{
				for (int aPreload = 0; aPreload < anObjectPos->mPreloadFrames; aPreload++)
					IncSpriteInstFrame(aSpriteInst, anObjectPos, 1000.0 / gSexyAppBase->mFrameTime / theSpriteInst->mDef->mAnimRate);
			}
		}
	}
	for (int aCmdNum = 0; aCmdNum < theFrame->mCommandVector.size(); aCmdNum++)
	{
		PACommand* aCommand = &theFrame->mCommandVector[aCmdNum];
		if (mListener != NULL && mListener->PopAnimCommand(mId, theSpriteInst, aCommand->mCommand, aCommand->mParam))
			continue;
		if (Lower(aCommand->mCommand) == "delay") //Not in H5
		{
			int aCommaPos = aCommand->mParam.find(',');
			if (aCommaPos != -1)
			{
				int aFirst = atoi(Trim(aCommand->mParam.substr(0, aCommaPos)).c_str());
				int aSecond = atoi(Trim(aCommand->mParam.substr(aCommaPos + 1)).c_str());
				if (aSecond <= aFirst)
					aSecond = aFirst + 1;
				theSpriteInst->mDelayFrames = aFirst + Rand() % (aSecond - aFirst);
			}
			else
				theSpriteInst->mDelayFrames = atoi(Trim(aCommand->mParam).c_str());
		}
		else if (Lower(aCommand->mCommand) == "playsample") //Not in H5
		{
			std::string aParams = aCommand->mParam;
			int aPan = 0;
			double aVolume = 1.0;
			double aNumSteps = 0.0;
			std::string aSampleName;
			bool firstParam = true;
			while (aParams.length() > 0)
			{
				std::string aCurParam;
				int aCommaPos = aCommand->mParam.find(',');
				if (aCommaPos != -1)
					aCurParam = aParams.substr(0, aParams.find(','));
				else
					aCurParam = aParams;
				if (firstParam)
				{
					aSampleName = aCurParam;
					firstParam = false;
				}
				else
				{
					int aSpacePos = aCurParam.find(' ');
					while (aSpacePos != -1)
						aCurParam.erase(aSpacePos);
					if (strnicmp(aCurParam.c_str(), "volume=", 7) == 0)
						StringToDouble(aCurParam.substr(7), &aVolume);
					else if (strnicmp(aCurParam.c_str(), "pan=", 4) == 0)
						StringToInt(aCurParam.substr(4), &aPan);
					else if (strnicmp(aCurParam.c_str(), "steps=", 6) == 0)
						StringToDouble(aCurParam.substr(6), &aNumSteps);
				}
				if (aCommaPos == -1)
					break;
				aParams = aParams.substr(aCommaPos + 1);
			}
			if (mListener)
				mListener->PopAnimPlaySample(aSampleName, aPan, aVolume, aNumSteps);
		}
		else
		{
			if (Lower(aCommand->mCommand) == "addparticleeffect")
				continue;
			std::string aParams = aCommand->mParam;
			PAParticleEffect aParticleEffect;
			aParticleEffect.mXOfs = 0.0;
			aParticleEffect.mYOfs = 0.0;
			aParticleEffect.mBehind = false;
			aParticleEffect.mEffect = NULL;
			aParticleEffect.mAttachEmitter = false;
			aParticleEffect.mTransform = false;
			aParticleEffect.mLastUpdated = mUpdateCnt;
			bool once = false;
			std::string aFileName;
			bool firstParam = true;
			while (aParams.length() > 0)
			{
				std::string aCurParam;
				int aCommaPos = aCommand->mParam.find(',');
				if (aCommaPos != -1)
					aCurParam = aParams.substr(0, aParams.find(','));
				else
					aCurParam = aParams;
				aCurParam = Trim(aCurParam);
				if (firstParam)
				{
					aParticleEffect.mName = aCurParam;
					aFileName = aCurParam;
					firstParam = false;
				}
				else
				{
					int aSpacePos = aCurParam.find(' ');
					while (aSpacePos != -1)
						aCurParam.erase(aSpacePos);
					if (strnicmp(aCurParam.c_str(), "x=", 2) == 0)
						StringToDouble(aCurParam.substr(2), &aParticleEffect.mXOfs);
					else if (strnicmp(aCurParam.c_str(), "y=", 2) == 0)
						StringToDouble(aCurParam.substr(2), &aParticleEffect.mYOfs);
					else if (stricmp(aCurParam.c_str(), "attachemitter") == 0)
						aParticleEffect.mAttachEmitter = true;
					else if (stricmp(aCurParam.c_str(), "once") == 0)
						once = true;
					else if (stricmp(aCurParam.c_str(), "transform") == 0)
						aParticleEffect.mTransform = true;
				}
				if (aCommaPos == -1)
					break;
				aParams = aParams.substr(aCommaPos + 1);
			}
			if (once)
			{
				for (int i = 0; i < theSpriteInst->mParticleEffectVector.size(); i++)
				{
					PAParticleEffect* aCheckParticleEffect = &theSpriteInst->mParticleEffectVector[i];
					if (aCheckParticleEffect->mName == aFileName)
						return;
				}
			}
			//Not in H5
			std::string aBackPath = GetPathFrom("..\\" + aFileName + "\\" + aFileName, GetFileDir(mLoadedPamFile));
			std::string aPath = GetPathFrom(aFileName + "\\" + aFileName, GetFileDir(mLoadedPamFile));
			if (mListener)
				aParticleEffect.mEffect = mListener->PopAnimLoadParticleEffect(aFileName);
			if (aParticleEffect.mEffect == NULL)
			{
				ResourceRef aResourceRef = gSexyAppBase->mResourceManager->GetResourceRefFromPath(aPath + ".ppf");
				if (&aResourceRef == NULL)
					aResourceRef = gSexyAppBase->mResourceManager->GetResourceRefFromPath(aBackPath + ".ppf");
				if (&aResourceRef == NULL)
					aResourceRef = gSexyAppBase->mResourceManager->GetResourceRef(ResourceManager::ResType_PIEffect, "PIEFFECT_" + Upper(aFileName));
				if (&aResourceRef != NULL)
				{
					//aParticleEffect.mEffect =  //?
					aParticleEffect.mResourceRef = aResourceRef;
				}
			}
			if (aParticleEffect.mEffect == NULL)
			{
				aParticleEffect.mEffect = new PIEffect();
				if (!aParticleEffect.mEffect->LoadEffect(aPath + ".ppf") && !aParticleEffect.mEffect->LoadEffect(aBackPath + ".ppf") && !aParticleEffect.mEffect->LoadEffect(aPath + ".ip3"))
				{
					bool didLoad = false;
					for (int i = 0; i < mImageSearchPathVector.size(); i++)
					{
						didLoad |= aParticleEffect.mEffect->LoadEffect(mImageSearchPathVector[i] + aFileName + ".ip3");
						if (didLoad)
							break;
					}
					if (!didLoad)
					{
						delete aParticleEffect.mEffect;
						aParticleEffect.mEffect = NULL;
					}
				}
			}
			if (aParticleEffect.mEffect == NULL)
				continue;
			if (!mRandUsed)
			{
				mRandUsed = true;
				mRand.SRand(Rand());
			}
			if (aParticleEffect.mEffect->mRandSeeds.size() > 0)
				aParticleEffect.mEffect->mRand.SRand(aParticleEffect.mEffect->mRandSeeds[aParticleEffect.mEffect->mRandSeeds.size()] % mRand.Next());
			else
				aParticleEffect.mEffect->mRand.SRand(mRand.Next());
			aParticleEffect.mEffect->mWantsSRand = false;
			if (theObjectPos != NULL)
			{
				int anIncs = 100.0 * theObjectPos->mPreloadFrames / theObjectPos->mTimeScale / theSpriteInst->mDef->mAnimRate + 0.5;
				for (int i = 0; i < anIncs; i++)
					aParticleEffect.mEffect->Update();
			}
			theSpriteInst->mParticleEffectVector.push_back(aParticleEffect);
		}
	}
}

void PopAnim::DoFramesHit(PASpriteInst* theSpriteInst, PAObjectPos* theObjectPos) //2011-2026
{
	PAFrame* aCurFrame = &theSpriteInst->mDef->mFrames[theSpriteInst->mFrameNum];
	FrameHit(theSpriteInst, aCurFrame, theObjectPos);
	for (int anObjectPosIdx = 0; anObjectPosIdx < aCurFrame->mFrameObjectPosVector.size(); anObjectPosIdx++)
	{
		PAObjectPos* anObjectPos = &aCurFrame->mFrameObjectPosVector[anObjectPosIdx];
		if (anObjectPos->mIsSprite)
		{
			PASpriteInst* aSpriteInst = theSpriteInst->mChildren[anObjectPos->mObjectNum].mSpriteInst;
			if (aSpriteInst != NULL)
				DoFramesHit(aSpriteInst, anObjectPos);
		}
	}
}

void PopAnim::CalcObjectPos(PASpriteInst* theSpriteInst, int theObjectPosIdx, bool frozen, PATransform* theTransform, Color* theColor) //TODO | 2029-2143
{
	PAFrame* aFrame = &theSpriteInst->mDef->mFrames[theSpriteInst->mFrameNum];
	PAObjectPos* anObjectPos = &aFrame->mFrameObjectPosVector[theObjectPosIdx];
	PAObjectInst* anObjectInst = &theSpriteInst->mChildren[anObjectPos->mObjectNum];
	PAObjectPos* aNextObjectPos[3]; //m var in H5
	memset(aNextObjectPos, 0, sizeof(aNextObjectPos));
	int anOfsTab[3] = { theSpriteInst->mDef->mFrames.size() - 1, 1, 2};
	if (theSpriteInst == mMainSpriteInst && theSpriteInst->mFrameNum >= theSpriteInst->mDef->mWorkAreaStart)
		anOfsTab[0] = theSpriteInst->mDef->mWorkAreaDuration - 1;
	PATransform aCurTransform;
	Color aCurColor;
	if (mInterpolate && !frozen)
	{
		for (int anOfsIdx = 0; anOfsIdx < 3; anOfsIdx++)
		{
			PAFrame* aNextFrame = &theSpriteInst->mDef->mFrames[theSpriteInst->mFrameNum + anOfsTab[anOfsIdx] % theSpriteInst->mDef->mFrames.size()];
			if (theSpriteInst == mMainSpriteInst && theSpriteInst->mFrameNum >= theSpriteInst->mDef->mWorkAreaStart)
				aNextFrame = &theSpriteInst->mDef->mFrames[theSpriteInst->mDef->mWorkAreaStart + anOfsTab[anOfsIdx] + theSpriteInst->mFrameNum - theSpriteInst->mDef->mWorkAreaStart % theSpriteInst->mDef->mWorkAreaDuration + 1];
			else
				aNextFrame = &theSpriteInst->mDef->mFrames[(anOfsTab[anOfsIdx] + (int)theSpriteInst->mFrameNum) % theSpriteInst->mDef->mFrames.size()]; //?
			if (aFrame->mHasStop)
				aNextFrame = aFrame;
			if (aNextFrame->mFrameObjectPosVector.size() > theObjectPosIdx)
			{
				aNextObjectPos[anOfsIdx] = &aNextFrame->mFrameObjectPosVector[theObjectPosIdx];
				if (aNextObjectPos[anOfsIdx]->mObjectNum != anObjectPos->mObjectNum)
					aNextObjectPos[anOfsIdx] = NULL;
				if (aNextObjectPos[anOfsIdx] != NULL)
				{
					for (int aCheckObjectPosIdx = 0; aCheckObjectPosIdx < aNextFrame->mFrameObjectPosVector.size(); aCheckObjectPosIdx++)
					{
						aNextObjectPos[anOfsIdx] = &aNextFrame->mFrameObjectPosVector[aCheckObjectPosIdx];
						break;
					}
				}
			}
		}
		if (aNextObjectPos[1] != NULL)
		{
			float anInterp = theSpriteInst->mFrameNum - (double)(int)theSpriteInst->mFrameNum;
			bool isFar = false; //Not in H5
			SexyVector2 aCur = anObjectPos->mTransform.mMatrix * SexyVector2(0.0, 0.0);
			SexyVector2 aNext = aNextObjectPos[1]->mTransform.mMatrix * SexyVector2(0.0, 0.0);
			if (aNextObjectPos[0] != NULL && aNextObjectPos[2] != NULL)
			{
				SexyVector2 aPrev = aNextObjectPos[0]->mTransform.mMatrix * SexyVector2(0.0, 0.0);
				SexyVector2 aNextNext = aNextObjectPos[2]->mTransform.mMatrix * SexyVector2(0.0, 0.0);
				SexyVector2 aPrevMove = aCur - aPrev;
				SexyVector2 aCurMove = aCur - aNext;
				SexyVector2 aNextMove = aNextNext - aNext;
				SexyVector2 aPrevToCurMoveDelta = aCurMove - aPrevMove;
				SexyVector2 aCurToNextMoveDelta = aNextMove - aCurMove;
				float aMaxSurroundMag = max(aPrevMove.Magnitude(), aNextMove.Magnitude()) * 0.5f + aPrevMove.Magnitude() * 0.25f + aNextMove.Magnitude() * 0.25f;
				if (aCurToNextMoveDelta.Magnitude() > aMaxSurroundMag * 4.0)
					isFar = true;
			}
			if (isFar) //Not in H5
				anInterp = anInterp < 0.5 ? 0.0 : 1.0;
			memcpy(&aCurTransform, &anObjectPos->mTransform.InterpolateTo(aNextObjectPos[1]->mTransform, anInterp), sizeof aCurTransform);
			aCurColor = Color(anObjectPos->mColor.mRed * (1.0 - anInterp) + aNextObjectPos[1]->mColor.mRed * anInterp + 0.5, anObjectPos->mColor.mGreen * (1.0 - anInterp) + aNextObjectPos[1]->mColor.mGreen * anInterp + 0.5, anObjectPos->mColor.mBlue * (1.0 - anInterp) + aNextObjectPos[1]->mColor.mBlue * anInterp + 0.5, anObjectPos->mColor.mAlpha * (1.0 - anInterp) + aNextObjectPos[1]->mColor.mAlpha * anInterp + 0.5);
		}
		else
		{
			memcpy(&aCurTransform, &anObjectPos->mTransform, sizeof(aCurTransform));
			aCurColor = anObjectPos->mColor;
		}
	}
	else
	{
		memcpy(&aCurTransform, &anObjectPos->mTransform, sizeof(aCurTransform));
		aCurColor = anObjectPos->mColor;
	}
	memcpy(&aCurTransform, anObjectPos->mTransform * &aCurTransform.mMatrix, sizeof(aCurTransform)); //?
	if (anObjectInst->mIsBlending && mBlendTicksTotal != 0.0 && theSpriteInst == mMainSpriteInst)
	{
		float anInterp = mBlendTicksCur / mBlendTicksTotal;
		memcpy(&aCurTransform, &anObjectInst->mBlendSrcTransform.InterpolateTo(aCurTransform, anInterp), sizeof(aCurTransform));
		aCurColor = Color((anObjectInst->mBlendSrcColor.mRed * (1.0 - anInterp) + aCurColor.mRed * anInterp + 0.5), (anObjectInst->mBlendSrcColor.mGreen * (1.0 - anInterp) + aCurColor.mGreen * anInterp + 0.5), (anObjectInst->mBlendSrcColor.mBlue * (1.0 - anInterp) + aCurColor.mBlue * anInterp + 0.5), (anObjectInst->mBlendSrcColor.mAlpha * (1.0 - anInterp) + aCurColor.mAlpha * anInterp + 0.5));
	}
	memcpy(&theTransform, &aCurTransform, sizeof(aCurTransform));
	theColor = &aCurColor;
}

void PopAnim::UpdateTransforms(PASpriteInst* theSpriteInst, PATransform* theTransform, const Color& theColor, bool parentFrozen) //2146-2204
{
	if (theTransform != NULL)
		memcpy(&theSpriteInst->mCurTransform, theTransform, sizeof theSpriteInst->mCurTransform);
	else
		memcpy(&theSpriteInst->mCurTransform, &mTransform, sizeof theSpriteInst->mCurTransform);
	theSpriteInst->mCurColor = theColor;
	PAFrame* aFrame = &theSpriteInst->mDef->mFrames[theSpriteInst->mFrameNum];
	PATransform aCurTransform;
	Color aCurColor;
	bool frozen = parentFrozen || theSpriteInst->mDelayFrames > 0 || aFrame->mHasStop;
	for (int anObjectPosIdx = 0; anObjectPosIdx < aFrame->mFrameObjectPosVector.size(); anObjectPosIdx++)
	{
		PAObjectPos* anObjectPos = &aFrame->mFrameObjectPosVector[anObjectPosIdx];
		if (anObjectPos->mIsSprite)
		{
			CalcObjectPos(theSpriteInst, anObjectPosIdx, frozen, &aCurTransform, &aCurColor);
			if (theTransform)
				memcpy(&aCurTransform, &theTransform->TransformSrc(aCurTransform), sizeof aCurTransform);
			UpdateTransforms(theSpriteInst->mChildren[anObjectPos->mObjectNum].mSpriteInst, &aCurTransform, aCurColor, frozen);
		}
	}
	for (int i = 0; i < theSpriteInst->mParticleEffectVector.size(); i++)
	{
		PAParticleEffect* aParticleEffect = &theSpriteInst->mParticleEffectVector[i];
		if (aParticleEffect->mAttachEmitter)
		{
			if (aParticleEffect->mTransform)
			{
				SexyTransform2D aTransform;
				aTransform.Translate(aParticleEffect->mEffect->mWidth / 2.0f, aParticleEffect->mEffect->mHeight / 2.0f);
				aTransform = theSpriteInst->mCurTransform.mMatrix * aTransform;
				memcpy(&aParticleEffect->mEffect->mEmitterTransform, &aTransform, sizeof(aParticleEffect->mEffect->mEmitterTransform));
			}
			else
			{
				SexyVector2 aVec(aParticleEffect->mXOfs, aParticleEffect->mYOfs);
				theSpriteInst->mCurTransform.mMatrix* aVec;
				SexyTransform2D aTransform;
				aTransform.Translate(aVec.x, aVec.y);
				memcpy(&aParticleEffect->mEffect->mEmitterTransform, &aTransform, sizeof(aParticleEffect->mEffect->mEmitterTransform));
			}
			aParticleEffect->mEffect->mEmitterTransform.Translate(mParticleAttachOffset.mX, mParticleAttachOffset.mY);
		}
	}
}

void PopAnim::UpdateParticles(PASpriteInst* theSpriteInst, PAObjectPos* theObjectPos) //ALMOST? | 2207-2254
{
	if (theSpriteInst == NULL)
		return;
	for (int i = 0; i < theSpriteInst->mParticleEffectVector.size(); i++)
	{
		PAParticleEffect* aParticleEffect = &theSpriteInst->mParticleEffectVector[i];
		SexyVector2 aVec;
		if (aParticleEffect->mAttachEmitter)
			aVec = theSpriteInst->mCurTransform.mMatrix * SexyVector2(aParticleEffect->mXOfs, aParticleEffect->mYOfs);
		aParticleEffect->mEffect->mDrawTransform.LoadIdentity();
		aParticleEffect->mEffect->mDrawTransform.Translate(aVec.x, aVec.y);
		if (mMirror)
		{
			aVec.x = (float)mAnimRect.mWidth - aVec.x;
			aParticleEffect->mEffect->mDrawTransform.Translate(-((float)mAnimRect.mWidth / 2), 0.0);
			aParticleEffect->mEffect->mDrawTransform.Scale(-1.0, 1.0);
			aParticleEffect->mEffect->mDrawTransform.Translate((float)mAnimRect.mWidth / 2, 0.0);
		}
		aParticleEffect->mEffect->mDrawTransform.Scale(mDrawScale, mDrawScale);
		if (aParticleEffect->mTransform && theObjectPos != NULL)
			aParticleEffect->mEffect->mAnimSpeed = 1.0 / theObjectPos->mTimeScale;
		aParticleEffect->mEffect->Update();
		if (!aParticleEffect->mEffect->IsActive())
		{
			if (aParticleEffect->mEffect != NULL)
				delete aParticleEffect->mEffect;
			theSpriteInst->mParticleEffectVector.erase(theSpriteInst->mParticleEffectVector.begin() + i); //?
			i--;
		}
	}
	PAFrame* aFrame = &theSpriteInst->mDef->mFrames[theSpriteInst->mFrameNum];
	for (int anObjectPosIdx = 0; anObjectPosIdx < aFrame->mFrameObjectPosVector.size(); anObjectPosIdx++)
	{
		PAObjectPos* anObjectPos = &aFrame->mFrameObjectPosVector[anObjectPosIdx];
		if (anObjectPos->mIsSprite)
		{
			PASpriteInst* aChildSpriteInst = theSpriteInst->mChildren[anObjectPos->mObjectNum].mSpriteInst;
			UpdateParticles(aChildSpriteInst, anObjectPos);
		}
	}
}

void PopAnim::CleanParticles(PASpriteInst* theSpriteInst, bool force) //2257-2280
{
	if (theSpriteInst == NULL)
		return;
	for (int i = 0; i < theSpriteInst->mParticleEffectVector.size(); i++)
	{
		PAParticleEffect* aParticleEffect = &theSpriteInst->mParticleEffectVector[i];
		if (aParticleEffect->mLastUpdated != mUpdateCnt || force)
		{
			if (aParticleEffect->mEffect != NULL)
				delete aParticleEffect->mEffect;
			theSpriteInst->mParticleEffectVector.erase(theSpriteInst->mParticleEffectVector.begin() + i);
			i--;
		}
	}
	for (int i = 0; i < (theSpriteInst->mChildren.size()); i++)
	{
		PASpriteInst* aChildSpriteInst = theSpriteInst->mChildren[i].mSpriteInst;
		if (aChildSpriteInst != NULL)
			CleanParticles(aChildSpriteInst, force);
	}
}

bool PopAnim::HasParticles(PASpriteInst* theSpriteInst) //2283-2301
{
	if (theSpriteInst == false)
		return false;


	if (theSpriteInst->mParticleEffectVector.size() != 0)
		return true;

	for (int i = 0; i < theSpriteInst->mChildren.size(); i++)
	{
		PASpriteInst* aChildSpriteInst = theSpriteInst->mChildren[i].mSpriteInst;
		if (aChildSpriteInst != NULL)
		{
			if (HasParticles(aChildSpriteInst))
				return true;
		}
	}
	return false;
}

void PopAnim::IncSpriteInstFrame(PASpriteInst* theSpriteInst, PAObjectPos* theObjectPos, float theFrac) //Correct? | 2307-2374
{
	int aLastFrameNum = theSpriteInst->mFrameNum;
	PAFrame* aLastFrame = &theSpriteInst->mDef->mFrames[aLastFrameNum];
	if (aLastFrame->mHasStop)
		return;

	float aTimeScale = theObjectPos != NULL ? theObjectPos->mTimeScale : 1.0;
	theSpriteInst->mFrameNum += theFrac * theSpriteInst->mDef->mAnimRate / 1000.0 / gSexyAppBase->mFrameTime / aTimeScale;
	if (theSpriteInst == mMainSpriteInst)
	{
		if (!theSpriteInst->mDef->mFrames.back().mHasStop)
		{
			if ((int)theSpriteInst->mFrameNum >= theSpriteInst->mDef->mWorkAreaStart + theSpriteInst->mDef->mWorkAreaDuration + 1)
			{
				theSpriteInst->mFrameRepeats++;
				theSpriteInst->mFrameNum -= theSpriteInst->mDef->mWorkAreaDuration + 1;
			}
		}
		else if ((int)theSpriteInst->mFrameNum >= theSpriteInst->mDef->mWorkAreaStart + theSpriteInst->mDef->mWorkAreaDuration)
		{
			theSpriteInst->mOnNewFrame = true;
			theSpriteInst->mFrameNum = theSpriteInst->mDef->mWorkAreaStart + theSpriteInst->mDef->mWorkAreaDuration;
			if (theSpriteInst->mDef->mWorkAreaDuration != 0)
			{
				mAnimRunning = false;
				if (mListener != NULL)
					mListener->PopAnimStopped(mId);
				return;
			}
			theSpriteInst->mFrameRepeats++;
		}
	}
	else if ((int)theSpriteInst->mFrameNum >= theSpriteInst->mDef->mFrames.size())
	{
		theSpriteInst->mFrameRepeats++;
		theSpriteInst->mFrameNum -= theSpriteInst->mDef->mFrames.size();
	}
	theSpriteInst->mOnNewFrame = (int)theSpriteInst->mFrameNum != aLastFrameNum;
	if (theSpriteInst->mOnNewFrame && theSpriteInst->mDelayFrames > 0)
	{
		theSpriteInst->mOnNewFrame = false;
		theSpriteInst->mFrameNum = aLastFrameNum;
		theSpriteInst->mDelayFrames--;
		return;
	}
	for (int anObjectPosIdx = 0; anObjectPosIdx < aLastFrame->mFrameObjectPosVector.size(); anObjectPosIdx++)
	{
		PAObjectPos* anObjectPos = &aLastFrame->mFrameObjectPosVector[anObjectPosIdx];
		if (anObjectPos->mIsSprite)
		{
			PASpriteInst* aChildSpriteInst = theSpriteInst->mChildren[anObjectPos->mObjectNum].mSpriteInst; //?
			IncSpriteInstFrame(aChildSpriteInst, anObjectPos, theFrac / aTimeScale);
		}
	}
}

void PopAnim::PrepSpriteInstFrame(PASpriteInst* theSpriteInst, PAObjectPos* theObjectPos) //2377-2424
{
	PAFrame* aCurFrame = &theSpriteInst->mDef->mFrames[theSpriteInst->mFrameNum];
	if (theSpriteInst->mOnNewFrame)
		FrameHit(theSpriteInst, aCurFrame, theObjectPos);
	if (aCurFrame->mHasStop)
	{
		if (theSpriteInst == mMainSpriteInst)
		{
			mAnimRunning = 0;
			if (mListener)
				mListener->PopAnimStopped(mId);
		}
	}
	else
	{
		for (int anObjectPosIdx = 0; anObjectPosIdx < aCurFrame->mFrameObjectPosVector.size(); anObjectPosIdx++)
		{
			PAObjectPos* anObjectPos = &aCurFrame->mFrameObjectPosVector[anObjectPosIdx];
			if (anObjectPos->mIsSprite)
			{
				PASpriteInst* aSpriteInst = theSpriteInst->mChildren[anObjectPos->mObjectNum].mSpriteInst;
				if (aSpriteInst != NULL)
				{
					int aPhysFrameNum = theSpriteInst->mFrameNum + theSpriteInst->mFrameRepeats * theSpriteInst->mDef->mFrames.size();
					int aPhysLastFrameNum = aPhysFrameNum - 1;
					if (aSpriteInst->mLastUpdated != aPhysLastFrameNum && aSpriteInst->mLastUpdated != aPhysFrameNum)
					{
						aSpriteInst->mFrameNum = 0;
						aSpriteInst->mFrameRepeats = 0;
						aSpriteInst->mDelayFrames = 0;
						aSpriteInst->mOnNewFrame = true;
					}
					PrepSpriteInstFrame(aSpriteInst, anObjectPos);
					aSpriteInst->mLastUpdated = aPhysFrameNum;
				}
			}
		}
	}
}

void PopAnim::AnimUpdate(float theFrac) //2430-2457
{
	if (!mAnimRunning)
		return;

	if (mBlendTicksTotal > 0.0f)
	{
		mBlendTicksCur += theFrac;
		if (mBlendTicksCur >= mBlendTicksTotal)
			mBlendTicksTotal = 0.0f;
	}
	mTransDirty = true;
	if (mBlendDelay > 0.0f)
	{
		mBlendDelay -= theFrac;
		if (mBlendDelay <= 0.0f)
		{
			mBlendDelay = 0.0f;
			DoFramesHit(mMainSpriteInst, NULL);
		}
		else
		{
			IncSpriteInstFrame(mMainSpriteInst, NULL, theFrac);
			PrepSpriteInstFrame(mMainSpriteInst, NULL);
			MarkDirty();
		}
	}
}

void PopAnim::DrawSpriteMirrored(Graphics* g, PASpriteInst* theSpriteInst, PATransform* theTransform, Color& theColor, bool additive, bool parentFrozen) //Not in H5, ALMOST | 2463-2625
{
	DrawParticleEffects(g, theSpriteInst, theTransform, theColor, false);
	PAFrame* aFrame = &theSpriteInst->mDef->mFrames[(int)theSpriteInst->mFrameNum];
	PATransform aCurTransform;
	Color aCurColor;
	bool frozen = parentFrozen || theSpriteInst->mDelayFrames > 0 || aFrame->mHasStop;
	for (int anObjectPosIdx = 0; anObjectPosIdx < aFrame->mFrameObjectPosVector.size(); anObjectPosIdx++)
	{
		PAObjectPos* anObjectPos = &aFrame->mFrameObjectPosVector[anObjectPosIdx];
		if (anObjectPos->mIsSprite)
		{
			PASpriteInst* aChildSpriteInst = theSpriteInst->mChildren[anObjectPos->mObjectNum].mSpriteInst;
			aCurColor = aChildSpriteInst->mCurColor;
			memcpy(&aCurTransform, &aChildSpriteInst->mCurTransform, sizeof(aCurTransform));
		}
		else
			CalcObjectPos(theSpriteInst, anObjectPosIdx, frozen, &aCurTransform, &aCurColor);
		PATransform aNewTransform;
		if (!theTransform && mDrawScale != 1.0)
		{
			PATransform aTrans;
			aTrans.mMatrix.m00 = mDrawScale;
			aTrans.mMatrix.m11 = mDrawScale;
			memcpy(&aNewTransform, &aTrans.TransformSrc(aCurTransform), sizeof(aNewTransform));
		}
		else if (!theTransform || anObjectPos->mIsSprite)
			memcpy(&aNewTransform, &aCurTransform, sizeof(aNewTransform));
		else
			memcpy(&aNewTransform, &theTransform->TransformSrc(aCurTransform), sizeof(aNewTransform));
		Color aNewColor(theColor.mRed * aCurColor.mRed / 255, theColor.mGreen * aCurColor.mGreen / 255, theColor.mBlue * aCurColor.mBlue / 255, theColor.mAlpha * aCurColor.mAlpha / 255);
		if (aNewColor.mAlpha == NULL)
			continue;
		if (anObjectPos->mIsSprite)
		{
			DrawSpriteMirrored(g, theSpriteInst->mChildren[anObjectPos->mObjectNum].mSpriteInst, &aNewTransform, aNewColor, anObjectPos->mIsAdditive || additive, frozen);
			continue;
		}
		PAImage* anImage = &mImageVector[anObjectPos->mResNum];
		PATransform anImageTransform;
		aNewTransform.TransformSrc(anImage->mTransform);
		g->SetColorizeImages(true);
		g->SetColor(aNewColor);
		if (additive || anObjectPos->mIsAdditive)
			g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		else
			g->SetDrawMode(anImage->mDrawMode);
		DeviceImage* aDrawImage = NULL; //Image in XNA
		Rect aSrcRect;
		if (anObjectPos->mAnimFrameNum == 0 || anImage->mImages.size() == 1)
		{
			aDrawImage = anImage->mImages[0];
			aSrcRect = aDrawImage->GetCelRect(anObjectPos->mAnimFrameNum);
		}
		else
		{
			aDrawImage = anImage->mImages[anObjectPos->mAnimFrameNum];
			aSrcRect = aDrawImage->GetCelRect(0);
		}
		if (anObjectPos->mHasSrcRect)
			aSrcRect = anObjectPos->mSrcRect;
		if (mImgScale != 1.0)
		{
			float aPrevX = anImageTransform.mMatrix.m02;
			float aPrevY = anImageTransform.mMatrix.m12;
			PATransform aTransform;
			aTransform.mMatrix.m00 = 1.0f / mImgScale;
			aTransform.mMatrix.m11 = 1.0f / mImgScale;
			memcpy(&anImageTransform, &aTransform.TransformSrc(anImageTransform), sizeof anImageTransform);
			anImageTransform.mMatrix.m02 = aPrevX;
			anImageTransform.mMatrix.m12 = aPrevY;
		}
		if (mDrawScale != 1.0)
			anImageTransform.mMatrix.m02 += (float)mAnimRect.mWidth * (1.0f - mDrawScale);
		if (anImageTransform.mMatrix.m00 == 1.0f && anImageTransform.mMatrix.m01 == 0.0f && anImageTransform.mMatrix.m10 == 0.0f && anImageTransform.mMatrix.m11 == 1.0f)
		{
			float x = (float)(mAnimRect.mWidth - aSrcRect.mWidth) / 2.0f - (anImageTransform.mMatrix.m02 + (float)(aSrcRect.mWidth - mAnimRect.mWidth) / 2.0f);
			float y = anImageTransform.mMatrix.m12;
			g->DrawImageF(aDrawImage, x, y, aSrcRect);
		}
		else if (mVersion == 1 || (anImageTransform.mMatrix.m00 == anImageTransform.mMatrix.m11 && anImageTransform.mMatrix.m01 == 0.0f - anImageTransform.mMatrix.m10 && fabs((anImageTransform.mMatrix.m00 * anImageTransform.mMatrix.m00 + anImageTransform.mMatrix.m01 * anImageTransform.mMatrix.m01) - 1.0f) < 0.01f))
		{
			float aRot = atan2(anImageTransform.mMatrix.m01, anImageTransform.mMatrix.m00);
			float aDownAngle = -aRot;
			float aCenterX = anImageTransform.mMatrix.m02 + cosf(aDownAngle) * aSrcRect.mWidth / 2.0f - sinf(aDownAngle) * aSrcRect.mHeight / 2.0f;
			float aCenterY = anImageTransform.mMatrix.m12 + sinf(aDownAngle) * aSrcRect.mWidth / 2.0f + cosf(aDownAngle) * aSrcRect.mHeight / 2.0f;
			float aTopX = aCenterX - aSrcRect.mWidth / 2.0;
			float theY = aCenterY - aSrcRect.mHeight / 2.0;
			aTopX = (mAnimRect.mWidth - aSrcRect.mWidth) / 2.0 - (aTopX + (aSrcRect.mWidth - mAnimRect.mWidth) / 2.0);
			g->DrawImageRotatedF(aDrawImage, aTopX, theY, 0.0 - aRot, &aSrcRect);
		}
		else
		{
			SexyMatrix3 aSexyMatrix;
			aSexyMatrix.LoadIdentity();
			aSexyMatrix.m02 = aSrcRect.mWidth / 2.0f;
			aSexyMatrix.m12 = aSrcRect.mHeight / 2.0f;
			//memcpy(&anImageTransform, &anImageTransformMatrixm. * aSexyMatrix, sizeof(anImageTransform)); //?
			anImageTransform.mMatrix.m02 = mAnimRect.mWidth - anImageTransform.mMatrix.m02;
			anImageTransform.mMatrix.m01 = -anImageTransform.mMatrix.m01;
			anImageTransform.mMatrix.m10 = -anImageTransform.mMatrix.m10;
			g->DrawImageMatrix(aDrawImage, anImageTransform.mMatrix, aSrcRect);
		}
	}
	DrawParticleEffects(g, theSpriteInst, theTransform, theColor, true);
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

void PopAnim::DrawSprite(Graphics* g, PASpriteInst* theSpriteInst, PATransform* theTransform, Color& theColor, bool additive, bool parentFrozen) //ALMOST | 2631-2852
{
	DrawParticleEffects(g, theSpriteInst, theTransform, theColor, false);
	PAFrame* aFrame = &theSpriteInst->mDef->mFrames[theSpriteInst->mFrameNum];
	PATransform aCurTransform;
	Color aCurColor; //Defined later in XNA?
	PASpriteInst* aChildSpriteInst;
	bool frozen = parentFrozen || theSpriteInst->mDelayFrames > 0 || aFrame->mHasStop;
	for (int anObjectPosIdx = 0; anObjectPosIdx < aFrame->mFrameObjectPosVector.size(); anObjectPosIdx++)
	{
		PAObjectPos* anObjectPos = &aFrame->mFrameObjectPosVector[anObjectPosIdx];
		PAObjectInst* anObjectInst = &theSpriteInst->mChildren[anObjectPos->mObjectNum];
		if (mListener != NULL && anObjectInst->mPredrawCallback)
			anObjectInst->mPredrawCallback = mListener->PopAnimObjectPredraw(mId, g, theSpriteInst, anObjectInst, theTransform, theColor);
		if (anObjectPos->mIsSprite)
		{
			aChildSpriteInst = theSpriteInst->mChildren[anObjectPos->mObjectNum].mSpriteInst;
			aCurColor = aChildSpriteInst->mCurColor;
			memcpy(&aCurTransform, &aChildSpriteInst->mCurTransform, sizeof(aCurTransform));
		}
		else
			CalcObjectPos(theSpriteInst, anObjectPosIdx, frozen, &aCurTransform, &aCurColor);
		PATransform aNewTransform;
		if (!theTransform && mDrawScale != 1.0f)
		{
			PATransform aTrans;
			aTrans.mMatrix.m00 = mDrawScale;
			aTrans.mMatrix.m11 = mDrawScale;
			//memcpy(&aTrans, &mTransform * aTrans.mMatrix, sizeof(aNewTransform)); //?
			memcpy(&aNewTransform, &aTrans.TransformSrc(aCurTransform), sizeof(aNewTransform));
		}
		else if (!theTransform || anObjectPos->mIsSprite)
		{
			memcpy(&aNewTransform, &aCurTransform, sizeof(aNewTransform));
			if (mDrawScale != 1.0)
			{
				PATransform aTrans;
				aTrans.mMatrix.m00 = mDrawScale;
				aTrans.mMatrix.m11 = mDrawScale;
				//memcpy(&aNewTransform, &aTrans.mMatrix * aNewTransform.mMatrix, sizeof aNewTransform);
			}
			//memcpy(&aNewTransform, &aTrans * aNewTransform.mMatrix, sizeof aNewTransform);
		}
		else
			theTransform->TransformSrc(aNewTransform);
		Color aNewColor(anObjectInst->mColorMult.mRed * aCurColor.mRed / 65025, anObjectInst->mColorMult.mGreen * aCurColor.mGreen / 65025, anObjectInst->mColorMult.mBlue * aCurColor.mBlue / 65025, anObjectInst->mColorMult.mAlpha * aCurColor.mAlpha / 65025);
		if (aNewColor.mAlpha)
		{
			if (anObjectPos->mIsSprite)
			{
				PASpriteInst* aChildSpriteInst = theSpriteInst->mChildren[anObjectPos->mObjectNum].mSpriteInst;
				DrawSprite(g, aChildSpriteInst, &aNewTransform, aNewColor, anObjectPos->mIsAdditive || additive, frozen);
			}
			else
			{
				for (int anImageDrawCount = 0; true; anImageDrawCount++)
				{
					PAImage* anImage = &mImageVector[anObjectPos->mResNum];
					PATransform anImageTransform;
					aNewTransform.TransformSrc(anImage->mTransform); //?
					g->SetColorizeImages(true);
					g->SetColor(aNewColor);
					if (additive || anObjectPos->mIsAdditive)
						g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
					else
						g->SetDrawMode(anImage->mDrawMode);
					DeviceImage* aDrawImage = NULL;
					Rect aSrcRect;
					if (anObjectPos->mAnimFrameNum == 0 || anImage->mImages.size() == 1)
					{
						aDrawImage = anImage->mImages[0];
						aSrcRect = aDrawImage->GetCelRect(anObjectPos->mAnimFrameNum);
					}
					else
					{
						aDrawImage = anImage->mImages[anObjectPos->mAnimFrameNum];
						aSrcRect = aDrawImage->GetCelRect(0);
					}
					if (anObjectPos->mHasSrcRect)
						aSrcRect = anObjectPos->mSrcRect;
					if (mImgScale != 1.0)
					{
						float aPrevX = anImageTransform.mMatrix.m02;
						float aPrevY = aNewTransform.mMatrix.m12;
						PATransform aTransform;
						aTransform.mMatrix.m00 = 1.0f / mImgScale;
						aTransform.mMatrix.m11 = 1.0f / mImgScale;
						aTransform.TransformSrc(anImageTransform);
						aNewTransform.mMatrix.m02 = aPrevX;
						aNewTransform.mMatrix.m12 = aPrevY;
					}
					PopAnimListener::ImagePredrawResult anImagePrerawResult = PopAnimListener::ImagePredraw_DontAsk; //lol
					if (mListener != NULL && anObjectInst->mImagePredrawCallback)
					{
						anImagePrerawResult = mListener->PopAnimImagePredraw(theSpriteInst, anObjectInst, &anImageTransform, aDrawImage, g, anImageDrawCount);
						if (anImagePrerawResult == PopAnimListener::ImagePredraw_DontAsk)
							anObjectInst->mImagePredrawCallback = false;
						if (anImagePrerawResult == PopAnimListener::ImagePredraw_Skip)
							break;
					}
					if (fabs(anImageTransform.mMatrix.m00 - 1.0f) > 0.01f || fabs(anImageTransform.mMatrix.m01 - 0.0f) > 0.01f || fabs(anImageTransform.mMatrix.m10 - 0.0f) > 0.01f || fabs(anImageTransform.mMatrix.m11 - 1.0f) > 0.01f) //C++ only (Similar in C# DrawSpriteMirrored)
					{
						if (mVersion == 1 || (anImageTransform.mMatrix.m00 == anImageTransform.mMatrix.m11 && anImageTransform.mMatrix.m01 == 0.0f - anImageTransform.mMatrix.m10 && fabs((anImageTransform.mMatrix.m00 * anImageTransform.mMatrix.m00 + anImageTransform.mMatrix.m01 * anImageTransform.mMatrix.m01) - 1.0f) < 0.01f))
						{
							float aRot = atan2(anImageTransform.mMatrix.m01, anImageTransform.mMatrix.m00);
							float aDownAngle = -aRot;
							float aCenterX = anImageTransform.mMatrix.m02 + cosf(aDownAngle) * aSrcRect.mWidth / 2.0f - sinf(aDownAngle) * aSrcRect.mHeight / 2.0f;
							float aCenterY = anImageTransform.mMatrix.m12 + sinf(aDownAngle) * aSrcRect.mWidth / 2.0f + cosf(aDownAngle) * aSrcRect.mHeight / 2.0f;
							float aTopX = aCenterX - aSrcRect.mWidth / 2.0;
							float aTopY = aCenterY - aSrcRect.mWidth / 2.0;
							aTopX = (mAnimRect.mWidth - aSrcRect.mWidth) / 2.0 - (aTopX + (aSrcRect.mWidth - mAnimRect.mWidth) / 2.0);
							if (mColorizeType)
								g->SetColor(aNewColor == Color::White ? Color(0, 0, 255) : Color(64, 64, 255));
							g->DrawImageRotatedF(aDrawImage, aTopX, aTopY, aRot, &aSrcRect);
						}
						else
						{
							SexyMatrix3 aSexyMatrix;
							aSexyMatrix.LoadIdentity();
							aSexyMatrix.m02 = aSrcRect.mWidth / 2.0f;
							aSexyMatrix.m12 = aSrcRect.mHeight / 2.0f;
							memcpy(&anImageTransform, &anImageTransform.mMatrix * aSexyMatrix, sizeof(anImageTransform)); //?
							if (mColorizeType)
								g->SetColor(Color(255, 0, 0));
							g->DrawImageMatrix(aDrawImage, anImageTransform.mMatrix, aSrcRect);
						}
					}
					else
					{
						if (fabs(anImageTransform.mMatrix.m02 - anImageTransform.mMatrix.m02) > 0.01f || fabs(anImageTransform.mMatrix.m12 - anImageTransform.mMatrix.m12) > 0.01f)
						{
							if (mColorizeType)
								g->SetColor(aNewColor == Color::White ? Color(0, 0, 255) : Color(128, 0, 255));
							g->DrawImageF(aDrawImage, anImageTransform.mMatrix.m02, anImageTransform.mMatrix.m12, aSrcRect);
						}
						else
						{
							if (mColorizeType)
								g->SetColor(aNewColor == Color::White ? Color(0, 255, 0) : Color(128, 255, 255));
							g->DrawImage(aDrawImage, anImageTransform.mMatrix.m02, anImageTransform.mMatrix.m12, aSrcRect);
						}
					}
					if (anImagePrerawResult != PopAnimListener::ImagePredraw_Repeat)
						break;
				}
			}
		}
	}
	DrawParticleEffects(g, theSpriteInst, theTransform, theColor, true);
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

void PopAnim::Draw(Graphics* g) //2858-2875
{
	if (!mLoaded)
		return;

	if (!SetupSpriteInst())
		return;

	if (mTransDirty)
	{
		UpdateTransforms(mMainSpriteInst, NULL, mColor, false);
		mTransDirty = false;
	}

	if (mMirror)
		DrawSpriteMirrored(g, mMainSpriteInst, NULL, mColor, mAdditive, false);
	else
		DrawSprite(g, mMainSpriteInst, NULL, mColor, mAdditive, false);
}

void PopAnim::Update() //2881-2901
{
	if (!mLoaded)
		return;

	if (!SetupSpriteInst())
		return;

	if (!gSexyAppBase->mVSyncUpdates)
		WidgetContainer::UpdateF(1.0f);

	UpdateTransforms(mMainSpriteInst, NULL, mColor, false);
	mTransDirty = false;
	if (!mPaused)
	{
		UpdateParticles(mMainSpriteInst, NULL);
		CleanParticles(mMainSpriteInst);
	}
}

void PopAnim::UpdateF(float theFrac) //2906-2911
{
	if (mPaused)
		return;

	AnimUpdate(theFrac);
}

int PopAnim::GetLabelFrame(const std::string& theFrameLabel) //2917-2919
{
	return mMainAnimDef->mMainSpriteDef->GetLabelFrame(theFrameLabel);
}

bool PopAnim::SetupSpriteInst(const std::string& theName) //2922-2965
{
	if (mMainSpriteInst == NULL)
		return false;

	if (mMainSpriteInst->mDef != NULL && theName.length() == 0)
		return true;

	if (mMainAnimDef->mMainSpriteDef != NULL)
	{
		InitSpriteInst(mMainSpriteInst, mMainAnimDef->mMainSpriteDef);
		return true;
	}

	if (mMainAnimDef->mSpriteDefVector.size() == 0)
		return false;

	std::string aName = theName;
	if (aName.length() == 0)
		aName = "main";
	PASpriteDef* aWantDef = NULL;
	for (int i = 0; i < mMainAnimDef->mSpriteDefVector.size(); i++)
	{
		if (mMainAnimDef->mSpriteDefVector[i].mName != NULL)
		{
			if (stricmp(mMainAnimDef->mSpriteDefVector[i].mName, aName.c_str()) == 0)
				aWantDef = &mMainAnimDef->mSpriteDefVector[i];
		}
	}
	if (aWantDef == NULL)
		aWantDef = &mMainAnimDef->mSpriteDefVector[0];
	if (aWantDef != mMainSpriteInst->mDef)
	{
		if (mMainSpriteInst->mDef != NULL)
		{
			if (mMainSpriteInst != NULL)
			{
				delete mMainSpriteInst;
				mMainSpriteInst = new PASpriteInst();
			}
			mMainSpriteInst->mParent = NULL;
		}
		InitSpriteInst(mMainSpriteInst, aWantDef);
		mTransDirty = true;
	}
	return true;
}

PASpriteDef* PopAnim::FindSpriteDef(const char* theAnimName) //Not in H5 | 2971-2982
{
	if (mMainAnimDef == NULL)
		return NULL;

	for (int i = 0; i < mMainAnimDef->mSpriteDefVector.size(); ++i)
	{
		if (stricmp(mMainAnimDef->mSpriteDefVector[i].mName, theAnimName) == 0)
			return &mMainAnimDef->mSpriteDefVector[i];
	}
}