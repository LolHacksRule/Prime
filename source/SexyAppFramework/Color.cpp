#include "Color.h"

using namespace Sexy;
//Only the first two exist in public and H5
Color Color::Black(0, 0, 0); //5
Color Color::White(255, 255, 255); //6 
Color Color::Red(255, 0, 0); //7
Color Color::Green(0, 255, 0); //8
Color Color::Blue(0, 0, 255); //9
Color Color::Yellow(255, 255, 0); //10

Color::Color() : //17-18
	mRed(0),
	mGreen(0),
	mBlue(0),
	mAlpha(255)
{
}

Color::Color(int theColor) : //25-28
	mAlpha((theColor >> 24) & 0xFF),
	mRed((theColor   >> 16) & 0xFF),
	mGreen((theColor >> 8 ) & 0xFF),
	mBlue((theColor       ) & 0xFF)
{
	if(mAlpha==0)
		mAlpha = 0xff;
}

Color::Color(int theColor, int theAlpha) : //35-36
	mRed((theColor   >> 16) & 0xFF),
	mGreen((theColor >> 8 ) & 0xFF),
	mBlue((theColor       ) & 0xFF),
	mAlpha(theAlpha)
{
}

Color::Color(int theRed, int theGreen, int theBlue) : //43-44
	mRed(theRed),
	mGreen(theGreen),
	mBlue(theBlue),
	mAlpha(0xFF)
{
}

Color::Color(int theRed, int theGreen, int theBlue, int theAlpha) : //51-52
	mRed(theRed),
	mGreen(theGreen),
	mBlue(theBlue),
	mAlpha(theAlpha)
{
}

Color::Color(const SexyRGBA &theColor) : //59-60
	mRed(theColor.r),
	mGreen(theColor.g),
	mBlue(theColor.b),
	mAlpha(theColor.a)
{
}

Color::Color(const uchar* theElements) : //67-68
	mRed(theElements[0]),
	mGreen(theElements[1]),
	mBlue(theElements[2]),
	mAlpha(0xFF)
{
}

Color::Color(const int* theElements) : //75-76
	mRed(theElements[0]),
	mGreen(theElements[1]),
	mBlue(theElements[2]),
	mAlpha(0xFF)
{
}

int	Color::GetRed() const //79-81
{
	return mRed;	
}

int Color::GetGreen() const //84-86
{
	return mGreen;
}

int	Color::GetBlue() const //89-91
{
	return mBlue;
}

int	Color::GetAlpha() const //94-96
{
	return mAlpha;
}

int& Color::operator[](int theIdx) //Not in XNA | 99-115
{
	static int aJunk = 0;

	switch (theIdx)
	{
	case 0:
		return mRed;
	case 1:
		return mGreen;
	case 2:
		return mBlue;
	case 3:
		return mAlpha;
	default:
		return aJunk;
	}
}

int Color::operator[](int theIdx) const //Not in XNA | 118-132
{
	switch (theIdx)
	{
	case 0:
		return mRed;
	case 1:
		return mGreen;
	case 2:
		return mBlue;
	case 3:
		return mAlpha;
	default:
		return 0;
	}
}

Color Color::operator*(const Color& theColor) const //? | 135-141
{
	return Color(
		theColor.mRed * mRed / 255,
		theColor.mGreen * mGreen / 255,
		theColor.mBlue * mBlue / 255,
		theColor.mAlpha * mAlpha / 255);
}

Color Color::operator*(float theAlphaPct) const //144-146
{
	return Color(mRed, mGreen, mBlue, (int)(float)(mAlpha * theAlphaPct)); //Do we need the casts?
}

ulong Color::ToInt() const //149-151
{
	return (mAlpha << 24) | (mRed << 16) | (mGreen << 8) | (mBlue);
}

Color Color::FromInt(ulong theColor) //Not in XNA | 154-160
{
	return Color(
		((theColor >> 16) & 0xFF),
		((theColor >> 8) & 0xFF),
		((theColor) & 0xFF),
		((theColor >> 24) & 0xFF)); //Correct?
}

Color Color::FAlpha(float theAlpha) //163-165
{
	return Color(255, 255, 255, (int)(theAlpha * 255.0)); //Do we need the cast?
}

SexyRGBA Color::ToRGBA() const //168-176
{
	SexyRGBA anRGBA;
	anRGBA.r = mRed;
	anRGBA.g = mGreen;
	anRGBA.b = mBlue;
	anRGBA.a = mAlpha;

	return anRGBA;
}

bool Sexy::operator==(const Color& theColor1, const Color& theColor2) //179-185
{
	return 
		(theColor1.mRed == theColor2.mRed) &&
		(theColor1.mGreen == theColor2.mGreen) &&
		(theColor1.mBlue == theColor2.mBlue) && 
		(theColor1.mAlpha == theColor2.mAlpha);
}

bool Sexy::operator!=(const Color& theColor1, const Color& theColor2) //188-194
{
	return 
		(theColor1.mRed != theColor2.mRed) ||
		(theColor1.mGreen != theColor2.mGreen) ||
		(theColor1.mBlue != theColor2.mBlue) ||
		(theColor1.mAlpha != theColor2.mAlpha);
}