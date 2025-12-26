#ifndef __SEXYVECTOR_H__
#define __SEXYVECTOR_H__

#include "SexyCoords.h"

namespace Sexy
{

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class SexyVector2
{
public:
	float x,y;

public:
	SexyVector2() : x(0), y(0) { } //17
	SexyVector2(float theX, float theY) : x(theX), y(theY) { } //18

	float Dot(const SexyVector2 &v) const { return x*v.x + y*v.y; } //20
	SexyVector2 operator+(const SexyVector2 &v) const { return SexyVector2(x+v.x, y+v.y); } //21
	SexyVector2 operator-(const SexyVector2 &v) const { return SexyVector2(x-v.x, y-v.y); } //22
	SexyVector2 operator-() const { return SexyVector2(-x, -y); }
	SexyVector2 operator*(float t) const { return SexyVector2(t*x, t*y); } //24
	SexyVector2 operator/(float t) const { return SexyVector2(x/t, y/t); } //25

	SexyVector2& operator+=(const SexyVector2 &v) { x+=v.x; y+=v.y; }
	SexyVector2& operator-=(const SexyVector2 &v) { x-=v.x; y-=v.y; } //28
	SexyVector2& operator*=(float t) { x*=t; y*=t; } //29
	SexyVector2& operator/=(float t) { x/=t; y/=t; }

	bool operator==(const SexyVector2 &v) { return x==v.x && y==v.y; }
	bool operator!=(const SexyVector2 &v) { return x!=v.x || y!=v.y; }

	float Magnitude() const { return sqrtf(x*x + y*y); } //35
	float MagnitudeSquared() const { return x*x+y*y; }

	SexyVector2 Normalize() const //39-42
	{
		float aMag = Magnitude();
		return aMag!=0 ? (*this)/aMag : *this;
	}

	SexyVector2 Perp() const //45-47
	{
		return SexyVector2(-y, x);
	}
};

///////////////////////////////////////////////////////////////////////////////
// 
// 
// 
///////////////////////////////////////////////////////////////////////////////
class SexyVector3 //TODO
{
public:
	float x,y,z;

public:
	SexyVector3() : x(0), y(0), z(0) { } //61
	SexyVector3(float theX, float theY, float theZ) : x(theX), y(theY), z(theZ) { } //62

	float Dot(const SexyVector3 &v) const { return x*v.x + y*v.y + z*v.z; }
	SexyVector3 Cross(const SexyVector3& v) const { return SexyVector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }

	SexyVector3& operator=(const SexyVector3 &v) { return (SexyVector3)v; } //67
	SexyVector3& operator+=(const SexyVector3& v) { x += v.x; y += v.y; z += v.z; }
	SexyVector3& operator-=(const SexyVector3& v) { x -= v.x; y -= v.y; z -= v.z; }
	SexyVector3& operator*=(const SexyVector3& v) { x *= v.x; y *= v.y; z *= v.z; }
	SexyVector3& operator*=(float t) { x *= t; y *= t; z *= t; }
	SexyVector3& operator/=(const SexyVector3& v) { x /= v.x; y /= v.y; z /= v.z; }
	SexyVector3& operator/=(float t) { x /= t; y /= t; z /= t; }
	SexyVector3 operator-(const SexyVector3& v) const { return SexyVector3(x - v.x, y - v.y, z - v.z); }
	SexyVector3 operator-() const { return SexyVector3(-x, -y, -z); }
	SexyVector3 operator+(const SexyVector3& v) const { return SexyVector3(x + v.x, y + v.y, z + v.z); }
	SexyVector3 operator*(const SexyVector3& v) const { return SexyVector3(x * v.x, y * v.y, z * v.z); }
	SexyVector3 operator*(float t) const { return SexyVector3(t * x, t * y, t * z); }
	SexyVector3 operator/(const SexyVector3& v) const { return SexyVector3(x / v.x, y / v.y, z / v.z); }
	SexyVector3 operator/(float t) const { float oot = 1.0 / t; return SexyVector3(x / oot, y / oot, z / oot); } //80 | ScaleDiv in H5

	float Magnitude() const { return sqrtf(x*x + y*y + z*z); } //83

	SexyVector3 Normalize() const //86-89
	{ 
		float aMag = Magnitude();
		return aMag!=0 ? (*this)/aMag : *this;
	}

	bool ApproxEquals(const SexyVector3& inV, float inTol = 0.001f) const /*{  }*/;
	bool ApproxZero(float inTol = 0.001f) const /*{ return ApproxEquals(SexyVector3(0, 0, 0), inTol); }*/;

	SexyVector3 Enter(const SexyCoords3& inCoords) const /*{ return SexyVector3(-inCoords.t.Enter(inCoords.r) / inCoords.s); }*/ ;
	SexyVector3 Enter(const SexyAxes3& inAxes) const /*{ return SexyVector3(Dot(inAxes.vX), Dot(inAxes.vY), Dot(inAxes.vZ)); }*/ ;
	SexyVector3 Leave(const SexyCoords3& inCoords) const /*{ return SexyVector3(inCoords.s.Leave(inCoords.r) + inCoords.t); }*/ ;
	SexyVector3 Leave(const SexyAxes3& inAxes) const /*{ return SexyVector3(x * inAxes.vX.x + y * inAxes.vY.x + z * inAxes.vZ.x, x * inAxes.vX.y + y * inAxes.vY.y + z * inAxes.vZ.y, x * inAxes.vX.z + y * inAxes.vY.z + z * inAxes.vZ.z); }*/ ;
	SexyVector3& operator>>=(const SexyCoords3& inCoords);
	SexyVector3& operator>>=(const SexyAxes3& inAxes);
	SexyVector3& operator<<=(const SexyCoords3& inCoords);
	SexyVector3& operator<<=(const SexyAxes3& inAxes);
	SexyVector3 operator>>(const SexyCoords3& inCoords) const;
	SexyVector3 operator>>(const SexyAxes3& inAxes) const;
	SexyVector3 operator<<(const SexyCoords3& inCoords) const;
	SexyVector3 operator<<(const SexyAxes3& inAxes) const;

};

};

#endif