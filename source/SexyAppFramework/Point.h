#ifndef __POINT_H__
#define __POINT_H__

#include "Common.h"

namespace Sexy
{

template<class _T> class TPoint
{
public:
	_T						mX;
	_T						mY;

public:
	TPoint(_T theX, _T theY) : //24-25
		mX(theX),
		mY(theY)
	{
	}

	TPoint(const TPoint<_T>& theTPoint) : //30-31
		mX(theTPoint.mX),
		mY(theTPoint.mY)
	{
	}

	TPoint() : //36-37
		mX(0),
		mY(0)
	{
	}

	inline bool operator==(const TPoint& p)
	{
		return ((p.mX == mX) && (p.mY == mY));
	}

	inline bool operator!=(const TPoint& p)
	{
		return ((p.mX != mX) || (p.mY != mY));
	}
	
	_T Magnitude() //50 (C++ only, TPoint<*>.Length in XNA, *.x = 0.0, *.y = 0.0 in H5)
	{
		return sqrt(mX * mX + mY * mY);
	}
	TPoint operator+(const TPoint& p) const {return TPoint(mX+p.mX, mY+p.mY);} //54
	TPoint operator-(const TPoint& p) const {return TPoint(mX-p.mX, mY-p.mY);} //55
	TPoint operator*(const TPoint& p) const {return TPoint(mX*p.mX, mY*p.mY);}
	TPoint operator/(const TPoint& p) const {return TPoint(mX/p.mX, mY/p.mY);}
	TPoint& operator+=(const TPoint& p)  {mX+=p.mX; mY+=p.mY; return *this;} //58
	TPoint& operator-=(const TPoint& p)  {mX-=p.mX; mY-=p.mY; return *this;}
	TPoint& operator*=(const TPoint& p)  {mX*=p.mX; mY*=p.mY; return *this;}
	TPoint& operator/=(const TPoint& p)  {mX/=p.mX; mY/=p.mY; return *this;}
	TPoint operator*(_T s) const {return TPoint(mX*s, mY*s);} //62
	TPoint operator/(_T s) const {return TPoint(mX/s, mY/s);} //63
};

typedef TPoint<int> Point;
typedef TPoint<double> FPoint; //FSPoint in BejBlitz

};

#endif //__POINT_H__