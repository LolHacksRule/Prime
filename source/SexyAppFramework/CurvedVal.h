#ifndef __CURVEDVAL_H__
#define __CURVEDVAL_H__

#include "SexyMath.h"

//const int CV_NUM_SPLINE_POINTS = 256; //XNA + H5

const double PI = 3.14159; //You have SexyMath PI though

namespace Sexy
{
    class CurvedVal
    {
	public:
        enum
        {
            MODE_CLAMP,
            MODE_REPEAT,
            MODE_PING_PONG
        };
        enum
        {
            RAMP_NONE,
            RAMP_LINEAR,
            RAMP_SLOW_TO_FAST,
            RAMP_FAST_TO_SLOW,
            RAMP_SLOW_FAST_SLOW,
            RAMP_FAST_SLOW_FAST,
            RAMP_CURVEDATA
        };
        enum
        {
            DFLAG_NOCLIP = 1,
            DFLAG_SINGLETRIGGER = 2,
            DFLAG_OUTPUTSYNC = 4,
            DFLAG_HERMITE = 8,
            DFLAG_AUTOINC = 16
        };
		class DataPoint
		{
		public:
			float mX;
			float mY;
			float mAngleDeg;
		};
		class CurveCacheRecord
		{
		public:
			float mTable[8192]; //On XNA, CV_NUM_SPLINE_POINTS
			SexyMathHermite mHermiteCurve;
			std::string mDataStr;
		};
		typedef std::map<std::string, CurveCacheRecord> CurveCacheMap;
		typedef std::vector<DataPoint> DataPointVector;
		static CurveCacheMap mCurveCacheMap;
		int mMode;
		int mRamp;
		double mIncRate;
		double mOutMin;
		double mOutMax;
		const char** mDataP;
		const char* mCurDataPStr;
		int* mInitAppUpdateCount;
		int* mAppUpdateCountSrc;
		CurvedVal* mLinkedVal;
		CurveCacheRecord* mCurveCacheRecord;
		double mCurOutVal;
		double mPrevOutVal;
		double mInMin;
		double mInMax;
		bool mNoClip;
		bool mSingleTrigger;
		bool mOutputSync;
		bool mTriggered;
		bool mIsHermite;
		bool mAutoInc;
		double mPrevInVal;
		double mInVal;
		CurvedVal();
		CurvedVal(const char** theDataP, CurvedVal* theLinkedVal);
		CurvedVal(const std::string& theData, CurvedVal* theLinkedVal);
		void SetCurve(const std::string& theData, CurvedVal* theLinkedVal);
		void SetCurve(const char** theData, CurvedVal* theLinkedVal);
		void SetCurveMult(const char** theData, CurvedVal* theLinkedVal);
		void SetCurveMult(const std::string& theData, CurvedVal* theLinkedVal);
		void SetConstant(double theValue);
		bool IsInitialized();
		void SetMode(int theMode);
		void SetRamp(int theRamp);
		void SetOutRange(double theMin, double theMax);
		void SetInRange(double theMin, double theMax);
		double GetOutVal(double theInVal);
		double GetOutVal();
		double GetOutValDelta();
		double GetOutFinalVal();
		double GetInVal();
		bool SetInVal(double theVal, bool theRealignAutoInc);
		bool IncInVal();
		bool IncInVal(double theInc);
		void Intercept(const std::string&, CurvedVal* theInterceptCv, double theCheckInIncrPct, bool theStopAtLocalMin);
		void Intercept(const char** theDataP, CurvedVal* theInterceptCv, double theCheckInIncrPct, bool theStopAtLocalMin);
		double FindClosestInToOutVal(double theTargetOutVal, double theCheckInIncrPct, double theCheckInRangeMinPct, double theCheckInRangeMaxPct, bool theStopAtLocalMin);
		double GetInValAtUpdate(int theUpdateCount);
		int GetLengthInUpdates();
		bool CheckInThreshold(double theInVal);
		bool CheckUpdatesFromEndThreshold(int theUpdateCount);
		bool HasBeenTriggered();
		operator double() { return GetOutVal(); } //149
		void ClearTrigger();
		bool IsDoingCurve();			
		
		static const char* REFLECT_ATTR$CLASS$ToStringMethod() { return "ToStringProxy"; } void	ToStringProxy(char* theBuffer, int theBufferLen) { std::string s =ToString(); strncpy_s(theBuffer, theBufferLen, s.c_str(), theBufferLen); } //152 (Both on same line) | C++ only (Autogen?)
		std::string				ToString() //153-155
			{
				return StrFormat("CV(%f -> %f)", GetInVal(), GetOutVal());
			}
	protected:
		void InitVarDefaults();
		void GenerateTable(DataPointVector* theDataPointVector, float* theBuffer, int theSize); //TODO
		void ParseDataString(const std::string& theString); //TODO
		bool CheckCurveChange();
		bool CheckClamping();
	};
}

#endif //__CURVEDVAL_H__