#include "Common.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"
#include "SysFont.h"
#include "ImageFont.h"

using namespace Sexy;


SexyString Sexy::DIALOG_YES_STRING				= _S("YES"); //12
SexyString Sexy::DIALOG_NO_STRING				= _S("NO"); //13
SexyString Sexy::DIALOG_OK_STRING				= _S("OK"); //14
SexyString Sexy::DIALOG_CANCEL_STRING			= _S("CANCEL"); //15

static int gDialogColors[][3] = 
{{255, 255, 255},
{255, 255, 0},
{255, 255, 255},
{255, 255, 255},
{255, 255, 255},

{80, 80, 80},
{255, 255, 255}};

Dialog::Dialog(Image* theComponentImage, Image* theButtonComponentImage, int theId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode) //28-111
{
	mId = theId;
	mResult = 0x7FFFFFFF;
	mComponentImage = theComponentImage;
	mStretchBG = false;
	mIsModal = isModal;
	mContentInsets = Insets(24, 24, 24, 24);
	mTextAlign = 0;
	mLineSpacingOffset = 0;
	mSpaceAfterHeader = 10;
	mButtonSidePadding = 0;
	mButtonHorzSpacing = 8;
	mDialogListener = gSexyAppBase;

	mDialogHeader = theDialogHeader;
	mDialogFooter = theDialogFooter;
	mButtonMode = theButtonMode;

	if ((mButtonMode == BUTTONS_YES_NO) || (mButtonMode == BUTTONS_OK_CANCEL))
	{
		mYesButton = new DialogButton(theButtonComponentImage, ID_YES, this);
		mNoButton = new DialogButton(theButtonComponentImage, ID_NO, this);						

		if (mButtonMode == BUTTONS_YES_NO)
		{
			mYesButton->mLabel = DIALOG_YES_STRING;
			mNoButton->mLabel = DIALOG_NO_STRING;
		}
		else
		{
			mYesButton->mLabel = DIALOG_OK_STRING;
			mNoButton->mLabel = DIALOG_CANCEL_STRING;
		}
	}
	else if (mButtonMode == BUTTONS_FOOTER)
	{
		mYesButton = new DialogButton(theButtonComponentImage, ID_FOOTER, this);		
		mYesButton->mLabel = mDialogFooter;
		mNoButton = NULL;		
	}
	else
	{
		mYesButton = NULL;
		mNoButton = NULL;
		mNumButtons = 0;
	}

	mDialogLines = theDialogLines;

	mButtonHeight = (theButtonComponentImage == NULL) ? 24 : theButtonComponentImage->GetCelHeight();

	mHasTransparencies = true;	
	mHasAlpha = true;

	mHeaderFont = NULL; 
	mLinesFont = NULL; 

	mDragging = false;	
	mPriority = 1;

	if (theButtonComponentImage == NULL)
	{
		gDialogColors[COLOR_BUTTON_TEXT][0] = 0;
		gDialogColors[COLOR_BUTTON_TEXT][1] = 0;
		gDialogColors[COLOR_BUTTON_TEXT][2] = 0;
		gDialogColors[COLOR_BUTTON_TEXT_HILITE][0] = 0;
		gDialogColors[COLOR_BUTTON_TEXT_HILITE][1] = 0;
		gDialogColors[COLOR_BUTTON_TEXT_HILITE][2] = 0;
	}

	else
	{
		gDialogColors[COLOR_BUTTON_TEXT][0] = 255;
		gDialogColors[COLOR_BUTTON_TEXT][1] = 255;
		gDialogColors[COLOR_BUTTON_TEXT][2] = 255;
		gDialogColors[COLOR_BUTTON_TEXT_HILITE][0] = 255;
		gDialogColors[COLOR_BUTTON_TEXT_HILITE][1] = 255;
		gDialogColors[COLOR_BUTTON_TEXT_HILITE][2] = 255;
	}

	SetColors(gDialogColors, NUM_COLORS); //SetColors3 in XNA
}	


Dialog::~Dialog() //114-119
{
	RemoveAllWidgets(true);

	delete mHeaderFont;
	delete mLinesFont;
}

void Dialog::SetColor(int theIdx, const Color& theColor) //123-139
{
	Widget::SetColor(theIdx, theColor);
	
	if (theIdx == COLOR_BUTTON_TEXT)
	{
		if (mYesButton != NULL)
			mYesButton->SetColor(DialogButton::COLOR_LABEL, theColor);
		if (mNoButton != NULL)
			mNoButton->SetColor(DialogButton::COLOR_LABEL, theColor);
	}
	else if (theIdx == COLOR_BUTTON_TEXT_HILITE)
	{
		if (mYesButton != NULL)
			mYesButton->SetColor(DialogButton::COLOR_LABEL_HILITE, theColor);
		if (mNoButton != NULL)
			mNoButton->SetColor(DialogButton::COLOR_LABEL_HILITE, theColor);
	}	
}

void Dialog::SetButtonFont(Font* theFont) //142-148
{
	if (mYesButton != NULL)
		mYesButton->SetFont(theFont);

	if (mNoButton != NULL)
		mNoButton->SetFont(theFont);
}

void Dialog::SetHeaderFont(Font* theFont) //151-154
{
	delete mHeaderFont;
	mHeaderFont = theFont->Duplicate();
}

void Dialog::SetLinesFont(Font* theFont) //157-160
{
	delete mLinesFont;
	mLinesFont = theFont->Duplicate();
}

void Dialog::EnsureFonts() //Stubbed in non-C++ | 163-170
{
	if (mHeaderFont == NULL)
		mHeaderFont = new SysFont(gSexyAppBase, "Arial Unicode MS", 14);
	if (mLinesFont == NULL)
		mLinesFont = new SysFont(gSexyAppBase, "Arial Unicode MS", 12);
}

int	Dialog::GetPreferredHeight(int theWidth) //173-211
{
	EnsureFonts();

	int aHeight = mContentInsets.mTop + mContentInsets.mBottom + mBackgroundInsets.mTop + mBackgroundInsets.mBottom;

	bool needSpace = false;
	if (mDialogHeader.length() > 0)
	{
		aHeight += mHeaderFont->GetHeight() - mHeaderFont->GetAscentPadding();
		needSpace = true;
	}
	
	if (mDialogLines.length() > 0)
	{
		if (needSpace)
			aHeight += mSpaceAfterHeader;
		Graphics g;
		g.SetFont(mLinesFont);	
		aHeight += GetWordWrappedHeight(&g, theWidth-mContentInsets.mLeft-mContentInsets.mRight-mBackgroundInsets.mLeft-mBackgroundInsets.mRight-4, mDialogLines, mLinesFont->GetLineSpacing() + mLineSpacingOffset);
		needSpace = true;
	}

	if ((mDialogFooter.length() != 0) && (mButtonMode != BUTTONS_FOOTER))
	{
		if (needSpace)
			aHeight += 8;
		aHeight += mHeaderFont->GetLineSpacing();
		needSpace = true;
	}

	if (mYesButton != NULL)
	{
		if (needSpace)
			aHeight += 8;
		aHeight += mButtonHeight + 8;		
	}	

	return aHeight;
}

void Dialog::Draw(Graphics* g) //214-274
{
	EnsureFonts();

	Rect aBoxRect(mBackgroundInsets.mLeft,mBackgroundInsets.mTop,mWidth-mBackgroundInsets.mLeft-mBackgroundInsets.mRight,mHeight-mBackgroundInsets.mTop-mBackgroundInsets.mBottom);
	if (mComponentImage != NULL)
	{
		!mStretchBG ? g->DrawImageBox(aBoxRect,mComponentImage) : g->DrawImage(mComponentImage, aBoxRect, Rect(0,0,mComponentImage->mWidth, mComponentImage->mHeight)); //?
	}
	else
	{
		g->SetColor(GetColor(COLOR_OUTLINE, Color(gDialogColors[COLOR_OUTLINE])));
		g->DrawRect(12, 12, mWidth - 12*2 - 1, mHeight - 12*2 - 1);
		g->SetColor(GetColor(COLOR_BKG, Color(gDialogColors[COLOR_BKG])));
		g->FillRect(12+1, 12+1, mWidth - 12*2 - 2, mHeight - 12*2 - 2);
		
		g->SetColor(Color(0, 0, 0, 128));
		g->FillRect(mWidth - 12, 12*2, 12, mHeight - 12*3);
		g->FillRect(12*2, mHeight-12, mWidth - 12*2, 12);
	}

	int aCurY = mContentInsets.mTop + mBackgroundInsets.mTop;

	if (mDialogHeader.length() > 0)
	{
		aCurY += mHeaderFont->GetAscent() - mHeaderFont->GetAscentPadding();
		
		g->SetFont(mHeaderFont);
		g->SetColor(mColors[COLOR_HEADER]);
		WriteCenteredLine(g, aCurY, mDialogHeader);		

		aCurY += mHeaderFont->GetHeight() - mHeaderFont->GetAscent();

		aCurY += mSpaceAfterHeader;
	}

	//g->SetFont(mLinesFont);	
	g->SetFont(mLinesFont);
	g->SetColor(mColors[COLOR_LINES]);
	
	/*for (int i = 0; i < mDialogLines.size(); i++)
	{
		WriteCenteredLine(g, aCurY, mDialogLines[i]);
		aCurY += mLinesFont->GetHeight();
	}*/	

	Rect aRect(mBackgroundInsets.mLeft+mContentInsets.mLeft+2, aCurY, mWidth-mContentInsets.mLeft-mContentInsets.mRight-mBackgroundInsets.mLeft-mBackgroundInsets.mRight-4, 0);
	aCurY += WriteWordWrapped(g, aRect, mDialogLines, mLinesFont->GetLineSpacing() + mLineSpacingOffset, mTextAlign);	

	if ((mDialogFooter.length() != 0) && (mButtonMode != BUTTONS_FOOTER))
	{		
		aCurY += 8;
		aCurY += mHeaderFont->GetLineSpacing();
		
		g->SetFont(mHeaderFont);
		g->SetColor(mColors[COLOR_FOOTER]);
		WriteCenteredLine(g, aCurY, mDialogFooter);				
	}	
}

void Dialog::Resize(int theX, int theY, int theWidth, int theHeight) //277-295
{
	Widget::Resize(theX, theY, theWidth, theHeight);	

	if ((mYesButton != NULL) && (mNoButton != NULL))
	{
		int aBtnWidth = (mWidth - mContentInsets.mLeft - mContentInsets.mRight - mBackgroundInsets.mLeft - mBackgroundInsets.mRight - mButtonSidePadding*2 - mButtonHorzSpacing) / 2;
		int aBtnHeight = mButtonHeight;

		mYesButton->Resize(mX + mBackgroundInsets.mLeft + mContentInsets.mLeft + mButtonSidePadding, mY + mHeight - mContentInsets.mBottom - mBackgroundInsets.mBottom - aBtnHeight, aBtnWidth, aBtnHeight);
		mNoButton->Resize(mYesButton->mX + aBtnWidth + mButtonHorzSpacing, mYesButton->mY, aBtnWidth, aBtnHeight);
	}
	else if (mYesButton != NULL)
	{
		int aBtnHeight = mButtonHeight;

		mYesButton->Resize(mX + mContentInsets.mLeft + mBackgroundInsets.mLeft, mY + mHeight - mContentInsets.mBottom - mBackgroundInsets.mBottom - aBtnHeight, 
			mWidth - mContentInsets.mLeft - mContentInsets.mRight - mBackgroundInsets.mLeft - mBackgroundInsets.mRight, aBtnHeight);	
	}
}

void Dialog::MouseDown(int x, int y, int theBtnNum, int theClickCount) //298-307
{
	if (theClickCount == 1)
	{
		mWidgetManager->mApp->SetCursor(CURSOR_DRAGGING);
		mDragging = true;
		mDragMouseX = x;
		mDragMouseY = y;
	}
	Widget::MouseDown(x,y,theBtnNum, theClickCount);
}

void Dialog::MouseDrag(int x, int y) //310-341
{
	if (mDragging)
	{
		int aNewX = mX + x - mDragMouseX;
		int aNewY = mY + y - mDragMouseY;	

		if (aNewX < -8)
			aNewX = -8;
		else if (aNewX + mWidth > mWidgetManager->mWidth + 8)
			aNewX = mWidgetManager->mWidth - mWidth + 8;

		if (aNewY < -8)
			aNewY = -8;
		else if (aNewY + mHeight > mWidgetManager->mHeight + 8)
			aNewY = mWidgetManager->mHeight- mHeight + 8;

		mDragMouseX = mX + x - aNewX;
		mDragMouseY = mY + y - aNewY;

		if (mDragMouseX < 8)
			mDragMouseX = 8;
		else if (mDragMouseX > mWidth-9)
			mDragMouseX = mWidth-9;

		if (mDragMouseY < 8)
			mDragMouseY = 8;
		else if (mDragMouseY > mHeight-9)
			mDragMouseY = mHeight-9;

		Move(aNewX, aNewY);
	}
}

void Dialog::MouseUp(int x, int y, int theBtnNum, int theClickCount) //344-351
{
	if (mDragging)
	{
		mWidgetManager->mApp->SetCursor(CURSOR_POINTER);		
		mDragging = false;
	}
	Widget::MouseUp(x,y, theBtnNum, theClickCount);
}

void Dialog::Update() //354-358
{
	Widget::Update();

	//Move(mX, mY+1);
}

bool Dialog::IsModal() //361-363
{
	return mIsModal;
}

int Dialog::WaitForResult(bool autoKill) //366-375
{	
	//gSexyAppBase->DoMainLoop(mId);	

	while ((gSexyAppBase->UpdateAppStep(NULL)) && (mWidgetManager != NULL) && (mResult == 0x7FFFFFFF));

	if (autoKill)
		gSexyAppBase->KillDialog(mId);

	return mResult;
}

void Dialog::ButtonPress(int theId) //378-381
{
	if ((theId == ID_YES) || (theId == ID_NO))
		mDialogListener->DialogButtonPress(mId, theId);
}

void Dialog::ButtonDepress(int theId) //384-390
{
	if ((theId == ID_YES) || (theId == ID_NO))
	{
		mResult = theId;		
		mDialogListener->DialogButtonDepress(mId, theId);
	}
}

void Dialog::ButtonDownTick(int theId) //393-394
{	
}

#ifdef _SEXYDECOMP_USE_LATEST_CODE//Beyond PC and not in Transmension
void Dialog::GameAxisMove(int theAxis, int theMovement, ulong thePlayer)
{
}

void Dialog::GameButtonDown(int theButton, int theMovement, ulong thePlayer)
{
}

void Dialog::GameButtonUp(int theButton, int theMovement, ulong thePlayer)
{
}

void Dialog::GotFocus()
{
	Widget::GotFocus();

	if (mYesButton != NULL)
		mWidgetManager->SetGamepadSelection(mYesButton, LINK_DIR_NONE);
	else if (mYesButton != NULL)
		mWidgetManager->SetGamepadSelection(mNoButton, LINK_DIR_NONE);
}
#endif