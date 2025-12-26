#ifndef __BEZIER_H__
#define __BEZIER_H__

#include "Buffer.h"
#include "Graphics.h"

namespace Sexy
{
    class Bezier
    {
    public:
        float* mTimes;
        float* mLengths;
        float mTotalLength;
        int mCount;
        FPoint* mControls;
        FPoint* mPositions;
        int mCurveDetail;
#ifdef SEXYDECOMP_USE_LATEST_CODE //The rest are present in XNA and probably Mobile/current + Prime
        Color mCurveColor;
        MemoryImage mImage;
        int mImageX;
        int mImageY;
#endif
    protected:
        void SubdivideRender(Graphics* g, const FPoint& P0, const FPoint& P1, const FPoint& P2, const FPoint& P3); //Not an exe function but exists in XNA, todo
        float SubdivideLength(const FPoint& P0, const FPoint& P1, const FPoint& P2, const FPoint& P3);
        float SegmentArcLength(int i, float u1, float u2);
    public:
        Bezier(const Bezier& rhs);
        Bezier();
        ~Bezier();
        Bezier& operator=(const Bezier& rhs);
        bool Init(const FPoint* positions, const float* times, int count);
        bool Init(const FPoint* positions, const FPoint* controls, const float* times, int count);
        void Serialize(Buffer* b);
        void Deserialize(Buffer* b);
        FPoint Evaluate(float t);
        FPoint Velocity(float t, bool clamp);
        FPoint Acceleration(float t);
        float ArcLength(float t1, float t2);
        float GetTotalLength() const { return mTotalLength; }
        int GetNumPoints() const { return mCount; }
        bool IsInitialized() { return mCount > 0; } //45
        void Clean();
    };
}
#endif //__BEZIER_H__