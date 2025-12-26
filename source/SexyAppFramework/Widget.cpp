#include "Widget.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "Font.h"
#include "Image.h"
#include "SexyAppBase.h"
#include "Debug.h"

using namespace Sexy;

bool Widget::mWriteColoredString = true;

Widget::Widget() //15-35
{
	mWidgetManager = NULL;	
	mVisible = true;
	mDisabled = false;
	mIsDown = false;
	mIsOver = false;
	mDoFinger = false;	
	mMouseVisible = true;		
	mHasFocus = false;
	mHasTransparencies = false;	
	mWantsFocus = false;
	mTabPrev = NULL;
	mTabNext = NULL;

	mIsGamepadSelection = false;
	mGamepadParent = NULL;
	mGamepadLinkUp = NULL;
	mGamepadLinkDown = NULL;
	mGamepadLinkLeft = NULL;
	mGamepadLinkRight = NULL;
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	mDataMenuId = -1;
#endif
}

Widget::~Widget() //38-40
{	
	mColors.clear();
}

void Widget::WidgetRemovedHelper() //43-77
{
	if (mWidgetManager==NULL)
		return;

	// Call RemovedFromManager on all child widgets and disable them and stuff like that
	for (WidgetList::iterator aWidgetItr = mWidgets.begin(); aWidgetItr != mWidgets.end(); ++aWidgetItr)
	{
		Widget *aWidget = *aWidgetItr;
		aWidget->WidgetRemovedHelper();
	}	

	mWidgetManager->DisableWidget(this);

	PreModalInfoList::iterator anItr = mWidgetManager->mPreModalInfoList.begin();
	while (anItr != mWidgetManager->mPreModalInfoList.end())
	{
		PreModalInfo* aPreModalInfo = &(*anItr);
		if (aPreModalInfo->mPrevBaseModalWidget == this)
			aPreModalInfo->mPrevBaseModalWidget = NULL;
		if (aPreModalInfo->mPrevFocusWidget == this)
			aPreModalInfo->mPrevFocusWidget = NULL;
		++anItr;
	}
	
	RemovedFromManager(mWidgetManager);
	MarkDirtyFull(this);

	if (mWidgetManager->GetGamepadSelection() == this)
		mWidgetManager->SetGamepadSelection(NULL, LINK_DIR_NONE);

	mWidgetManager = NULL;
}

void Widget::OrderInManagerChanged() //80-81
{
}

bool Widget::IsPointVisible(int x, int y) //84-86
{
	return true;
}

void Widget::SetVisible(bool isVisible) //89-102
{
	if (mVisible == isVisible)
		return;
	
	mVisible = isVisible;
	
	if (mVisible)
		MarkDirty();
	else
		MarkDirtyFull();

	if (mWidgetManager != NULL)
		mWidgetManager->RehupMouse();
}

void Widget::Draw(Graphics* g) // Already translated | 105-106
{
}

void Widget::DrawOverlay(Graphics* g) //109-110
{
}

void Widget::DrawOverlay(Graphics* g, int thePriority) //113-115
{
	DrawOverlay(g);
}

void Widget::SetColors(int theColors[][3], int theNumColors) //118-124
{
	mColors.clear();

	for (int i = 0; i < theNumColors; i++)
		SetColor(i, Color(theColors[i][0], theColors[i][1], theColors[i][2]));
	MarkDirty();
}

void Widget::SetColors(int theColors[][4], int theNumColors) //127-134
{	
	mColors.clear();

	for (int i = 0; i < theNumColors; i++)
		SetColor(i, Color(theColors[i][0], theColors[i][1], theColors[i][2], theColors[i][3]));		

	MarkDirty();
}

void Widget::SetColor(int theIdx, const Color& theColor) //137-143
{
	if (theIdx >= (int)mColors.size())
		mColors.resize(theIdx + 1);

	mColors[theIdx] = theColor;
	MarkDirty();
}

const Color& Widget::GetColor(int theIdx) //146-151
{
	static Color aColor;
	if (theIdx < (int) mColors.size())
		return mColors[theIdx];
	return aColor;
}

Color Widget::GetColor(int theIdx, const Color& theDefaultColor) //154-158
{
	if (theIdx < (int) mColors.size())
		return mColors[theIdx];
	return theDefaultColor;
}

void Widget::Resize(int theX, int theY, int theWidth, int theHeight) //161-178
{
	if ((mX == theX) && (mY == theY) && (mWidth == theWidth) && (mHeight == theHeight))
		return;

	// Mark everything dirty that is over or under the old position
	MarkDirtyFull();
	
	mX = theX;
	mY = theY;
	mWidth = theWidth;
	mHeight = theHeight;
		
	// Mark things dirty that are over the new position
	MarkDirty();

	if (mWidgetManager != NULL)
		mWidgetManager->RehupMouse();
}

void Widget::Resize(const Rect& theRect) //181-183
{
	Resize(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

void Widget::Move(int theNewX, int theNewY) //186-188
{
	Resize(theNewX, theNewY, mWidth, mHeight);
}

bool Widget::WantsFocus() //191-193
{
	return mWantsFocus;
}

void Widget::SetDisabled(bool isDisabled) //196-210
{
	if (mDisabled == isDisabled)
		return;

	mDisabled = isDisabled;

	if ((isDisabled) && (mWidgetManager != NULL))
		mWidgetManager->DisableWidget(this);
		
	MarkDirty();
	
	// Incase a widget is enabled right under our cursor
	if ((!isDisabled) && (mWidgetManager != NULL) && (Contains(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY)))
		mWidgetManager->MousePosition(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY);
}

void Widget::GotFocus() //213-215
{
	mHasFocus = true;		
}

void Widget::LostFocus() //218-220
{
	mHasFocus = false;		
}

void Widget::Update() //223-225
{
	WidgetContainer::Update();
}

void Widget::UpdateF(float theFrac) //228-229
{
}

void Widget::KeyChar(SexyChar theChar) //232-233
{
}

void Widget::KeyDown(KeyCode theKey) //236-250
{
	if (theKey == KEYCODE_TAB)
	{
		if (mWidgetManager->mKeyDown[KEYCODE_SHIFT])
		{
			if (mTabPrev != NULL)
				mWidgetManager->SetFocus(mTabPrev);
		}
		else
		{
			if (mTabNext != NULL)
				mWidgetManager->SetFocus(mTabNext);
		}
	}
}

void Widget::KeyUp(KeyCode theKey) //253-254
{		
}

//////// Mouse functions
void Widget::ShowFinger(bool on) //Different on XNA | 258-271
{
	if (mWidgetManager == NULL)
		return;

	if (on)
		mWidgetManager->mApp->SetCursor(CURSOR_HAND);
	else
		mWidgetManager->mApp->SetCursor(CURSOR_POINTER);

	/*if (on)
		mWidgetManager->mApplet.setCursor(new Cursor(Cursor.HAND_CURSOR));
	else
		mWidgetManager->mApplet.setCursor(new Cursor(Cursor.DEFAULT_CURSOR));*/
}

void Widget::MouseEnter() //274-276
{
	
}

void Widget::MouseLeave() //279-281
{
	
}

void Widget::MouseMove(int x, int y) //284-285
{
}

void Widget::MouseDown(int x, int y, int theClickCount) //288-295
{
	if (theClickCount == 3)
		MouseDown(x, y, 2, 1);
	else if (theClickCount >= 0)
		MouseDown(x, y, 0, theClickCount);
	else
		MouseDown(x, y, 1, -theClickCount);
}

void Widget::MouseDown(int x, int y, int theBtnNum, int theClickCount) //298-299
{
}

void Widget::MouseUp(int x, int y) //302-303
{
}

void Widget::MouseUp(int x, int y, int theLastDownButtonId) //306-315
{
	MouseUp(x, y);

	if (theLastDownButtonId == 3)
		MouseUp(x, y, 2, 1);
	else if (theLastDownButtonId >= 0)
		MouseUp(x, y, 0, theLastDownButtonId);
	else
		MouseUp(x, y, 1, -theLastDownButtonId);
}

void Widget::MouseUp(int x, int y, int theBtnNum, int theClickCount) //318-319
{
}

void Widget::MouseDrag(int x, int y) //322-323
{
}

void Widget::MouseWheel(int theDelta) //326-327
{
}

//////// Helper functions

Rect Widget::WriteCenteredLine(Graphics* g, int anOffset, const SexyString& theLine) //332-340
{
	Font* aFont = g->GetFont();
	int aWidth = aFont->StringWidth(theLine);
	int aX = (mWidth - aWidth) / 2;

	g->DrawString(theLine, aX, anOffset);

	return Rect(aX, anOffset - aFont->GetAscent(), aWidth, aFont->GetHeight());
}

Rect Widget::WriteCenteredLine(Graphics* g, int anOffset, const SexyString& theLine, Color theColor1, Color theColor2, const Point& theShadowOffset) //Not in XNA | 343-361
{
	Font* aFont = g->GetFont();
	int aWidth = aFont->StringWidth(theLine);
	int aX = (mWidth - aWidth) / 2;
	
	g->SetColor(theColor2);
	g->DrawString(theLine, (mWidth - aWidth)/2 + theShadowOffset.mX, anOffset + theShadowOffset.mY);
	
	g->SetColor(theColor1);
	g->DrawString(theLine, (mWidth - aWidth)/2, anOffset);

	// account for shadow in position and size
	// TODO: this may not be necessary.
	return Rect(
		aX + min(0,theShadowOffset.mX),
		anOffset - aFont->GetAscent() + min(0,theShadowOffset.mY), 
		aWidth + abs(theShadowOffset.mX), 
		aFont->GetHeight() + abs(theShadowOffset.mY));
}

int Widget::WriteString(Graphics* g, const SexyString& theString, int theX, int theY, int theWidth, int theJustification, bool drawString, int theOffset, int theLength) //364-371
{
	bool oldColored = g->mWriteColoredString;
	g->mWriteColoredString = mWriteColoredString;
	int aXOffset = g->WriteString(theString,theX,theY,theWidth,theJustification,drawString,theOffset,theLength);
	g->mWriteColoredString = oldColored;

	return aXOffset;
}

int	Widget::WriteWordWrapped(Graphics* g, const Rect& theRect, const SexyString& theLine, int theLineSpacing, int theJustification) //374-381
{
	bool oldColored = g->mWriteColoredString;
	g->mWriteColoredString = mWriteColoredString;
	int aReturn = g->WriteWordWrapped(theRect,theLine,theLineSpacing,theJustification);
	g->mWriteColoredString = oldColored;

	return aReturn;
}

int Widget::GetWordWrappedHeight(Graphics* g, int theWidth, const SexyString& theLine, int aLineSpacing) //384-386
{
	return g->GetWordWrappedHeight(theWidth,theLine,aLineSpacing);
}

int Widget::GetNumDigits(int theNumber) //389-399
{		
	int aDivisor = 10;
	int aNumDigits = 1;
	while (theNumber >= aDivisor)
	{
		aNumDigits++;
		aDivisor *= 10;
	}			
		
	return aNumDigits;
}

void Widget::WriteNumberFromStrip(Graphics* g, int theNumber, int theX, int theY, Image* theNumberStrip, int aSpacing) //402-425
{
	int aDivisor = 10;
	int aNumDigits = 1;
	while (theNumber >= aDivisor)
	{
		aNumDigits++;
		aDivisor *= 10;
	}
	if (theNumber == 0)
		aDivisor = 10;

	int aDigitLen = theNumberStrip->GetWidth() / 10;
	
	for (int aDigitIdx = 0; aDigitIdx < aNumDigits; aDigitIdx++)
	{				
		aDivisor /= 10;
		int aDigit = (theNumber / aDivisor) % 10;				
			
		Graphics* aClipG = g->Create();
		aClipG->PushState();
		aClipG->ClipRect(theX + aDigitIdx*(aDigitLen + aSpacing), theY, aDigitLen, theNumberStrip->GetHeight());
		aClipG->DrawImage(theNumberStrip, theX + aDigitIdx*(aDigitLen + aSpacing) - aDigit*aDigitLen, theY);		
		aClipG->PopState();
	}
}										 
								 
bool Widget::Contains(int theX, int theY) //428-431
{
	return ((theX >= mX) && (theX < mX + mWidth) &&
			(theY >= mY) && (theY < mY + mHeight));
}

Rect Widget::GetInsetRect() //434-438
{
	return Rect(mX + mMouseInsets.mLeft, mY + mMouseInsets.mTop, 
						 mWidth - mMouseInsets.mLeft - mMouseInsets.mRight,
						 mHeight - mMouseInsets.mTop - mMouseInsets.mBottom);
}

void Widget::DeferOverlay(int thePriority) //441-443
{
	mWidgetManager->DeferOverlay(this, thePriority);
}

void Widget::Layout(int theLayoutFlags, Widget *theRelativeWidget, int theLeftPad, int theTopPad, int theWidthPad, int theHeightPad) //446-504
{
	int aRelLeft = theRelativeWidget->Left();
	int aRelTop = theRelativeWidget->Top();
	if (theRelativeWidget==mParent)
	{
		aRelLeft = 0;
		aRelTop = 0;
	}

	int aRelWidth = theRelativeWidget->Width();
	int aRelHeight = theRelativeWidget->Height();
	int aRelRight = aRelLeft + aRelWidth;
	int aRelBottom = aRelTop + aRelHeight;

	int aLeft = Left();
	int aTop = Top();
	int aWidth = Width();
	int aHeight = Height();

	int aType = 1;
	while(aType<LAY_Max)
	{
		if(theLayoutFlags&aType)
		{
			switch(aType)
			{
				case LAY_SameWidth: aWidth = aRelWidth+theWidthPad; break;
				case LAY_SameHeight: aHeight = aRelHeight+theHeightPad; break;
	
				case LAY_Above: aTop = aRelTop-aHeight+theTopPad; break;
				case LAY_Below: aTop = aRelBottom+theTopPad; break;
				case LAY_Right: aLeft = aRelRight+theLeftPad; break;
				case LAY_Left:  aLeft = aRelLeft-aWidth+theLeftPad; break;
			
				case LAY_SameLeft: aLeft = aRelLeft+theLeftPad; break;
				case LAY_SameRight: aLeft = aRelRight-aWidth+theLeftPad; break;
				case LAY_SameTop: aTop = aRelTop+theTopPad; break;
				case LAY_SameBottom: aTop = aRelBottom-aHeight+theTopPad; break;

				case LAY_GrowToRight: aWidth = aRelRight-aLeft+theWidthPad; break;
				case LAY_GrowToLeft: aWidth = aRelLeft-aLeft+theWidthPad; break;
				case LAY_GrowToTop: aHeight = aRelTop-aTop+theHeightPad; break;
				case LAY_GrowToBottom: aHeight = aRelBottom-aTop+theHeightPad; break;

				case LAY_SetLeft: aLeft = theLeftPad; break;
				case LAY_SetTop: aTop = theTopPad; break;
				case LAY_SetWidth: aWidth = theWidthPad; break;
				case LAY_SetHeight: aHeight = theHeightPad; break;

				case LAY_HCenter: aLeft = aRelLeft+(aRelWidth-aWidth)/2 + theLeftPad; break;
				case LAY_VCenter: aTop = aRelTop+(aRelHeight-aHeight)/2 + theTopPad; break;
			}
		}

		aType<<=1;
	}

	Resize(aLeft,aTop,aWidth,aHeight);
}

void Widget::TouchBegan(Touch* touch) //507-511
{
	int x = touch->location.mX;
	int y = touch->location.mY;
	MouseDown(x, y, 1);
}

void Widget::TouchMoved(Touch* touch) //514-518
{
	int x = touch->location.mX;
	int y = touch->location.mY;
	MouseDrag(x, y);
}

void Widget::TouchEnded(Touch* touch) //521-525
{
	int x = touch->location.mX;
	int y = touch->location.mY;
	MouseUp(x, y, 1);
}

void Widget::TouchesCanceled() //528-529
{
}

//////// Gamepad functions

void Widget::GotGamepadSelection(WidgetLinkDir theDirection) //532-534
{
	mIsGamepadSelection = true;
}

void Widget::LostGamepadSelection() //537-539
{
	mIsGamepadSelection = false;
}
#ifdef _SEXYDECOMP_USE_LATEST_CODE
void Widget::GamepadButtonDown(GamepadButton theButton, int thePlayer, ulong theFlags)
#else
void Widget::GamepadButtonDown(int theButton, int thePlayer, ulong theFlags) //542-620
#endif
{
	switch (theButton)
	{
	case GAMEPAD_BUTTON_UP:
	case GAMEPAD_BUTTON_DPAD_UP: //Not in XNA function
		if (mGamepadLinkUp != NULL && mWidgetManager != NULL)
		{
			Widget* link = mGamepadLinkUp;
			while (link != NULL && !link->mVisible)
				link = link->mGamepadLinkUp;
			if (link != NULL)
				mWidgetManager->SetGamepadSelection(link, LINK_DIR_UP);
		}
		break;
	case GAMEPAD_BUTTON_DOWN:
	case GAMEPAD_BUTTON_DPAD_DOWN: //Not in XNA function
		if (mGamepadLinkUp != NULL && mWidgetManager != NULL)
		{
			Widget* link = mGamepadLinkDown;
			while (link != NULL && !link->mVisible)
				link = link->mGamepadLinkDown;
			if (link != NULL)
				mWidgetManager->SetGamepadSelection(link, LINK_DIR_DOWN);
		}
		break;
	case GAMEPAD_BUTTON_LEFT:
	case GAMEPAD_BUTTON_DPAD_LEFT: //Not in XNA function
		if (mGamepadLinkUp != NULL && mWidgetManager != NULL)
		{
			Widget* link = mGamepadLinkLeft;
			while (link != NULL && !link->mVisible)
				link = link->mGamepadLinkLeft;
			if (link != NULL)
				mWidgetManager->SetGamepadSelection(link, LINK_DIR_LEFT);
		}
		break;
	case GAMEPAD_BUTTON_RIGHT:
	case GAMEPAD_BUTTON_DPAD_RIGHT: //Not in XNA function
		if (mGamepadLinkUp != NULL && mWidgetManager != NULL)
		{
			Widget* link = mGamepadLinkRight;
			while (link != NULL && !link->mVisible)
				link = link->mGamepadLinkRight;
			if (link != NULL)
				mWidgetManager->SetGamepadSelection(link, LINK_DIR_RIGHT);
		}
		break;
	}
	if (mGamepadParent)
		mGamepadParent->GamepadButtonDown(theButton, thePlayer, theFlags);
}
#ifdef _SEXYDECOMP_USE_LATEST_CODE
void Widget::GamepadButtonUp(GamepadButton theButton, int thePlayer, ulong theFlags) //623-628
#else
void Widget::GamepadButtonUp(int theButton, int thePlayer, ulong theFlags) //623-628
#endif
{
	if (mGamepadParent)
	{
		mGamepadParent->GamepadButtonUp(theButton, thePlayer, theFlags);
	}
}
#ifdef _SEXYDECOMP_USE_LATEST_CODE
void Widget::GamepadAxisMove(GamepadAxis theAxis, int thePlayer, float theAxisValue)
#else
void Widget::GamepadAxisMove(int theAxis, int thePlayer, int theAxisValue) //631-636
#endif
{
	if (mGamepadParent)
	{
		mGamepadParent->GamepadAxisMove(theAxis, thePlayer, theAxisValue);
	}
}

void Widget::SetGamepadLinks(Widget* up, Widget* down, Widget* left, Widget* right) //639-644
{
	mGamepadLinkUp = up;
	mGamepadLinkDown = down;
	mGamepadLinkLeft = left;
	mGamepadLinkRight = right;
}

void Widget::SetGamepadParent(Widget* theParent) //647-649
{
	mGamepadParent = theParent;
}