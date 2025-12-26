#include "Bezier.h"
#include "Debug.h"

using namespace Sexy;

static float Distance(const FPoint& p1, const FPoint& p2, bool sqrt) //8-13
{
	float x = p2.mX - p1.mX;
	float y = p2.mY - p1.mY;
	float val = x * x + y * y;
	return sqrt? sqrtf(val) : val;
}

Bezier::Bezier() : //23-24
	mTimes(0),
	mLengths(0),
	mTotalLength(0.0),
	mCount(0),
	mControls(0),
	mPositions(0),
	mCurveDetail(-1)
{
}

Bezier::Bezier(const Bezier& rhs) : //32-34
	mTimes(rhs.mTimes),
	mLengths(rhs.mLengths),
	mTotalLength(rhs.mTotalLength),
	mCount(rhs.mCount),
	mControls(rhs.mControls),
	mPositions(rhs.mPositions)
{
	mCurveDetail = -1;
}

Bezier::~Bezier() //37-39
{
	Clean();
}

void Bezier::Clean() //42-50
{
	delete mTimes;
	delete mLengths;
	delete mControls;
	delete mPositions;

	mCount = 0;
	mTotalLength = 0.0;
}

Bezier& Bezier::operator=(const Bezier& rhs) //Correct? | 53-85
{
	if (this == &rhs) //?
		return *this;

	Clean();

	mCount = rhs.mCount;
	mTotalLength = rhs.mTotalLength;

	if (mCount > 0)
	{
		mTimes = new float[mCount];
		mPositions = new FPoint[mCount];
		mControls = new FPoint[2 * (mCount - 2)];
		mLengths = new float[mCount - 1];
		for (int i = 0; i < 2 * (mCount - 2); i++)
		{
			if (i < mCount)
			{
				mTimes[i] = rhs.mTimes[i];
				rhs.mPositions[i] = mPositions[i];
			}
			if (i < mCount - 1)
				mLengths[i] = rhs.mLengths[i];
			mControls[i] = rhs.mControls[i];
		}
	}
}

bool Bezier::Init(const FPoint* positions, const FPoint* controls, const float* times, int count) //88-125
{
	if (mCount != 0)
		return false;
	if (count < 2 || positions == NULL || times == NULL || controls == NULL)
		return false;
	mPositions = new FPoint[count];
	mControls = new FPoint[2 * (count - 1)];
	mTimes = new float[count];
	mCount = count;
	for (int i = 0; i < count; i++)
	{
		FPoint reference = mPositions[i];
		reference = positions[i];
		mTimes[i] = times[i];
	}
	for (int i = 0; i < 2 * (count - 1); i++)
	{
		FPoint reference2 = mControls[i];
		reference2 = controls[i];
	}
	mLengths = new float[count - 1];
	mTotalLength = 0.0;
	for (int i = 0; i < count - 1; i++)
	{
		mLengths[i] = SegmentArcLength(i, 0.0, 1.0);
		mTotalLength += mLengths[i];
	}
	return true;
}

bool Bezier::Init(const FPoint* positions, const float* times, int count) //Gross | 128-172
{
	if (mCount != 0)
		return false;
	if (count < 2 || positions == NULL || times == NULL) //Why
		return false;
	mPositions = new FPoint[count];
	mControls = new FPoint[2 * (count - 1)];
	mTimes = new float[count];
	mCount = count;
	for (int i = 0; i < count; i++)
	{
		mPositions[i] = positions[i];
		mTimes[i] = times[i];
	}
	for (int i = 0; i < count - 1; i++) //i_* in H5
	{
		if (i > 0)
			mControls[2 * i] = mPositions[i] + (mPositions[i + 1] - mPositions[i - 1]) / 3.0;
		if (i < count - 2)
			mControls[2 * i + 1] = mPositions[i + 1] - (mPositions[i + 2] - mPositions[i]) / 3.0;
	}
	mControls[0] = mControls[1] - (mPositions[1] - mPositions[0]) / 3.0;
	mControls[2 * count - 3] = mControls[2 * count - 4] + (mPositions[count - 1] - mPositions[count - 2]) / 3.0;
	mLengths = new float[count - 1];
	mTotalLength = 0.0;
	for (int i = 0; i < count - 1; i++)
	{
		mLengths[i] = SegmentArcLength(i, 0.0, 1.0);
		mTotalLength += mLengths[i];
	}
	return true;
}

FPoint Bezier::Evaluate(float t) //175-206
{
	DBG_ASSERT(mCount >= 2); //177 | 179 BejLiveWin8
	if (mCount < 2)
		return FPoint(0.0, 0.0);
	if (t <= mTimes[0])
		return mPositions[0];
	else if (t >= mTimes[mCount - 1])
		return mPositions[mCount - 1];
	int i = 0;
	for (i = 0; i < mCount - 1; ++i)
	{
		if (t < mTimes[i + 1])
			break;
	}
	float t0 = mTimes[i];
	float t1 = mTimes[i + 1];
	float u = (t - t0) / (t1 - t0);
	FPoint A = mPositions[i + 1] - mControls[2 * i + 1] * 3.0 + mControls[2 * i] * 3.0 - mPositions[i];
	FPoint B = mControls[2 * i + 1] * 3.0 - mControls[2 * i] * 6.0 + mPositions[i] * 3.0;
	FPoint C = mControls[2 * i] * 3.0 - mPositions[i] * 3.0;
	return FPoint(mPositions[i] + (C + (B + A * u) * u) * u);
}

FPoint Bezier::Velocity(float t, bool clamp) //209-247
{
	//Something
	DBG_ASSERT(mCount >= 2); //211 | 213 BejLiveWin8
	if (mCount < 2)
		return FPoint(0.0, 0.0);
	if (t <= mTimes[0])
	{
		if (!clamp)
			return FPoint(0.0, 0.0);
		return mPositions[0];
	}
	if (t >= mTimes[mCount - 1])
	{
		if (!clamp)
			return FPoint(0.0, 0.0);
		return mPositions[mCount - 1];
	}
	int i;
	for (i = 0; i < mCount - 1; ++i)
	{
		if (t < mTimes[i + 1])
			break;
	}
	float t0 = mTimes[i];
	float t1 = mTimes[i + 1];
	float u = (t - t0) / (t1 - t0);
	FPoint A = mPositions[i + 1] - mControls[2 * i + 1] * 3.0 + mControls[2 * i] * 3.0 - mPositions[i];
	FPoint B = mControls[2 * i + 1] * 6.0 - mControls[2 * i] * 12.0 + mPositions[i] * 6.0;
	FPoint C = mControls[2 * i] * 3.0 - mPositions[i] * 3.0;
	return FPoint(C + (B + A * u * 3.0) * u);
}

FPoint Bezier::Acceleration(float t) //250-280
{
	//Something
	DBG_ASSERT(mCount >= 2); //252
	if (mCount < 2)
		return FPoint(0.0, 0.0);
	if (t <= mTimes[0])
		return mPositions[0];
	if (t >= mTimes[mCount - 1])
		return mPositions[mCount - 1];
	int i;
	for (i = 0; i < mCount - 1; ++i)
	{
		if (t < mTimes[i + 1])
			break;
	}
	float t0 = mTimes[i];
	float t1 = mTimes[i + 1];
	float u = (t - t0) / (t1 - t0);
	FPoint A = mPositions[i + 1] - mControls[2 * i + 1] * 3.0 + mControls[2 * i] * 3.0 - mPositions[i];
	FPoint B = mControls[2 * i + 1] * 6.0 - mControls[2 * i] * 12.0 + mPositions[i] * 6.0;
	return (B + A * u * 6.0);
}

float Bezier::ArcLength(float t1, float t2) //283-328
{
	if (t2 <= t1)
		return 0.0;
	if (t1 < mTimes[0])
		t1 = mTimes[0];
	if (t2 > mTimes[mCount - 1])
		t2 = mTimes[mCount - 1];
	int seg1;
	for (seg1 = 0; seg1 < mCount - 1; ++seg1)
	{
		if (t1 < mTimes[seg1 + 1])
			break;
	}
	float u1 = (t1 - mTimes[seg1]) / (mTimes[seg1 + 1] - mTimes[seg1]);
	int seg2;
	for (seg2 = 0; seg2 < mCount - 1; ++seg2)
	{
		if (t1 < mTimes[seg2 + 1])
			break;
	}
	float u2 = (t2 - mTimes[seg2]) / (mTimes[seg2 + 1] - mTimes[seg2]);
	float result;
	if (seg1 == seg2)
		result = SegmentArcLength(seg1, u1, u2);
	else
	{
		result = SegmentArcLength(seg1, u1, 1.0);
		for (int i = seg1 + 1; i < seg2; i++)
			result += mLengths[i];
		result += SegmentArcLength(seg2, 0.0, u2);
	}
	return result;
}

float Bezier::SegmentArcLength(int i, float u1, float u2) //331-366
{
	DBG_ASSERT((i >= 0) && (i < mCount - 1)); //332
	if (u2 <= u1)
		return 0.0;
	if (u1 < 0.0)
		u1 = 0.0;
	if (u2 > 1.0)
		u2 = 1.0;
	FPoint P0 = mPositions[i];
	FPoint P1 = mControls[2 * i];
	FPoint P2 = mControls[2 * i + 1];
	FPoint P3 = mPositions[i + 1];
	float minus_u2 = 1.0 - u2;
	FPoint L1 = P0 * minus_u2 + P1 * u2;
	FPoint H = P1 * minus_u2 + P2 * u2;
	FPoint L2 = L1 * minus_u2 + H * u2;
	FPoint L3 = L2 * minus_u2 + (H * minus_u2 + (P2 * minus_u2 + P3 * u2) * u2) * u2;
	float minus_u1 = 1.0 - u1;
	H = L1 * minus_u1 + L2 * u1;
	FPoint R3 = L3;
	FPoint R2 = L2 * minus_u1 + L3 * u1;
	FPoint R1 = H * minus_u1 + R2 * u1;
	FPoint R0 = ((P0 * minus_u1 + L1 * u1) * minus_u1 + H * u1) * minus_u1 + R1 * u1;
	return SubdivideLength(R0, R1, R2, R3);
}

float Bezier::SubdivideLength(const FPoint& P0, const FPoint& P1, const FPoint& P2, const FPoint& P3) //369-388
{
	float Lmin = Distance(P0, P3, true); //In Common.h, in H5 no bool
	float Lmax = Distance(P0, P1, true) + Distance(P1, P2, true) + Distance(P2, P3, true);
	float diff = Lmin - Lmax;
	if (diff* diff < 0.001f) //0.001000000047497451?
		return 0.5 * (Lmin + Lmax);
	FPoint L1 = (P0 + P1) * 0.5;
	FPoint H = (P1 + P2) * 0.5;
	FPoint L2 = (L1 + H) * 0.5;
	FPoint R2 = (P2 + P3) * 0.5;
	FPoint R1 = (H + R2) * 0.5;
	FPoint mid = (L2 + R1) * 0.5;
	return SubdivideLength(P0, L1, L2, mid) + SubdivideLength(mid, L1, R2, P3);
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE
void Bezier::Serialize(Buffer* b) //Not in PC Prime
{
	b->WriteFloat(mTotalLength);
	b->WriteLong(mCount);
	b->WriteLong(mCurveDetail);
	b->WriteLong(mCurveColor);
	for (int i = 0; i < mCount; i++)
	{
		b->WriteFloat(mTimes[i]);
		b->WriteFloat(mPositions[i].mX);
		b->WriteFloat(mPositions[i].mY);
	}
	for (int j = 0; j < 2 * (mCount - 1); j++)
	{
		b->WriteFloat(mControls[j].mX);
		b->WriteFloat(mControls[j].mY);
	}
	for (int k = 0; k < mCount - 1; k++)
	{
		b->WriteFloat(mLengths[k]);
	}
}

void Bezier::Deserialize(Buffer* b) //Not in PC Prime
{
	Clean();
	mTotalLength = b->ReadFloat();
	mCount = b->ReadLong();
	mCurveDetail = b->ReadLong();
	mCurveColor = new Color(b->ReadLong());
	if (mCount > 0)
	{
		mTimes = new float[mCount];
		mPositions = new FPoint[mCount];
		mControls = new FPoint[2 * (mCount - 1)];
		mLengths = new float[mCount - 1];
		for (int i = 0; i < mCount; i++)
		{
			mTimes[i] = b->ReadFloat();
			mPositions[i].mX = b->ReadFloat();
			mPositions[i].mY = b->ReadFloat();
		}
		for (int j = 0; j < 2 * (mCount - 1); j++)
		{
			mControls[j].mX = b->ReadFloat();
			mControls[j].mY = b->ReadFloat();
		}
		for (int k = 0; k < mCount - 1; k++)
		{
			mLengths[k] = b->ReadFloat();
		}
	}
	if (mCurveDetail > 0)
	{
		GenerateCurveImage(mCurveColor, mCurveDetail);
	}
}
#endif