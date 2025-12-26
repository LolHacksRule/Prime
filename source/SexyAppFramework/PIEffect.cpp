#include "PIEffect.h"
#include "Debug.h"
#include "SexyMath.h"
#include "AutoCrit.h"
#include "ImageLib/ImageLib.h"
#include "PakLib/PakInterface.h"
#include "Endian.h"
#include "PerfTimer.h"
#include "SexyCache.h"
#include "Graphics.h"
#include "PolyDivide.h"

using namespace Sexy;

static void* GetData(PIEffect* theEffect, void* thePtr, int theSize) //C++ only | 52-71
{
	for (int i = 0; i < theSize; i++)
	{
		if (theEffect->mBufPos == PI_BUFSIZE)
		{
			theEffect->mBufPos = 0;
			int anICount = p_fread(theEffect->mBuf, 1, PI_BUFSIZE, theEffect->mReadFP) >> 2;
			for (int j = 0; j < anICount; j++)
				theEffect->mFileChecksum += j ^ *&theEffect->mBuf[4 * j];
		}
		((uchar*)thePtr)[i] = theEffect->mBuf[theEffect->mBufPos++];
	}
	return thePtr;
}

bool IsIdentityMatrix(const SexyMatrix3& theMatrix) //88-91
{

	return theMatrix.m01 == 0.0 && theMatrix.m02 == 0.0 && theMatrix.m10 == 0.0 && theMatrix.m12 == 0.0 && theMatrix.m20 == 0.0 && theMatrix.m21 == 0.0 && theMatrix.m00 == 1.0 && theMatrix.m11 == 1.0 && theMatrix.m22 == 1.0;
}

float GetMatrixScale(const SexyMatrix3& theMatrix) //C++ only | 94-96
{
	return sqrt(theMatrix.m10 * theMatrix.m10 + theMatrix.m11 * theMatrix.m11), sqrt(theMatrix.m00 * theMatrix.m00 + theMatrix.m01 * theMatrix.m01);
}

FPoint TransformFPoint(const SexyMatrix3& theMatrix, const FPoint& thePoint) //99-103
{
	SexyVector2 aVector(thePoint.mX, thePoint.mY);
	aVector = theMatrix*aVector; //?
	return FPoint(aVector.x, aVector.y);
}

static float WrapFloat(float theNum, int theRepeat) //107-115
{
	if (theRepeat == 1)
		return theNum;
	else
		theNum *= (float)theRepeat;
	return theNum - (int)theNum;
}

static float DegToRad(float theDeg) //118-120 (Not in XNA [MathHelper.ToRadians])
{
	return theDeg * SEXYMATH_PI / 180.0;
}

ulong InterpColor(ulong theColor1, ulong theColor2, float thePct) //Correct? | 123-136
{
	ulong aColor; //?
	if (thePct == 0.0)
		return theColor1;
	if (thePct <= 0.0)
		aColor = 0.0;
	else
		aColor = thePct;
	if ((theColor1 & 0xFF000000) * (1.0 - aColor) + (theColor2 & 0xFF000000) * aColor >= 4278190080.0)
		aColor = 4278190080.0;
	else
		aColor = (theColor1 & 0xFF000000) * (1.0 - aColor) + (theColor2 & 0xFF000000) * aColor;
	if ((theColor1 & 0xFF0000i64) * (1.0 - aColor) + (theColor2 & 0xFF0000i64) * aColor >= 16711680.0)
		aColor = 16711680.0;
	else
		aColor = (theColor1 & 0xFF0000i64) * (1.0 - aColor) + (theColor2 & 0xFF0000i64) * aColor;
	if ((theColor1 & 0xFF00) * (1.0 - aColor) + (theColor2 & 0xFF00) * aColor >= 65280.0)
		aColor = 65280.0;
	else
		aColor = (theColor1 & 0xFF00) * (1.0 - aColor) + (theColor2 & 0xFF00) * aColor;
	if (theColor1 * (1.0 - aColor) + theColor2 * aColor >= 255.0)
		aColor = 255.0;
	else
		aColor = theColor1 * (1.0 - aColor) + theColor2 * aColor;
	return aColor | aColor & 0xFF00 | aColor & 0xFF0000 | aColor & 0xFF000000;
}

bool LineSegmentIntersects(const FPoint& aPtA1, const FPoint& aPtA2, const FPoint& aPtB1, const FPoint& aPtB2, float* thePos, FPoint* theIntersectionPoint) //Correct? | 139-178
{
	double aDenom = (aPtB2.mY - aPtB1.mY) * (aPtA2.mX - aPtA1.mX) - (aPtB2.mX - aPtB1.mX) * (aPtA2.mY - aPtA1.mY);
	if (aDenom == 0.0)
		return false;
	double aUa = ((aPtB2.mX - aPtB1.mX) * (aPtA1.mY - aPtB1.mY) - (aPtB2.mY - aPtB1.mY) * (aPtA1.mX - aPtB1.mX)) / aDenom;
	if (aUa < 0.0 || aUa > 1.0)
		return false;
	double aUb = ((aPtA2.mX - aPtA1.mX) * (aPtA1.mY - aPtB1.mY) - (aPtA2.mY - aPtA1.mY) * (aPtA1.mX - aPtB1.mX)) / aDenom;
	if (aUb < 0.0 || aUb > 1.0)
		return false;
	if (thePos)
		*thePos = aUa;
	if (theIntersectionPoint)
		*theIntersectionPoint = aPtA1 + (aPtA2 - aPtA1) * aUa;
	return true;
}

static void GetBestStripSize(int theCount, int theCelWidth, int theCelHeight, int& theNumCols, int& theNumRows) //Not in H5 | 181-204
{
	float aBestRatio = 100.0;
	theNumCols = theCount;
	theNumRows = 1;
	for (int aCheckRows = 1; aCheckRows <= theCount; aCheckRows++)
	{
		int aCheckCols = theCount / aCheckRows;
		if (aCheckCols * aCheckRows == theCount)
		{
			float aRatioH = (theCelWidth * aCheckCols) / (theCelHeight * aCheckRows);
			float aLowestRatio = max(aRatioH, 1.0 / aRatioH); //?
			if (aLowestRatio + 0.0001f < aBestRatio)
			{
				theNumRows = aCheckRows;
				theNumCols = aCheckCols;
				aBestRatio = aLowestRatio;
			}
		}
	}
}

//PILayer

void PILayer::SetVisible(bool isVisible) //209-211
{
	mVisible = isVisible;
}

PIEmitterInstance* PILayer::GetEmitter(int theIdx) //214-218
{
	if (theIdx < mEmitterInstanceVector.size())
		return &mEmitterInstanceVector[theIdx];
	else
		return NULL;
}

PIEmitterInstance* PILayer::GetEmitter(const std::string& theName) ///221-226
{
	for (int i = 0; i < mEmitterInstanceVector.size(); i++)
	{
		if (theName.length() || stricmp(mEmitterInstanceVector[i].mEmitterInstanceDef->mName.c_str(), theName.c_str()))
			return &mEmitterInstanceVector[i];
	}
	return NULL;
}

//PI Value

void PIValue::QuantizeCurve() //233-296
{
	float aMinTime = mValuePointVector.front().mTime;
	float aMaxTime = mValuePointVector.back().mTime;
	mQuantTable.clear();
	mQuantTable.resize(PI_QUANT_SIZE);
	bool first = true;
	int aLastGX = 0;
	int aLastX = 0;
	int aLastY = 0;
	float aCurT = aMinTime;
	float aTIncr = (aMaxTime - aMinTime) / PI_QUANT_SIZE / 2.0;
	int aPointIdx = 0;
	for (;;)
	{
		FPoint aFoundPoint = mBezier.Evaluate(aCurT);
		int aGX = (int)((aFoundPoint.mX - aMinTime) / (aMaxTime - aMinTime) * PI_QUANT_SIZE - 1 + 0.5); //TIME_TO_X according to XNA?
		bool done = false;
		while (aFoundPoint.mX >= mValuePointVector[aPointIdx + 1].mTime)
		{
			if (mValuePointVector[aPointIdx + 1].mTime >= aFoundPoint.mX) //On C++
				break;
			aPointIdx++;
			if (aPointIdx >= mValuePointVector.size() - 1) //Why not use back?
			{
				done = true;
				break;
			}
		}
		if (done)
			break;
		if (aFoundPoint.mX >= mValuePointVector[aPointIdx].mTime)
		{
			if (!first && (aGX <= aLastGX + 1))
			{
				for (int aX = aLastGX; aX <= aGX; aX++)
				{
					float aDist = (double)(aX - aLastGX) / (double)(aGX - aLastGX);
					float aVal = aDist * aFoundPoint.mY + (1.0 - aDist) * aLastY;
					mQuantTable[aX] = aVal;
				}
			}
			else
			{
				float aVal = aFoundPoint.mY;
				mQuantTable[aGX] = aVal;
			}
			aLastGX = aGX;
			aLastX = aFoundPoint.mX;
			aLastY = aFoundPoint.mX;
		}
		first = false;
		aCurT += aTIncr;
	}
	for (int i = 0; i < mValuePointVector.size(); i++)
		mQuantTable[(mValuePointVector[i].mTime - aMinTime) / (aMaxTime - aMinTime) * ((PI_QUANT_SIZE - 1) + 0.5)] = mValuePointVector[i].mValue; //TIME_TO_X according to XNA?
}

float PIValue::GetValueAt(float theTime, float theDefault) //299-391
{
	//Something in H5 is here
	if (theTime == mLastTime)
		return mLastValue;
	float aPrevTime = mLastTime;
	mLastTime = theTime;
	if (mValuePointVector.size() == 1)
		return mLastValue = mValuePointVector[0].mValue;
	if (mBezier.IsInitialized())
	{
		float aMinTime = mValuePointVector.front().mTime;
		float aMaxTime = mValuePointVector.back().mTime;
		if (aMaxTime <= 1.001)
		{
			if (mQuantTable.size() == NULL)
				QuantizeCurve();
			float aQPos = (theTime - aMinTime) / (aMaxTime - aMinTime) * (PI_QUANT_SIZE - 1) + 0.5; //TIME_TO_X
			if (aQPos <= 0.0)
				return mLastValue = mValuePointVector.front().mValue;
			if (aQPos >= PI_QUANT_SIZE - 1)
				return mLastValue = mValuePointVector.back().mValue;
			int aLeft = (int)aQPos;
			float aFrac = aQPos - (float)aLeft;
			mLastValue = mQuantTable[aLeft] * (1.0 - aFrac) + mQuantTable[aLeft + 1] * aFrac;
			return mLastValue;
		}
		float aMaxError = min(0.1, (aMaxTime - aMinTime) / 1000.0);
		if (theTime <= aMinTime)
			return mLastValue = mValuePointVector.front().mValue;
		if (theTime >= aMaxTime)
			return mLastValue = mValuePointVector.back().mValue;
		float aL = aMinTime;
		float aR = aMaxTime;
		FPoint aPt;
		float aTryT = 0;
		bool isBigChange = ((theTime - aPrevTime) / (aMaxTime - aMinTime)) > 0.05;
		float anErrorFactor[4] = { 0.1, 0.1, 0.1, 0.5 };
		float aFactors[3] = { 1.0, 0.75, 1.25 };
		for (int aTryCount = 0; aTryCount < 1000; aTryCount++)
		{
			float aWantError = aMaxError;
			if (aTryCount < 4 && !isBigChange)
				aWantError *= anErrorFactor[aTryCount];
			if (aTryCount < 3 && mLastCurveTDelta != 0 && !isBigChange)
				aTryT = mLastCurveT + mLastCurveTDelta * aFactors[aTryCount];
			else
				aTryT = aL + (aR - aL) / 2.0;
			if (aTryT >= aL && aTryT <= aR)
			{
				aPt = mBezier.Evaluate(aTryT);
				float aDiff = aPt.mX - theTime;
				if (fabs(aDiff) <= aWantError)
					break;
				if (aDiff < 0)
					aL = aTryT;
				else
					aR = aTryT;
			}
		}
		mLastCurveTDelta = mLastCurveTDelta * 0.5 + (aTryT - mLastCurveT) * 0.5;
		mLastCurveT = aTryT;
		return mLastValue = aPt.mY;
	}
	for (int i = 1; i < mValuePointVector.size(); i++)
	{
		PIValuePoint* aP1 = &mValuePointVector[i - 1];
		PIValuePoint* aP2 = &mValuePointVector[i];
		if (theTime >= aP1->mTime && theTime <= aP2->mTime || i == mValuePointVector.size() - 1)
			return mLastValue = aP1->mValue + (aP2->mValue - aP1->mValue) * min(1.0, (theTime - aP1->mTime) / (aP2->mTime - aP1->mTime));
	}
	return mLastValue = theDefault;
}

float PIValue::GetLastKeyframe(float theTime) //394-403
{
	for (int i = mValuePointVector.size() - 1; i >= 0; i--)
	{
		PIValuePoint* aPt = &mValuePointVector[i];
		if (theTime >= aPt->mTime)
			return aPt->mValue;
	}
	return 0;
}

float PIValue::GetLastKeyframeTime(float theTime) //406-415
{
	for (int i = mValuePointVector.size() - 1; i >= 0; i--)
	{
		PIValuePoint* aPt = &mValuePointVector[i];
		if (theTime >= aPt->mTime)
			return aPt->mTime;
	}
	return 0;
}

float PIValue::GetNextKeyframeTime(float theTime) //418-427
{
	for (int i = 0; i < mValuePointVector.size(); i++)
	{
		PIValuePoint* aPt = &mValuePointVector[i];
		if (aPt->mTime >= theTime)
			return aPt->mTime;
	}
	return 0;
}

int PIValue::GetNextKeyframeIdx(float theTime) //430-439
{
	for (int i = 0; i < mValuePointVector.size(); i++)
	{
		PIValuePoint* aPt = &mValuePointVector[i];
		if (theTime <= mValuePointVector[i].mTime)
		{
			if (aPt->mTime >= theTime)
				return i;
		}
	}
	return -1;
}

int PIInterpolator::GetValueAt(float theTime) //442-461
{
	if (mInterpolatorPointVector.size() == 1)
		return mInterpolatorPointVector[0].mValue;
	float aScaledTime = mInterpolatorPointVector.front().mTime + theTime * mInterpolatorPointVector.back().mTime - mInterpolatorPointVector.front().mTime; //?
	for (int i = 1; i < mInterpolatorPointVector.size(); i++)
	{
		PIInterpolatorPoint* aP1 = &mInterpolatorPointVector[i - 1];
		PIInterpolatorPoint* aP2 = &mInterpolatorPointVector[i];
		if ((aScaledTime >= aP1->mTime) && (aScaledTime <= aP2->mTime))
			return InterpColor(aP1->mValue, aP2->mValue, min(1.0, (aScaledTime - aP1->mTime) / (aP2->mTime - aP1->mTime))); //LerpColor in H5
		if (i == (mInterpolatorPointVector.size() - 1))
			return aP2->mValue;
	}
	return 0;
}

int PIInterpolator::GetKeyframeNum(int theIdx) //464-468 (Match)
{
	if (mInterpolatorPointVector.size() == 0)
		return 0;
	return mInterpolatorPointVector[theIdx % mInterpolatorPointVector.size()].mValue;
}

float PIInterpolator::GetKeyframeTime(int theIdx) //471-475 (Match)
{
	if (mInterpolatorPointVector.size() == 0)
		return 0;
	return mInterpolatorPointVector[theIdx % mInterpolatorPointVector.size()].mTime;
}

FPoint PIValue2D::GetValueAt(float theTime) //478-501
{
	if (mLastTime == theTime)
		return mLastPoint;
	mLastTime = theTime;
	if (mValuePoint2DVector.size() == 1)
		return mLastPoint = mValuePoint2DVector[0].mValue;
	if (mBezier.IsInitialized())
	{
		mLastPoint = mBezier.Evaluate(theTime);
		return mLastPoint;
	}
	for (int i = 1; i < mValuePoint2DVector.size(); i++)
	{
		PIValuePoint2D* aP1 = &mValuePoint2DVector[i - 1];
		PIValuePoint2D* aP2 = &mValuePoint2DVector[i];
		if ((theTime >= aP1->mTime && theTime <= aP2->mTime) || (i == (mValuePoint2DVector.size() - 1)))
		{
			mLastPoint = aP2->mValue, aP1->mValue, min(1.0, (theTime - aP1->mTime) / (aP2->mTime - aP1->mTime));
			return mLastPoint;
		}
	}
	return mLastPoint = FPoint(0, 0);
}

FPoint PIValue2D::GetVelocityAt(float theTime) //504-527
{
	if (mLastVelocityTime == theTime)
		return mLastVelocity;
	mLastVelocityTime = theTime;
	if (mValuePoint2DVector.size() == 1)
		return FPoint(0, 0);
	if (mBezier.IsInitialized())
		mLastVelocity = mBezier.Velocity(theTime, false);
	for (int i = 1; i < mValuePoint2DVector.size(); i++)
	{
		PIValuePoint2D* aP1 = &mValuePoint2DVector[i - 1];
		PIValuePoint2D* aP2 = &mValuePoint2DVector[i];
		if ((theTime >= aP1->mTime && theTime <= aP2->mTime) || (i == (mValuePoint2DVector.size() - 1)))
			return mLastVelocity = aP2->mValue - aP1->mValue;
	}
	return mLastVelocity = FPoint(0, 0);
}

PIEffect::PIEffect() //532-564
{

	AutoCrit aCrit(gSexyAppBase->mCritSect);
	gSexyAppBase->mPIEffectSet.insert(this); //C++ only
	mLoaded = false;

	mFileIdx = 0;
	mReadFP = NULL; //C++ only
	mWriteFP = NULL; //C++ only
	mAutoPadImages = true;

	mFrameNum = 0;
	mUpdateCnt = 0;

	mCurNumParticles = 0;
	mCurNumEmitters = 0;
	mLastDrawnPixelCount = 0;

	mFirstFrameNum = 0;
	mLastFrameNum = 0;
	mWantsSRand = true; //C++ only

	mAnimSpeed = 1.0;
	mColor = Color::White;
	mDebug = false;
	mDrawBlockers = true;
	mEmitAfterTimeline = false;
	mDrawTransform.LoadIdentity();
	mEmitterTransform.LoadIdentity();

	mDef = new PIEffectDef();
}

PIEffect::PIEffect(const PIEffect& rhs) //590-638
{
	mFileChecksum = rhs.mFileChecksum;
	mVersion = rhs.mVersion;
	mSrcFileName = rhs.mSrcFileName;
	mStartupState = rhs.mStartupState;
	mNotes = rhs.mNotes;
	mWidth = rhs.mWidth;
	mHeight = rhs.mHeight;
	mBkgColor = rhs.mBkgColor;
	mFramerate = rhs.mFramerate;
	mFirstFrameNum = rhs.mFirstFrameNum;
	mLastFrameNum = rhs.mLastFrameNum;
	mNotesParams = rhs.mNotesParams;
	mError = rhs.mError;
	mLoaded = rhs.mLoaded;
	mAnimSpeed = rhs.mAnimSpeed;
	mColor = rhs.mColor;
	mDebug = rhs.mDebug;
	mDrawBlockers = rhs.mDrawBlockers;
	mEmitAfterTimeline = rhs.mEmitAfterTimeline;
	mRandSeeds = rhs.mRandSeeds;
	mWantsSRand = rhs.mWantsSRand;
	mDrawTransform = rhs.mDrawTransform; //?
	mEmitterTransform = rhs.mEmitterTransform; //?
	AutoCrit aCrit(gSexyAppBase->mCritSect);
	gSexyAppBase->mPIEffectSet.insert(this);
	mFileIdx = 0;
	mReadFP = NULL;
	mWriteFP = NULL;
	mFrameNum = 0.0;
	mUpdateCnt = 0;
	mIsNewFrame = false;
	mHasEmitterTransform = false;
	mHasDrawTransform = false;
	mDrawTransformSimple = false;
	mCurNumParticles = 0;
	mCurNumEmitters = 0;
	mLastDrawnPixelCount = 0;
	mDef = rhs.mDef;
	++mDef->mRefCount;
	mLayerVector.resize(mDef->mLayerDefVector.size());
	for (int aLayerIdx = 0; aLayerIdx < mLayerVector.size(); aLayerIdx++)
	{
		PILayerDef* aLayerDef = &mDef->mLayerDefVector[aLayerIdx];
		PILayer* aLayer = &mLayerVector[aLayerIdx];
		aLayer->mLayerDef = aLayerDef;
		aLayer->mEmitterInstanceVector.resize(aLayerDef->mEmitterInstanceDefVector.size());
		for (int anEmitterIdx = 0; anEmitterIdx < aLayerDef->mEmitterInstanceDefVector.size(); anEmitterIdx++)
		{
			const PIEmitterInstance* aRHSEmitterInstance = &rhs.mLayerVector[aLayerIdx].mEmitterInstanceVector[anEmitterIdx];
			PIEmitterInstanceDef* anEmitterInstanceDef = &aLayerDef->mEmitterInstanceDefVector[anEmitterIdx];
			PIEmitterInstance* anEmitterInstance = &aLayer->mEmitterInstanceVector[anEmitterIdx];
			PIEmitter* anEmitter = mDef->mEmitterVector[anEmitterInstanceDef->mEmitterDefIdx];
			anEmitterInstance->mEmitterInstanceDef = anEmitterInstanceDef;
			anEmitterInstance->mTintColor = aRHSEmitterInstance->mTintColor;
			anEmitterInstance->mParticleDefInstanceVector.resize(anEmitter->mParticleDefVector.size());
			anEmitterInstance->mSuperEmitterParticleDefInstanceVector.resize(aRHSEmitterInstance->mSuperEmitterParticleDefInstanceVector.size());
		}
	}
	ResetAnim();
}

PIEffect::~PIEffect() //641-649
{
	AutoCrit aCrit (gSexyAppBase->mCritSect);
	gSexyAppBase->mPIEffectSet.erase(gSexyAppBase->mPIEffectSet.find(this)); //?
	ResetAnim();
	Deref();
}

PIEffect* PIEffect::Duplicate() //652-654
{
	return new PIEffect();
}

SharedImageRef PIEffect::GetImage(const std::string& theName, const std::string& theFilename) //657-660
{
	return gSexyAppBase->GetSharedImage(GetPathFrom(theFilename, GetFileDir(mSrcFileName, true)));
}

void PIEffect::SetImageOpts(DeviceImage* theImage) //Not in H5, empty in C++ | 663-668
{
}

std::string PIEffect::WriteImage(const std::string& theName, int theIdx, DeviceImage* theImage, bool* hasPadding) //Unimplemented in XNA, Correct? | 671-684
{
	int aDotPos = mDestFileName.find('.', 0);
	std::string aDestName = mDestFileName.substr(0, aDotPos); //?
	aDestName += StrFormat("_%d_", theIdx) + theName;
	ImageLib::Image* anImage;
	anImage->mBits = theImage->GetBits(); //?
	anImage->mWidth = theImage->GetWidth(); //?
	anImage->mHeight = theImage->GetHeight(); //?
	WritePNGImage(aDestName + ".png", anImage, PSEARCH_PAK_THEN_FILES);
	return GetFileName(aDestName, true); //?
}

bool PIEffect::Fail(const std::string& theError) //687-691 (Matched)
{
	if (mError.length() > 0)
		mError = theError;
	return false;
}

void PIEffect::Deref() //694-702
{
	--mDef->mRefCount;
	DBG_ASSERTE(mDef->mRefCount >= 0); //696 | 711 BejLiveWin8
	if (mDef->mRefCount <= 0)
	{
		if (mDef != NULL)
			delete mDef;
		mDef = NULL;
	}
}

float PIEffect::GetRandFloat() //705-707
{
	return (float)(mRand.Next() % 20000000 / 10000000 - 1); //?
}

float PIEffect::GetRandFloatU() //710-712
{
	return (float)(mRand.Next() % 10000000 / 10000000); //?
}

float PIEffect::GetRandSign() //715-720
{
	if (mRand.Next() % 2 == 0)
		return 1;
	else
		return -1;
}

float PIEffect::GetVariationScalar() //723-725
{
	return GetRandFloat() * GetRandFloat();
}

float PIEffect::GetVariationScalarU() //728-730
{
	return GetRandFloatU() * GetRandFloatU();
}

std::string PIEffect::ReadString() //781-792
{
	int aLen = (byte)GetData(this, &mBufTemp, 1); //?
	std::string aString;
	aString.resize(aLen);
	if (aLen > 0)
		aString = (char)GetData(this, &aString[0], aLen); //?
	return aString;
}

std::string PIEffect::ReadStringS() //795-817
{
	int aLen = EndianShort((short)GetData(this, &mBufTemp, 2));
	if (aLen == -1)
	{
		int aSomething = EndianShort((short)GetData(this, &mBufTemp, 2));
		aLen = EndianShort((short)GetData(this, &mBufTemp, 2));
	}
	else if ((aLen & 0x8000) != NULL)
	{
		std::string aString = mStringVector[aLen & 0x7FFF];
		mStringVector.push_back(aString);
		return aString;
	}
	std::string aString;
	aString.resize(aLen);
	if (aLen > 0)
		GetData(this, &aString[0], aLen);
	mStringVector.push_back(aString);
	mStringVector.push_back(aString); //No clue why this happens twice
	return aString;
}

bool PIEffect::ExpectCmd(const std::string& theCmdExpected) //820-827
{
	if (mIsPPF)
		return true;
	std::string aString = ReadStringS();
	if (aString != theCmdExpected)
		return Fail("Expected '" + theCmdExpected + "'");
	return true;
}

void PIEffect::ReadValue2D(PIValue2D* theValue2D) //830-882
{
	int aKeyCount = EndianShort((short)GetData(this, &mBufTemp, 2));
	std::vector<float> aTimes;
	std::vector<FPoint> aPoints;
	std::vector<FPoint> aControlPoints;
	bool hasCurve = false;
	if (mIsPPF && aKeyCount > 1)
	{
		if (mIsPPF)
			hasCurve = GetData(this, &mBufTemp, 1) != 0;
		else
			hasCurve = (DWORD)GetData(this, &mBufTemp, 4) != 0;
	}
	for (int aKeyIdx = 0; aKeyIdx < aKeyCount; aKeyIdx++)
	{
		ExpectCmd("CKey");
		float aTime = EndianInt((int)GetData(this, &mBufTemp, 4));
		aTimes.push_back(aTime);
		FPoint aPt;
		aPt.mX = EndianFloat((int)GetData(this, &mBufTemp, 4));
		aPt.mY = EndianFloat((int)GetData(this, &mBufTemp, 4));
		aPoints.push_back(aPt);
		if (!mIsPPF || hasCurve)
		{
			FPoint aControlPt1;
			aControlPt1.mX = EndianFloat((int)GetData(this, &mBufTemp, 4));
			aControlPt1.mY = EndianFloat((int)GetData(this, &mBufTemp, 4));
			if (aKeyIdx > 0)
				aControlPoints.push_back(aPt + aControlPt1);
			FPoint aControlPt2;
			aControlPt2.mX = EndianFloat((int)GetData(this, &mBufTemp, 4));
			aControlPt2.mY = EndianFloat((int)GetData(this, &mBufTemp, 4));
			aControlPoints.push_back(aPt + aControlPt2);
		}
		if (!mIsPPF)
		{
			int aFlags1 = (int)GetData(this, &mBufTemp, 4);
			int aFlags2 = (int)GetData(this, &mBufTemp, 4);
			hasCurve |= (aFlags2 & 1) == 0;
		}
		PIValuePoint2D aValuePoint2D;
		aValuePoint2D.mValue = aPt;
		aValuePoint2D.mTime = aTime;
		theValue2D->mValuePoint2DVector.push_back(aValuePoint2D);
	}
	if (aKeyCount > 1 && hasCurve)
		theValue2D->mBezier.Init(&aPoints[0], &aControlPoints[0], &aTimes[0], aKeyCount);
}

void PIEffect::ReadEPoint(PIValue2D* theValue2D) //885-897
{
	int aPointCount = EndianShort((short)GetData(this, &mBufTemp, 2));
	for (int i = 0; i < aPointCount; i++)
	{
		ExpectCmd("CPointKey");
		PIValuePoint2D aPoint;
		aPoint.mTime = (float)EndianInt((int)GetData(this, &mBufTemp, 4));
		aPoint.mValue.mX = (double)EndianFloat((int)GetData(this, &mBufTemp, 4));
		aPoint.mValue.mY = (double)EndianFloat((int)GetData(this, &mBufTemp, 4));
		theValue2D->mValuePoint2DVector.push_back(aPoint);
	}
}

void PIEffect::ReadValue(PIValue* theValue) //900-993
{
	std::vector<float> aTimes; //Set later in H5
	std::vector<FPoint> aPoints;
	std::vector<FPoint> aControlPoints;
	int aFlags = mIsPPF ? (char)GetData(this, &mBufTemp, 1) : 0; //What flags ae these
	int aDataCount = aFlags & 7;
	if (!mIsPPF || aDataCount == 7)
		aDataCount = EndianShort((short)GetData(this, &mBufTemp, 2));
	bool hasCurve = false;
	if (aDataCount > 1)
		hasCurve |= (aFlags & 8) != 0;
	theValue->mValuePointVector.resize(aDataCount);	
	for (int aDataIdx = 0; aDataIdx < aDataCount; aDataIdx++)
	{
		bool okay = true;
		std::string aCmd;
		if (!mIsPPF)
		{
			aCmd = ReadStringS();
			okay = (aCmd == "CDataKey") || (aCmd == "CDataOverLifeKey");
		}
		if (okay)
		{
			float aTime;
			if (((aFlags & 0x10) != 0 && aDataIdx == 0))
				aTime = 0.0;
			else if (aCmd == "CDataKey")
				aTime = (float)EndianInt((int)GetData(this, &mBufTemp, 4));
			else
				aTime = (float)EndianFloat((int)GetData(this, &mBufTemp, 4));
			aTimes.push_back(aTime);
			float aValue;
			if (aDataIdx != 0 || (aFlags & 0x60) == 0x0)
				aValue = (float)EndianFloat((int)GetData(this, &mBufTemp, 4));
			else if ((aFlags & 0x60) == 0x20)
				aValue = 0.0;
			else if ((aFlags & 0x60) == 0x40)
				aValue = 1.0;
			else
				aValue = 2.0;
			FPoint aPt;
			aPt.mX = aTime;
			aPt.mY = aValue;
			aPoints.push_back(aPt);
			if (mIsPPF || hasCurve)
			{
				FPoint aControlPt1;
				aControlPt1.mX = EndianFloat((int)GetData(this, &mBufTemp, 4));
				aControlPt1.mY = EndianFloat((int)GetData(this, &mBufTemp, 4));
				if (aDataIdx > 0)
					aControlPoints.push_back(aPt + aControlPt1);
				FPoint aControlPt2;
				aControlPt2.mX = EndianFloat((int)GetData(this, &mBufTemp, 4));
				aControlPt2.mY = EndianFloat((int)GetData(this, &mBufTemp, 4));
				aControlPoints.push_back(aPt + aControlPt2);
			}
			if (!mIsPPF)
			{
				int aFlags1 = EndianInt((int)GetData(this, &mBufTemp, 4));
				int aFlags2 = EndianInt((int)GetData(this, &mBufTemp, 4));
				hasCurve |= (aFlags2 & 1) == 0;
			}
			PIValuePoint &aValuePoint = theValue->mValuePointVector[aDataIdx];
			aValuePoint.mValue = aPt.mY;
			aValuePoint.mTime = aTime;
		}
		else
			Fail("CDataKey or CDataOverLifeKey expected");
	}
	if (!hasCurve && theValue->mValuePointVector.size() == 2 && theValue->mValuePointVector[0].mValue == theValue->mValuePointVector[1].mValue)
		theValue->mValuePointVector.pop_back();
	if (aDataCount > 1 && hasCurve)
		theValue->mBezier.Init(&aPoints[0], &aControlPoints[0], &aTimes[0], aDataCount);
}

void PIEffect::ReadEmitterType(PIEmitter* theEmitter) //996-1178
{
	int aMyst1 = EndianInt((int)GetData(this, &mBufTemp, 4));
	theEmitter->mName = ReadString(); //Prob correct
	theEmitter->mKeepInOrder = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : (DWORD)GetData(this, &mBufTemp, 4) != 0;
	int l = EndianInt((int)GetData(this, &mBufTemp, 4));
	theEmitter->mOldestInFront = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : (DWORD)GetData(this, &mBufTemp, 4) != 0;
	short aParticleCount = EndianShort((short)GetData(this, &mBufTemp, 2));
	for (int aParticleIdx = 0; aParticleIdx < aParticleCount; aParticleIdx++)
	{
		PIParticleDef aParticle;
		ExpectCmd("CEmParticleType");
		EndianInt((int)GetData(this, &mBufTemp, 4));
		EndianInt((int)GetData(this, &mBufTemp, 4));
		EndianInt((int)GetData(this, &mBufTemp, 4));
		EndianFloat((int)GetData(this, &mBufTemp, 4));
		EndianInt((int)GetData(this, &mBufTemp, 4));
		EndianInt((int)GetData(this, &mBufTemp, 4));
		EndianInt((int)GetData(this, &mBufTemp, 4));
		EndianInt((int)GetData(this, &mBufTemp, 4));
		EndianInt((int)GetData(this, &mBufTemp, 4));
		EndianInt((int)GetData(this, &mBufTemp, 4));
		EndianInt((int)GetData(this, &mBufTemp, 4));
		EndianInt((int)GetData(this, &mBufTemp, 4));
		EndianInt((int)GetData(this, &mBufTemp, 4));
		EndianInt((int)GetData(this, &mBufTemp, 4));
		EndianInt((int)GetData(this, &mBufTemp, 4));
		aParticle.mIntense = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : GetData(this, &mBufTemp, 4) != 0;
		aParticle.mSingleParticle = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : GetData(this, &mBufTemp, 4) != 0;
		aParticle.mPreserveColor = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : GetData(this, &mBufTemp, 4) != 0;
		//Check for mGlobalAllowPreserveColor in H5, not present in C++ and XNA
		aParticle.mAttachToEmitter = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : GetData(this, &mBufTemp, 4) != 0;
		aParticle.mAttachVal = EndianFloat((int)GetData(this, &mBufTemp, 4));
		aParticle.mFlipHorz = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : GetData(this, &mBufTemp, 4) != 0;
		aParticle.mFlipVert = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : GetData(this, &mBufTemp, 4) != 0;
		aParticle.mAnimStartOnRandomFrame = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : GetData(this, &mBufTemp, 4) != 0;
		aParticle.mRepeatColor = EndianInt((int)GetData(this, &mBufTemp, 4));
		aParticle.mRepeatAlpha = EndianInt((int)GetData(this, &mBufTemp, 4));
		aParticle.mRepeatAlpha = EndianInt((int)GetData(this, &mBufTemp, 4));
		aParticle.mLinkTransparencyToColor = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : GetData(this, &mBufTemp, 4) != 0;
		aParticle.mName = ReadString();
		aParticle.mAngleAlignToMotion = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : GetData(this, &mBufTemp, 4) != 0;
		aParticle.mAngleRandomAlign = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : GetData(this, &mBufTemp, 4) != 0;
		aParticle.mAngleRandomAlign = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : GetData(this, &mBufTemp, 4) != 0;
		aParticle.mAngleKeepAlignedToMotion = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : GetData(this, &mBufTemp, 4) != 0;
		aParticle.mAngleValue = EndianInt((int)GetData(this, &mBufTemp, 4));
		aParticle.mAngleAlignOffset = EndianInt((int)GetData(this, &mBufTemp, 4));
		aParticle.mAnimSpeed = EndianInt((int)GetData(this, &mBufTemp, 4));
		aParticle.mRandomGradientColor = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : GetData(this, &mBufTemp, 4) != 0;
		l = EndianInt((int)GetData(this, &mBufTemp, 4));
		aParticle.mTextureIdx = EndianInt((int)GetData(this, &mBufTemp, 4));
		int aColorPointCount = EndianShort((short)GetData(this, &mBufTemp, 2));
		for (int anIdx = 0; anIdx < aColorPointCount; anIdx++)
		{
			ExpectCmd("CColorPoint");
			uchar r = (byte)GetData(this, &mBufTemp, 1);
			uchar g = (byte)GetData(this, &mBufTemp, 1);
			uchar b = (byte)GetData(this, &mBufTemp, 1);
			ulong aColor = 0xFF000000 | (r << 16) | (g << 8) | (b << 0);
			float aPct = EndianFloat((int)GetData(this, &mBufTemp, 4));
			PIInterpolatorPoint aPoint;
			aPoint.mValue = aColor;
			aPoint.mTime = aPct;
			aParticle.mColor.mInterpolatorPointVector.push_back(aPoint);
		}
		int anAlphaPointCount = EndianShort((short)GetData(this, &mBufTemp, 2));
		for (int anIdx = 0; anIdx < anAlphaPointCount; anIdx++)
		{
			ExpectCmd("CAlphaPoint");
			uchar a = (uchar)GetData(this, &mBufTemp, 1);
			float aPct = EndianFloat((int)GetData(this, &mBufTemp, 4));
			PIInterpolatorPoint aPoint;
			aPoint.mValue = a;
			aPoint.mTime = aPct;
			aParticle.mAlpha.mInterpolatorPointVector.push_back(aPoint);
		}
		for (int aValIdx = 0; aValIdx < PIParticleDef::NUM_VALUES; ++aValIdx)
			ReadValue(&aParticle.mValues[aValIdx]);
		aParticle.mRefPointOfs.mX = EndianFloat((int)GetData(this, &mBufTemp, 4));
		aParticle.mRefPointOfs.mY = EndianFloat((int)GetData(this, &mBufTemp, 4));
		if (!mIsPPF)
		{
			Image* anImage = mDef->mTextureVector[aParticle.mTextureIdx]->mImageVector[0]; //?
			aParticle.mRefPointOfs.mX /= anImage->mWidth;
			aParticle.mRefPointOfs.mY /= anImage->mHeight;
		}
		l = EndianInt((int)GetData(this, &mBufTemp, 4));
		l = EndianInt((int)GetData(this, &mBufTemp, 4));
		aParticle.mLockAspect = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : (DWORD)GetData(this, &mBufTemp, 4) != 0;
		ReadValue(&aParticle.mValues[PIParticleDef::VALUE_SIZE_Y]);
		ReadValue(&aParticle.mValues[PIParticleDef::VALUE_SIZE_Y_VARIATION]);
		ReadValue(&aParticle.mValues[PIParticleDef::VALUE_SIZE_Y_OVER_LIFE]);
		aParticle.mAngleRange = EndianInt((int)GetData(this, &mBufTemp, 4));
		aParticle.mAngleOffset = EndianInt((int)GetData(this, &mBufTemp, 4));
		aParticle.mGetColorFromLayer = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : (DWORD)GetData(this, &mBufTemp, 4) != 0;
		aParticle.mUpdateColorFromLayer = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : (DWORD)GetData(this, &mBufTemp, 4) != 0;
		aParticle.mUseEmitterAngleAndRange = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : (DWORD)GetData(this, &mBufTemp, 4) != 0;
		ReadValue(&aParticle.mValues[PIParticleDef::VALUE_EMISSION_ANGLE]);
		ReadValue(&aParticle.mValues[PIParticleDef::VALUE_EMISSION_RANGE]);
		l = EndianInt((int)GetData(this, &mBufTemp, 4));
		PIValue aValue;
		ReadValue(&aValue);
		aParticle.mUseKeyColorsOnly = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : (DWORD)GetData(this, &mBufTemp, 4) != 0;
		aParticle.mUpdateTransparencyFromLayer = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : (DWORD)GetData(this, &mBufTemp, 4) != 0;
		aParticle.mUseNextColorKey = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : (DWORD)GetData(this, &mBufTemp, 4) != 0;
		aParticle.mNumberOfEachColor = EndianInt((int)GetData(this, &mBufTemp, 4));
		aParticle.mGetTransparencyFromLayer = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : (DWORD)GetData(this, &mBufTemp, 4) != 0;
		if (theEmitter->mOldestInFront)
			theEmitter->mParticleDefVector.insert(theEmitter->mParticleDefVector.begin(), aParticle); //?
		else
			theEmitter->mParticleDefVector.push_back(aParticle);
		int aMyst2 = EndianInt((int)GetData(this, &mBufTemp, 4));
		for (int aValIdx = 0; aValIdx < 42; aValIdx++)
			ReadValue(&theEmitter->mValues[aValIdx]);
		theEmitter->mIsSuperEmitter = theEmitter->mValues[0].mValuePointVector.size() != 0;
		int aMyst3 = EndianInt((int)GetData(this, &mBufTemp, 4));
		int aMyst4 = EndianInt((int)GetData(this, &mBufTemp, 4));
	}
}

void PIEffect::WriteByte(char theByte) //Not in H5, unimplemented in XNA | 1181-1183
{
	fwrite(&theByte, 1, 1, mWriteFP);
}

void PIEffect::WriteInt(int theInt) //Not in H5, unimplemented in XNA |1186-1188
{
	fwrite(&theInt, 4, 1, mWriteFP);
}

void PIEffect::WriteShort(short theShort) //Not in H5, unimplemented in XNA |1191-1193
{
	fwrite(&theShort, 2, 1, mWriteFP);
}

void PIEffect::WriteFloat(float theFloat) //Not in H5, unimplemented in XNA |1196-1198
{
	fwrite(&theFloat, 4, 1, mWriteFP);
}

void PIEffect::WriteBool(bool theValue) //Not in H5, unimplemented in XNA | 1201-1203
{
	WriteByte(theValue);
}

void PIEffect::WriteString(const std::string& theString) //Not in H5, unimplemented in XNA | 1206-1210
{
	WriteByte(theString.length());
	if (theString.length())
		fwrite(theString.c_str(), 1, theString.length(), mWriteFP);
}

void PIEffect::WriteValue2D(PIValue2D* theValue2D) //Not in H5, unimplemented in XNA | 1213-1256
{
	int aKeyCount = theValue2D->mValuePoint2DVector.size();
	bool hasCurve = aKeyCount > 1 && theValue2D->mBezier.mControls;
	WriteShort(aKeyCount);
	if (aKeyCount > 1)
		WriteBool(hasCurve);
	int aCtlPoint = 0;
	for (int aKeyIdx = 0; aKeyIdx < aKeyCount; aKeyIdx++)
	{
		PIValuePoint2D* aValuePoint = &theValue2D->mValuePoint2DVector[aKeyIdx];
		WriteInt((int)aValuePoint->mTime);
		WriteFloat(aValuePoint->mValue.mX);
		WriteFloat(aValuePoint->mValue.mY);
		if (hasCurve)
		{
			if (aKeyIdx)
			{
				WriteFloat(theValue2D->mBezier.mControls[aCtlPoint].mX - aValuePoint->mValue.mX);
				WriteFloat(theValue2D->mBezier.mControls[aCtlPoint].mY - aValuePoint->mValue.mY);
				aCtlPoint++;
			}
			else
			{
				WriteFloat(0.0);
				WriteFloat(0.0);
			}
			if (aKeyIdx == aKeyCount - 1)
			{
				WriteFloat(0.0);
				WriteFloat(0.0);
			}
			else
			{
				WriteFloat(theValue2D->mBezier.mControls[aCtlPoint].mX - aValuePoint->mValue.mX);
				WriteFloat(theValue2D->mBezier.mControls[aCtlPoint].mY - aValuePoint->mValue.mY);
			}
		}
		aCtlPoint++;
	}
}

void PIEffect::WriteEPoint(PIValue2D* theValue2D) //C++ only | 1259-1268
{
	WriteShort(theValue2D->mValuePoint2DVector.size());
	for (int aKeyIdx = 0; aKeyIdx < theValue2D->mValuePoint2DVector.size(); aKeyIdx++)
	{
		PIValuePoint2D* aValuePoint = &theValue2D->mValuePoint2DVector[aKeyIdx];
		WriteInt((int)aValuePoint->mTime);
		WriteFloat(aValuePoint->mValue.mX);
		WriteFloat(aValuePoint->mValue.mY);
	}
}

void PIEffect::WriteValue(PIValue* theValue) //Not in H5, unimplemented in XNA | 1271-1334
{
	int aKeyCount = theValue->mValuePointVector.size();
	bool hasCurve = aKeyCount > 1 && theValue->mBezier.mControls;
	uchar aFlags = NULL;
	aFlags = min(aKeyCount, 7);
	if (hasCurve)
		aFlags |= 8;
	if (theValue->mValuePointVector.size())
	{
		if (theValue->mValuePointVector[0].mTime == 0.0)
			aFlags |= 0x10;
		if (theValue->mValuePointVector[0].mValue == 0.0)
			aFlags |= 0x20;
		if (theValue->mValuePointVector[0].mValue == 1.0)
			aFlags |= 0x40;
		if (theValue->mValuePointVector[0].mValue == 2.0)
			aFlags |= 0x60;
	}
	WriteByte(aFlags);
	if (aKeyCount >= 7)
		WriteShort(aKeyCount);
	int aCtlPoint = 0;
	for (int aKeyIdx = 0; aKeyIdx <= theValue->mValuePointVector.size(); aKeyIdx++)
	{
		PIValuePoint* aValuePoint = &theValue->mValuePointVector[aKeyIdx];
		if (aKeyIdx || (aFlags & 0x10) == 0)
			WriteFloat(aValuePoint->mTime);
		if (aKeyIdx || (aFlags & 0x60) == 0)
			WriteFloat(aValuePoint->mValue);
		if (hasCurve)
		{
			FPoint aPt(aValuePoint->mTime, aValuePoint->mValue);
			if (aKeyIdx)
			{
				WriteFloat(theValue->mBezier.mControls[aCtlPoint].mX - aPt.mX);
				WriteFloat(theValue->mBezier.mControls[aCtlPoint].mY - aPt.mY);
				aCtlPoint++; //?
			}
			else
			{
				WriteFloat(0.0);
				WriteFloat(0.0);
			}
			if (aKeyIdx == theValue->mValuePointVector.size() - 1)
			{
				WriteFloat(0.0);
				WriteFloat(0.0);
			}
			else
			{
				WriteFloat(theValue->mBezier.mControls[aCtlPoint].mX - aPt.mX);
				WriteFloat(theValue->mBezier.mControls[aCtlPoint].mY - aPt.mY);
			}
			aCtlPoint++;
		}
	}
}

void PIEffect::WriteEmitterType(PIEmitter* theEmitter) //Not in H5, unimplemented in XNA | 1337-1458
{
	PIParticleDef* aParticle;
	WriteInt(0);
	WriteString(theEmitter->mName);
	WriteBool(theEmitter->mKeepInOrder);
	WriteInt(0);
	WriteBool(theEmitter->mOldestInFront);
	WriteShort(theEmitter->mParticleDefVector.size());
	for (int anIdx = 0; anIdx < theEmitter->mParticleDefVector.size(); anIdx++)
		*aParticle = theEmitter->mOldestInFront ? theEmitter->mParticleDefVector[theEmitter->mParticleDefVector.size() - anIdx - 1] : theEmitter->mParticleDefVector[anIdx];
	WriteInt(0);
	WriteInt(0);
	WriteInt(0);
	WriteFloat(0.0);
	WriteInt(0);
	WriteInt(0);
	WriteInt(0);
	WriteInt(1);
	WriteInt(0);
	WriteInt(0);
	WriteInt(0);
	WriteInt(2);
	WriteInt(0);
	WriteInt(0);
	WriteInt(0);
	WriteInt(3);
	WriteBool(aParticle->mIntense);
	WriteBool(aParticle->mSingleParticle);
	WriteBool(aParticle->mPreserveColor);
	WriteBool(aParticle->mAttachToEmitter);
	WriteFloat(aParticle->mAttachVal);
	WriteBool(aParticle->mFlipHorz);
	WriteBool(aParticle->mFlipVert);
	WriteBool(aParticle->mAnimStartOnRandomFrame);
	WriteInt(aParticle->mRepeatColor);
	WriteInt(aParticle->mRepeatAlpha);
	WriteBool(aParticle->mLinkTransparencyToColor);
	WriteString(aParticle->mName);
	WriteBool(aParticle->mAngleAlignToMotion);
	WriteBool(aParticle->mAngleRandomAlign);
	WriteBool(aParticle->mAngleKeepAlignedToMotion);
	WriteInt(aParticle->mAngleValue);
	WriteInt(aParticle->mAngleAlignOffset);
	WriteInt(aParticle->mAnimSpeed);
	WriteBool(aParticle->mRandomGradientColor);
	WriteInt(0);
	WriteInt(aParticle->mTextureIdx);
	WriteShort(aParticle->mColor.mInterpolatorPointVector.size());
	for (int aColorIdx = 0; aColorIdx < aParticle->mColor.mInterpolatorPointVector.size(); aColorIdx++)
	{
		PIInterpolatorPoint* aPoint = &aParticle->mColor.mInterpolatorPointVector[aColorIdx];
		WriteByte(aPoint->mValue >> 16);
		WriteByte(aPoint->mValue >> 8);
		WriteByte(aPoint->mValue);
		WriteFloat(aPoint->mTime);
	}
	WriteShort(aParticle->mAlpha.mInterpolatorPointVector.size());
	for (int aAlphaIdx = 0; aAlphaIdx < aParticle->mAlpha.mInterpolatorPointVector.size(); ++aAlphaIdx)
	{
		WriteByte(aParticle->mAlpha.mInterpolatorPointVector[aAlphaIdx].mValue);
		WriteFloat(aParticle->mAlpha.mInterpolatorPointVector[aAlphaIdx].mTime);
	}
	for (int aValIdx = 0; aValIdx < 23; ++aValIdx)
		WriteValue(&aParticle->mValues[aValIdx]);
	WriteFloat(aParticle->mRefPointOfs.mX);
	WriteFloat(aParticle->mRefPointOfs.mY);
	WriteInt(0);
	WriteInt(0);
	WriteBool(aParticle->mLockAspect);
	WriteValue(&aParticle->mValues[25]);
	WriteValue(&aParticle->mValues[26]);
	WriteValue(&aParticle->mValues[27]);
	WriteInt(aParticle->mAngleRange);
	WriteInt(aParticle->mAngleOffset);
	WriteBool(aParticle->mGetColorFromLayer);
	WriteBool(aParticle->mUpdateColorFromLayer);
	WriteBool(aParticle->mUseEmitterAngleAndRange);
	WriteValue(&aParticle->mValues[23]);
	WriteValue(&aParticle->mValues[24]);
	WriteInt(0);
	WriteValue(&aParticle->mValues[24]);
	WriteBool(aParticle->mUseKeyColorsOnly);
	WriteBool(aParticle->mUpdateTransparencyFromLayer);
	WriteBool(aParticle->mUseNextColorKey);
	WriteInt(aParticle->mNumberOfEachColor);
	WriteBool(aParticle->mGetTransparencyFromLayer);
	WriteInt(999);
	for (int i = 0; i < 42; ++i)
		WriteValue(&theEmitter->mValues[i]);
	WriteInt(0);
	WriteInt(0);
}

void PIEffect::SaveParticleDefInstance(Buffer& theBuffer, PIParticleDefInstance* theParticleDefInstance) //Not in H5 | 1461-1466
{
	theBuffer.WriteBytes((const uchar*)theParticleDefInstance, 4); //?
	theBuffer.WriteBytes((const uchar*)&theParticleDefInstance->mCurNumberVariation, 4); //?
	theBuffer.WriteLong(theParticleDefInstance->mParticlesEmitted);
	theBuffer.WriteLong(theParticleDefInstance->mTicks);
}

void PIEffect::SaveParticle(Buffer& theBuffer, PILayer* theLayer, PIParticleInstance* theParticle) //Not in H5 | 1469-1506
{
	theBuffer.WriteBytes((const uchar*)&theParticle->mTicks, 4);
	theBuffer.WriteBytes((const uchar*)&theParticle->mLife, 4);
	theBuffer.WriteBytes((const uchar*)&theParticle->mLifePct, 4);
	theBuffer.WriteBytes((const uchar*)&theParticle->mZoom, 4);
	theBuffer.WriteBytes((const uchar*)&theParticle->mPos, 16);
	theBuffer.WriteBytes((const uchar*)&theParticle->mVel, 16);
	theBuffer.WriteBytes((const uchar*)&theParticle->mEmittedPos, 16);
	if (theParticle->mParticleDef && theParticle->mParticleDef->mAttachToEmitter)
	{
		theBuffer.WriteBytes((const uchar*)&theParticle->mOrigPos, 16);
		theBuffer.WriteBytes((const uchar*)&theParticle->mOrigEmitterAng, 4);
	}
	theBuffer.WriteBytes((const uchar*)&theParticle->mImgAngle, 4);
	int aVariationFlags = 0;
	for (int aVar = 0; aVar < 9; ++aVar)
	{
		if (fabs(theParticle->mVariationValues[aVar]) >= 0.000009999999747378752) //?
			aVariationFlags |= PIParticleInstance::VARIATION_SIZE_X << aVar;
	}
	theBuffer.WriteShort(aVariationFlags);
	for (int i = 0; i < PIParticleInstance::NUM_VARIATIONS; ++i)
	{
		if ((aVariationFlags & (PIParticleInstance::VARIATION_SIZE_X << i)) != 0)
			theBuffer.WriteBytes((const uchar*)&theParticle->mVariationValues[i], 4);
	}
	theBuffer.WriteBytes((const uchar*)&theParticle->mSrcSizeXMult, 4);
	theBuffer.WriteBytes((const uchar*)&theParticle->mSrcSizeYMult, 4);
	if (theParticle->mParticleDef && theParticle->mParticleDef->mRandomGradientColor)
		theBuffer.WriteBytes((const uchar*)&theParticle->mGradientRand, 4);
	if (theParticle->mParticleDef && theParticle->mParticleDef->mAnimStartOnRandomFrame)
		theBuffer.WriteShort(theParticle->mAnimFrameRand);
	if (theLayer->mLayerDef->mDeflectorVector.size())
		theBuffer.WriteBytes((const uchar*)&theParticle->mThicknessHitVariation, 4);
}

void PIEffect::LoadParticleDefInstance(const Buffer& theBuffer, PIParticleDefInstance* theParticleDefInstance) //1509-1516
{
	theBuffer.ReadBytes((uchar*)theParticleDefInstance, 4);
	theParticleDefInstance->mNumberAcc = EndianFloat(theParticleDefInstance->mNumberAcc);
	theBuffer.ReadBytes((uchar*)&theParticleDefInstance->mCurNumberVariation, 4);
	theParticleDefInstance->mCurNumberVariation = EndianFloat(theParticleDefInstance->mCurNumberVariation);
	theParticleDefInstance->mParticlesEmitted = theBuffer.ReadLong();
	theParticleDefInstance->mTicks = theBuffer.ReadLong();
}

void PIEffect::LoadParticle(const Buffer& theBuffer, PILayer* theLayer, PIParticleInstance* theParticle) //1519-1600
{
	theBuffer.ReadBytes((uchar*)&theParticle->mTicks, 4);
	theParticle->mTicks = EndianFloat(theParticle->mTicks);
	theBuffer.ReadBytes((uchar*)&theParticle->mLife, 4);
	theParticle->mLife = EndianFloat(theParticle->mLife);
	theBuffer.ReadBytes((uchar*)&theParticle->mLifePct, 4);
	theParticle->mLifePct = EndianFloat(theParticle->mLifePct);
	theBuffer.ReadBytes((uchar*)&theParticle->mZoom, 4);
	theParticle->mZoom = EndianFloat(theParticle->mZoom);
	theBuffer.ReadBytes((uchar*)&theParticle->mPos, 16);
	theParticle->mPos.mX = EndianFloat(theParticle->mPos.mX);
	theParticle->mPos.mY = EndianFloat(theParticle->mPos.mY);
	theBuffer.ReadBytes((uchar*)&theParticle->mVel, 16);
	theParticle->mVel.mX = EndianFloat(theParticle->mVel.mX);
	theParticle->mVel.mY = EndianFloat(theParticle->mVel.mY);
	theBuffer.ReadBytes((uchar*)&theParticle->mEmittedPos, 16);
	theParticle->mEmittedPos.mX = EndianFloat(theParticle->mEmittedPos.mX);
	theParticle->mEmittedPos.mY = EndianFloat(theParticle->mEmittedPos.mY);
	if (theParticle->mParticleDef && theParticle->mParticleDef->mAttachToEmitter)
	{
		theBuffer.ReadBytes((uchar*)&theParticle->mOrigPos, 16);
		theParticle->mOrigPos.mX = EndianFloat(theParticle->mOrigPos.mX);
		theParticle->mOrigPos.mY = EndianFloat(theParticle->mOrigPos.mY);
		theBuffer.ReadBytes((uchar*)&theParticle->mOrigEmitterAng, 4);
		theParticle->mOrigEmitterAng = EndianFloat(theParticle->mOrigEmitterAng);
	}
	theBuffer.ReadBytes((uchar*)&theParticle->mImgAngle, 4);
	theParticle->mImgAngle = EndianFloat(theParticle->mImgAngle);
	int aVariationFlags = theBuffer.ReadShort();
	for (int aVar = 0; aVar < PIParticleInstance::NUM_VARIATIONS; ++aVar)
	{
		if ((aVariationFlags & (1 << aVar)) != 0)
		{
			theBuffer.ReadBytes((uchar*)&theParticle->mVariationValues[aVar], 4);
			theParticle->mVariationValues[aVar] = EndianFloat(theParticle->mVariationValues[aVar]);
		}
		else
			theParticle->mVariationValues[aVar] = 0.0;
	}
	theBuffer.ReadBytes((uchar*)&theParticle->mSrcSizeXMult, 4);
	theParticle->mSrcSizeXMult = EndianFloat(theParticle->mSrcSizeXMult);
	theBuffer.ReadBytes((uchar*)&theParticle->mSrcSizeYMult, 4);
	theParticle->mSrcSizeYMult = EndianFloat(theParticle->mSrcSizeYMult);
	if (theParticle->mParticleDef && theParticle->mParticleDef->mRandomGradientColor)
	{
		theBuffer.ReadBytes((uchar*)&theParticle->mGradientRand, 4);
		theParticle->mGradientRand = EndianFloat(theParticle->mGradientRand);
	}
	if (theParticle->mParticleDef && theParticle->mParticleDef->mAnimStartOnRandomFrame)
		theParticle->mAnimFrameRand = theBuffer.ReadShort();
	if (theLayer->mLayerDef->mDeflectorVector.size())
	{
		theBuffer.ReadBytes((uchar*)&theParticle->mThicknessHitVariation, 4);
		theParticle->mThicknessHitVariation = EndianFloat(theParticle->mThicknessHitVariation);
	}
	if (theParticle->mParticleDef && theParticle->mParticleDef->mAnimStartOnRandomFrame)
		theParticle->mAnimFrameRand = mRand.Next() & 0x7FFF;
	else
		theParticle->mAnimFrameRand = 0;
}

FPoint PIEffect::GetGeomPos(PIEmitterInstance* theEmitterInstance, PIParticleInstance* theParticleInstance, float* theTravelAngle, bool* isMaskedOut) //Correct? | 1603-1792
{
	FPoint aPos;
	PIEmitterInstanceDef* anEmitterInstanceDef = theEmitterInstance->mEmitterInstanceDef;
	switch (anEmitterInstanceDef->mEmitterGeom)
	{
	case PIEmitterInstanceDef::GEOM_LINE:
	{
		if (anEmitterInstanceDef->mPoints.size() >= 2)
		{
			int aStartIdx = 0;
			float aPct = 0;
			FPoint aPt1;
			FPoint aPt2;
			FPoint aPtDiff;
			float aLenSq;
			int aTotalLengthSq = 0;
			for (int i = 0; i < anEmitterInstanceDef->mPoints.size() - 1; i++)
			{
				aPt1 = anEmitterInstanceDef->mPoints[i].GetValueAt(mFrameNum);
				aPt2 = anEmitterInstanceDef->mPoints[i + 1].GetValueAt(mFrameNum);
				aPtDiff = aPt2 - aPt1;
				aLenSq = aPtDiff.mX * aPtDiff.mX + aPtDiff.mY * aPtDiff.mY;
				aTotalLengthSq += (int)aLenSq;
			}
			float aWantLenSq;
			if (anEmitterInstanceDef->mEmitAtPointsNum)
			{
				int aPointIdx = theParticleInstance->mNum % anEmitterInstanceDef->mEmitAtPointsNum;
				aWantLenSq = (float)(aPointIdx * aTotalLengthSq) / (float)(anEmitterInstanceDef->mEmitAtPointsNum - 1);
			}
			else
				aWantLenSq = GetRandFloatU() * (float)aTotalLengthSq;
			aTotalLengthSq = 0;
			for (int i = 0; i < anEmitterInstanceDef->mPoints.size() - 1; i++)
			{
				aPt1 = anEmitterInstanceDef->mPoints[i].GetValueAt(mFrameNum);
				aPt2 = anEmitterInstanceDef->mPoints[i + 1].GetValueAt(mFrameNum);
				aPtDiff = aPt2 - aPt1;
				aLenSq = aPtDiff.mX * aPtDiff.mX + aPtDiff.mY * aPtDiff.mY;
				if (aWantLenSq >= (float)aTotalLengthSq && aWantLenSq <= (float)aTotalLengthSq + aLenSq)
				{
					aPct = (aWantLenSq - (float)aTotalLengthSq) / aLenSq;
					aStartIdx = i;
					break;
				}
				aTotalLengthSq += (int)aLenSq;
			}
			aPt1 = anEmitterInstanceDef->mPoints[aStartIdx].GetValueAt(mFrameNum);
			aPt2 = anEmitterInstanceDef->mPoints[aStartIdx + 1].GetValueAt(mFrameNum);
			aPtDiff = aPt2 - aPt1;
			aPos.mX = aPt1.mX + aPct + 1.0 - aPct; //?
			aPos.mY = aPt1.mY + aPct + 1.0 - aPct; //?
			float aSign = anEmitterInstanceDef->mEmitIn ? (anEmitterInstanceDef->mEmitOut ? GetRandSign() : -1) : 1;
			if (theTravelAngle)
			{
				float anAngleChange = atan2(aPtDiff.mY, aPtDiff.mX) + PI / 2.0 + aSign * PI / 2.0;
				*theTravelAngle += anAngleChange;
			}
		}
		break;
	}
	case PIEmitterInstanceDef::GEOM_ECLIPSE:
	{
		float aXRad = theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_XRADIUS].GetValueAt(mFrameNum);
		float aYRad = theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_YRADIUS].GetValueAt(mFrameNum);
		float anAng;
		if (anEmitterInstanceDef->mEmitAtPointsNum)
		{
			int aPointIdx = theParticleInstance->mNum % anEmitterInstanceDef->mEmitAtPointsNum;
			anAng = (aPointIdx * PI * 2) / anEmitterInstanceDef->mEmitAtPointsNum;
			if (anAng > PI)
				anAng -= PI * 2;
		}
		else
			anAng = GetRandFloat() * PI;
		if (aXRad > aYRad)
		{
			float aFavorSidesFactor = 1.0 + (aXRad / aYRad - 1.0) * 0.3;
			if (anAng < -PI / 2)
				anAng = PI + pow((anAng + PI) / PI / 2, aFavorSidesFactor) * PI / 2;
			else if (anAng < 0)
				anAng = -pow(-anAng / PI / 2, aFavorSidesFactor) * PI / 2;
			else if (anAng < PI / 2)
				anAng = pow(anAng / PI / 2, aFavorSidesFactor) * PI / 2;
			else
				anAng = PI - pow((PI - anAng) / PI / 2, aFavorSidesFactor) * PI / 2;
		}
		else if (aYRad > aXRad)
		{
			float aFavorSidesFactor = 1.0 + ((aYRad / aXRad) - 1.0) * 0.3;
			if (anAng < -PI / 2)
				anAng = -PI / 2 - pow((-PI / 2 - anAng) / PI / 2, aFavorSidesFactor) * PI / 2;

			else if (anAng < 0)
				anAng = -PI / 2 + pow((anAng + PI / 2) / PI / 2, aFavorSidesFactor) * PI / 2;

			else if (anAng < PI / 2)
				anAng = PI / 2 - pow((PI / 2 - anAng) / PI / 2, aFavorSidesFactor) * PI / 2;

			else
				anAng = PI / 2 + pow((anAng - PI / 2) / PI / 2, aFavorSidesFactor) * PI / 2;
		}
		aPos = FPoint(cos(anAng) * aXRad, sin(anAng) * aYRad);
		if (theTravelAngle)
		{
			float aSign = anEmitterInstanceDef->mEmitIn ? (anEmitterInstanceDef->mEmitOut ? GetRandSign() : -1) : 1;
			float anAngleChange = anAng + aSign * PI / 2.0;
			*theTravelAngle += anAngleChange;
		}
		break;
	}
	case PIEmitterInstanceDef::GEOM_CIRCLE: //Occurs before in H5
	{
		float aRad = theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_XRADIUS].GetValueAt(mFrameNum);
		float anAng;
		if (anEmitterInstanceDef->mEmitAtPointsNum)
		{
			int aPointIdx = theParticleInstance->mNum % anEmitterInstanceDef->mEmitAtPointsNum;
			anAng = (aPointIdx * PI * 2) / anEmitterInstanceDef->mEmitAtPointsNum;
		}
		else
			anAng = GetRandFloat() * PI;
		aPos = FPoint(cos(anAng) * aRad, sin(anAng) * aRad);
		if (theTravelAngle)
		{
			float aSign = anEmitterInstanceDef->mEmitIn ? (anEmitterInstanceDef->mEmitOut ? GetRandSign() : -1) : 1;
			float anAngleChange = anAng + aSign * PI / 2.0;
			*theTravelAngle += anAngleChange;
		}
		break;
	}
	case PIEmitterInstanceDef::GEOM_AREA:
	{
		float aW = (theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_XRADIUS].GetValueAt(mFrameNum));
		float aH = (theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_YRADIUS].GetValueAt(mFrameNum));
		if (anEmitterInstanceDef->mEmitAtPointsNum)
		{
			float aPointIdxX = theParticleInstance->mNum % anEmitterInstanceDef->mEmitAtPointsNum;
			float aPointIdxY = (((theParticleInstance->mNum / anEmitterInstanceDef->mEmitAtPointsNum)) % anEmitterInstanceDef->mEmitAtPointsNum2);
			if (anEmitterInstanceDef->mEmitAtPointsNum > 1) {
				aPos.mX = (aPointIdxX / (anEmitterInstanceDef->mEmitAtPointsNum - 1) - 0.5) * aW;
			}
			if (anEmitterInstanceDef->mEmitAtPointsNum2 > 1) {
				aPos.mY = (aPointIdxY / (anEmitterInstanceDef->mEmitAtPointsNum2 - 1) - 0.5) * aH;
			}
		}
		else
			aPos = FPoint(GetRandFloat() * aW / 2.0, GetRandFloat() * aH / 2.0);
		if (&theEmitterInstance->mMaskImage && isMaskedOut)
		{
			float aXPct = aPos.mX / aW + 0.5;
			float aYPct = aPos.mY / aH + 0.5;
			int anImgW = theEmitterInstance->mMaskImage->mWidth;
			int anImgH = theEmitterInstance->mMaskImage->mHeight; //The later is C++ only
			int aCheckX = min((int)(aXPct * (float)anImgW), anImgW - 1);
			int aCheckY = min((int)(aYPct * (float)anImgH), anImgH - 1);
			ulong* aBits = theEmitterInstance->mMaskImage->GetBits();
			ulong aColor = aBits[aCheckX + anImgW * aCheckY];
			if (((aColor & 0x80000000u) == 0) ^ anEmitterInstanceDef->mInvertMask)
				*isMaskedOut = true;
		}
		break;
	}
}
}

bool PIEffect::LoadEffect(const std::string& theFileName) //TODO | 1795-2434
{
	/*if (mDef->mRefCount > 1)
		Deref();
	mDef = new PIEffectDef();
	Clear();
	mVersion = 0;
	mFileChecksum = 0;
	mSrcFileName = theFileName;
	mReadFP = p_fopen(theFileName.c_str(), "rb");
	mIsPPF = Lower(theFileName).find(".ppf", 0) != -1;
	mBufPos = PI_BUFSIZE;
	if (mReadFP == NULL)
		Fail("Unable to open file: " + theFileName);

	std::string aHeader = ReadString();
	if (mIsPPF)
		mVersion = EndianInt((int)GetData(this, &mBufTemp, 4));
	if (mVersion < PPF_MIN_VERSION)
		Fail("PPF version too old");
	mNotes = ReadString();
	int anImgNum = 0; //?
	short aTexCount = EndianShort((short)GetData(this, &mBufTemp, 2));
	for (int aTexIdx = 0; aTexIdx < aTexCount; aTexIdx++)
	{
		ExpectCmd("CMultiTexture");
		PITexture* aTexture = new PITexture();
		aTexture->mName = ReadString();
		int aCount = EndianShort((short)GetData(this, &mBufTemp, 2));
		aTexture->mNumCels = aCount;
		if (mIsPPF)
		{
			short aRowCount = EndianShort((short)GetData(this, &mBufTemp, 2));
			aTexture->mPadded = mIsPPF ? GetData(this, &mBufTemp, 1) != 0 : GetData(this, &mBufTemp, 4) != 0;
			std::string aFileName = ReadString();
			aTexture->mImageStrip = PIEffect::GetImage(aTexture->mName, aFileName); //Not in H5
			if (&aTexture->mImageStrip == NULL)
				Fail("Unable to load image: " + aFileName);
			else if (aTexture->mImageStrip->mNumCols == 1 && aTexture->mImageStrip->mNumRows == 1)
			{
				aTexture->mImageStrip->mNumCols = aTexCount / aRowCount;
				aTexture->mImageStrip->mNumRows = aRowCount;
			}
			SetImageOpts(aTexture->mImageStrip);
		}
		else //C++ only (TODO)
		{
			aTexture->mPadded = false;
			for (int j = 0; j < aTexCount; j++)
			{
				ExpectCmd("CTexture");
				aTexture->mName = ReadString();
				bool isNew = true;
				//?
				//int ? = EndianInt((int)GetData(this, &mBufTemp, 4)); //aShapeIdx or aNumBits or aMyst?
				int aWidth = EndianInt((int)GetData(this, &mBufTemp, 4));
				int aHeight = EndianInt((int)GetData(this, &mBufTemp, 4));
				//int aRowCount = EndianShort((short)GetData(this, &mBufTemp, 2));
				gSexyAppBase->GetSharedImage("!" + mSrcFileName + ":" + StrFormat("%d", anImgNum), "", &isNew, true); //Correct?
				anImgNum++;
				SharedImageRef anImage = NULL;
				anImage->Create(aWidth, aHeight);
				SetImageOpts(anImage);
				ulong* aBits = anImage->GetBits();
				for (int k = 0; k < aHeight * aWidth; ++k)
				{
					//? = (char)GetData(mBufTemp, 1) << 24) | 0xFFFFFF);
				}
				if (isNew)
					anImage->BitsChanged();
				aTexture->mImageVector.push_back(anImage);
			}
		}
		mDef->mTextureVector.push_back(aTexture);
	}
	short anEmitterCount = EndianShort((short)GetData(this, &mBufTemp, 2));
	mDef->mEmitterVector.resize(anEmitterCount);
	for (int anEmitterIdx = 0; anEmitterIdx < anEmitterCount; anEmitterIdx++)
	{
		mDef->mEmitterVector[anEmitterIdx] = new PIEmitter();
		ExpectCmd("CEmitterType");
		if (!mIsPPF) {
			mDef->mEmitterRefMap[mStringVector.size()] = anEmitterIdx;
		}
		ReadEmitterType(mDef->mEmitterVector[anEmitterIdx]);
	}
	std::vector<bool> anEmitterDefUsedVector;
	anEmitterDefUsedVector.resize(mDef->mEmitterVector.size());
	std::vector<bool> aTextureUsedVector;
	aTextureUsedVector.resize(mDef->mTextureVector.size());
	short aLayerCount = EndianShort((short)GetData(this, &mBufTemp, 2));*/
	return false;
}

bool PIEffect::SaveAsPPF(const std::string& theFileName, bool saveInitialState) //C++ only, TODO | 2437-2782
{
	mError.clear();
	mDestFileName = theFileName;
	mWriteFP = fopen(theFileName.c_str(), "wb");
	if (mReadFP == NULL)
		Fail("Unable to open file: " + theFileName);

	WriteString("PPF1");
	WriteInt(PPF_VERSION); //Prob correct
	WriteString(mNotes);
	int anImageCount;
	WriteShort(mDef->mTextureVector.size());
	for (int aTexIdx = 0; aTexIdx < mDef->mTextureVector.size(); aTexIdx++)
	{
		PITexture* aTexture = mDef->mTextureVector[aTexIdx];
		int aRowCount = 1;
		bool hasPadding = false;
		std::string aFileName;
		if (aTexture->mImageVector.size())
		{
			if (aTexture->mImageVector.size() == 1)
			{
				aFileName = WriteImage(aFileName, anImageCount, aTexture->mImageVector[0], &hasPadding);
				anImageCount++;
			}
			else
			{
				if (mAutoPadImages)
				{
					for (int aCelIdx = 0; aCelIdx < aTexture->mNumCels; aCelIdx++)
					{
						DeviceImage* anImage = aTexture->mImageVector[aCelIdx];
						ulong* aBits = anImage->GetBits();
						for (int x = 0; x < anImage->mWidth; ++x)
						{
							hasPadding |= (aBits[x] & 0xFF000000) != 0;
							hasPadding |= (aBits[x + anImage->mWidth * (anImage->mHeight - 1)] & 0xFF000000) != 0;
						}
						for (int y = 1; y < anImage->mHeight - 1; ++y)
						{
							hasPadding |= (aBits[anImage->mWidth * y] & 0xFF000000) != 0;
							hasPadding |= (aBits[anImage->mWidth - 1 + anImage->mWidth * y] & 0xFF000000) != 0;
						}
					}
				}
				int aPadding = hasPadding;
				int aCelWidth = aTexture->mImageVector[0]->mWidth + 2 * aPadding;
				int aCelHeight = aTexture->mImageVector[0]->mHeight + 2 * aPadding;
				int aNumCols = aTexture->mNumCels;
				int aNumRows = 1;
				GetBestStripSize(aTexture->mNumCels, aCelWidth, aCelHeight, aNumCols, aNumRows);
				DeviceImage* aNewImage = new DeviceImage();
				aNewImage->Create(aNumCols * aCelWidth, aNumRows * aCelHeight);
				aNewImage->SetImageMode(true, true);
				Graphics g(aNewImage);
				//for (int i = 0; i < aTexture->mNumCels; i++)
					//g.DrawImage(&aTexture->mImageVector[i] + aPadding + aCelWidth * (i % aNumCols), aPadding + aCelHeight * (i / aNumCols));
				aNewImage->mNumCols = aNumCols;
				aNewImage->mNumRows = aNumRows;
				aRowCount = aNumRows;
				aFileName = WriteImage(aFileName, anImageCount, aNewImage, &hasPadding);
				anImageCount++;
				if (aNewImage)
					aNewImage->DeleteExtraBuffers();
			}
		}
		else
		{
			aRowCount = aTexture->mImageStrip->mNumRows;
			aFileName = WriteImage(aFileName, anImageCount, aTexture->mImageStrip, &hasPadding);
		}
		WriteString(aTexture->mName);
		WriteShort(aTexture->mNumCels);
		WriteShort(aRowCount);
		WriteBool(hasPadding);
		WriteString(aFileName);
	}
	WriteShort(mDef->mEmitterVector.size());
	for (int anEmitterIdx = 0; anEmitterIdx < mDef->mEmitterVector.size(); anEmitterIdx++)
		WriteEmitterType(mDef->mEmitterVector[anEmitterIdx]);
	WriteShort(mDef->mLayerDefVector.size());
	for (int aLayerIdx = 0; aLayerIdx < mDef->mLayerDefVector.size(); aLayerIdx++)
	{
		PILayerDef* aLayerDef = &mDef->mLayerDefVector[aLayerIdx];
		PILayer* aLayer = &mLayerVector[aLayerIdx];
		WriteString(aLayerDef->mName);
		WriteShort(aLayerDef->mEmitterInstanceDefVector.size());
		for (int j = 0; j < aLayerDef->mEmitterInstanceDefVector.size(); j++) //Don't know what the var is
		{
			PIEmitterInstanceDef* anEmitterInstanceDef = &aLayerDef->mEmitterInstanceDefVector[j];
			PIEmitterInstance* anEmitterInstance = &aLayer->mEmitterInstanceVector[j];
			WriteFloat(0.0);
			WriteFloat(0.0);
			WriteFloat(0.0);
			WriteFloat(0.0);
			WriteFloat(0.0);
			WriteFloat(0.0);
			WriteFloat(0.0);
			WriteFloat(0.0);
			WriteFloat(0.0);
			WriteFloat(0.0);
			WriteFloat(0.0);
			WriteFloat(0.0);
			WriteInt(0);
			WriteInt(0);
			WriteInt(anEmitterInstanceDef->mFramesToPreload);
			WriteInt(0);
			WriteString(anEmitterInstanceDef->mName);
			WriteInt(anEmitterInstanceDef->mEmitterGeom);
			WriteFloat(0.0);
			WriteFloat(0.0);
			WriteBool(anEmitterInstanceDef->mEmitterGeom == 4);
			WriteBool(anEmitterInstanceDef->mEmitIn);
			WriteBool(anEmitterInstanceDef->mEmitOut);
			WriteInt(anEmitterInstance->mTintColor.mRed);
			WriteInt(anEmitterInstance->mTintColor.mGreen);
			WriteInt(anEmitterInstance->mTintColor.mBlue);
			WriteInt(0);
			WriteInt(anEmitterInstanceDef->mEmitAtPointsNum);
			WriteInt(anEmitterInstanceDef->mEmitterDefIdx);
			WriteValue2D(&anEmitterInstanceDef->mPosition);
			WriteShort(anEmitterInstanceDef->mPoints.size());
			for (int i = 0; i < anEmitterInstanceDef->mPoints.size(); i++) //Don't know what the var is
			{
				WriteFloat(0.0);
				WriteFloat(0.0);
				WriteEPoint(&anEmitterInstanceDef->mPoints[i]);
			}
			for (int i = 0; i < 17; i++) //Don't know what the var is
				WriteValue(&anEmitterInstanceDef->mValues[i]);
			WriteInt(anEmitterInstanceDef->mEmitAtPointsNum2);
			WriteInt(0);
			WriteValue(&anEmitterInstanceDef->mValues[17]);
			WriteInt(0);
			WriteValue(&anEmitterInstanceDef->mValues[18]);
			if (&anEmitterInstance->mMaskImage != NULL)
			{
				WriteShort(1);
				anImageCount++;
				WriteString(WriteImage(anEmitterInstanceDef->mName, anImageCount, anEmitterInstance->mMaskImage, false));
				WriteBool(true);
				WriteString("");
			}
			else
			{
				WriteShort(0);
				WriteBool(false);
				WriteString("empty");
			}
			WriteInt(0);
			WriteInt(0);
			WriteBool(anEmitterInstanceDef->mInvertMask);
			WriteInt(0);
			WriteInt(0);
			WriteBool(anEmitterInstanceDef->mIsSuperEmitter);
			WriteShort(anEmitterInstanceDef->mFreeEmitterIndices.size());
			for (int i = 0; i < anEmitterInstanceDef->mFreeEmitterIndices.size(); i++) //?
				WriteShort(anEmitterInstanceDef->mFreeEmitterIndices[i]);
			WriteInt(0);
			WriteFloat(0.0);
			WriteFloat(0.0);
			WriteShort(aLayerDef->mDeflectorVector.size());
			for (int i = 0; i < aLayerDef->mDeflectorVector.size(); i++) //?
			{
				PIDeflector* aDeflector = &aLayerDef->mDeflectorVector[i];
				WriteString(aDeflector->mName);
				WriteFloat(aDeflector->mBounce);
				WriteFloat(aDeflector->mHits);
				WriteFloat(aDeflector->mThickness);
				WriteBool(aDeflector->mVisible);
				WriteValue2D(&aDeflector->mPos);
				WriteShort(aDeflector->mPoints.size());
				for (int j = 0; j < aDeflector->mPoints.size(); j++) //?
				{
					WriteFloat(0.0);
					WriteFloat(0.0);
					WriteEPoint(&aDeflector->mPoints[j]);
				}
				WriteValue(&aDeflector->mActive);
				WriteValue(&aDeflector->mAngle);
			}
			WriteShort(aLayerDef->mBlockerVector.size());
			for (int i = 0; i < aLayerDef->mBlockerVector.size(); i++) //?
			{
				PIBlocker* aBlocker = &aLayerDef->mBlockerVector[i];
				WriteString(aBlocker->mName);
				WriteInt(0);
				WriteInt(0);
				WriteInt(0);
				WriteInt(0);
				WriteInt(0);
				WriteValue2D(&aBlocker->mPos);
				WriteShort(aBlocker->mPoints.size());
				for (int j = 0; j < aBlocker->mPoints.size(); j++) //?
				{
					WriteFloat(0.0);
					WriteFloat(0.0);
					WriteEPoint(&aBlocker->mPoints[j]);
				}
				WriteValue(&aBlocker->mActive);
				WriteValue(&aBlocker->mAngle);
			}
			WriteValue2D(&aLayerDef->mOffset);
			WriteValue(&aLayerDef->mAngle);
			WriteString("");
			for (int i = 0; i < 32; i++)
				WriteByte(0);
			WriteShort(0);
			for (int i = 0; i < 36; i++)
				WriteByte(0);
			WriteShort(aLayerDef->mForceVector.size());
			for (int aForceIdx = 0; aForceIdx < aLayerDef->mForceVector.size(); aForceIdx++) //?
			{
				PIForce* aForce = &aLayerDef->mForceVector[aForceIdx];
				WriteString(aForce->mName);
				WriteBool(aForce->mVisible);
				WriteValue2D(&aForce->mPos);
				WriteValue(&aForce->mActive);
				WriteValue(&aForce->mWidth);
				WriteValue(&aForce->mStrength);
				WriteValue(&aForce->mWidth);
				WriteValue(&aForce->mHeight);
				WriteValue(&aForce->mAngle);
				WriteValue(&aForce->mDirection);
			}
			for (int i = 0; i < 28; i++)
				WriteByte(0);
		}
		WriteInt(mBkgColor.mRed);
		WriteInt(mBkgColor.mGreen);
		WriteInt(mBkgColor.mBlue);
		WriteInt(0);
		WriteInt(0);
		WriteShort(mFramerate);
		WriteShort(0);
		WriteShort(0);
		WriteShort(0);
		WriteInt(mWidth);
		WriteInt(mHeight);
		WriteInt(0);
		WriteInt(0);
		WriteInt(0);
		WriteInt(0);
		WriteInt(0);
		WriteInt(mFirstFrameNum);
		WriteInt(mLastFrameNum);
		WriteString("");
		WriteByte(0);
		WriteShort(0);
		WriteShort(0);
		ResetAnim();
		if (mRandSeeds.size())
			mRand.SRand(mRandSeeds[0]);
		else
			mRand.SRand(0);
		if (saveInitialState)
		{
			Update();
			Buffer aStartupState;
			SaveState(aStartupState, 1);
			WriteInt(aStartupState.GetDataLen());
			fwrite(aStartupState.GetDataPtr(), 1u, aStartupState.GetDataLen(), mWriteFP);
		}
		else
			WriteInt(0);
		fclose(mWriteFP);
		return true;
	}
}

bool PIEffect::LoadState(Buffer& theBuffer, bool shortened) //Correct? | 2785-2967
{
	if (mError.length() != NULL)
		return false;

	ResetAnim();
	theBuffer.mReadBitPos = (theBuffer.mReadBitPos + 7) & -8; //Not in H5
	int aSize = theBuffer.ReadLong();
	int anEnd = aSize + theBuffer.mReadBitPos / 8;
	int aVersion = theBuffer.ReadShort();
	if (!shortened)
	{
		std::string aSrcFileName = theBuffer.ReadString();
		if (!mLoaded)
			LoadEffect(aSrcFileName);
		int aChecksum = theBuffer.ReadLong();
		if (aChecksum != mFileChecksum)
		{
			theBuffer.mReadBitPos = anEnd * 8;
			return false;
		}
	}
	theBuffer.ReadBytes((uchar*)&mFrameNum, 4);
	mFrameNum = EndianFloat(mFrameNum);
	if (!shortened)
	{
		mRand.SRand(theBuffer.ReadString());
		mWantsSRand = false;
	}
	if (!shortened)
	{
		mEmitAfterTimeline = theBuffer.ReadBoolean();
		theBuffer.ReadBytes((uchar*)&mEmitterTransform, 36);
		theBuffer.ReadBytes((uchar*)&mDrawTransform, 36);
		for (int j = 0; j < 3; j++) //C++ only
		{
			for (int i = 0; i < 3; i++)
			{
				mEmitterTransform.m[i][j] = EndianFloat(mEmitterTransform.m[i][j]);
				mDrawTransform.m[i][j] = EndianFloat(mDrawTransform.m[i][j]);
			}
		}
	}
	else if (aVersion == 0)
	{
		theBuffer.ReadBoolean();
		SexyTransform2D aTrans;
		theBuffer.ReadBytes((uchar*)&aTrans, 36);
		theBuffer.ReadBytes((uchar*)&aTrans, 36);
	}
	if (mFrameNum > 0.0)
	{
		for (int aLayerIdx = 0; aLayerIdx < mDef->mLayerDefVector.size(); aLayerIdx++)
		{
			PILayer* aLayer = &mLayerVector[aLayerIdx];
			PILayerDef* aLayerDef = &mDef->mLayerDefVector[aLayerIdx];
			for (int anEmitterInstanceIdx = 0; anEmitterInstanceIdx < aLayerDef->mEmitterInstanceDefVector.size(); anEmitterInstanceIdx++)
			{
				PIEmitterInstance* anEmitterInstance = &aLayer->mEmitterInstanceVector[anEmitterInstanceIdx];
				PIEmitterInstanceDef* anEmitterInstanceDef = &aLayerDef->mEmitterInstanceDefVector[anEmitterInstanceIdx];
				if (theBuffer.ReadBoolean() != false)
				{
					for (int j = 0; j < 3; j++) //C++ only
					{
						for (int i = 0; i < 3; i++)
							anEmitterInstance->mTransform.m[i][j] = EndianFloat(anEmitterInstance->mTransform.m[i][j]);
					}
				}
				anEmitterInstance->mWasActive = theBuffer.ReadBoolean();
				anEmitterInstance->mWithinLifeFrame = theBuffer.ReadBoolean();
				PIEmitter* aDefEmitter = mDef->mEmitterVector[anEmitterInstanceDef->mEmitterDefIdx];
				for (int aParticleDefIdx = 0; aParticleDefIdx < aDefEmitter->mParticleDefVector.size(); aParticleDefIdx++)
				{
					PIParticleDefInstance* aParticleDefInstance = &anEmitterInstance->mParticleDefInstanceVector[aParticleDefIdx];
					LoadParticleDefInstance(theBuffer, aParticleDefInstance);
				}
				for (int aFreeEmitterIdx = 0; aFreeEmitterIdx < anEmitterInstanceDef->mFreeEmitterIndices.size(); aFreeEmitterIdx++)
				{
					PIParticleDefInstance* aParticleDefInstance = &anEmitterInstance->mSuperEmitterParticleDefInstanceVector[aFreeEmitterIdx];
					LoadParticleDefInstance(theBuffer, aParticleDefInstance);
				}
				int aSuperEmitterCount = theBuffer.ReadLong();
				for (int aSuperEmitterIdx = 0; aSuperEmitterIdx < aSuperEmitterCount; aSuperEmitterIdx++)
				{
					PIFreeEmitterInstance* aChildEmitterInstance = mFreeEmitterPool.Alloc();
					int anEmitterIdx = theBuffer.ReadShort();
					aChildEmitterInstance->mEmitterSrc = mDef->mEmitterVector[anEmitterInstanceDef->mFreeEmitterIndices[anEmitterIdx]];
					aChildEmitterInstance->mParentFreeEmitter = NULL;
					aChildEmitterInstance->mParticleDef = NULL;
					aChildEmitterInstance->mNum = aSuperEmitterIdx;
					LoadParticle(theBuffer, aLayer, aChildEmitterInstance);
					PIEmitter* anEmitter = aChildEmitterInstance->mEmitterSrc;
					aChildEmitterInstance->mEmitter.mParticleDefInstanceVector.resize(anEmitter->mParticleDefVector.size());
					for (int aParticleDefIdx = 0; aParticleDefIdx < anEmitter->mParticleDefVector.size(); aParticleDefIdx++)
					{
						PIParticleDefInstance* aParticleDefInstance = &anEmitterInstance->mParticleDefInstanceVector[aParticleDefIdx];
						LoadParticleDefInstance(theBuffer, aParticleDefInstance);
					}
					if (aSuperEmitterIdx > 0)
					{
						anEmitterInstance->mSuperEmitterGroup.mTail->mNext = aChildEmitterInstance;
						aChildEmitterInstance->mPrev = anEmitterInstance->mSuperEmitterGroup.mTail;
					}
					else
						anEmitterInstance->mSuperEmitterGroup.mHead = aChildEmitterInstance;
					anEmitterInstance->mSuperEmitterGroup.mTail = aChildEmitterInstance;
					anEmitterInstance->mSuperEmitterGroup.mCount++;
					int aParticlesCount = theBuffer.ReadLong();
					for (int aParticleIdx = 0; aParticleIdx < aParticlesCount; aParticleIdx++)
					{
						PIParticleInstance* aParticleInstance = mParticlePool.Alloc();
						aParticleInstance->mEmitterSrc = aChildEmitterInstance->mEmitterSrc;
						aParticleInstance->mParentFreeEmitter = aChildEmitterInstance;
						int aParticleDefIdx = theBuffer.ReadShort();
						aParticleInstance->mParticleDef = &aParticleInstance->mEmitterSrc->mParticleDefVector[aParticleDefIdx];
						aParticleInstance->mNum = aParticleIdx;
						LoadParticle(theBuffer, aLayer, aParticleInstance);
						CalcParticleTransform(aLayer, anEmitterInstance, aParticleInstance->mEmitterSrc, aParticleInstance->mParticleDef, &aChildEmitterInstance->mEmitter.mParticleGroup, aParticleInstance);
						if (aParticleIdx > 0)
						{
							aChildEmitterInstance->mEmitter.mParticleGroup.mTail->mNext = aParticleInstance;
							aParticleInstance->mPrev = aChildEmitterInstance->mEmitter.mParticleGroup.mTail;
						}
						else
							aChildEmitterInstance->mEmitter.mParticleGroup.mHead = aParticleInstance;
						aChildEmitterInstance->mEmitter.mParticleGroup.mTail = aParticleInstance;
						aChildEmitterInstance->mEmitter.mParticleGroup.mCount++;
					}
				}
				int aParticleCount = theBuffer.ReadLong();
				for (int aParticleIdx = 0; aParticleIdx < aParticleCount; aParticleIdx++)
				{
					PIParticleInstance* aParticleInstance = mParticlePool.Alloc();
					aParticleInstance->mEmitterSrc = aDefEmitter;
					aParticleInstance->mParentFreeEmitter = NULL;
					int aParticleDefIdx = theBuffer.ReadShort();
					aParticleInstance->mParticleDef = &aParticleInstance->mEmitterSrc->mParticleDefVector[aParticleDefIdx];
					aParticleInstance->mNum = aParticleIdx;
					LoadParticle(theBuffer, aLayer, aParticleInstance);
					CalcParticleTransform(aLayer, anEmitterInstance, aParticleInstance->mEmitterSrc, aParticleInstance->mParticleDef, &anEmitterInstance->mParticleGroup, aParticleInstance);
					if (aParticleIdx > 0) {
						anEmitterInstance->mParticleGroup.mTail->mNext = aParticleInstance;
						aParticleInstance->mPrev = anEmitterInstance->mParticleGroup.mTail;
					}
					else
						anEmitterInstance->mParticleGroup.mHead = aParticleInstance;
					anEmitterInstance->mParticleGroup.mTail = aParticleInstance;
					anEmitterInstance->mParticleGroup.mCount++;
				}
			}
		}
	}
	else
		theBuffer.mReadBitPos = 8 * anEnd;
	return true;
}

bool PIEffect::SaveState(Buffer& theBuffer, bool shortened) //Not in H5 TODO | 2975-3091
{
	//if (mError.length() != NULL)
		return false;

	/*theBuffer.mWriteBitPos = (theBuffer.mWriteBitPos + 7) & 0xFFFFFFF8;
	int aSizeWritePos = theBuffer.mWriteBitPos / 8;
	theBuffer.WriteLong(0);
	theBuffer.WriteShort(PPF_STATE_VERSION);
	if (!shortened)
	{
		theBuffer.WriteString(mSrcFileName);
		theBuffer.WriteLong(mFileChecksum);
	}
	theBuffer.WriteBytes((uchar*)&mFrameNum, 4);
	if (!shortened)
	{
		theBuffer.WriteString(mRand.Serialize());
		theBuffer.WriteBoolean(mEmitAfterTimeline);
		theBuffer.WriteBytes((uchar*)&mEmitterTransform, 36);
		theBuffer.WriteBytes((uchar*)&mDrawTransform, 36);
	}
	if (mFrameNum > 0)
	{
		for (int aLayerIdx = 0; aLayerIdx < mDef->mLayerDefVector.size(); aLayerIdx++)
		{
			PILayer* aLayer = &mLayerVector[aLayerIdx];
			PILayerDef* aLayerDef = &mDef->mLayerDefVector[aLayerIdx];
			for (int anEmitterInstanceIdx = 0; anEmitterInstanceIdx < aLayer->mEmitterInstanceVector.size(); anEmitterInstanceIdx++)
			{
				PIEmitterInstance* anEmitterInstance = &aLayer->mEmitterInstanceVector[anEmitterInstanceIdx];
				PIEmitterInstanceDef* anEmitterInstanceDef = &aLayerDef->mEmitterInstanceDefVector[anEmitterInstanceIdx];
				if (!IsIdentityMatrix(anEmitterInstance->mTransform))
				{
					theBuffer.WriteBoolean(true);
					theBuffer.WriteBytes((uchar*)&anEmitterInstance->mTransform, 36);
				}
				else
					theBuffer.WriteBoolean(false);
				theBuffer.WriteBoolean(anEmitterInstance->mWasActive);
				theBuffer.WriteBoolean(anEmitterInstance->mWithinLifeFrame);
				std::map<PIEmitter*, PIEmitterToIdMap> anEmitterParticleDefMap;
				PIEmitter* aDefEmitter = mDef->mEmitterVector[anEmitterInstanceDef->mEmitterDefIdx];
				for (int aParticleDefIdx = 0; aParticleDefIdx < aDefEmitter->mParticleDefVector.size(); aParticleDefIdx++)
				{
					PIParticleDef* aParticleDef = &aDefEmitter->mParticleDefVector[aParticleDefIdx];
					PIParticleDefInstance* aParticleDefInstance = &anEmitterInstance->mParticleDefInstanceVector[aParticleDefIdx];
					anEmitterParticleDefMap[aDefEmitter][aParticleDef] = aParticleDefIdx;
					SaveParticleDefInstance(theBuffer, aParticleDefInstance);
				}
				PIEmitterToIdMap aFreeEmitterMap;
				for (int aFreeEmitterIdx = 0; aFreeEmitterIdx < anEmitterInstanceDef->mFreeEmitterIndices.size(); aFreeEmitterIdx++)
				{
					PIEmitter* anEmitter = mDef->mEmitterVector[anEmitterInstanceDef->mFreeEmitterIndices[aFreeEmitterIdx]];
					for (int aParticleDefIdx = 0; aParticleDefIdx < anEmitter->mParticleDefVector.size(); aParticleDefIdx++)
					{
						PIParticleDef* aParticleDef = &anEmitter->mParticleDefVector[aParticleDefIdx];
						anEmitterParticleDefMap[anEmitter][aParticleDef] = aParticleDefIdx;
					}
					PIParticleDefInstance* aParticleDefInstance = &anEmitterInstance->mSuperEmitterParticleDefInstanceVector[aFreeEmitterIdx];
					SaveParticleDefInstance(theBuffer, aParticleDefInstance);
					//aFreeEmitterMap.insert(anEmitter[aFreeEmitterIdx]); //?
				}
				PIFreeEmitterInstance* aChildEmitterInstance = (PIFreeEmitterInstance*)&anEmitterInstance->mSuperEmitterGroup.mHead;
				theBuffer.WriteLong(CountParticles(aChildEmitterInstance));
				while (aChildEmitterInstance != NULL)
				{
					theBuffer.WriteShort((short)aFreeEmitterMap[aChildEmitterInstance->mEmitterSrc]);
					SaveParticle(theBuffer, aLayer, aChildEmitterInstance);
					PIEmitter* anEmitter = aChildEmitterInstance->mEmitterSrc;
					for (int aParticleDefIdx = 0; aParticleDefIdx < aChildEmitterInstance->mEmitterSrc->mParticleDefVector.size(); aParticleDefIdx++)
					{
						PIParticleDefInstance* aParticleDefInstance = &aChildEmitterInstance->mEmitter.mParticleDefInstanceVector[aParticleDefIdx];
						SaveParticleDefInstance(theBuffer, aParticleDefInstance);
					}
					PIParticleInstance* aParticleInstance = aChildEmitterInstance->mEmitter.mParticleGroup.mHead;
					theBuffer.WriteLong(CountParticles(aParticleInstance));
					while (aParticleInstance != NULL)
					{
						theBuffer.WriteShort((short)anEmitterParticleDefMap[aParticleInstance->mEmitterSrc][aParticleInstance->mParticleDef]);
						SaveParticle(theBuffer, aLayer, aParticleInstance);
						aParticleInstance = aParticleInstance->mNext;
					}
					aChildEmitterInstance = (PIFreeEmitterInstance*)aChildEmitterInstance->mNext;
				}
			}
		}
	}
	int aSize = theBuffer.mWriteBitPos / 8 - aSizeWritePos - 4;
	theBuffer.mData[aSizeWritePos]; //?
	return true;*/
}

void PIEffect::ResetAnim() //Correct? | 3094-3161
{
	mFrameNum = 0;
	for (int aLayerIdx = 0; aLayerIdx < mDef->mLayerDefVector.size(); aLayerIdx++)
	{
		PILayerDef* aLayerDef = &mDef->mLayerDefVector[aLayerIdx];
		PILayer* aLayer = &mLayerVector[aLayerIdx];
		for (int anEmitterInstanceIdx = 0; anEmitterInstanceIdx < aLayer->mEmitterInstanceVector.size(); anEmitterInstanceIdx++)
		{
			PIEmitterInstanceDef* anEmitterInstanceDef = &aLayerDef->mEmitterInstanceDefVector[anEmitterInstanceIdx];
			PIEmitterInstance* anEmitterInstance = &aLayer->mEmitterInstanceVector[anEmitterInstanceIdx];
			PIParticleInstance* aParticleInstance;
			PIFreeEmitterInstance* aChildEmitterInstance = (PIFreeEmitterInstance*)anEmitterInstance->mSuperEmitterGroup.mHead;
			while (aChildEmitterInstance != NULL)
			{
				PIParticleInstance* aNext = aChildEmitterInstance->mNext;
				aParticleInstance = aChildEmitterInstance->mEmitter.mParticleGroup.mHead;
				while (aParticleInstance != NULL)
				{
					PIParticleInstance* anotherNext = aParticleInstance->mNext;
					mParticlePool.Free(aParticleInstance);
				}
				mFreeEmitterPool.Free(aChildEmitterInstance);
			}
			anEmitterInstance->mSuperEmitterGroup.mHead = NULL;
			anEmitterInstance->mSuperEmitterGroup.mTail = NULL;
			anEmitterInstance->mSuperEmitterGroup.mCount = 0;
			while (aParticleInstance != NULL)
			{
				PIParticleInstance* aNext = aParticleInstance->mNext;
				mParticlePool.Free(aParticleInstance);
			}
			anEmitterInstance->mSuperEmitterGroup.mHead = NULL;
			anEmitterInstance->mSuperEmitterGroup.mTail = NULL;
			anEmitterInstance->mSuperEmitterGroup.mCount = 0;
			aParticleInstance = anEmitterInstance->mParticleGroup.mHead;
			while (aParticleInstance != NULL)
			{
				PIParticleInstance* aNext = aParticleInstance->mNext;
				mParticlePool.Free(aParticleInstance);
			}
			anEmitterInstance->mParticleGroup.mHead = NULL;
			anEmitterInstance->mParticleGroup.mTail = NULL;
			anEmitterInstance->mParticleGroup.mCount = 0;
			for (int aFreeEmitterIdx = 0; aFreeEmitterIdx < anEmitterInstanceDef->mFreeEmitterIndices.size(); aFreeEmitterIdx++)
			{
				PIParticleDefInstance* aParticleDefInstance = &anEmitterInstance->mSuperEmitterParticleDefInstanceVector[aFreeEmitterIdx];
				aParticleDefInstance->Reset();
			}
			PIEmitter* anEmitter = mDef->mEmitterVector[anEmitterInstanceDef->mEmitterDefIdx];
			for (int aParticleDefIdx = 0; aParticleDefIdx < anEmitter->mParticleDefVector.size(); aParticleDefIdx++)
			{
				PIParticleDefInstance* aParticleDefInstance = &anEmitterInstance->mParticleDefInstanceVector[aParticleDefIdx];
				aParticleDefInstance->Reset();
			}
			anEmitterInstance->mWithinLifeFrame = true;
			anEmitterInstance->mWasActive = false;
		}
	}
	mCurNumEmitters = 0;
	mCurNumParticles = 0;
	mLastDrawnPixelCount = 0;
	mWantsSRand = true;
}

void PIEffect::Clear() //3164-3176
{
	mError.clear();
	ResetAnim();
	mStringVector.clear();
	mNotesParams.clear();
	mDef->mEmitterVector.clear();
	mDef->mTextureVector.clear();
	mDef->mLayerDefVector.clear();
	mDef->mEmitterRefMap.clear();
	mRandSeeds.clear();
	mVersion = 0;
	mLoaded = false;
}

PILayer* PIEffect::GetLayer(int theIdx) //3179-3183
{
	if (theIdx < mDef->mLayerDefVector.size())
		return &mLayerVector[theIdx];
	return NULL;
}

PILayer* PIEffect::GetLayer(const std::string& theName) //3186-3191
{
	for (int i = 0; i < mDef->mLayerDefVector.size(); i++)
	{
		if (theName.length() || stricmp(mDef->mLayerDefVector[i].mName.c_str(), theName.c_str()))
			return &mLayerVector[i];
	}
	return NULL;
}

FPoint PIEffect::GetEmitterPos(PIEmitterInstance* theEmitterInstance, bool doTransform) //3194-3203
{
	FPoint aCurPoint = theEmitterInstance->mEmitterInstanceDef->mPosition.GetValueAt(mFrameNum);
	if (doTransform)
	{
		aCurPoint = TransformFPoint(theEmitterInstance->mTransform, aCurPoint);
		aCurPoint = TransformFPoint(mEmitterTransform, aCurPoint); //Bool in H5
		aCurPoint += theEmitterInstance->mOffset;
	}
	return aCurPoint;
}

int PIEffect::CountParticles(PIParticleInstance* theStart) //3206-3215
{
	int aCount = 0;
	while (theStart != NULL)
	{
		aCount++;
		theStart = theStart->mNext;
	}
	return aCount;
}

void PIEffect::CalcParticleTransform(PILayer* theLayer, PIEmitterInstance* theEmitterInstance, PIEmitter* theEmitter, PIParticleDef* theParticleDef, PIParticleGroup* theParticleGroup, PIParticleInstance* theParticleInstance) //TODO | 3218-3367
{
	/*PIEmitter* anEmitter = theEmitter;
	PIParticleDef* aParticleDef = theParticleDef;
	PIParticleInstance* aParticleInstance = theParticleInstance;
	float aLifePct = theParticleInstance->mLifePct;
	float aScaleX;
	float aScaleY;
	float aRefXScale = 1.0;
	float aRefYScale = 1.0;
	Rect aSrcRect;
	if (theParticleDef != NULL)
	{
		PITexture* aTexture = mDef->mTextureVector[theParticleDef->mTextureIdx];
		if (aTexture->mImageVector.size() != 0)
		{
			DeviceImage* anImage = aTexture->mImageVector[theParticleInstance->mImgIdx];
			aSrcRect = Rect(0, 0, anImage->mWidth, anImage->mHeight);
		}
		else
		{
			DeviceImage* anImage = aTexture->mImageStrip;
			aSrcRect = anImage->GetCelRect(aParticleInstance->mImgIdx);
			if (aTexture->mPadded)
			{
				aSrcRect.mX++;
				aSrcRect.mWidth -= 2;
				aSrcRect.mY++;
				aSrcRect.mHeight -= 2;
			}
		}
		if (theParticleDef->mSingleParticle)
		{
			aParticleInstance->mSrcSizeXMult = theParticleGroup->mWasEmitted ? anEmitter->mValues[PIEmitter::VALUE_SIZE_X].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_SIZE_X].GetValueAt(mFrameNum) * (aParticleDef->mValues[PIParticleDef::VALUE_SIZE_X].GetValueAt(mFrameNum) + aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SIZE_X]);
			aParticleInstance->mSrcSizeXMult = theParticleGroup->mWasEmitted ? anEmitter->mValues[PIEmitter::VALUE_SIZE_Y].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_SIZE_Y].GetValueAt(mFrameNum) * (aParticleDef->mValues[PIParticleDef::VALUE_SIZE_Y].GetValueAt(mFrameNum) + aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SIZE_Y]);
		}
		float aSizeX = max(aParticleDef->mValues[PIParticleDef::VALUE_SIZE_X_OVER_LIFE].GetValueAt(aLifePct, 0.0) * aParticleInstance->mSrcSizeXMult, 0.1);
		float aSizeY = max(aParticleDef->mValues[PIParticleDef::VALUE_SIZE_Y_OVER_LIFE].GetValueAt(aLifePct, 0.0) * aParticleInstance->mSrcSizeYMult, 0.1);
		int aScaleRef = max(aSrcRect.mWidth, aSrcRect.mHeight);
		float aRefXScale = aScaleRef / aSrcRect.mWidth;
		float aRefYScale = aScaleRef / aSrcRect.mHeight;
		aScaleX = aSizeX / aScaleRef * 2;
		aScaleY = aSizeY / aScaleRef * 2;
	}
	else
	{
		aScaleX = 1.0;
		aScaleY = 1.0;
	}
	SexyTransform2D aBaseRotTrans;
	float anEmitterRot = theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ANGLE].GetValueAt(mFrameNum);
	if (anEmitterRot != 0.0)
		aBaseRotTrans.RotateDeg(anEmitterRot);
	if (aParticleInstance->mParentFreeEmitter && aParticleInstance->mParentFreeEmitter->mImgAngle != 0.0)
		aBaseRotTrans.RotateRad(-aParticleInstance->mParentFreeEmitter->mImgAngle);
	Transform aTransform;
	float aScaleFactor = 1.0; //Not in H5
	if (theParticleDef)
	{
		aTransform.Translate(-theParticleDef->mRefPointOfs.mY * aRefYScale * aSrcRect.mHeight, -theParticleDef->mRefPointOfs.mX * aRefXScale * aSrcRect.mWidth);
		if (theParticleDef->mFlipHorz)
			aTransform.Scale(-1.0, 1.0);
		if (theParticleDef->mFlipVert)
			aTransform.Scale(1.0, -1.0);
	}
	aScaleFactor *= aScaleX * aScaleY;
	if (aScaleX != 1.0 || aScaleY != 1.0)
		aTransform.Scale(aScaleX, aScaleY);
	if (aParticleInstance->mImgAngle != 0.0)
		aTransform.RotateRad(aParticleInstance->mImgAngle);
	if (theParticleDef && theParticleDef->mAttachToEmitter)
	{
		float aRot = 0.0;
		if (aParticleInstance->mParentFreeEmitter)
			aRot = (aParticleInstance->mParentFreeEmitter->mImgAngle - aParticleInstance->mOrigEmitterAng) * theParticleDef->mAttachVal;
		else
			aRot = DegToRad(theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ANGLE].GetValueAt(mFrameNum) * theParticleDef->mAttachVal);
		if (aRot != 0.0)
			aTransform.RotateRad(aRot);
		if (theParticleDef && theParticleDef->mSingleParticle && (!theParticleDef->mAngleKeepAlignedToMotion || theParticleDef->mAttachToEmitter))
			aTransform.RotateDeg(theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ANGLE].GetValueAt(mFrameNum));
		FPoint aParticlePos;
		if (theParticleDef->mAttachToEmitter)
		{
			SexyTransform2D aBackTrans;
			aBackTrans.RotateRad(aParticleInstance->mOrigEmitterAng);
			FPoint aBackPoint = TransformFPoint(aBackTrans, aParticlePos);
			FPoint aCurRotPos = TransformFPoint(aBaseRotTrans, aBackPoint);
			aParticlePos.mX = (aParticlePos.mX * (1.0 - theParticleDef->mAttachVal)) + (aCurRotPos.mX * theParticleDef->mAttachVal);
			aParticlePos.mY = (aParticlePos.mY * (1.0 - theParticleDef->mAttachVal)) + (aCurRotPos.mY * theParticleDef->mAttachVal);
		}
		aTransform.Translate(aParticlePos.mX, aParticlePos.mY);
		if (theParticleDef && theParticleDef->mSingleParticle)
			aParticleInstance->mZoom = (anEmitter->mValues[PIEmitter::VALUE_ZOOM].GetValueAt(mFrameNum, 1.0)) * theParticleGroup->mWasEmitted ? anEmitter->mValues[PIEmitter::VALUE_ZOOM].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ZOOM].GetValueAt(mFrameNum);
		aScaleFactor = aParticleInstance->mZoom * aParticleInstance->mZoom * aScaleFactor;
		if (aParticleInstance->mZoom != 1.0)
			aTransform.Scale(aParticleInstance->mZoom, aParticleInstance->mZoom);
		FPoint anEmitterPos = aParticleInstance->mEmittedPos;
		if (theParticleDef && theParticleDef->mSingleParticle)
		{
			FPoint aCurEmitPos = TransformFPoint(aBaseRotTrans, aParticleInstance->mOrigPos);
			aCurEmitPos + GetEmitterPos(theEmitterInstance, !theParticleGroup->mWasEmitted);
			anEmitterPos.mX = aCurEmitPos.mX;
			anEmitterPos.mY = aCurEmitPos.mY;
		}
		else if (theParticleDef && theParticleDef->mAttachToEmitter && !theParticleGroup->mIsSuperEmitter)
		{
			FPoint aCurEmitPos;
			if (aParticleInstance->mParentFreeEmitter != NULL)
				aCurEmitPos = aParticleInstance->mParentFreeEmitter->mLastEmitterPos + aParticleInstance->mParentFreeEmitter->mOrigPos;
		}
		else
		{
			FPoint aCurEmitPos;
			aCurEmitPos = TransformFPoint(aBaseRotTrans, aParticleInstance->mOrigPos);
			aCurEmitPos += GetEmitterPos(theEmitterInstance, !theParticleGroup->mWasEmitted);
		}
		anEmitterPos = anEmitterPos * (1.0 - theParticleDef->mAttachVal) + &aCurEmitPos * theParticleDef->mAttachVal;
	}
	aParticleInstance->mLastEmitterPos = anEmitterPos;
	aTransform.Translate(anEmitterPos.mX, anEmitterPos.mY);
	theLayer->mLayerDef->mOffset.GetValueAt(mFrameNum) - theLayer->mLayerDef->mOrigOffset;
	aTransform.Translate(anOffset.mX, anOffset.mY);
	float anAngle = theLayer->mLayerDef->mAngle.GetValueAt(mFrameNum);
	if (anAngle != 0.0)
		aTransform.RotateDeg(anAngle);
	memcpy(&theParticleInstance->mTransform, &aTransform, sizeof(theParticleInstance->mTransform));
	theParticleInstance->mTransformScaleFactor = aScaleFactor;*/
}

void PIEffect::UpdateParticleDef(PILayer* theLayer, PIEmitter* theEmitter, PIEmitterInstance* theEmitterInstance, PIParticleDef* theParticleDef, PIParticleDefInstance* theParticleDefInstance, PIParticleGroup* theParticleGroup, PIFreeEmitterInstance* theFreeEmitter) //Correct? | 3370-3729
{
	PIEmitterInstanceDef* anEmitterInstanceDef = theEmitterInstance->mEmitterInstanceDef;
	float anUpdateRate = 100.0 / mAnimSpeed;
	PIEmitter* anEmitter = theEmitter;
	PIParticleDef* aParticleDef = theParticleDef;
	float anEmitterLifePct = 0.0;
	if (theFreeEmitter != NULL)
		anEmitterLifePct = theFreeEmitter->mLifePct;
	if (theParticleDefInstance->mTicks % 25 == 0)
	{
		if (!theParticleGroup->mIsSuperEmitter)
		{
			if (theParticleDefInstance->mTicks == 0)
				theParticleDefInstance->mCurNumberVariation = GetRandFloat() * 0.5 * (aParticleDef->mValues[PIParticleDef::VALUE_NUMBER_VARIATION].GetValueAt(mFrameNum) / 2.0);
			else
				theParticleDefInstance->mCurNumberVariation = GetRandFloat() * 0.75 * (aParticleDef->mValues[PIParticleDef::VALUE_NUMBER_VARIATION].GetValueAt(mFrameNum) / 2.0);
		}
	}
	theParticleDefInstance->mTicks++;
	float aNumber;
	if (theParticleGroup->mIsSuperEmitter)
		aNumber = anEmitter->mValues[PIEmitter::VALUE_F_NUMBER].GetValueAt(mFrameNum) * theParticleGroup->mWasEmitted ? (anEmitter->mValues[PIEmitter::VALUE_NUMBER].GetValueAt(mFrameNum)) : (theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_NUMBER].GetValueAt(mFrameNum));
	else
	{
		aNumber = anEmitter->mValues[PIEmitter::VALUE_F_NUMBER_OVER_LIFE].GetValueAt(anEmitterLifePct, 1.0) * theParticleGroup->mWasEmitted ? (anEmitter->mValues[PIEmitter::VALUE_NUMBER].GetValueAt(mFrameNum)) : (theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_NUMBER].GetValueAt(mFrameNum));
		aNumber = max(0.0, aNumber);
		if (theParticleGroup->mWasEmitted && anEmitterLifePct >= 1.0)
			aNumber = 0.0;
	}

	aNumber *= theEmitterInstance->mNumberScale;
	if (theParticleGroup->mIsSuperEmitter)
		aNumber *= 30.0;
	else if (!theParticleGroup->mWasEmitted)
	{
		switch (anEmitterInstanceDef->mEmitterGeom)
		{
		case PIEmitterInstanceDef::GEOM_LINE:
		{
			if (anEmitterInstanceDef->mEmitAtPointsNum != 0)
				aNumber *= anEmitterInstanceDef->mEmitAtPointsNum;
			else
			{
				int aTotalLength = 0.0;
				for (int i = 0; i < anEmitterInstanceDef->mPoints.size() - 1; i++)
				{
					FPoint aPt1 = anEmitterInstanceDef->mPoints[i].GetValueAt(mFrameNum);
					FPoint aPt2 = anEmitterInstanceDef->mPoints[i + 1].GetValueAt(mFrameNum);
					FPoint aPtDiff = aPt2 - aPt1; //Not in H5
					float aLen = sqrt(aPtDiff.mX * aPtDiff.mX + aPtDiff.mY * aPtDiff.mY); //Not in H5
					aTotalLength += aLen;
				}
				aNumber *= (aTotalLength / 35.0);
			}
		
		break;
		}
		case PIEmitterInstanceDef::GEOM_ECLIPSE:
		{
			float aXRad = (theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_XRADIUS].GetValueAt(mFrameNum));
			float aYRad = (theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_YRADIUS].GetValueAt(mFrameNum));
			if (anEmitterInstanceDef->mEmitAtPointsNum != 0)
				aNumber *= anEmitterInstanceDef->mEmitAtPointsNum;
			else {
				float aCircumf = PI * 2.0 * sqrt((aXRad * aXRad + aYRad * aYRad) / 2.0);
				aNumber *= aCircumf / 35.0;
			}
			break;
		}
		case PIEmitterInstanceDef::GEOM_AREA:
		{
			if (anEmitterInstanceDef->mEmitAtPointsNum)
				aNumber *= (anEmitterInstanceDef->mEmitAtPointsNum2 * anEmitterInstanceDef->mEmitAtPointsNum);
			else
			{
				float aW = theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_XRADIUS].GetValueAt(mFrameNum);
				float aH = theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_YRADIUS].GetValueAt(mFrameNum);
				aNumber *= 1.0 + (aW * aH / 900.0 / 4.0);
			}
			break;
		}
		case PIEmitterInstanceDef::GEOM_CIRCLE:
		{
			float aRad = theEmitterInstance->mEmitterInstanceDef->mValues[15].GetValueAt(mFrameNum);
			if (anEmitterInstanceDef->mEmitAtPointsNum)
				aNumber *= anEmitterInstanceDef->mEmitAtPointsNum;
			else
				aNumber *= (sqrt(aRad * aRad) * PI) / 35.0;
			break;
		}
		}
		theParticleDefInstance->mNumberAcc += aNumber / anUpdateRate; //0.16 in H5?
		if (!anEmitterInstanceDef->mIsSuperEmitter && !theEmitterInstance->mWasActive || !theEmitterInstance->mWithinLifeFrame)
			theParticleDefInstance->mNumberAcc = 0.0;
		bool canUseGeom = true;
		if (!theParticleGroup->mIsSuperEmitter && theParticleDef->mSingleParticle)
		{
			int aWantCount;
			if (anEmitterInstanceDef->mEmitterGeom == PIEmitterInstanceDef::GEOM_LINE || anEmitterInstanceDef->mEmitterGeom == PIEmitterInstanceDef::GEOM_CIRCLE)
				aWantCount = anEmitterInstanceDef->mEmitAtPointsNum;
			else if (anEmitterInstanceDef->mEmitterGeom == PIEmitterInstanceDef::GEOM_AREA)
				aWantCount = anEmitterInstanceDef->mEmitAtPointsNum * anEmitterInstanceDef->mEmitAtPointsNum2;
			else
				aWantCount = 1;
			if (aWantCount == 0)
			{
				canUseGeom = false;
				aWantCount = 1;
			}
			int aCount = 0;
			PIParticleInstance* aCheckInstance = theParticleGroup->mHead;
			while (aCheckInstance != NULL)
			{
				if (aCheckInstance->mParticleDef == aParticleDef)
					aCount++;
				aCheckInstance = aCheckInstance->mNext;
			}
			theParticleDefInstance->mNumberAcc = aWantCount - aCount;
			while (theParticleDefInstance->mNumberAcc >= 1.0)
			{
				theParticleDefInstance->mNumberAcc -= 1.0;
				PIParticleInstance* aParticleInstance;
				if (theParticleGroup->mIsSuperEmitter)
				{
					PIFreeEmitterInstance* anEmitterInstance = mFreeEmitterPool.Alloc();
					anEmitterInstance->mEmitter.mParticleDefInstanceVector.resize(theEmitter->mParticleDefVector.size());
					aParticleInstance = anEmitterInstance;
				}
				else
					aParticleInstance = mParticlePool.Alloc();
				aParticleInstance->mParticleDef = theParticleDef;
				aParticleInstance->mEmitterSrc = theEmitter;
				aParticleInstance->mParentFreeEmitter = theFreeEmitter;
				aParticleInstance->mNum = theParticleDefInstance->mParticlesEmitted++;
				float anAngle;
				if (theParticleGroup->mIsSuperEmitter)
					anAngle = GetRandFloat() * theParticleGroup->mWasEmitted ? anEmitter->mValues[PIEmitter::VALUE_EMISSION_RANGE].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_EMISSION_RANGE].GetValueAt(mFrameNum) / 2.0 + theParticleGroup->mWasEmitted ? anEmitter->mValues[PIEmitter::VALUE_EMISSION_ANGLE].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_EMISSION_ANGLE].GetValueAt(mFrameNum);
				else if (theParticleDef->mUseEmitterAngleAndRange)
				{
					if (theParticleGroup->mWasEmitted)
						anAngle = GetRandFloat() * anEmitter->mValues[PIEmitter::VALUE_EMISSION_RANGE].GetValueAt(mFrameNum) + anEmitter->mValues[PIEmitter::VALUE_EMISSION_ANGLE].GetValueAt(mFrameNum);
					else
						anAngle = GetRandFloat() * theParticleGroup->mWasEmitted ? anEmitter->mValues[PIEmitter::VALUE_EMISSION_RANGE].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_EMISSION_RANGE].GetValueAt(mFrameNum) + theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_EMISSION_ANGLE].GetValueAt(mFrameNum);
				}
				else
					anAngle = aParticleDef->mValues[PIParticleDef::VALUE_EMISSION_ANGLE].GetValueAt(mFrameNum) + aParticleDef->mValues[PIParticleDef::VALUE_EMISSION_RANGE].GetValueAt(mFrameNum) * GetRandFloat() / 2.0;
				anAngle - DegToRad(-anAngle);
				float anEmitterAngle;
				if (theFreeEmitter)
					float anEmitterAngle = theFreeEmitter->mImgAngle;
				else
					float anEmitterAngle = DegToRad(-theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ANGLE].GetValueAt(mFrameNum));
				anAngle += anEmitterAngle;
				aParticleInstance->mOrigEmitterAng = anEmitterAngle;
				if (theParticleDef != NULL && theParticleDef->mAnimStartOnRandomFrame)
					aParticleInstance->mAnimFrameRand = mRand.Next() & 0x7fff;
				else
					aParticleInstance->mAnimFrameRand = 0;
				aParticleInstance->mZoom = theParticleGroup->mWasEmitted ? anEmitter->mValues[PIEmitter::VALUE_ZOOM].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ZOOM].GetValueAt(mFrameNum);
				if (!theParticleGroup->mIsSuperEmitter)
				{
					aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_LIFE] = GetVariationScalar() * anEmitter->mValues[PIEmitter::VALUE_F_LIFE_VARIATION].GetValueAt(mFrameNum);
					aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SIZE_X] = GetVariationScalar() * anEmitter->mValues[PIEmitter::VALUE_F_SIZE_X_VARIATION].GetValueAt(mFrameNum);
					if (theParticleDef == NULL || theParticleDef->mLockAspect)
						aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SIZE_Y] = aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SIZE_X];
					else
						aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SIZE_Y] = GetVariationScalar() * aParticleDef->mValues[PIParticleDef::VALUE_SIZE_Y_VARIATION].GetValueAt(mFrameNum);
					aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_VELOCITY] = GetVariationScalar() * aParticleDef->mValues[PIParticleDef::VALUE_VELOCITY_VARIATION].GetValueAt(mFrameNum);
					aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_WEIGHT] = GetVariationScalar() * aParticleDef->mValues[PIParticleDef::VALUE_WEIGHT_VARIATION].GetValueAt(mFrameNum);
					aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SPIN] = GetVariationScalar() * aParticleDef->mValues[PIParticleDef::VALUE_SPIN_VARIATION].GetValueAt(mFrameNum);
					aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_MOTION_RAND] = GetVariationScalar() * aParticleDef->mValues[PIParticleDef::VALUE_MOTION_RAND_VARIATION].GetValueAt(mFrameNum);
					aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_BOUNCE] = GetVariationScalar() * aParticleDef->mValues[PIParticleDef::VALUE_BOUNCE_VARIATION].GetValueAt(mFrameNum);
					aParticleInstance->mSrcSizeXMult = theParticleGroup->mWasEmitted ? anEmitter->mValues[PIEmitter::VALUE_SIZE_X].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_SIZE_X].GetValueAt(mFrameNum) * aParticleDef->mValues[PIParticleDef::VALUE_SIZE_X].GetValueAt(mFrameNum) + aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SIZE_X];
					aParticleInstance->mSrcSizeYMult = theParticleGroup->mWasEmitted ? anEmitter->mValues[PIEmitter::VALUE_SIZE_Y].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_SIZE_Y].GetValueAt(mFrameNum) * aParticleDef->mValues[PIParticleDef::VALUE_SIZE_Y].GetValueAt(mFrameNum) + aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SIZE_Y];
					if (theParticleGroup->mWasEmitted)
					{
						aParticleInstance->mSrcSizeXMult *= 1.0 + theFreeEmitter->mVariationValues[PIParticleInstance::VARIATION_SIZE_X] * anEmitter->mValues[PIEmitter::VALUE_F_SIZE_X_OVER_LIFE].GetValueAt(anEmitterLifePct, 1.0);
						aParticleInstance->mSrcSizeYMult *= 1.0 + theFreeEmitter->mVariationValues[PIParticleInstance::VARIATION_SIZE_Y] * anEmitter->mValues[PIEmitter::VALUE_F_SIZE_Y_OVER_LIFE].GetValueAt(anEmitterLifePct, 1.0);
						aParticleInstance->mZoom *= 1.0 + theFreeEmitter->mVariationValues[PIParticleInstance::VARIATION_ZOOM] * anEmitter->mValues[PIEmitter::VALUE_F_ZOOM_OVER_LIFE].GetValueAt(anEmitterLifePct, 1.0);
					}
				}
				else
				{
					aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_LIFE] = GetVariationScalar() * (anEmitter->mValues[PIEmitter::VALUE_F_LIFE_VARIATION].GetValueAt(mFrameNum));
					aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SIZE_X] = GetRandFloat() * (anEmitter->mValues[PIEmitter::VALUE_F_SIZE_X_VARIATION].GetValueAt(mFrameNum));
					if ((theParticleDef == NULL) || theParticleDef->mLockAspect)
						aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SIZE_Y] = aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SIZE_X];
					else
						aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SIZE_Y] = GetRandFloat() * (anEmitter->mValues[PIEmitter::VALUE_F_SIZE_Y_VARIATION].GetValueAt(mFrameNum));
					aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_VELOCITY] = GetVariationScalar() * (anEmitter->mValues[PIEmitter::VALUE_F_VELOCITY_VARIATION].GetValueAt(mFrameNum));
					aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_WEIGHT] = GetVariationScalar() * (anEmitter->mValues[PIEmitter::VALUE_F_WEIGHT_VARIATION].GetValueAt(mFrameNum));
					aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SPIN] = GetVariationScalar() * (anEmitter->mValues[PIEmitter::VALUE_F_SPIN_VARIATION].GetValueAt(mFrameNum));
					aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_MOTION_RAND] = GetVariationScalar() * (anEmitter->mValues[PIEmitter::VALUE_F_MOTION_RAND_VARIATION].GetValueAt(mFrameNum));
					aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_BOUNCE] = GetVariationScalar() * (anEmitter->mValues[PIEmitter::VALUE_F_BOUNCE_VARIATION].GetValueAt(mFrameNum));
					aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_ZOOM] = GetVariationScalar() * (anEmitter->mValues[PIEmitter::VALUE_F_ZOOM_VARIATION].GetValueAt(mFrameNum));
				}
				float aTravelAngle = anAngle; //Not in H5
				aParticleInstance->mGradientRand = GetRandFloatU();
				aParticleInstance->mTicks = 0.0;
				aParticleInstance->mThicknessHitVariation = GetRandFloat();
				aParticleInstance->mImgAngle = 0.0;
				if (theParticleGroup->mIsSuperEmitter)
					aParticleInstance->mLife = ((anEmitter->mValues[PIEmitter::VALUE_F_LIFE].GetValueAt(mFrameNum)) + aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_LIFE]) * 5 * (theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_LIFE].GetValueAt(mFrameNum));
				else
					aParticleInstance->mLife = (theParticleGroup->mWasEmitted ? (anEmitter->mValues[PIEmitter::VALUE_LIFE].GetValueAt(mFrameNum)) : (theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_LIFE].GetValueAt(mFrameNum))) * ((aParticleDef->mValues[PIParticleDef::VALUE_LIFE].GetValueAt(mFrameNum)) + aParticleInstance->mVariationValues[PIParticleDef::VALUE_LIFE]);
				FPoint aPos;
				if (theParticleGroup->mWasEmitted)
				{
					aParticleInstance->mEmittedPos = theFreeEmitter->mLastEmitterPos + theFreeEmitter->mPos;
					aParticleInstance->mLastEmitterPos = aParticleInstance->mEmittedPos;
				}
				else
				{
					aParticleInstance->mEmittedPos = GetEmitterPos(theEmitterInstance, true);
					aParticleInstance->mLastEmitterPos = aParticleInstance->mEmittedPos;
					bool isMaskedOut = false;
					if (canUseGeom)
						aPos = GetGeomPos(theEmitterInstance, aParticleInstance, &aTravelAngle, &isMaskedOut) - aParticleInstance->mEmittedPos;
					if (isMaskedOut)
						continue;
				}
				aParticleInstance->mVel = FPoint((float)cos(aTravelAngle), (float)sin(aTravelAngle));
				if (theParticleGroup->mIsSuperEmitter)
					aParticleInstance->mVel = aParticleInstance->mVel * ((theParticleGroup->mWasEmitted ? theEmitter->mValues[PIEmitter::VALUE_VELOCITY].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_VELOCITY].GetValueAt(mFrameNum)) * (theEmitter->mValues[PIEmitter::VALUE_F_VELOCITY].GetValueAt(mFrameNum) + aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_VELOCITY])) * 160.0;
				else
					aParticleInstance->mVel = aParticleInstance->mVel * (theParticleGroup->mWasEmitted ? theEmitter->mValues[PIEmitter::VALUE_VELOCITY].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_VELOCITY].GetValueAt(mFrameNum)) * (theParticleDef->mValues[PIParticleDef::VALUE_VELOCITY].GetValueAt(mFrameNum) + aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_VELOCITY]); //?
				if (!theParticleGroup->mIsSuperEmitter)
				{
					if (theParticleDef->mAngleAlignToMotion)
					{
						if (aParticleInstance->mVel.Magnitude() == 0.0)
						{
							aTravelAngle = 0.0;
							if (cos(aTravelAngle) > 0.0)
								aParticleInstance->mImgAngle = 0.0;
							else
								aParticleInstance->mImgAngle = M_PI;
							if (theParticleDef->mSingleParticle && theParticleDef->mAngleKeepAlignedToMotion && !theParticleDef->mAttachToEmitter)
								aParticleInstance->mImgAngle += DegToRad(theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ANGLE].GetValueAt(mFrameNum));
						}
						else
							aParticleInstance->mImgAngle = -aTravelAngle;
						aParticleInstance->mImgAngle += DegToRad(-theParticleDef->mAngleAlignOffset);
					}
					else if (theParticleDef->mAngleRandomAlign)
						aParticleInstance->mImgAngle = DegToRad(-(theParticleDef->mAngleOffset + GetRandFloat() * theParticleDef->mAngleRange / 2.0));
					else
						aParticleInstance->mImgAngle = DegToRad(theParticleDef->mAngleValue);
				}
				aParticleInstance->mOrigPos = aPos;
				SexyTransform2D aTransform;
				aTransform.RotateDeg(theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ANGLE].GetValueAt(mFrameNum));
				aParticleInstance->mEmittedPos += TransformFPoint(aTransform, aPos);
				if (theEmitter->mOldestInFront)
				{
					if (theParticleGroup->mHead != NULL)
						theParticleGroup->mHead->mPrev = aParticleInstance;
					aParticleInstance->mNext = theParticleGroup->mHead;
					if (theParticleGroup->mTail == NULL)
						theParticleGroup->mTail = aParticleInstance;
					theParticleGroup->mHead = aParticleInstance;
				}
				else
				{
					if (theParticleGroup->mTail != NULL)
						theParticleGroup->mTail->mNext = aParticleInstance;
					aParticleInstance->mPrev = theParticleGroup->mTail;
					if (theParticleGroup->mHead == NULL)
						theParticleGroup->mHead = aParticleInstance;
					theParticleGroup->mTail = aParticleInstance;
				}
				theParticleGroup->mCount++;
			}
		}
	}
}

void PIEffect::UpdateParticleGroup(PILayer* theLayer, PIEmitterInstance* theEmitterInstance, PIParticleGroup* theParticleGroup) //Correct? | 3732-4025
{
	float anUpdateRate = 100.0 / mAnimSpeed;
	PIParticleInstance* aParticleInstance = theParticleGroup->mHead;
	PILayerDef* aLayerDef = theLayer->mLayerDef;
	PIEmitterInstanceDef* anEmitterInstanceDef = theEmitterInstance->mEmitterInstanceDef;
	while (aParticleInstance != NULL)
	{
		PIParticleInstance* aNext = aParticleInstance->mNext;
		PIEmitter* anEmitter = aParticleInstance->mEmitterSrc;
		PIParticleDef* aParticleDef = aParticleInstance->mParticleDef;
		float anEmitterLifePct = 0.0;
		if (aParticleInstance->mParentFreeEmitter)
			anEmitterLifePct = aParticleInstance->mParentFreeEmitter->mLifePct;
		bool isNew = aParticleInstance->mTicks == 0.0;
		aParticleInstance->mTicks += 1.0 / anUpdateRate;
		float aLifePct = 0.0;
		if (aParticleDef != NULL && aParticleDef->mSingleParticle)
		{
			float aNextToggleTime = theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ACTIVE].GetNextKeyframeTime(mFrameNum);
			int aNextKeyIdx = theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ACTIVE].GetNextKeyframeIdx(mFrameNum);
			if (aNextToggleTime >= mFrameNum && aNextKeyIdx == 1)
				aLifePct = min(1.0, (mFrameNum + anEmitterInstanceDef->mFramesToPreload) / max(1, aNextToggleTime));
			else
				aLifePct = 0.02;
		}
		else
			aLifePct = aParticleInstance->mTicks / aParticleInstance->mLife;
		if (aParticleInstance->mLifePct >= 0.9999999 || aParticleInstance->mLife <= 0.00000001 || (!theEmitterInstance->mWasActive && !anEmitterInstanceDef->mIsSuperEmitter))
		{
			if (theParticleGroup->mIsSuperEmitter && ((PIFreeEmitterInstance*)aParticleInstance)->mEmitter.mParticleGroup.mHead != NULL)
			{
				aParticleInstance = aNext;
				continue;
			}
			if (!theParticleGroup->mIsSuperEmitter && aParticleDef->mSingleParticle && theEmitterInstance->mWasActive) {} //?
			else
			{
				if (theParticleGroup->mIsSuperEmitter)
					mFreeEmitterPool.Free((PIFreeEmitterInstance*)aParticleInstance);
				else
					mParticlePool.Free(aParticleInstance);
				if (aParticleInstance->mPrev != NULL)
					aParticleInstance->mPrev->mNext = aParticleInstance->mNext;
				if (aParticleInstance->mNext != NULL)
					aParticleInstance->mNext->mPrev = aParticleInstance->mPrev;
				if (theParticleGroup->mHead == aParticleInstance)
					theParticleGroup->mHead = aParticleInstance->mNext;
				if (theParticleGroup->mTail == aParticleInstance)
					theParticleGroup->mTail = aParticleInstance->mPrev;
				theParticleGroup->mCount--;
				aParticleInstance = aNext;
				continue;
			}
		}
		if (aParticleDef != NULL) {
			PITexture* aTexture = mDef->mTextureVector[aParticleDef->mTextureIdx];
			if (aParticleDef->mAnimSpeed == -1)
				aParticleInstance->mImgIdx = aParticleInstance->mAnimFrameRand % aTexture->mNumCels;
			else
				aParticleInstance->mImgIdx = ((int)(aParticleInstance->mTicks * (float)mFramerate / (float)(aParticleDef->mAnimSpeed + 1)) + aParticleInstance->mAnimFrameRand) % aTexture->mNumCels;
		}
		if ((theParticleGroup->mIsSuperEmitter) || (!aParticleDef->mSingleParticle))
		{
			if (mIsNewFrame)
			{
				float aRand1 = GetRandFloat() * GetRandFloat(); //Why not use GetVariationScalar? You could save on a bit of code
				float aRand2 = GetRandFloat() * GetRandFloat();
				float aMotionRand;
				if (theParticleGroup->mIsSuperEmitter)
					aMotionRand = max(0.0, (theParticleGroup->mWasEmitted ? anEmitter->mValues[PIEmitter::VALUE_MOTION_RAND].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_MOTION_RAND].GetValueAt(mFrameNum)) * anEmitter->mValues[PIEmitter::VALUE_F_MOTION_RAND_OVER_LIFE].GetValueAt(aLifePct, 1.0) * (anEmitter->mValues[PIEmitter::VALUE_F_MOTION_RAND].GetValueAt(mFrameNum) + aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_MOTION_RAND]) * 30.0); //?
				else
					aMotionRand = max(0.0, (theParticleGroup->mWasEmitted ? anEmitter->mValues[PIEmitter::VALUE_MOTION_RAND].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_MOTION_RAND].GetValueAt(mFrameNum)) * aParticleDef->mValues[PIParticleDef::VALUE_MOTION_RAND_OVER_LIFE].GetValueAt(aLifePct) * (aParticleDef->mValues[PIParticleDef::VALUE_MOTION_RAND].GetValueAt(mFrameNum) + aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_MOTION_RAND])); //?
				aParticleInstance->mVel.mX += aRand1 * aMotionRand;
				aParticleInstance->mVel.mY += aRand2 * aMotionRand;
			}
			float aWeight;
			if (theParticleGroup->mIsSuperEmitter)
			{
				aWeight = (theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_WEIGHT].GetValueAt(mFrameNum)) * ((anEmitter->mValues[PIEmitter::VALUE_F_WEIGHT_OVER_LIFE].GetValueAt(aLifePct, 1.0)) - 1.0) * ((anEmitter->mValues[PIEmitter::VALUE_F_WEIGHT].GetValueAt(mFrameNum)) + (aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_WEIGHT])) / 2.0 * 100.0;
			}
			else
				aWeight = (theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_WEIGHT].GetValueAt(mFrameNum)) * ((aParticleDef->mValues[PIParticleDef::VALUE_WEIGHT_OVER_LIFE].GetValueAt(aLifePct)) - 1.0) * ((aParticleDef->mValues[PIParticleDef::VALUE_WEIGHT].GetValueAt(mFrameNum)) + (aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_WEIGHT])) * 100.0;
			aWeight *= 1.0 + (mFramerate - 100.0) * 0.0005;
			aParticleInstance->mVel.mY += aWeight / anUpdateRate;
			FPoint aCurVel = aParticleInstance->mVel / anUpdateRate;
			if (theParticleGroup->mIsSuperEmitter)
				aCurVel * anEmitter->mValues[PIEmitter::VALUE_F_VELOCITY_OVER_LIFE].GetValueAt(aLifePct, 1.0);
			else
				aCurVel * aParticleDef->mValues[PIParticleDef::VALUE_VELOCITY_OVER_LIFE].GetValueAt(aLifePct);
			FPoint aCurPhysPoint;
			if (!isNew && aLayerDef->mDeflectorVector.size() > 0)
			{
				FPoint aPrevPhysPoint = TransformFPoint(aParticleInstance->mTransform.GetMatrix(), FPoint(0.0, 0.0));
				FPoint aPrevPos = aParticleInstance->mPos;
				aParticleInstance->mPos += aCurVel;
				CalcParticleTransform(theLayer, theEmitterInstance, anEmitter, aParticleDef, theParticleGroup, aParticleInstance);
				aCurPhysPoint = TransformFPoint(aParticleInstance->mTransform.GetMatrix(), FPoint(0.0, 0.0));
				for (int aDeflectorIdx = 0; aDeflectorIdx < aLayerDef->mDeflectorVector.size(); aDeflectorIdx++)
				{
					PIDeflector* aDeflector = &aLayerDef->mDeflectorVector[aDeflectorIdx];
					if (aDeflector->mActive.GetLastKeyframe(mFrameNum) < 0.99)
						continue;
					for (int aPtIdx = 1; aPtIdx < aDeflector->mCurPoints.size(); aPtIdx++)
					{
						FPoint aPt1 = aDeflector->mCurPoints[aPtIdx - 1] - FPoint(mDrawTransform.m02, mDrawTransform.m12);
						FPoint aPt2 = aDeflector->mCurPoints[aPtIdx] - FPoint(mDrawTransform.m02, mDrawTransform.m12);
						SexyVector2 aLineDir = SexyVector2(aPt2.mX - aPt1.mX, aPt2.mY - aPt1.mY);
						SexyVector2 aLineNormal = aLineDir.Normalize().Perp();
						FPoint aLineTranslate = FPoint(aLineNormal.x, aLineNormal.y);
						aLineTranslate * aDeflector->mThickness * aParticleInstance->mThicknessHitVariation;
						FPoint aCollPoint;
						if (LineSegmentIntersects(aPrevPhysPoint, aCurPhysPoint, aPt1 + aLineTranslate, aPt2 + aLineTranslate, 0, &aCollPoint))
						{
							if (GetRandFloatU() > aDeflector->mHits)
								continue;
							float aBounce = aDeflector->mBounce;
							if (theParticleGroup->mIsSuperEmitter)
								aBounce *= theParticleGroup->mWasEmitted ? anEmitter->mValues[PIEmitter::VALUE_BOUNCE].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_BOUNCE].GetValueAt(mFrameNum) * anEmitter->mValues[PIEmitter::VALUE_F_BOUNCE_OVER_LIFE].GetValueAt(aLifePct, 1.0) * anEmitter->mValues[PIEmitter::VALUE_F_BOUNCE].GetValueAt(mFrameNum) + aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_BOUNCE];
							else
								aBounce *= theParticleGroup->mWasEmitted ? anEmitter->mValues[PIEmitter::VALUE_BOUNCE].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_BOUNCE].GetValueAt(mFrameNum) * aParticleDef->mValues[PIParticleDef::VALUE_BOUNCE_OVER_LIFE].GetValueAt(aLifePct) * aParticleDef->mValues[PIParticleDef::VALUE_BOUNCE].GetValueAt(mFrameNum) + aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_BOUNCE];
							SexyVector2 aCurVelVec = SexyVector2(aCurVel.mX, aCurVel.mY);
							float aDot = aCurVelVec.Dot(aLineNormal);
							SexyVector2 aNewVel = aCurVelVec - aLineNormal * 2 * aDot;
							float aPctBounce = min(1.0, fabs(aNewVel.y / aNewVel.x));
							aNewVel.y *= 1.0 - aPctBounce + aPctBounce * pow(aBounce, 0.5);
							aParticleInstance->mVel = FPoint(aNewVel.x, aNewVel.y * 100.0);
							if (aBounce > 0.001)
								aParticleInstance->mPos = aPrevPos;
							CalcParticleTransform(theLayer, theEmitterInstance, anEmitter, aParticleDef, theParticleGroup, aParticleInstance);
							aCurPhysPoint = TransformFPoint(aParticleInstance->mTransform.GetMatrix(), FPoint(0.0, 0.0));
						}
					}
				}
			}
			else
			{
				aParticleInstance->mPos += aCurVel;
				CalcParticleTransform(theLayer, theEmitterInstance, anEmitter, aParticleDef, theParticleGroup, aParticleInstance);
				if (aLayerDef->mForceVector.size() > 0)
					aCurPhysPoint = TransformFPoint(aParticleInstance->mTransform.GetMatrix(), FPoint(0.0, 0.0));
			}
			for (int aForceIdx = 0; aForceIdx < aLayerDef->mForceVector.size() > 0; aForceIdx++)
			{
				PIForce* aForce = &aLayerDef->mForceVector[aForceIdx];
				if (aForce->mActive.GetLastKeyframe(mFrameNum) < 0.99)
					continue;
				bool inside = false;
				int i = 0;
				int j = 3;
				for (i = 0, j = 4 - 1; i < 4; j = i++)
				{
					if ((aCurPhysPoint.mY >= aForce->mCurPoints[i].mY && aForce->mCurPoints[j].mY > aCurPhysPoint.mY || aCurPhysPoint.mY >= aForce->mCurPoints[j].mY && aForce->mCurPoints[i].mY > aCurPhysPoint.mY) && (aForce->mCurPoints[j].mX - aForce->mCurPoints[i].mX) * (aCurPhysPoint.mY - aForce->mCurPoints[i].mY) / (aForce->mCurPoints[j].mY - aForce->mCurPoints[i].mY) + aForce->mCurPoints[i].mX > aCurPhysPoint.mX)
						inside = !inside;
				}
				if (inside)
				{
					float anAngle = DegToRad(-aForce->mDirection.GetValueAt(mFrameNum)) + DegToRad(-aForce->mAngle.GetValueAt(mFrameNum));
					float aFactor = 0.085 * mFramerate / 100.0;
					aFactor *= 1.0 + mFramerate - 100.0 * 0.004;
					float aStrength = aForce->mStrength.GetValueAt(mFrameNum) * aFactor;
					aParticleInstance->mVel.mX += cos(anAngle) * aStrength * 100.0;
					aParticleInstance->mVel.mY += sin(anAngle) * aStrength * 100.0;
				}
			}
			if (!theParticleGroup->mIsSuperEmitter && aParticleDef->mAngleAlignToMotion && aParticleDef->mAngleKeepAlignedToMotion)
				aParticleInstance->mImgAngle = atan2(aCurVel.mY, aCurVel.mX) + DegToRad((float)aParticleDef->mAngleAlignOffset);
		}
		else if (aParticleDef->mSingleParticle)
		{
			bool canUseGeom = false;
			if (anEmitterInstanceDef->mEmitterGeom == PIEmitterInstanceDef::GEOM_LINE || anEmitterInstanceDef->mEmitterGeom == PIEmitterInstanceDef::GEOM_CIRCLE)
				canUseGeom = anEmitterInstanceDef->mEmitAtPointsNum != 0;
			else if (anEmitterInstanceDef->mEmitterGeom == PIEmitterInstanceDef::GEOM_AREA)
				canUseGeom = anEmitterInstanceDef->mEmitAtPointsNum * anEmitterInstanceDef->mEmitAtPointsNum2 != 0;
			if (canUseGeom)
			{
				FPoint aPos = GetGeomPos(theEmitterInstance, aParticleInstance, 0, false);
				aParticleInstance->mEmittedPos = GetEmitterPos(theEmitterInstance, true);
				aParticleInstance->mLastEmitterPos = aParticleInstance->mEmittedPos;
				aParticleInstance->mOrigPos = aPos - aParticleInstance->mEmittedPos;
				SexyTransform2D aTransform;
				aTransform.RotateDeg(DegToRad(theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ANGLE].GetValueAt(mFrameNum)));
				aParticleInstance->mEmittedPos += TransformFPoint(aTransform, aPos);
			}
			if (aParticleDef->mAngleKeepAlignedToMotion && !aParticleDef->mAttachToEmitter)
			{
				FPoint aCurVel = anEmitterInstanceDef->mPosition.GetValueAt(mFrameNum);
				if (aCurVel.Magnitude() != 0)
					aParticleInstance->mImgAngle = atan2(aCurVel.mY, aCurVel.mX);
				else
					aParticleInstance->mImgAngle = 0;
				aParticleInstance->mImgAngle += DegToRad(aParticleDef->mAngleAlignOffset);
			}
		}
		if (aParticleDef != NULL)
		{
			bool wantColor = (!aParticleInstance->mHasDrawn && aParticleDef->mGetColorFromLayer) || aParticleDef->mUpdateColorFromLayer;
			bool wantTransparency = (!aParticleInstance->mHasDrawn && aParticleDef->mGetTransparencyFromLayer) || aParticleDef->mUpdateTransparencyFromLayer;
			if (wantColor || wantTransparency)
			{
				FPoint aDrawPoint = TransformFPoint(aParticleInstance->mTransform.GetMatrix(), FPoint(0.0, 0.0));
				int aCheckX = theLayer->mBkgImgDrawOfs.mX + (int)aDrawPoint.mX;
				int aCheckY = theLayer->mBkgImgDrawOfs.mY + (int)aDrawPoint.mY;
				ulong aColor;
				if (theLayer->mBkgImage != NULL && aCheckX >= 0 && aCheckY >= 0 && aCheckX < theLayer->mBkgImage->mWidth && aCheckY < theLayer->mBkgImage->mHeight) //C++ only
				{
					ulong* aBits = theLayer->mBkgImage->MemoryImage::GetBits();
					aColor = aBits[aCheckX + theLayer->mBkgImage->mWidth * aCheckY];
				}
				else
					aColor = 0;
				if (wantColor)
					aParticleInstance->mBkgColor = aParticleInstance->mBkgColor & 0xFF000000 | aColor & 0xFFFFFF;
				if (wantTransparency)
					aParticleInstance->mBkgColor = aParticleInstance->mBkgColor & 0xFFFFFF | aColor & 0xFF000000;
			}
		}
		if (theParticleGroup->mIsSuperEmitter)
			aParticleInstance->mImgAngle += DegToRad(-(theParticleGroup->mWasEmitted ? anEmitter->mValues[PIEmitter::VALUE_F_SPIN].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_SPIN].GetValueAt(mFrameNum)) * (anEmitter->mValues[PIEmitter::VALUE_F_SPIN_OVER_LIFE].GetValueAt(aLifePct, 1.0) - 1.0) * (anEmitter->mValues[PIEmitter::VALUE_F_SPIN].GetValueAt(mFrameNum) + aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SPIN])) / anUpdateRate * 160.0;
		else if (!aParticleDef->mAngleKeepAlignedToMotion)
			aParticleInstance->mImgAngle += DegToRad(-(theParticleGroup->mWasEmitted ? anEmitter->mValues[PIEmitter::VALUE_SPIN].GetValueAt(mFrameNum) : theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_SPIN].GetValueAt(mFrameNum)) * (aParticleDef->mValues[PIParticleDef::VALUE_SPIN_OVER_LIFE].GetValueAt(aLifePct) - 1.0) * (aParticleDef->mValues[PIParticleDef::VALUE_SPIN].GetValueAt(mFrameNum) + aParticleInstance->mVariationValues[PIParticleInstance::VARIATION_SPIN])) / anUpdateRate;
		aParticleInstance = aNext;
	}
}

void PIEffect::Update() //4028-4208
{
	if (!mError.length())
		return;
	mUpdateCnt++;
	bool isFirstFrame = mFrameNum == 0.0;
	if (mWantsSRand) //Not in H5
	{
		if (mRandSeeds.size())
			mRand.SRand(mRandSeeds[Rand() % mRandSeeds.size()]);
		else
			mRand.SRand(Rand());
		mWantsSRand = false;
	}
	if (isFirstFrame && mStartupState.GetDataLen() != NULL)
	{
		mStartupState.SeekFront();
		LoadState(mStartupState, true);
		mWantsSRand = false;
		return;
	}
	bool doOneFrame = true;
	while (mFrameNum < mFirstFrameNum || doOneFrame)
	{
		doOneFrame = false;
		mCurNumEmitters = 0;
		mCurNumParticles = 0;
		float anUpdateRate = 1000.0 / mAnimSpeed;
		float aPrevFrameI = mFrameNum;
		if (isFirstFrame)
			mFrameNum += 0.0001;
		else
			mFrameNum += mFramerate / anUpdateRate;
		mIsNewFrame = aPrevFrameI != mFrameNum;
		for (int aLayerIdx = 0; aLayerIdx < mDef->mLayerDefVector.size(); ++aLayerIdx)
		{
			PILayerDef* aLayerDef = &mDef->mLayerDefVector[aLayerIdx];
			PILayer* aLayer = &mLayerVector[aLayerIdx];
			if (aLayer->mVisible)
			{
				for (int aDeflectorIdx = 0; aDeflectorIdx < aLayerDef->mDeflectorVector.size(); aDeflectorIdx++)
				{
					PIDeflector* aDeflector = &aLayerDef->mDeflectorVector[aDeflectorIdx];
					SexyTransform2D aTransform;
					float aDeflectorAng = aDeflector->mAngle.GetValueAt(mFrameNum);
					if (aDeflectorAng != 0)
						aTransform.RotateDeg(aDeflectorAng);
					FPoint aDeflectorPos = aDeflector->mPos.GetValueAt(mFrameNum);
					aTransform.Translate(aDeflectorPos.mX, aDeflectorPos.mY);
					FPoint anOffset = aLayerDef->mOffset.GetValueAt(mFrameNum);
					aTransform.Translate(anOffset.mX, anOffset.mY);
					float anAngle = aLayerDef->mAngle.GetValueAt(mFrameNum);
					if (anAngle != 0)
						aTransform.RotateDeg(anAngle);
					aTransform = mDrawTransform * aTransform;
					for (int i = 0; i < aDeflector->mPoints.size(); i++)
						aDeflector->mCurPoints[i] = TransformFPoint(aTransform, aDeflector->mPoints[i].GetValueAt(mFrameNum));
					for (int aForceIdx = 0; aForceIdx < aLayerDef->mForceVector.size(); aForceIdx++)
					{
						PIForce* aForce = &aLayerDef->mForceVector[aForceIdx];
						SexyTransform2D aTransform;
						aTransform.Scale(aForce->mWidth.GetValueAt(mFrameNum) / 2.0, aForce->mHeight.GetValueAt(mFrameNum) / 2.0);
						float aForceAngle = aForce->mAngle.GetValueAt(mFrameNum);
						if (aForceAngle != 0)
							aTransform.RotateDeg(aForceAngle);
						FPoint aForcePos = aForce->mPos.GetValueAt(mFrameNum);
						aTransform.Translate(aForcePos.mX, aForcePos.mY);
						FPoint anOffset = aLayerDef->mOffset.GetValueAt(mFrameNum);
						aTransform.Translate(anOffset.mX, anOffset.mY);
						float anAngle = aLayerDef->mAngle.GetValueAt(mFrameNum);
						if (anAngle != 0)
							aTransform.RotateDeg(anAngle);
						aTransform = mDrawTransform * aTransform;
						FPoint aPoints[5];
						aPoints[0] = FPoint(-1.0, -1.0);
						aPoints[1] = FPoint(1.0, -1.0);
						aPoints[2] = FPoint(1.0, 1.0);
						aPoints[3] = FPoint(-1.0, 1.0);
						aPoints[4] = FPoint(0.0, 0.0);
						for (int aPtIdx = 0; aPtIdx < 5; aPtIdx++)
							aForce->mCurPoints[aPtIdx] = TransformFPoint(aTransform, aPoints[aPtIdx]);
					}
					for (int anEmitterInstanceIdx = 0; anEmitterInstanceIdx < aLayer->mEmitterInstanceVector.size(); anEmitterInstanceIdx++)
					{
						PIEmitterInstanceDef* anEmitterInstanceDef = &aLayerDef->mEmitterInstanceDefVector[anEmitterInstanceIdx];
						PIEmitterInstance* anEmitterInstance = &aLayer->mEmitterInstanceVector[anEmitterInstanceIdx];
						int anEmitterCount = 0;
						int aParticleCount = 0;
						int anIterationsLeft = 1;
						while (anEmitterInstance->mVisible && anIterationsLeft > 0)
						{
							anEmitterCount = 0;
							aParticleCount = 0;
							anIterationsLeft--;
							bool isActive = anEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ACTIVE].GetLastKeyframe(mFrameNum) > 0.99;
							if (!isActive)
								anIterationsLeft = 0;
							else if (!anEmitterInstance->mWasActive)
								anIterationsLeft += (anEmitterInstanceDef->mFramesToPreload * anUpdateRate / mFramerate);
							anEmitterInstance->mWasActive = isActive;
							float aFirstTime = anEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ACTIVE].GetNextKeyframeTime(0.0);
							float aLastTime = anEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ACTIVE].GetLastKeyframeTime(mLastFrameNum + 1.0);
							float aLastValue = anEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ACTIVE].GetLastKeyframe(mLastFrameNum + 1.0);
							anEmitterInstance->mWithinLifeFrame = mFrameNum >= aFirstTime && (mFrameNum < aLastTime || aLastValue > 0.99) && (mEmitAfterTimeline || mFrameNum < mLastFrameNum);
							if (isActive || anEmitterInstanceDef->mIsSuperEmitter && anEmitterInstance->mWithinLifeFrame)
								anEmitterCount++;
							if (anEmitterInstanceDef->mIsSuperEmitter)
							{
								for (int aFreeEmitterIdx = 0; aFreeEmitterIdx < anEmitterInstanceDef->mFreeEmitterIndices.size(); aFreeEmitterIdx++)
								{
									PIEmitter* anEmitter = mDef->mEmitterVector[anEmitterInstanceDef->mFreeEmitterIndices[aFreeEmitterIdx]];
									PIParticleDefInstance* aParticleDefInstance = &anEmitterInstance->mSuperEmitterParticleDefInstanceVector[aFreeEmitterIdx];
									UpdateParticleDef(aLayer, anEmitter, anEmitterInstance, NULL, aParticleDefInstance, &anEmitterInstance->mSuperEmitterGroup, NULL);
								}
								UpdateParticleGroup(aLayer, anEmitterInstance, &anEmitterInstance->mSuperEmitterGroup);
								PIFreeEmitterInstance* aChildEmitterInstance = (PIFreeEmitterInstance*)anEmitterInstance->mSuperEmitterGroup.mHead; //?
								while (aChildEmitterInstance != NULL)
								{
									PIFreeEmitterInstance* aNext = (PIFreeEmitterInstance*)aChildEmitterInstance->mNext;
									PIEmitter* anEmitter = aChildEmitterInstance->mEmitterSrc;
									for (int aParticleDefIdx = 0; aParticleDefIdx < anEmitter->mParticleDefVector.size(); aParticleDefIdx++)
									{
										PIParticleDef* aParticleDef = &anEmitter->mParticleDefVector[aParticleDefIdx];
										PIParticleDefInstance* aParticleDefInstance = &aChildEmitterInstance->mEmitter.mParticleDefInstanceVector[aParticleDefIdx];
										UpdateParticleDef(aLayer, anEmitter, anEmitterInstance, aParticleDef, aParticleDefInstance, &aChildEmitterInstance->mEmitter.mParticleGroup, aChildEmitterInstance);
									}
									UpdateParticleGroup(aLayer, anEmitterInstance, &aChildEmitterInstance->mEmitter.mParticleGroup);
									aParticleCount += aChildEmitterInstance->mEmitter.mParticleGroup.mCount;
									anEmitterCount++;
									aChildEmitterInstance = aNext;
								}
							}
							else
							{
								PIEmitter* anEmitter = mDef->mEmitterVector[anEmitterInstanceDef->mEmitterDefIdx];
								for (int aParticleDefIdx = 0; aParticleDefIdx < anEmitter->mParticleDefVector.size(); aParticleDefIdx++)
								{
									PIParticleGroup* aParticleGroup = &anEmitterInstance->mParticleGroup;
									PIParticleDef* aParticleDef = &anEmitter->mParticleDefVector[aParticleDefIdx];
									PIParticleDefInstance* aParticleDefInstance = &anEmitterInstance->mParticleDefInstanceVector[aParticleDefIdx];
									UpdateParticleDef(aLayer, anEmitter, anEmitterInstance, aParticleDef, aParticleDefInstance, aParticleGroup, NULL);
								}
								UpdateParticleGroup(aLayer, anEmitterInstance, &anEmitterInstance->mParticleGroup);
								aParticleCount += anEmitterInstance->mParticleGroup.mCount;
							}
						}
						mCurNumEmitters += anEmitterCount;
						mCurNumParticles += aParticleCount;
					}
				}
			}
		}
		isFirstFrame = false;
	}
}

void PIEffect::DrawParticleGroup(Graphics* g, PILayer* theLayer, PIEmitterInstance* theEmitterInstance, PIParticleGroup* theParticleGroup, bool isDarkeningPass) //Split in XNA, Correct? | 4211-4331
{
	if (!theEmitterInstance->mWasActive)
		return;
	Color aColorMult(theLayer->mColor.mRed * mColor.mRed / 255, theLayer->mColor.mGreen * mColor.mGreen / 255, theLayer->mColor.mBlue * mColor.mBlue / 255, theLayer->mColor.mAlpha * mColor.mAlpha / 255); //?
	bool hasColor = aColorMult != Color::White;
	PIParticleInstance* aParticleInstance = theParticleGroup->mHead;
	while (aParticleInstance != NULL)
	{
		PIParticleInstance* aNext = aParticleInstance->mNext;
		float aLifePct = aParticleInstance->mLifePct;
		PIParticleDef* aParticleDef = aParticleInstance->mParticleDef;
		if (aParticleDef->mIntense && aParticleDef->mPreserveColor || !isDarkeningPass)
		{
			PIEmitter* anEmitter = aParticleInstance->mEmitterSrc;
			float anEmitterLifePct = 0.0;
			if (aParticleInstance->mParentFreeEmitter)
				anEmitterLifePct = aParticleInstance->mParentFreeEmitter->mLifePct;
			if (!aParticleDef->mIntense || isDarkeningPass)
				g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
			else
				g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
			float aTintPct;
			PITexture* aTexture = mDef->mTextureVector[aParticleDef->mTextureIdx];
			DeviceImage* anImage;
			Rect aSrcRect;
			if (aTexture->mImageVector.size() > 0)
			{
				anImage = aTexture->mImageVector[aParticleInstance->mImgIdx];
				aSrcRect = Rect(0, 0, anImage->mWidth, anImage->mHeight);
			}
			else
			{
				anImage = aTexture->mImageStrip;
				aSrcRect = anImage->GetCelRect(aParticleInstance->mImgIdx);
			}
			ulong aColorI;
			if (aParticleDef->mRandomGradientColor)
			{
				if (aParticleDef->mUseKeyColorsOnly)
				{
					int aKeyframe = (int)min(aParticleDef->mColor.mInterpolatorPointVector.size() * aParticleInstance->mGradientRand, (int)aParticleDef->mColor.mInterpolatorPointVector.size() - 1); //Max on H5 (min in C++?)
					aColorI = aParticleDef->mColor.GetKeyframeNum(aKeyframe);
				}
				else
				{
					float aColorPosUsed = aParticleInstance->mGradientRand;
					aColorI = aParticleDef->mColor.GetKeyframeNum(aColorPosUsed);
				}
			}
			else if (aParticleDef->mUseNextColorKey)
			{
				int aKeyframe = aParticleInstance->mNum / aParticleDef->mNumberOfEachColor % aParticleDef->mColor.mInterpolatorPointVector.size();
				aColorI = aParticleDef->mColor.GetKeyframeNum(aKeyframe);
			}
			else
			{
				float aColorPosUsed = WrapFloat(aLifePct, aParticleDef->mRepeatColor + 1);
				aColorI = aParticleDef->mColor.GetValueAt(aColorPosUsed);
			}
			if (aParticleDef->mGetColorFromLayer)
				aColorI = (aColorI & 0xFF000000) | (aParticleInstance->mBkgColor & 0xFFFFFF);
			if (aParticleDef->mGetTransparencyFromLayer)
				aColorI = (aColorI & 0xFFFFFF) | (aParticleInstance->mBkgColor & 0xFF000000);
			aTintPct = theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_TINT_STRENGTH].GetValueAt(mFrameNum) * anEmitter->mValues[PIEmitter::VALUE_TINT_STRENGTH].GetValueAt(mFrameNum, 1.0);
			aColorI = InterpColor(aColorI, theEmitterInstance->mTintColor.ToInt(), aTintPct);
			ulong anAlpha = aParticleDef->mAlpha.GetValueAt(WrapFloat(aLifePct, aParticleDef->mRepeatAlpha + 1));
			anAlpha = ((int)((float)anAlpha * (theEmitterInstance->mEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_VISIBILITY].GetValueAt(mFrameNum) * aParticleDef->mValues[PIParticleDef::VALUE_VISIBILITY].GetValueAt(mFrameNum) * anEmitter->mValues[PIEmitter::VALUE_VISIBILITY].GetValueAt(mFrameNum, 1.0))));
			if (isDarkeningPass)
				aColorI &= 0xFF000000;
			Color aColor = aColor.FromInt((anAlpha << 24) | aColorI & 0xFFFFFF);
			if (hasColor)
				aColor = (aColorMult.mRed * aColor.mRed / 255, aColorMult.mGreen* aColor.mGreen / 255, aColorMult.mBlue* aColor.mBlue / 255, aColorMult.mAlpha* aColor.mAlpha / 255);
			if (aColor.mAlpha != 0) //C++ only
			{
				g->SetColor(aColor);
				Transform aTransform;
				SexyTransform2D mat = mDrawTransform * aParticleInstance->mTransform.GetMatrix();
				aTransform.SetMatrix(mat);
				g->DrawImageTransform(anImage, aTransform, aSrcRect);
				mLastDrawnPixelCount = fabs(aParticleInstance->mTransformScaleFactor * anImage->mHeight * anImage->mWidth + mLastDrawnPixelCount);
				aParticleInstance->mHasDrawn = true;
			}
			aParticleInstance = aNext;
		}
		else
			aParticleInstance = aNext;
	}
}

void PIEffect::DrawLayer(Graphics* g, PILayer* theLayer) //TODO | 4334-4531
{
	//g->PushState();
	//g->SetColorizeImages(true);
	//PILayerDef* aLayerDef = theLayer->mLayerDef;
	//for (int anEmitterInstanceIdx = 0; anEmitterInstanceIdx < theLayer->mEmitterInstanceVector.size(); anEmitterInstanceIdx++)
	//{
	//	PIEmitterInstanceDef* anEmitterInstanceDef = &aLayerDef->mEmitterInstanceDefVector[anEmitterInstanceIdx];
	//	PIEmitterInstance* anEmitterInstance = &theLayer->mEmitterInstanceVector[anEmitterInstanceIdx];
	//	if (!anEmitterInstance->mVisible)
	//		continue;
	//	for (int aPass = 0; aPass < 2; aPass++)
	//	{
	//		if (aPass == 0)
	//			continue;
	//		if (anEmitterInstanceDef->mIsSuperEmitter)
	//		{
	//			for (int aFreeEmitterIdx = 0; aFreeEmitterIdx < anEmitterInstanceDef->mFreeEmitterIndices.size(); aFreeEmitterIdx++)
	//			{
	//				PIFreeEmitterInstance* aChildEmitterInstance = (PIFreeEmitterInstance*)anEmitterInstance->mSuperEmitterGroup.mHead;
	//				while (aChildEmitterInstance != NULL)
	//				{
	//					DrawParticleGroup(g, theLayer, anEmitterInstance, &aChildEmitterInstance->mEmitter.mParticleGroup, aPass == 0);
	//					if (mDebug) //Not in H5 or XNA
	//					{
	//						g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	//						/*FPoint aPt = TransformFPoint(aChildEmitterInstance->mTransform.GetMatrix(), FPoint(0.0, 0.0)); //?
	//						SexyTransform2D aTransform = mDrawTransform * aChildEmitterInstance->mTransform.GetMatrix(); //?
	//						g->SetColor(Color::Black);
	//						g->DrawRect(aPt.mX - 3.0, aPt.mY - 3.0, 3, 3);
	//						g->SetColor(Color::White);
	//						g->DrawRect(aPt.mX - 2.0, aPt.mY - 2.0, 3, 3);*/
	//					}						
	//					aChildEmitterInstance = (PIFreeEmitterInstance*)aChildEmitterInstance->mNext;
	//				}
	//			}
	//		}
	//		else
	//			DrawParticleGroup(g, theLayer, anEmitterInstance, &anEmitterInstance->mParticleGroup, aPass == 0);
	//	}
	//}
	////Not in H5 (DrawPhisycalLayer [typo] in XNA)
	//g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	//for (int aBlockerIdx = 0; aBlockerIdx < aLayerDef->mBlockerVector.size(); aBlockerIdx++)
	//{
	//	PIBlocker* aBlocker = &aLayerDef->mBlockerVector[aBlockerIdx];
	//	bool active = aBlocker->mActive.GetLastKeyframe(mFrameNum) > 0.99;
	//	if (!mDebug && !active)
	//		continue;
	//	SexyTransform2D aTransform;
	//	float aBlockerAng = aBlocker->mAngle.GetValueAt(mFrameNum);
	//	if (aBlockerAng != 0.0)
	//		aTransform.RotateDeg(aBlockerAng);
	//	FPoint aBlockerPos = aBlocker->mPos.GetValueAt(mFrameNum);
	//	aTransform.Translate(aBlockerPos.mX, aBlockerPos.mY);
	//	FPoint anOffset = aLayerDef->mOffset.GetValueAt(mFrameNum);
	//	aTransform.Translate(anOffset.mX, anOffset.mY);
	//	float anAngle = aLayerDef->mAngle.GetValueAt(mFrameNum);
	//	if (anAngle != 0.0)
	//		aTransform.RotateDeg(anAngle);
	//	//Transform aTrans = mDrawTransform * aTransform; //?
	//	FPoint aPoints[512];
	//	int aNumPoints = min(512, aBlocker->mPoints.size()); //?
	//	for (int aPtIdx = 0; aPtIdx < aNumPoints; aPtIdx++) //?
	//		FPoint aPt = TransformFPoint(aTransform, aBlocker->mPoints[aPtIdx].GetValueAt(mFrameNum)); //?
	//
	//	FPoint aTris[3][256];
	//	int aNumTris = 0;
	//	DividePoly(aPoints, aNumPoints, aTris, 256, &aNumTris); //?
	//	if (active)
	//	{
	//		for (int i = 0; i < aNumTris; i++)
	//		{
	//			if (theLayer->mBkgImage != NULL)
	//			{
	//				SexyVertex2D aTri[3];
	//				for (int aPtIdx = 0; aPtIdx < 3; aPtIdx++)
	//				{
	//					//memcpy(aTris[i, aPtIdx].mX, aTris[i, aPtIdx].mY, (aTris[i, aPtIdx].mX + theLayer.mBkgImgDrawOfs.mX) / (float)theLayer.mBkgImage.mWidth, (aTris[i, aPtxIdx].Y + //theLayer.mBkgImgDrawOfs.mY) / (float)theLayer.mBkgImage.mHeight) //?
	//				}
	//				g->SetColor(Color::White);
	//				//g->DrawTriangleTex(theLayer->mBkgImage, ) //?
	//			}
	//			else
	//			{
	//				Point aTri[3];
	//				for (int aPtIdx = 0; aPtIdx < 3; aPtIdx++)
	//				{
	//					//aTri[aPtIdx].mY = //?
	//				}
	//				g->SetColor(mBkgColor);
	//				g->PolyFill(aTri, 3, true); //C++ only
	//			}
	//		}
	//	}
	//	if (mDebug)
	//	{
	//		g->SetColor(active ? (0, 255, 255) : (0, 64, 64));
	//		for (int aPtIdx = 0; aPtIdx < 3; aPtIdx++)
	//		{
	//			FPoint aPt1 = aPoints[aPtIdx];
	//			FPoint aPt2 = aPoints[aPtIdx + 1 % aPtIdx];
	//			g->DrawLine(aPt1.mX, aPt1.mY, aPt2.mX, aPt2.mY);
	//		}
	//	}
	//}
	//for (int aDeflectorIdx = 0; aDeflectorIdx < aLayerDef->mDeflectorVector.size(); aDeflectorIdx++)
	//{
	//	PIDeflector* aDeflector = &aLayerDef->mDeflectorVector[aDeflectorIdx];
	//	bool active = aDeflector->mActive.GetLastKeyframe(mFrameNum) < 0.99;
	//	if (!aDeflector->mVisible || active && !mDebug)
	//		continue;
	//	g->SetColor(active ? (255, 0, 0) : (64, 0, 0));
	//	for (int aPtIdx = 1; aPtIdx < aDeflector->mCurPoints.size(); aPtIdx++)
	//	{
	//		FPoint aPt1 = aDeflector->mCurPoints[aPtIdx - 1];
	//		FPoint aPt2 = aDeflector->mCurPoints[aPtIdx];
	//		if (aDeflector->mThickness <= 1.5)
	//		{
	//			g->DrawLine((int)aPt1.mX, (int)aPt1.mY, (int)aPt2.mX, (int)aPt2.mY);
	//			continue;
	//		}
	//		//SexyVector2 aLineDir = SexyVector2(aPt2.mX - aPt1.mX, aPt2.mY - aPt1.mY).Normalize().Perp();
	//		//FPoint aPt = TransformFPoint(FPoint(aLineDir.x, aLineDir.y), mDrawTransform, aPt)
	//	}
	//}
}

void PIEffect::Draw(Graphics* g) //4534-4557
{
	SEXY_PERF_BEGIN("PIEffect::Draw"); //ACCORDING TO BLITZ A PERF IS HERE
	mLastDrawnPixelCount = 0;
	for (int aLayerIdx = 0; aLayerIdx < mDef->mLayerDefVector.size(); ++aLayerIdx)
	{
		PILayer* aLayer = &mLayerVector[aLayerIdx];
		if (aLayer->mVisible)
			DrawLayer(g, aLayer);
	}
	if (mDebug) //C++ only
	{
		g->PushState();
		g->SetColor(Color::Black);
		g->FillRect(-4, -4, 8, 8);
		g->SetColor(Color::White);
		g->FillRect(-3, -3, 6, 6);
		g->PopState();
	}
	mLastDrawnPixelCount = (GetMatrixScale(mDrawTransform) * mLastDrawnPixelCount);
	SEXY_PERF_END("PIEffect::Draw");
}

bool PIEffect::HasTimelineExpired() //4560-4562 (Matched)
{
	return mFrameNum >= mLastFrameNum; //Cast double needed?
}

bool PIEffect::IsActive() //4565-4594
{
	for (int aLayerIdx = 0; aLayerIdx < mDef->mLayerDefVector.size(); aLayerIdx++)
	{
		PILayerDef* aLayerDef = &mDef->mLayerDefVector[aLayerIdx];
		PILayer* aLayer = &mLayerVector[aLayerIdx];
		if (aLayer->mVisible)
		{
			for (int anEmitterInstanceIdx = 0; anEmitterInstanceIdx < aLayer->mEmitterInstanceVector.size(); anEmitterInstanceIdx++)
			{
				PIEmitterInstanceDef* anEmitterInstanceDef = &aLayerDef->mEmitterInstanceDefVector[anEmitterInstanceIdx];
				PIEmitterInstance* anEmitterInstance = &aLayer->mEmitterInstanceVector[anEmitterInstanceIdx];
				if (anEmitterInstance->mVisible)
				{
					if (anEmitterInstanceDef->mValues[PIEmitterInstanceDef::VALUE_ACTIVE].GetNextKeyframeTime(mFrameNum) >= mFrameNum)
						return true;
					if (anEmitterInstance->mWithinLifeFrame)
						return true;
					if (anEmitterInstance->mSuperEmitterGroup.mHead != NULL)
						return true;
					if (anEmitterInstance->mParticleGroup.mHead != NULL)
						return true;
				}
			}
		}
	}
	return false;
}

std::string PIEffect::GetNotesParam(const std::string& theName, const std::string& theDefault) //4597-4602
{
	DefinesMap::iterator anItr = mNotesParams.find(Upper(theName));
	if (anItr != mNotesParams.end())
		return anItr->second;
	else
		return theDefault;
}

bool PIEffect::CheckCache() //Not in H5, dummied in XNA | 4605-4607
{
	return gSexyCache.CheckData(GetAppFullPath(mSrcFileName), "PIEffect0");
}

bool PIEffect::SetCacheUpToDate() //Not in H5, dummied in XNA | 4610-4612
{
	return gSexyCache.SetUpToDate(GetAppFullPath(mSrcFileName), "PIEffect0");
}

void PIEffect::WriteToCache() //4615-4627
{
	if (!gSexyCache.Connected())
		return;

	void* thePtr = gSexyCache.AllocSetData(GetAppFullPath(mSrcFileName), "PIEffect0", 4);
	if (thePtr)
	{
		gSexyCache.SetData(thePtr);
		gSexyCache.FreeSetData(thePtr);
		gSexyCache.SetFileDeps(GetAppFullPath(mSrcFileName), "PIEffect0", GetAppFullPath(mSrcFileName));
	}
}

