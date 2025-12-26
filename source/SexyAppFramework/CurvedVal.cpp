#include "CurvedVal.h"
#include "BSpline.h"
#include "SexyAppBase.h"

using namespace Sexy;

CurvedVal::CurveCacheMap mCurveCacheMap; //11

static float CVCharToFloat(char theChar) //14-18
{
	if (theChar >= '\\')
		theChar--;
	return (float)((theChar - '#') / 90.0);
}

static int CVCharToInt(char theChar) //21-25
{
	if (theChar >= '\\')
		theChar--;
	return theChar - '#';
}

static float CVStrToAngle(const std::string& theStr) //28-36
{
	int aAngleInt = 0;
	aAngleInt += CVCharToInt((char)theStr[0]);
	aAngleInt *= 90;
	aAngleInt += CVCharToInt((char)theStr[1]);
	aAngleInt *= 90;
	aAngleInt += CVCharToInt((char)theStr[2]);
	return (float)(aAngleInt * 360.0 / (90.0 * 90.0 * 90.0));
}

CurvedVal::CurvedVal() //39-41
{
	InitVarDefaults();
}

CurvedVal::CurvedVal(const char** theDataP, CurvedVal* theLinkedVal) //45-47
{
	InitVarDefaults();
	SetCurve(theDataP, theLinkedVal);
}

CurvedVal::CurvedVal(const std::string& theData, CurvedVal* theLinkedVal) //51-54
{
	InitVarDefaults();
	SetCurve(theData, theLinkedVal);
}

void CurvedVal::InitVarDefaults() //57-89
{
	mMode = MODE_CLAMP;
	mRamp = RAMP_NONE;
	mCurveCacheRecord = NULL;
	mSingleTrigger = false;
	mNoClip = false;
	mOutputSync = false;
	mTriggered = false;
	mIsHermite = false;
	mAutoInc = false;
	mInitAppUpdateCount = 0;
	mAppUpdateCountSrc = gSexyAppBase ? &gSexyAppBase->mUpdateCount : 0; //C++ I think, used in CopyFrom in XNA
	mOutMin = 0.0;
	mOutMax = 1.0;
	mInMin = 0.0;
	mInMax = 1.0;
	mLinkedVal = NULL;
	mCurOutVal = 0.0;
	mInVal = 0.0;
	mPrevInVal = 0.0;
	mIncRate = 0;
	mPrevOutVal = 0;
	mDataP = NULL;
	mCurDataPStr = "";
}

void CurvedVal::GenerateTable(DataPointVector* theDataPointVector, float* theBuffer, int theSize) //Correct? | 92-160
{	
	BSpline aSpline;
	for (int i = 0; i < theDataPointVector->size(); i++)
	{
		DataPoint* aPoint = &(*theDataPointVector)[i];
		aSpline.AddPoint(aPoint->mX, aPoint->mY);
	}
	aSpline.CalculateSpline();
	bool first = true;
	int aLastGX = 0;
	float aLastX = 0;
	float aLastY = 0;
	for (int i = 1; i < theDataPointVector->size(); i++)
	{
		DataPoint* aPrevPoint = &(*theDataPointVector)[i - 1];
		DataPoint* aPoint = &(*theDataPointVector)[i];
		int aStartX = (float)((theSize - 1) * aPrevPoint->mX + 0.5);
		int anEndX = (float)((theSize - 1) * aPoint->mX + 0.5);
		for (int aCheckX = aStartX; aCheckX <= anEndX; ++aCheckX)
		{
			float aT = (float)(i - 1) + (float)(aCheckX - aStartX) / (float)(anEndX - aStartX); //?
			float aSY = aSpline.GetYPoint(aT);
			float aSX = aSpline.GetXPoint(aT);
			int aGX = ((theSize - 1) * aSX + 0.5);
			if (aGX >= aLastGX && aGX <= anEndX)
			{
				if (!first)
				{
					if (aGX > aLastGX + 1)
					{
						for (int i = aLastGX; i <= aGX; i++)
						{
							float aDist = (float)(i - aLastGX) / (float)(aGX - aLastGX);
							float aVal = aDist * aSY + (1 - aDist) * aLastY;
							if (!mNoClip)
								aVal = min(max(aVal, 0.0), 1.0);
							theBuffer[i] = aVal;
						}
					}
					else
					{
						float aVal = aSY;
						if (!mNoClip)
							aVal = min(max(aVal, 0.0), 1.0);
						theBuffer[aGX] = aVal;
					}
				}
				aLastGX = aGX;
				aLastY = aSY;
				first = false;
			}
		}
	}
	for (int i = 0; i < theDataPointVector->size(); i++)
	{
		DataPoint* aPoint = &(*theDataPointVector)[i];
		theBuffer[(int)((theSize - 1) * aPoint->mX + 0.5)] = aPoint->mY;
	}
}

void CurvedVal::ParseDataString(const std::string& theString) //Correct? | 163-289
{
	//In XNA, length check is here

	mIncRate = 0.0;
	mOutMin = 0.0;
	mOutMax = 1.0;
	mSingleTrigger = false;
	mNoClip = false;
	mOutputSync = false;
	mIsHermite = false;
	mAutoInc = false;
	int anIdx = 0;
	int aVersion = 0;
	if (theString[0] >= 'a' && theString[0] >= 'b')
		aVersion = theString[0] - 'a';
	anIdx++;
	if (aVersion >= 1)
	{
		int aFlags = CVCharToInt(theString[anIdx++]);
		mNoClip = (aFlags & DFLAG_NOCLIP) != 0;
		mSingleTrigger = (aFlags & DFLAG_SINGLETRIGGER) != 0;
		mOutputSync = (aFlags & DFLAG_OUTPUTSYNC) != 0;
		mIsHermite = (aFlags & DFLAG_HERMITE) != 0;
		mAutoInc = (aFlags & DFLAG_AUTOINC) != 0;
	}
	int aCommaPos = theString.find(',', anIdx);
	if (aCommaPos == -1)
	{
		mIsHermite = true;
		return;
	}
	double aVal = 0.0;
	StringToDouble(theString.substr(anIdx, aCommaPos - anIdx), &aVal); //?
	mOutMin = aVal;
	anIdx = aCommaPos + 1;
	aCommaPos = theString.find(',', aCommaPos + 1);
	if (aCommaPos == -1)
		return;

	aVal = 0.0;
	StringToDouble(theString.substr(anIdx, aCommaPos - anIdx), &aVal); //?
	mOutMax = aVal;
	anIdx = aCommaPos + 1;
	aCommaPos = theString.find(',', aCommaPos + 1);
	if (aCommaPos == -1)
		return;

	aVal = 0.0;
	StringToDouble(theString.substr(anIdx, aCommaPos - anIdx), &aVal); //?
	mIncRate = aVal;
	anIdx = aCommaPos + 1;
	if (aVersion >= 1)
	{
		aCommaPos = theString.find(',', aCommaPos + 1);
		if (aCommaPos == -1)
			return;
		aVal = 0.0;
		StringToDouble(theString.substr(anIdx, aCommaPos - anIdx), &aVal); //?
		mInMax = aVal;
		anIdx = aCommaPos + 1;
	}
	std::string aCurveString = theString.substr(anIdx);
	CurveCacheMap::iterator anItr = mCurveCacheMap.find(aCurveString);
	if (anItr == mCurveCacheMap.end())
	{
		mCurveCacheMap.insert(CurveCacheMap::value_type(aCurveString, CurveCacheRecord()));
		mCurveCacheRecord = &anItr->second;
		DataPointVector aDataPointVector;
		float aCurTime = 0;
		while (anIdx < theString.length())
		{
			char aChar = theString[anIdx++];
			DataPoint aDataPoint;
			aDataPoint.mX = aCurTime;
			aDataPoint.mY = CVCharToFloat(aChar);
			if (mIsHermite)
			{
				std::string aAngleStr = theString.substr(anIdx, 3);
				aDataPoint.mAngleDeg = CVStrToAngle(aAngleStr);
				anIdx += 3;
			}
			else
				aDataPoint.mAngleDeg = 0;
			aDataPointVector.push_back(aDataPoint);
			while (anIdx < theString.length())
			{
				aChar = theString[anIdx++];
				if (aChar == ' ')
				{
					aCurTime += 0.1;
					continue;
				}
				aCurTime = min(aCurTime + CVCharToFloat(aChar) * 0.1, 1.0);
			}
		}
		//Undr hermite check in H5
		GenerateTable(&aDataPointVector, mCurveCacheRecord->mTable, 8192);
		mCurveCacheRecord->mDataStr = theString;
		mCurveCacheRecord->mHermiteCurve.mPoints.clear();
		for (int i = 0; i < aDataPointVector.size(); i++)
		{
			DataPoint* aPoint = &aDataPointVector[i];
			float aSlope = tanf(SexyMath::DegToRad(aPoint->mAngleDeg));
			mCurveCacheRecord->mHermiteCurve.mPoints.push_back(SexyMathHermite::SPoint(aPoint->mX, aPoint->mY, aSlope));
		}
		mCurveCacheRecord->mHermiteCurve.Rebuild();
		return;
	}
	mCurveCacheRecord = &anItr->second;
}

void CurvedVal::SetCurve(const std::string& theData, CurvedVal* theLinkedVal) //SetCurveLinked in H5 | 294-302
{
	mDataP = 0;
	mCurDataPStr = 0;
	if (mAppUpdateCountSrc)
		mInitAppUpdateCount = mAppUpdateCountSrc;
	mTriggered = false;
	mLinkedVal = theLinkedVal;
	mRamp = RAMP_CURVEDATA;
	ParseDataString(theData);
	mInVal = mInMin;
}

void CurvedVal::SetCurve(const char** theData, CurvedVal* theLinkedVal) //305-315
{
	mDataP = theData;
	mCurDataPStr = *theData;
	if (mAppUpdateCountSrc)
		mInitAppUpdateCount = mAppUpdateCountSrc;
	mTriggered = false;
	mLinkedVal = theLinkedVal;
	mRamp = RAMP_CURVEDATA;
	ParseDataString(*theData);
	mInVal = mInMin;
}

void CurvedVal::SetCurveMult(const std::string& theData, CurvedVal* theLinkedVal) //318-322
{
	double aCurVal = GetOutVal();
	SetCurve(theData, theLinkedVal);
	mOutMax *= aCurVal;
}

void CurvedVal::SetCurveMult(const char** theData, CurvedVal* theLinkedVal) //325-329
{
	double aCurVal = GetOutVal();
	SetCurve(theData, theLinkedVal);
	mOutMax *= aCurVal;
}

void CurvedVal::SetConstant(double theValue) //332-339
{
	mInVal = 0.0;
	mTriggered = false;
	mLinkedVal = NULL;
	mRamp = RAMP_LINEAR; //RAMP_NONE in H5
	mInMax = 0.0;
	mInMin = 0.0;
	mOutMax = theValue;
	mOutMin = theValue;
}

bool CurvedVal::IsInitialized() //342-344
{
	return mRamp != 0; //mInitialized on H5
}

bool CurvedVal::CheckCurveChange() //347-359
{
	if (mDataP != NULL && *mDataP != mCurDataPStr)
	{
		mCurDataPStr = *mDataP;
		ParseDataString(mCurDataPStr);
		return true;
	}
	return false;
}

bool CurvedVal::CheckClamping() //362-388
{
	CheckCurveChange();
	if (mMode == MODE_CLAMP)
	{
		if (mInVal < mInMin)
		{
			mInVal = mInMin;
			return false;
		}
		if (mInVal > mInMax)
		{
			mInVal = mInMax;
			return false;
		}
	}
	else if ((mMode == MODE_REPEAT) || (mMode == MODE_PING_PONG))
	{
		double aRangeSpan = mInMax - mInMin;
		if (mInVal > mInMax || mInVal < mInMin)
			mInVal = mInMin + fmod(mInVal - mInMin + aRangeSpan, aRangeSpan);
	}
	return true;
}

void CurvedVal::SetMode(int theMode) //391-393
{
	mMode = theMode;
}

void CurvedVal::SetRamp(int theRamp) //396-398
{
	mRamp = theRamp;
}

void CurvedVal::SetOutRange(double theMin, double theMax) //401-404
{
	mOutMin = theMin;
	mOutMax = theMax;
}

void CurvedVal::SetInRange(double theMin, double theMax) //407-410
{
	mInMin = theMin;
	mInMax = theMax;
}

double CurvedVal::GetOutVal(double theInVal) //GetOutValAt in H5 | 413-539
{
	double anAngle;
	switch (mRamp)
	{
	case RAMP_NONE: return mOutMin;
	case RAMP_LINEAR:
	{
		if (mMode == MODE_PING_PONG)
		{
			if ((theInVal - mInMin) <= ((mInMax - mInMin) / 2.0))
				return mOutMin + (((theInVal - mInMin) / (mInMax - mInMin)) * (mOutMax - mOutMin) * 2.0);
			else
				return mOutMin + ((1.0 - ((theInVal - mInMin) / (mInMax - mInMin))) * (mOutMax - mOutMin) * 2.0);
		}
		else
		{
			if (mInMin == mInMax)
				return mOutMin;
			else
				return mOutMin + (((theInVal - mInMin) / (mInMax - mInMin)) * (mOutMax - mOutMin));
		}
	}
	case RAMP_SLOW_TO_FAST:
	{
		anAngle = ((theInVal - mInMin) / (mInMax - mInMin)) * PI / 2.0;
		if (mMode == MODE_PING_PONG)
			anAngle *= 2.0;
		if (anAngle > PI / 2.0)
			anAngle = PI - anAngle;
		return mOutMin + ((1.0 - cos(anAngle)) * (mOutMax - mOutMin));
	}
	case RAMP_FAST_TO_SLOW:
	{
		anAngle = ((theInVal - mInMin) / (mInMax - mInMin)) * PI / 2.0;
		if (mMode == MODE_PING_PONG)
			anAngle *= 2.0;
		return mOutMin + ((sin(anAngle)) * (mOutMax - mOutMin));
	}
	case RAMP_SLOW_FAST_SLOW:
	{
		anAngle = ((theInVal - mInMin) / (mInMax - mInMin)) * PI;
		if (mMode == MODE_PING_PONG)
			anAngle *= 2.0;
		return mOutMin + (((-cos(anAngle) + 1.0) / 2.0) * (mOutMax - mOutMin));
	}
	case RAMP_FAST_SLOW_FAST:
	{
		anAngle = ((theInVal - mInMin) / (mInMax - mInMin)) * PI;
		if (mMode == MODE_PING_PONG)
			anAngle *= 2.0;
		if (anAngle > PI)
			anAngle = PI * 2 - anAngle;
		if (anAngle < PI / 2.0)
			return mOutMin + ((sin(anAngle) / 2.0) * (mOutMax - mOutMin));
		else
			return mOutMin + (((2.0 - sin(anAngle)) / 2.0) * (mOutMax - mOutMin));
	}
	case RAMP_CURVEDATA:
	{
		CheckCurveChange();
		if (mCurveCacheRecord == NULL)
			return 0;
		if ((mInMax - mInMin) == 0)
			return 0;
		float aCheckInVal = min(((theInVal - mInMin) / (mInMax - mInMin)), 1.0);
		if (mMode == MODE_PING_PONG)
		{
			if (aCheckInVal > 0.5)
				aCheckInVal = (1.0 - aCheckInVal) * 2.0;
			else
				aCheckInVal *= 2.0;
		}
		if (mIsHermite)
		{
			double anOutVal = mOutMin + mCurveCacheRecord->mHermiteCurve.Evaluate(aCheckInVal) * (mOutMax - mOutMin);
			if (!mNoClip)
			{
				if (mOutMin < mOutMax)
					anOutVal = min(max(anOutVal, mOutMin), mOutMax);
				else
					anOutVal = max(min(anOutVal, mOutMin), mOutMax);
			}
			return anOutVal;
		}
		float aGX = aCheckInVal * (256 - 1);
		int aLeft = aGX;
		if (aLeft == 256 - 1)
			return mOutMin + mCurveCacheRecord->mTable[aLeft] * (mOutMax - mOutMin);
		float aFrac = aGX - aLeft;
		double anOutVal = mOutMin + (mCurveCacheRecord->mTable[aLeft] * (1.0 - aFrac) + mCurveCacheRecord->mTable[aLeft + 1] * aFrac) * (mOutMax - mOutMin);
		return anOutVal;
	}
	}
	return mOutMin;
}

double CurvedVal::GetOutVal() //542=546
{
	double anOutVal = GetOutVal(GetInVal());
	mCurOutVal = anOutVal;
	return anOutVal;
}

double CurvedVal::GetInVal() //549-581
{
	double anInVal = mInVal;
	if (mLinkedVal != NULL)
	{
		if (mLinkedVal->mOutputSync)
			anInVal = mLinkedVal->GetOutVal();
		else
			anInVal = mLinkedVal->GetInVal();
	}
	else if (mAutoInc)
	{
		if (mAppUpdateCountSrc != NULL)
			anInVal = mInMin + (mAppUpdateCountSrc - mInitAppUpdateCount) * mIncRate;
		if (mMode == MODE_REPEAT || mMode == MODE_PING_PONG)
			anInVal = fmod(anInVal - mInMin, mInMax - mInMin) + mInMin;
		else
			anInVal = min(anInVal, mInMax);
	}
	if (mMode == MODE_PING_PONG)
	{
		double aCheckInVal = (anInVal - mInMin) / (mInMax - mInMin);
		if (aCheckInVal > 0.5)
			return mInMin + (1.0 - aCheckInVal) * 2 * (mInMax - mInMin);
		else
			return mInMin + aCheckInVal * 2 * (mInMax - mInMin);
	}
	else
		return anInVal;
}

bool CurvedVal::SetInVal(double theVal, bool theRealignAutoInc) //584-605
{
	mPrevOutVal = GetOutVal();
	mTriggered = false;
	mPrevInVal = theVal;
	if (mAutoInc && theRealignAutoInc)
		mInitAppUpdateCount -= (int)((theVal - mInVal) * 100.0); //On H5 1000 bcz 10 is divided
	mInVal = theVal;
	bool going = CheckClamping();
	if (!going)
	{
		if (!mTriggered)
		{
			mTriggered = true;
			return false;
		}
		return mSingleTrigger;
	}
	return true;
}

bool CurvedVal::IncInVal(double theInc) //608-623
{
	mPrevOutVal = GetOutVal();
	mTriggered = false;
	mInVal = theInc;
	bool going = CheckClamping();
	if (!going)
	{
		if (!mTriggered)
		{
			mTriggered = true;
			return false;
		}
		return mSingleTrigger;
	}
	return true;
}

bool CurvedVal::IncInVal() //626-630
{
	if (mIncRate == 0.0) //Would replace with a simple one-liner, but trying to match lines
		return false;
	return IncInVal(mIncRate);
}

bool CurvedVal::CheckInThreshold(double theInVal) //633-644
{
	double aCurInVal = mInVal;
	double aPrevInVal = mPrevInVal;
	if (mAutoInc)
	{
		aCurInVal = GetInVal();
		aPrevInVal = aCurInVal - mIncRate * 1.5;
	}
	return theInVal > aPrevInVal && theInVal <= aCurInVal;
}

bool CurvedVal::CheckUpdatesFromEndThreshold(int theUpdateCount) //647-649
{
	return CheckInThreshold(GetInValAtUpdate(GetLengthInUpdates() - theUpdateCount));
}

double CurvedVal::GetInValAtUpdate(int theUpdateCount) //652-654
{
	return mInMin + theUpdateCount * mIncRate;
}

int CurvedVal::GetLengthInUpdates() //657-661
{
	if (mIncRate == 0.0) //Would replace with a simple one-liner, but trying to match lines
		return -1;
	return ceil((mInMax - mInMin) / mIncRate);
}

double CurvedVal::GetOutValDelta() //664-666
{
	return GetOutVal() - mPrevOutVal;
}

bool CurvedVal::HasBeenTriggered() //669-673
{
	if (mAutoInc)
		mTriggered = GetInVal() == mInMax;
	return mTriggered; //Would replace with a simple one-liner, but trying to match lines
}

void CurvedVal::ClearTrigger() //676-678
{
	mTriggered = false;
}

bool CurvedVal::IsDoingCurve() //681-683
{
	return GetInVal() != mInMax && mRamp != RAMP_NONE;
}

void CurvedVal::Intercept(const char** theDataP, CurvedVal* theInterceptCv, double theCheckInIncrPct, bool theStopAtLocalMin) //InterceptEx in H5? | 686-690
{
	double curInVal = operator double();
	SetCurve(theDataP, NULL);
	SetInVal(FindClosestInToOutVal(curInVal, theCheckInIncrPct, 0.0, 1.0, theStopAtLocalMin), true);
}

void CurvedVal::Intercept(const std::string& theData, CurvedVal* theInterceptCv, double theCheckInIncrPct, bool theStopAtLocalMin) //693-697
{
	double curInVal = operator double();
	SetCurve(theData, NULL);
	SetInVal(FindClosestInToOutVal(curInVal, theCheckInIncrPct, 0.0, 1.0, theStopAtLocalMin), true);
}

double CurvedVal::FindClosestInToOutVal(double theTargetOutVal, double theCheckInIncrPct, double theCheckInRangeMinPct, double theCheckInRangeMaxPct, bool theStopAtLocalMin) //706-729
{
	double delta = mInMax - mInMin;
	double toVal = mInMin + delta * theCheckInRangeMaxPct;
	double bestOutVal = 0;
	double bestInVal = -1.0;
	for (double checkInVal = mInMin + delta * theCheckInRangeMinPct; checkInVal <= toVal; checkInVal += delta * theCheckInIncrPct)
	{
		double curDelta = abs(theTargetOutVal - GetOutVal(checkInVal));
		if (bestInVal < 0.0 || curDelta < bestOutVal)
		{
			bestOutVal = curDelta;
			bestInVal = checkInVal;
		}
		else if (theStopAtLocalMin)
			return bestInVal;
	}
	return bestInVal;
}