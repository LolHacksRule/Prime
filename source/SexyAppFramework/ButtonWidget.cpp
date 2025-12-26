#include "ButtonWidget.h"
#include "Image.h"
#include "SysFont.h"
#include "WidgetManager.h"
#include "ButtonListener.h"

using namespace Sexy;

static int gButtonWidgetColors[][3] = {
	{0, 0, 0},
	{0, 0, 0},
	{0, 0, 0},
	{255, 255, 255},
	{132, 132, 132},
	{212, 212, 212}};

ButtonWidget::ButtonWidget(int theId, ButtonListener* theButtonListener) //19-42
{
	mId = theId;
	mFont = NULL;
	mLabelJustify = BUTTON_LABEL_CENTER;
	mButtonImage = NULL;
	mOverImage = NULL;
	mDownImage = NULL;
	mDisabledImage = NULL;
	mInverted = false;
	mBtnNoDraw = false;
	mFrameNoDraw = false;
	mButtonListener = theButtonListener;
	mHasAlpha = true;
	mOverAlpha = 0;

	mOverAlphaSpeed = 0;
	mOverAlphaFadeInSpeed = 0;
	mLabelOffsetY = 0;

	mLabelOffsetX = 0;

	SetColors(gButtonWidgetColors, NUM_COLORS);

	mLastPressedBy = -1;
}

ButtonWidget::~ButtonWidget() //46-48
{
	delete mFont;
}

void ButtonWidget::SetFont(Font* theFont) //51-54
{
	delete mFont;
	mFont = theFont->Duplicate();
}

bool ButtonWidget::IsButtonDown() //57-59
{
	return mIsDown && mIsOver && !mDisabled;
}

bool ButtonWidget::HaveButtonImage(Image *theImage, const Rect &theRect) //62-64
{
	return (theImage!=NULL || theRect.mWidth!=0);
}
	
void ButtonWidget::DrawButtonImage(Graphics *g, Image *theImage, const Rect &theRect, int x, int y) //67-72
{
	if (theRect.mWidth != 0)
		mButtonImage ? g->DrawImage(mButtonImage, x, y, theRect) : g->DrawImage(theImage, x, y, theRect);
	else
		g->DrawImage(theImage, x, y); //On PC
}

void ButtonWidget::Draw(Graphics* g) //Correct? | 75-229
{
	if (mBtnNoDraw)
		return;

	
	if ((mFont == NULL) && (mLabel.length() > 0)) //C++ only
		mFont = new SysFont(mWidgetManager->mApp, "Arial Unicode MS", 10);


	bool isDown = mIsDown && mIsOver && !mDisabled; //IsButtonDown on XNA
	isDown ^= mInverted;

	int aFontX = mLabelOffsetX; // BUTTON_LABEL_LEFT
	int aFontY = mLabelOffsetY;
	
	if (mFont != NULL)
	{
		if (mLabelJustify == BUTTON_LABEL_CENTER)
			aFontX = (mWidth - mFont->StringWidth(mLabel))/2;
		else if (mLabelJustify == BUTTON_LABEL_RIGHT)
			aFontX = mWidth - mFont->StringWidth(mLabel);
		aFontY = (mHeight + mFont->GetAscent() - mFont->GetAscent()/6 - 1)/2;

		//aFontX = (mWidth - mFont->StringWidth(mLabel))/2;
		//aFontY = (mHeight - mFont->GetHeight())/2 + mFont->GetAscent() - 1;		
	}

	int anIconX = 0;
	int anIconY = 0;

	if (mIconImage != NULL)
	{
		if (mLabelJustify == BUTTON_LABEL_CENTER)
			anIconX = (mWidth - mIconImage->GetWidth()) / 2 + mLabelOffsetX;
		else if (mLabelJustify == BUTTON_LABEL_RIGHT)
			anIconX = (mWidth - mIconImage->GetWidth());
		anIconY = (mHeight - mIconImage->GetHeight() / 2 + mLabelOffsetY);
	}

	g->SetFont(mFont);
	
	if ((mButtonImage == NULL) && (mDownImage == NULL))
	{
		if (!mFrameNoDraw)
		{
			g->SetColor(mColors[COLOR_BKG]);
			g->FillRect(0, 0, mWidth, mHeight);
		}

		if (isDown)
		{
			if (!mFrameNoDraw)
			{
				g->SetColor(mColors[COLOR_DARK_OUTLINE]);
				g->FillRect(0, 0, mWidth-1, 1);
				g->FillRect(0, 0, 1, mHeight-1);
				
				g->SetColor(mColors[COLOR_LIGHT_OUTLINE]);
				g->FillRect(0, mHeight - 1, mWidth, 1);
				g->FillRect(mWidth - 1, 0, 1, mHeight);									
		
				g->SetColor(mColors[COLOR_MEDIUM_OUTLINE]);
				g->FillRect(1, 1, mWidth - 3, 1);
				g->FillRect(1, 1, 1, mHeight - 3);
			}

			if (mIsOver)
				g->SetColor(mColors[COLOR_LABEL_HILITE]);
			else
				g->SetColor(mColors[COLOR_LABEL]);

			if (mIconImage == NULL)
				g->DrawString(mLabel, aFontX + 1, aFontY + 1); //+1 on PC, correct?
			else
				g->DrawImage(mIconImage, anIconX + 1, anIconY + 1); //+1 on PC, correct?
		}
		else
		{			
			if (!mFrameNoDraw)
			{
				g->SetColor(mColors[COLOR_LIGHT_OUTLINE]);
				g->FillRect(0, 0, mWidth-1, 1);
				g->FillRect(0, 0, 1, mHeight-1);
				
				g->SetColor(mColors[COLOR_DARK_OUTLINE]);
				g->FillRect(0, mHeight - 1, mWidth, 1);
				g->FillRect(mWidth - 1, 0, 1, mHeight);									
		
				g->SetColor(mColors[COLOR_MEDIUM_OUTLINE]);
				g->FillRect(1, mHeight - 2, mWidth - 2, 1);
				g->FillRect(mWidth - 2, 1, 1, mHeight - 2);			
			}
			
			if (mIsOver)
				g->SetColor(mColors[COLOR_LABEL_HILITE]);
			else
				g->SetColor(mColors[COLOR_LABEL]);

			if (mIconImage == NULL)
				g->DrawString(mLabel, aFontX, aFontY); //Correct?
			else
				g->DrawImage(mIconImage, anIconX, anIconY); //Correct?
		}		
	}
	else
	{
		if (!isDown)
		{
			if (mDisabled && HaveButtonImage(mDisabledImage,mDisabledRect))
				DrawButtonImage(g,mDisabledImage,mDisabledRect,0,0);
			else if ((mOverAlpha > 0) && HaveButtonImage(mOverImage,mOverRect))
			{
				if (HaveButtonImage(mButtonImage, mNormalRect)  && mOverAlpha<1)
					DrawButtonImage(g,mButtonImage,mNormalRect,0,0);

				g->SetColorizeImages(true);
				g->SetColor(Color(255,255,255,(int)(mOverAlpha * 255)));
				DrawButtonImage(g,mOverImage,mOverRect,0,0);
				g->SetColorizeImages(false);
			}
			else if ((mIsOver || mIsDown) && HaveButtonImage(mOverImage,mOverRect))
			{
				DrawButtonImage(g,mOverImage,mOverRect,0,0);
			}
			else if (HaveButtonImage(mButtonImage,mNormalRect))
				DrawButtonImage(g,mButtonImage,mNormalRect,0,0);

			if (mIsOver)
				g->SetColor(mColors[COLOR_LABEL_HILITE]);
			else
				g->SetColor(mColors[COLOR_LABEL]);
			
			if (mIconImage == NULL)
				g->DrawString(mLabel, aFontX, aFontY); //Correct?
			else
				g->DrawImage(mIconImage, anIconX, anIconY); //Correct?
		}
		else
		{
			if (HaveButtonImage(mDownImage, mDownRect))
				DrawButtonImage(g, mDownImage, mDownRect, 0, 0);
			else if (HaveButtonImage(mOverImage,mOverRect))
				DrawButtonImage(g, mOverImage, mOverRect, 1, 1);
			else
				DrawButtonImage(g, mButtonImage, mNormalRect, 1, 1);

			g->SetColor(mColors[COLOR_LABEL_HILITE]);
			
			if (mIconImage == NULL)
				g->DrawString(mLabel, aFontX, aFontY); //Correct?
			else
				g->DrawImage(mIconImage, anIconX, anIconY); //Correct?
		}
	}
}

void ButtonWidget::SetDisabled(bool isDisabled) //232-237
{
	Widget::SetDisabled(isDisabled);
	
	if (HaveButtonImage(mDisabledImage,mDisabledRect))
		MarkDirty();
}

void ButtonWidget::MouseEnter() //240-250
{
	Widget::MouseEnter();

	if (mOverAlphaFadeInSpeed==0 && mOverAlpha>0)
		mOverAlpha = 0;
	
	if (mIsDown || (HaveButtonImage(mOverImage,mOverRect)) || (mColors[COLOR_LABEL_HILITE] != mColors[COLOR_LABEL]))
		MarkDirty();
	//XNA calls MarkDirty also here
	mButtonListener->ButtonMouseEnter(mId);
}

void ButtonWidget::MouseLeave() //HasFocus points to mWidgetManager.mApp in XNA | 253-265
{
	Widget::MouseLeave();

	if (mOverAlphaSpeed==0 && mOverAlpha>0)
		mOverAlpha = 0;
	else if (mOverAlphaSpeed > 0 && mOverAlpha == 0 && gSexyAppBase->mHasFocus) // fade out from full
		mOverAlpha = min(1.0, mOverAlphaSpeed * 10.0); //Changed to min check

	if (mIsDown || HaveButtonImage(mOverImage,mOverRect) || (mColors[COLOR_LABEL_HILITE] != mColors[COLOR_LABEL]))
		MarkDirty();
	
	mButtonListener->ButtonMouseLeave(mId);
}

void ButtonWidget::MouseMove(int theX, int theY) //267-272
{
	Widget::MouseMove(theX, theY);
	
	mButtonListener->ButtonMouseMove(mId, theX, theY);
}

void ButtonWidget::MouseDown(int theX, int theY, int theBtnNum, int theClickCount) //275-281
{
	Widget::MouseDown(theX, theY, theBtnNum, theClickCount);
		
	mButtonListener->ButtonPress(mId, theClickCount);
	
	MarkDirty();
}

void ButtonWidget::MouseUp(int theX, int theY, int theBtnNum, int theClickCount) //284-291
{	
	Widget::MouseUp(theX, theY, theBtnNum, theClickCount);
	
	if (mIsOver && mWidgetManager->mHasFocus)
		mButtonListener->ButtonDepress(mId);
	
	MarkDirty();
}

void ButtonWidget::Update() //294-320
{
	Widget::Update();

	if (mIsDown && mIsOver)
		mButtonListener->ButtonDownTick(mId);

	if (!mIsDown && !mIsOver && (mOverAlpha > 0))
	{
		if (mOverAlphaSpeed>0)
		{
			mOverAlpha -= mOverAlphaSpeed;
			if (mOverAlpha < 0)
				mOverAlpha = 0;
		}
		else
			mOverAlpha = 0;

		MarkDirty();
	}
	else if (mIsOver && mOverAlphaFadeInSpeed>0 && mOverAlpha<1)
	{
		mOverAlpha += mOverAlphaFadeInSpeed;
		if (mOverAlpha > 1)
			mOverAlpha = 1;
		MarkDirty();
	}
}

void ButtonWidget::GotGamepadSelection(WidgetLinkDir theDirection) //323-329
{
	Widget::GotGamepadSelection(theDirection);

	//

	mIsOver = true;
}

void ButtonWidget::LostGamepadSelection() //332-336
{
	Widget::LostGamepadSelection();
	mIsOver = false;
	mIsDown = false;
}
#ifdef _SEXYDECOMP_USE_LATEST_CODE
void ButtonWidget::GamepadButtonDown(GamepadButton theButton, int thePlayer, uint theFlags)
#else
void ButtonWidget::GamepadButtonDown(int theButton, int thePlayer, ulong theFlags) //339-369
#endif
{
	if (theButton == GAMEPAD_BUTTON_A)
	{
		if ((theFlags & 1) == 0)
		{
			mLastPressedBy = thePlayer;
			OnPressed();
			mIsDown = true;
			if (mButtonListener != NULL)
				mButtonListener->ButtonPress(mId, 1);
			MarkDirty();
		}
	}
	else if (mIsDown)
	{
		if (mGamepadParent != NULL)
			mGamepadParent->GamepadButtonDown(theButton, thePlayer, theFlags);
	}
	else
		Widget::GamepadButtonDown(theButton, thePlayer, theFlags);
}
#ifdef _SEXYDECOMP_USE_LATEST_CODE
void ButtonWidget::GamepadButtonUp(GamepadButton theButton, int thePlayer, uint theFlags) //372-385
#else
void ButtonWidget::GamepadButtonUp(int theButton, int thePlayer, ulong theFlags)
#endif
{
	if (theButton == GAMEPAD_BUTTON_A)
	{
		if (mIsDown)
		{
			mLastPressedBy = thePlayer;
			if (mButtonListener != NULL)
				mButtonListener->ButtonPress(mId, 1);
			mIsDown = true;
			MarkDirty();
		}
	}
	else
		Widget::GamepadButtonUp(theButton, thePlayer, theFlags);
}

void ButtonWidget::OnPressed() //389-390
{
}