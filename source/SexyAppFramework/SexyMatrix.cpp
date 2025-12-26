#include "SexyMatrix.h"
#include "SexyMath.h"

#include <math.h>

using namespace Sexy;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SexyMatrix3::SexyMatrix3() //11-12
{
}

void SexyMatrix3::ZeroMatrix() //15-19
{
	m00 = m01 = m02 =
	m10 = m11 = m12 = 
	m20 = m21 = m22 = 0;
}

void SexyMatrix3::LoadIdentity() //22-25
{
	m01	= m02 = m10 = m12 = m20 = m21 = 0;
	m00 = m11 = m22 = 1;
}

SexyMatrix3 SexyMatrix3::operator*(const SexyMatrix3 &theMat) const //28-44
{
	SexyMatrix3 aResult;

	for(int i=0; i<3; i++)
	{
		for(int j=0; j<3; j++)
		{
			float x = 0;
			for(int k=0; k<3; k++)				
				x += m[i][k]*theMat.m[k][j];

			aResult.m[i][j] = x;
		}
	}

	return aResult;
}

SexyVector2 SexyMatrix3::operator*(const SexyVector2 &theVec) const //47-51
{
	return SexyVector2(
		m00*theVec.x + m01*theVec.y + m02,
		m10*theVec.x + m11*theVec.y + m12);
}

SexyVector3 SexyMatrix3::operator*(const SexyVector3 &theVec) const //54-59
{
	return SexyVector3(
		m00*theVec.x + m01*theVec.y + m02*theVec.z,
		m10*theVec.x + m11*theVec.y + m12*theVec.z,
		m20*theVec.x + m21*theVec.y + m22*theVec.z);
}

const SexyMatrix3& SexyMatrix3::operator*=(const SexyMatrix3 &theMat) //62-64
{
	return operator=(operator*(theMat));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SexyMatrix4::SexyMatrix4(float in00, float in01, float in02, float in03, float in10, float in11, float in12, float in13, float in20, float in21, float in22, float in23, float in30, float in31, float in32, float in33) //72-77
{
	m00 = in00; m01 = in00; m02 = in02; m03 = in03;
	m10 = in10; m11 = in11; m12 = in12; m13 = in13;
	m20 = in20; m21 = in21; m22 = in22; m23 = in23;
	m30 = in30; m31 = in31; m32 = in32; m32 = in33;
}

void SexyMatrix4::LoadIdentity() //80-86
{
	m01 = m02 = m03 = m10 = m12 = m13 = m20 = m21 = m23 = m30 = m31 = m32 = 0;
	m00 = m11 = m22 = m33 = 1;
}

SexyMatrix4 SexyMatrix4::operator*(const SexyMatrix4& theMat) const //89-105
{
	SexyMatrix4 aResult;

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			float x = 0;
			for (int k = 0; k < 4; k++)
				x += m[i][k] * theMat.m[k][j];

			aResult.m[i][j] = x;
		}
	}

	return aResult;
}

SexyVector3 SexyMatrix4::operator*(const SexyVector2& theVec) const //108-113
{
	return SexyVector3(
		m00*theVec.x + m10*theVec.y + m30,
		m01*theVec.x + m11*theVec.y + m31,
		m02*theVec.x + m12*theVec.y + m32);
}

SexyVector3 SexyMatrix4::operator*(const SexyVector3& theVec) const //116-121
{
	return SexyVector3(
		m00*theVec.x + m10*theVec.y + m20*theVec.z + m30,
		m01*theVec.x + m11*theVec.y + m21*theVec.z + m31,
		m02*theVec.x + m12*theVec.y + m22*theVec.z + m32);
}

SexyMatrix4& SexyMatrix4::operator*=(const SexyMatrix4& theMat) //159-161
{
	return operator=(operator*(theMat));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SexyTransform3D::SexyTransform3D() //166-168
{
	LoadIdentity();
}

SexyTransform3D::SexyTransform3D(bool loadIdentity) //171-174
{
	if (loadIdentity)
		LoadIdentity();
}

SexyTransform3D::SexyTransform3D(const SexyMatrix4& theMatrix) : SexyMatrix4(theMatrix) //177-178
{
}

const SexyTransform3D& SexyTransform3D::operator=(const SexyMatrix4& theMat) //181-184
{
	SexyMatrix4::operator=(theMat);
	return *this;
}

///////////////////////////////////////////////////////////////////////////////
void SexyTransform3D::Translate(float tx, float ty, float tz) //188-195
{
	SexyMatrix4 aMat;
	aMat.LoadIdentity();
	aMat.m30 = tx;
	aMat.m31 = ty;
	aMat.m32 = tz;
	*this = aMat * (*this);
}

void SexyTransform3D::RotateRadX(float rot) //198-209
{
	SexyMatrix4 aMat;
	aMat.LoadIdentity();

	float sinRot = -sinf(rot);
	float cosRot = cosf(rot);
	aMat.m11 = cosRot;
	aMat.m21 = -sinRot;
	aMat.m12 = sinRot;
	aMat.m22 = cosRot;
	*this = aMat * (*this);
}

void SexyTransform3D::RotateRadY(float rot) //212-223
{
	SexyMatrix4 aMat;
	aMat.LoadIdentity();

	float sinRot = -sinf(rot);
	float cosRot = cosf(rot);
	aMat.m00 = cosRot;
	aMat.m02 = -sinRot;
	aMat.m20 = sinRot;
	aMat.m22 = cosRot;
	*this = aMat * (*this);
}

void SexyTransform3D::RotateRadZ(float rot) //226-237
{
	SexyMatrix4 aMat;
	aMat.LoadIdentity();

	float sinRot = -sinf(rot);
	float cosRot = cosf(rot);

	aMat.m00 = cosRot;
	aMat.m01 = -sinRot;
	aMat.m10 = sinRot;
	aMat.m11 = cosRot;

	*this = aMat * (*this);
}

void SexyTransform3D::Scale(float sx, float sy, float sz) //240-248
{
	SexyMatrix4 aMat;
	aMat.LoadIdentity();
	aMat.m00 = sx;
	aMat.m11 = sy;
	aMat.m22 = sz;

	*this = aMat * (*this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SexyTransform2D::SexyTransform2D() //253-255
{
	LoadIdentity();
}

SexyTransform2D::SexyTransform2D(bool loadIdentity) //258-261
{
	if (loadIdentity)
		LoadIdentity();
}

SexyTransform2D::SexyTransform2D(const SexyMatrix3& theMatrix) : SexyMatrix3(theMatrix) //264-265
{
}

const SexyTransform2D& SexyTransform2D::operator=(const SexyMatrix3 &theMat) //268-271
{
	SexyMatrix3::operator=(theMat);
	return *this;
}

void SexyTransform2D::Translate(float tx, float ty) //274-282
{
	SexyMatrix3 aMat;
	aMat.LoadIdentity();
	aMat.m02 = tx;
	aMat.m12 = ty;
	aMat.m22 = 1;

	*this = aMat * (*this);
}

void SexyTransform2D::RotateRad(float rot) //285-298
{
	SexyMatrix3 aMat;
	aMat.LoadIdentity();

	float sinRot = -sinf(rot);
	float cosRot = cosf(rot);

	aMat.m00 = cosRot;
	aMat.m01 = -sinRot;
	aMat.m10 = sinRot;
	aMat.m11 = cosRot;

	*this = aMat * (*this);
}

void SexyTransform2D::RotateDeg(float rot) //301-303
{
	RotateRad(SEXYMATH_PI * rot / 180.0f);
}

void SexyTransform2D::Scale(float sx, float sy) //306-313
{
	SexyMatrix3 aMat;
	aMat.LoadIdentity();
	aMat.m00 = sx;
	aMat.m11 = sy;

	*this = aMat * (*this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
Transform::Transform() : mMatrix(false) //319-321
{
	Reset();
}

void Transform::Reset() //324-333
{
	mNeedCalcMatrix = true;
	mComplex = false;
	mTransX1 = mTransY1 = 0;
	mTransX2 = mTransY2 = 0;
	mScaleX = mScaleY = 1;
	mRot = 0;
	mHaveRot = false;
	mHaveScale = false;
}

void Transform::Translate(float tx, float ty) //336-353
{
	if (!mComplex)
	{
		mNeedCalcMatrix = true;
		if (mHaveRot || mHaveScale)
		{
			mTransX2 += tx;
			mTransY2 += ty;
		}
		else
		{
			mTransX1 += tx;
			mTransY1 += ty;
		}
	}
	else
		mMatrix.Translate(tx,ty);
}

void Transform::RotateRad(float rot) //356-373
{
	if (!mComplex)
	{
		if (mHaveScale)
		{
			MakeComplex();
			mMatrix.RotateRad(rot);
		}
		else 
		{
			mNeedCalcMatrix = true;
			mHaveRot = true;
			mRot += rot;
		}
	}
	else
		mMatrix.RotateRad(rot);
}

void Transform::RotateDeg(float rot) //476-378
{
	Transform::RotateRad(SEXYMATH_PI * rot / 180.0f);
}

void Transform::Scale(float sx, float sy) //380-401
{
	if (!mComplex)
	{
		if (mHaveRot || mTransX1!=0 || mTransY1!=0 || (sx<0 && mScaleX*sx!=-1) || sy<0 || mTransX2 != 0 || mTransY2 != 0 || sy != sx)
		{
			MakeComplex();
			mMatrix.Scale(sx,sy);
		}
		else
		{
			mNeedCalcMatrix = true;
			mHaveScale = true;
			mScaleX *= sx;
			mScaleY *= sy;
			mTransX2 *= sx;
			mTransY2 *= sy;
		}
	}
	else
		mMatrix.Scale(sx,sy);
}

void Transform::MakeComplex() //404-410
{
	if (!mComplex)
	{
		mComplex = true;
		CalcMatrix();
	}
}

void Transform::CalcMatrix() const //413-435
{
	if (mNeedCalcMatrix)
	{
		mNeedCalcMatrix = false;

		mMatrix.LoadIdentity();
		mMatrix.m02 = mTransX1;
		mMatrix.m12 = mTransY1;
		mMatrix.m22 = 1;

		if (mHaveScale)
		{
			mMatrix.m00 = mScaleX;
			mMatrix.m11 = mScaleY;
		}
		else if (mHaveRot)
			mMatrix.RotateRad(mRot);

		if (mTransX2!=0 || mTransY2!=0)
			mMatrix.Translate(mTransX2,mTransY2);
	}

}

const SexyTransform2D& Transform::GetMatrix() const //438-441
{
	CalcMatrix();
	return mMatrix;
}

void Transform::SetMatrix(const SexyTransform2D& mat) //444-448
{
	mMatrix = mat;
	mNeedCalcMatrix = false;
	mComplex = true;
}