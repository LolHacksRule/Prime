#ifndef __GRAPHICSMETRICS_H__
#define __GRAPHICSMETRICS_H__

#include "Common.h"

namespace Sexy
{
	class GraphicsMetrics //C++ only.
	{
		typedef __int64 DCount;
	public:
		enum ECounterType
		{
			CT_TriListCalls,
			CT_TriListPrims,
			CT_TriStripCalls,
			CT_TriStripPrims,
			CT_TriFanCalls,
			CT_TriFanPrims,
			CT_LineStripCalls,
			CT_LineStripPrims,
			CT_SetRenderTargetCalls,
			CT_SetRenderTargetCallsRedundant,
			CT_SetVertexShaderCalls,
			CT_SetPixelShaderCalls,
			CT_COUNT,
		};
		class CCounter
		{
		protected:
			int mFramesPerSlice;
			int mSliceFramesRemaining;
			DCount mCurFrameCount;
			DCount mSliceTotalCount;
			DCount mAvgCount;
		public:
			static const int kDefaultFramesPerSlice = 15;
			CCounter(int inFramesPerSlice) { mFramesPerSlice = inFramesPerSlice; mSliceFramesRemaining = inFramesPerSlice; mCurFrameCount, mSliceTotalCount, mAvgCount = 0; } //75
			CCounter& operator++(int inValue) { Add(inValue); return *this; }
			CCounter& operator++() { Add(1); return *this; } //85
			CCounter& operator+=(int inValue) { Add(inValue); return *this; } //86
			DCount GetAverage() const;
			void Add(DCount inValue) //? | 91-93
			{
				mCurFrameCount += inValue;
			}
			void NextFrame() //95-109
			{
				mSliceTotalCount += mCurFrameCount;
				mCurFrameCount = 0;
				if (mSliceFramesRemaining)
				{
					mSliceFramesRemaining--;
				}
				else
				{
					mAvgCount = mSliceTotalCount / mFramesPerSlice;
					mSliceTotalCount = 0;
					mSliceFramesRemaining = mFramesPerSlice;
				}
			}
		};
		class CContext
		{
		protected:
			CCounter mCounters[CT_COUNT];
		public:
			const CCounter& operator[](ECounterType inCounter) const { return mCounters[inCounter]; } //119 //Correct
			CCounter& operator[](ECounterType inCounter);
			void NextFrame() //123-126
			{
				for (int i = 0; i < CT_COUNT; i++)
					mCounters[i].NextFrame();
			}
			//CContext();
		};
	protected:
		CContext mCtxActive;
	public:
		CCounter& operator[](ECounterType inCounter) { return mCtxActive[inCounter]; } //132
		void NextFrame() //135-137
		{
			mCtxActive.NextFrame();
		}
		void ResetCounter(ECounterType, int);
		//GraphicsMetrics();
	};
}
#endif