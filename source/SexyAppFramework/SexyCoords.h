#ifndef __SEXYCOORDS_H__
#define __SEXYCOORDS_H__

#include "SexyMath.h"

namespace Sexy
{
    class SexyAxes3
    {
    public:
        SexyVector3 vX, vY, vZ;
    public:
        SexyAxes3(const SexyVector3& inX, const SexyVector3& inY, const SexyVector3& inZ);
        SexyAxes3(const SexyAxes3& inA);
        SexyAxes3();
        SexyAxes3& operator=(const SexyAxes3&);
        SexyAxes3 Enter(const SexyAxes3& inAxes) const;
        SexyAxes3 Leave(const SexyAxes3& inAxes) const;
        SexyAxes3& operator>>=(const SexyAxes3& inAxes);
        SexyAxes3& operator<<=(const SexyAxes3& inAxes);
        SexyAxes3 operator>>(const SexyAxes3& inAxes) const;
        SexyAxes3 operator<<(const SexyAxes3& inAxes) const;
        SexyAxes3 Inverse();
        SexyAxes3 OrthoNormalize() const;
        SexyAxes3 DeltaTo(const SexyAxes3&) const;
        SexyAxes3 SlerpTo(const SexyAxes3& inAxes, float inAlpha, bool inFastButLessAccurate) const;
        void RotateRadAxis(float inRot, const SexyVector3& inNormalizedAxis);
        void RotateRadX(float inRot);
        void RotateRadY(float inRot);
        void RotateRadZ(float inRot);
        void LookAt(const SexyVector3& inTargetDir, const SexyVector3& inUpVector);
    };
    class SexyCoords3
    {
    public:
        SexyVector3 t;
        SexyAxes3 r;
        SexyVector3 s;
    public:
        SexyCoords3(const SexyCoords3& inC);
        SexyCoords3(const SexyAxes3& inR);
        SexyCoords3(const SexyVector3& inT, const SexyAxes3& inR, const SexyVector3& inS);
        SexyCoords3();
        SexyCoords3& operator=(const SexyCoords3&);
        SexyCoords3 Enter(const SexyCoords3& inCoords) const;
        SexyCoords3 Leave(const SexyCoords3& inCoords) const;
        SexyCoords3& operator>>=(const SexyCoords3& inCoords);
        SexyCoords3& operator<<=(const SexyCoords3& inCoords);
        SexyCoords3 operator>>(const SexyCoords3& inCoords) const;
        SexyCoords3 operator<<(const SexyCoords3& inCoords) const;
        SexyCoords3 Inverse() const;
        SexyCoords3 DeltaTo(const SexyCoords3& inCoords) const;
        void Translate(float inX, float inY, float inZ);
        void RotateRadAxis(float inRot, const SexyVector3& inNormalizedAxis);
        void RotateRadX(float inRot);
        void RotateRadY(float inRot);
        void RotateRadZ(float inRot);
        void Scale(float inX, float inY, float inZ);
        bool LookAt(const SexyVector3& inTargetPos, const SexyVector3& inUpVector);
        bool LookAt(const SexyVector3& inViewPos, const SexyVector3& inTargetPos, const SexyVector3& inUpVector);
        void GetInboundMatrix(SexyMatrix4* outM) const;
        void GetOutboundMatrix(SexyMatrix4* outM) const;
    };
};
#endif