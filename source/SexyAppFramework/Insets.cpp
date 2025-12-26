#include "Insets.h"

using namespace Sexy;

Insets::Insets() : //This file matches 1:1 to the original (according to PL_D.dll) if you remove this comment.
	mLeft(0),
	mTop(0),
	mRight(0),
	mBottom(0)
{

}

Insets::Insets(int theLeft, int theTop, int theRight, int theBottom) :
	mLeft(theLeft),
	mTop(theTop),
	mRight(theRight),
	mBottom(theBottom)
{
}

Insets::Insets(const Insets& theInsets) :
	mLeft(theInsets.mLeft),
	mTop(theInsets.mTop),
	mRight(theInsets.mRight),
	mBottom(theInsets.mBottom)
{
}

