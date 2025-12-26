#ifndef __SEXYMATH_H__
#define __SEXYMATH_H__

#include "Common.h"

const float SEXYMATH_PI = 3.1415926535897932384626433832795028841971f;
const float SEXYMATH_2PI = SEXYMATH_PI*2;
const float SEXYMATH_E = 2.71828f;
const float SEXYMATH_EPSILON = 0.001f; //Never used
const float SEXYMATH_EPSILONSQ = SEXYMATH_EPSILON*SEXYMATH_EPSILON;

namespace Sexy
{
    class SexyMath
    {
    public:
        static float Fabs(double inX);/*
        {
            return fabs(inX);
        }*/

        static float Fabs(float inX) //42-49 (UNMATCHED)
        {
            return abs(inX);
        };

        static float DegToRad(float inX) { return (float)(inX * SEXYMATH_PI / 180.0); } //53
        static float RadToDeg(float inX); /*{ return inX * 180 / SEXYMATH_PI; }*/
        static bool IsPowerOfTwo(uint inX); /*{ return inX != 0 && (inX & (inX - 1)) == 0; }*/
        static float Lerp(float const& inA, float const& inB, float inAlpha) //64-66
        {
            return (inA + (inB - inA) * inAlpha);
        }
    };
    class SexyMathHermite
    {
    public:
        struct SPoint
        {
            float mX;
            float mFx;
            float mFxPrime;
            SPoint(float inX, float inFx, float inFxPrime) { mX = inX; mFx = inFx; mFxPrime = inFxPrime; } //92
        };
        std::vector<SPoint> mPoints;
#ifdef _SEXYDECOMP_USE_LATEST_CODE
        private static Stack<SPiece> unusedObjects; //From XNA
#endif
        struct SPiece
        {
            float mCoeffs[4];
        };
    protected:
        std::vector<SPiece> mPieces;
        bool mIsBuilt;
    public:
        SexyMathHermite() { mIsBuilt = false; } //98
        void Rebuild() //101-103
        {
            mIsBuilt = false;
        }
        float Evaluate(float inX) //105-121
        {
            if (!mIsBuilt)
            {
                if (!BuildCurve())
                    return 0.0;
                mIsBuilt = true;
            }
            uint pieceCount = mPieces.size();
            for (uint i = 0; i < pieceCount; i++)
            {
                if (inX < mPoints[i + 1].mX)
                    return EvaluatePiece(inX, &mPoints[i], &mPieces[i]);
            }
            return mPoints[mPoints.size() - 1].mFx;
        }
    protected:
        void CreatePiece(SPoint* inPoints, SPiece* outPiece) //Check? | 132-165
        {
            /*
            float[0x4], z
            float *[0x4], q
            const unsigned int, dim
            float[0x10], qbuf
            unsigned int, i
            unsigned int, i
            unsigned int, i2
            unsigned int, i
            unsigned int, j
            unsigned int, i
            */
            const uint dim = 4;
            float* q[dim];
            float z[dim];
            float qbuf[16];
            for (uint i = 0; i <= 1; i++)
            {
                uint i2 = 2 * i;
                z[i2] = inPoints[i].mX;
                z[i2 + 1] = inPoints[i].mX;
                *q[i2] = inPoints[i].mFx;
                *q[i2 + 1] = inPoints[i].mFx;
                q[i2 + 1][1] = inPoints[i].mFxPrime;
                if (i != 0)
                    q[i2][1] = q[i2][0] - q[i2 - 1][0] / z[i2] - z[i2 - 1]; //?
            }
            for (uint i = 2; i < dim; i++)
            {
                for (uint j = 2; j <= i; j++)
                    q[i][j] = (q[i + (j - 1) * dim] - q[i - 1 + (j - 1) * dim]) / (z[i] - z[i - j]); //?
            }
            for (uint i = 0; i < dim; i++)
                outPiece->mCoeffs[i] = q[i][i];
        }
        float EvaluatePiece(float inX, SPoint* inPoints, SPiece* inPiece) //168-185
        {
            float xSub[2];
            xSub[0] = inX - inPoints->mX;
            xSub[1] = inX - inPoints[1].mX;
            float f = 1.0;
            float h = inPiece->mCoeffs[0];
            const uint dim = 4;
            for (uint i = 1; i < dim; i++) {
                f *= xSub[(i - 1) / 2];
                h += f * inPiece->mCoeffs[i];
            }
            return h;
        }
        bool BuildCurve() //188-199
        {
            mPieces.clear();
            uint pointCount = mPoints.size();
            if (pointCount < 2)
                return false;
            uint pieceCount = pointCount - 1;
            mPieces.reserve(pieceCount);
            mPieces.resize(pieceCount);
            for (uint i = 0; i < pieceCount; i++)
                CreatePiece(&mPoints[i], &mPieces[i]);
            return true;
        }
    };
}
#endif