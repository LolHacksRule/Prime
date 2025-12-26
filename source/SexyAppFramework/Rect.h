#ifndef __TRectANGLE_H__
#define __TRectANGLE_H__

#include "Common.h"
#include "Point.h"

#include <list>

namespace Sexy
{

template<class _T> class TRect
{
public:
	/*static _T ZERO_RECT = TRect(0, 0, 0, 0);
	static _T INVALIDATE_RECT = TRect(-1, -1, -1, -1);*/
	_T mX;
	_T mY;
	_T mWidth;
	_T mHeight;

public:

	TRect(const RECT& theRect) :
		mX(theRect->left), mY(theRect->top), mWidth(theRect->right - theRect->left), mHeight(theRect->bottom - theRect->top) //24-25
	{
	}

	TRect(_T theX, _T theY, _T theWidth, _T theHeight) :
	  mX(theX), mY(theY), mWidth(theWidth), mHeight(theHeight) //30-31
	{
	}
	
	TRect(const TRect<_T>& theTRect) :
	  mX(theTRect.mX), mY(theTRect.mY), mWidth(theTRect.mWidth), mHeight(theTRect.mHeight) //35-36
	{
	}

	TRect() :
	  mX(0), mY(0), mWidth(0), mHeight(0) //40-41
	{
	}

	bool					Intersects(const TRect<_T>& theTRect) const //44-49
	{
		return !((theTRect.mX + theTRect.mWidth <= mX) ||
			(theTRect.mY + theTRect.mHeight <= mY) ||
			(theTRect.mX >= mX + mWidth) ||
			(theTRect.mY >= mY + mHeight));
	}

	TRect<_T>				Intersection(const TRect<_T>& theTRect)	const //52-61
	{
		_T x1 = max(mX, theTRect.mX);
		_T x2 = min(mX + mWidth, theTRect.mX + theTRect.mWidth);
		_T y1 = max(mY, theTRect.mY);
		_T y2 = min(mY + mHeight, theTRect.mY + theTRect.mHeight);
		if (((x2 - x1) < 0) || ((y2 - y1) < 0))
			return TRect<_T>(0,0,0,0);
		else
			return TRect<_T>(x1, y1, x2 - x1, y2 - y1);
	}

	TRect<_T>				Union(const TRect<_T>& theTRect)	
	{
		_T x1 = min(mX, theTRect.mX);
		_T x2 = max(mX + mWidth, theTRect.mX + theTRect.mWidth);
		_T y1 = min(mY, theTRect.mY);
		_T y2 = max(mY + mHeight, theTRect.mY + theTRect.mHeight);
			return TRect<_T>(x1, y1, x2 - x1, y2 - y1);
	}

	bool					Contains(_T theX, _T theY) const //73-76
	{
		return ((theX >= mX) && (theX < mX + mWidth) &&
			(theY >= mY) && (theY < mY + mHeight));
	}

	bool					Contains(const TPoint<_T>& thePoint) const //79-82
	{
		return ((thePoint.mX >= mX) && (thePoint.mX < mX + mWidth) &&
			(thePoint.mY >= mY) && (thePoint.mY < mY + mHeight));
	}

	void					Offset(_T theX, _T theY) //Only int? | 85-88
	{
		mX += theX;
		mY += theY;
	}

	void					Offset(const TPoint<_T>& thePoint)
	{
		mX += thePoint.mX;
		mY += thePoint.mY;
	}

	TRect					Inflate(_T theX, _T theY) //97-104
	{
		mX -= theX;
		mWidth += theX*2;
		mY -= theY;
		mHeight += theY*2;

		return *this;
	}

	bool operator==(const TRect<_T>& theRect) const //122-124
	{
		return (mX == theRect.mX) && (mY == theRect.mY) && (mWidth == theRect.mWidth) && (mHeight == theRect.mHeight);
	}

	RECT					ToRECT() const //133-136
	{
		RECT aRect = {mX, mY, mX + mWidth, mY + mHeight};
		return aRect;
	}
};

typedef TRect<int> Rect;
typedef TRect<double> FRect; //Double in XNA



}

#endif //__TRectANGLE_H__