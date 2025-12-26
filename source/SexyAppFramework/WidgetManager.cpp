#include "WidgetManager.h"
#include "Widget.h"
#include "Graphics.h"
#include "Image.h"
#include "KeyCodes.h" //?
#include "DeviceImage.h"
#include "SexyAppBase.h"
#include "PerfTimer.h"
#include "Debug.h"

#include <math.h>

using namespace Sexy;
using namespace std;

static Point			NO_TOUCH_MOUSE_POS(-1,-1); //16

WidgetManager::WidgetManager(SexyAppBase* theApp) //19-47
{
	mApp = theApp;

	mMinDeferredOverlayPriority = 0x7FFFFFFF;
	mWidgetManager = this;	
	mMouseIn = false;
	mDefaultTab = NULL;
	mImage = NULL;
	mLastHadTransients = false;
	mPopupCommandWidget = NULL;
	mFocusWidget = NULL;
	mLastDownWidget = NULL;
	mOverWidget = NULL;
	mBaseModalWidget = NULL;
	mGamepadSelectionWidget = NULL;
	mDefaultBelowModalFlagsMod.mRemoveFlags = WIDGETFLAGS_ALLOW_MOUSE | WIDGETFLAGS_ALLOW_FOCUS;	
	mWidth = 0;
	mHeight = 0;
	mHasFocus = true;
	mUpdateCnt = 0;
	mLastDownButtonId = 0;
	mDownButtons = 0;
	mActualDownButtons = 0;
	mWidgetFlags = WIDGETFLAGS_UPDATE | WIDGETFLAGS_DRAW | WIDGETFLAGS_CLIP |
		WIDGETFLAGS_ALLOW_MOUSE | WIDGETFLAGS_ALLOW_FOCUS;

	for (int i = 0; i < 0xFF; i++)
		mKeyDown[i] = false;
}

WidgetManager::~WidgetManager() //50-52
{	
	FreeResources();	
}

void WidgetManager::FreeResources() //55-57
{	
	
}

void WidgetManager::DisableWidget(Widget* theWidget) //60-85
{
	if (mOverWidget == theWidget)
	{
		Widget* aOverWidget = mOverWidget;
		mOverWidget = NULL;
		MouseLeave(aOverWidget);
	}
	
	if (mLastDownWidget	== theWidget)
	{
		Widget* aLastDownWidget = mLastDownWidget;
		mLastDownWidget = NULL;
		DoMouseUps(aLastDownWidget, mDownButtons);
		mDownButtons = 0;		
	}
	
	if (mFocusWidget == theWidget)
	{
		Widget* aFocusWidget = mFocusWidget;
		mFocusWidget = NULL;
		aFocusWidget->LostFocus();
	}
	
	if (mBaseModalWidget == theWidget)
		mBaseModalWidget = NULL;
}

int WidgetManager::GetWidgetFlags() //88-90
{
	return mHasFocus ? mWidgetFlags : GetModFlags(mWidgetFlags, mLostFocusFlagsMod);
}

Widget* WidgetManager::GetAnyWidgetAt(int x, int y, int* theWidgetX, int* theWidgetY) //93-96
{
	bool found;
	return GetWidgetAtHelper(x, y, GetWidgetFlags(), &found, theWidgetX, theWidgetY);
}

Widget* WidgetManager::GetWidgetAt(int x, int y, int* theWidgetX, int* theWidgetY) //99-104
{	
	Widget* aWidget = GetAnyWidgetAt(x, y, theWidgetX, theWidgetY);
	if ((aWidget != NULL) && (aWidget->mDisabled))
		aWidget = NULL;
	return aWidget;
}

bool WidgetManager::IsLeftButtonDown() //107-109
{
	return (mActualDownButtons&1)?true:false;
}

bool WidgetManager::IsMiddleButtonDown() //112-114
{
	return (mActualDownButtons&4)?true:false;
}

bool WidgetManager::IsRightButtonDown() //117-119
{
	return (mActualDownButtons&2)?true:false;
}

void WidgetManager::DoMouseUps() //122-129
{
	if (mLastDownWidget != NULL && mDownButtons != 0)
	{
		DoMouseUps(mLastDownWidget, mDownButtons);
		mDownButtons = 0;
		mLastDownWidget = NULL;
	}
}

void WidgetManager::DeferOverlay(Widget* theWidget, int thePriority) //132-136
{
	mDeferredOverlayWidgets.push_back(std::pair<Widget*, int>(theWidget, thePriority));
	if (thePriority < mMinDeferredOverlayPriority)
		mMinDeferredOverlayPriority = thePriority;
}

void WidgetManager::FlushDeferredOverlayWidgets(int theMaxPriority) //Looks different but to be safe | 139-187
{
	if (mCurG == NULL)
		return;

	Graphics g(*mCurG);
	while (mMinDeferredOverlayPriority <= theMaxPriority)
	{
		int aNextMinPriority = 0x7FFFFFFF;
		for (int i = 0; i < (int)mDeferredOverlayWidgets.size(); i++)
		{
			Widget* aWidget = mDeferredOverlayWidgets[i].first;
			if (aWidget != NULL)
			{
				int aPriority = mDeferredOverlayWidgets[i].second;
				if (aPriority == mMinDeferredOverlayPriority)
				{
					// Overlays don't get clipped
					g.PushState();
					g.Translate(-mMouseDestRect.mX, -mMouseDestRect.mY);
					g.Translate(aWidget->mX, aWidget->mY);
					g.SetFastStretch(!g.Is3D());
					g.SetLinearBlend(g.Is3D());
					mDeferredOverlayWidgets[i].first = NULL;
					aWidget->DrawOverlay(&g, aPriority);
					g.PopState();
				}
				else
				{
					if (aPriority < aNextMinPriority)
						aNextMinPriority = aPriority;
				}
			}
		}
		mMinDeferredOverlayPriority = aNextMinPriority;
		if (aNextMinPriority == 0x7FFFFFFF)
		{
			// No more widgets lined up for overlays, clear our vector
			mDeferredOverlayWidgets.resize(0);
			break;
		}
	}
}

void WidgetManager::DoMouseUps(Widget* theWidget, ulong theDownCode) //190-200
{
	int aClickCountTable[3] = { 1,-1, 3 };
	for (int i = 0; i < 3; i++)
	{
		if ((theDownCode & (1 << i)) != 0)
		{
			theWidget->mIsDown = false;
			theWidget->MouseUp(mLastMouseX - theWidget->mX, mLastMouseY - theWidget->mY, aClickCountTable[i]);
		}
	}
}

void WidgetManager::RemapMouse(int& theX, int& theY) //203-209
{
	if (mMouseSourceRect.mWidth != 0 && mMouseSourceRect.mHeight != 0)
	{
		theX = (theX - mMouseSourceRect.mX) * mMouseDestRect.mWidth / mMouseSourceRect.mWidth + mMouseDestRect.mX;
		theY = (theY - mMouseSourceRect.mY) * mMouseDestRect.mHeight / mMouseSourceRect.mHeight + mMouseDestRect.mY;
	}
}

void WidgetManager::MouseEnter(Widget* theWidget) //212-218
{
	theWidget->mIsOver = true;

	theWidget->MouseEnter();
	if (theWidget->mDoFinger)
		theWidget->ShowFinger(true);
}

void WidgetManager::MouseLeave(Widget* theWidget) //221-227
{
	theWidget->mIsOver = false;

	theWidget->MouseLeave();
	if (theWidget->mDoFinger)
		theWidget->ShowFinger(false);
}

void WidgetManager::SetBaseModal(Widget* theWidget, const FlagsMod& theBelowFlagsMod) ///230-259
{
	mBaseModalWidget = theWidget;
	mBelowModalFlagsMod = theBelowFlagsMod;

	if ((mOverWidget != NULL) && (mBelowModalFlagsMod.mRemoveFlags & WIDGETFLAGS_ALLOW_MOUSE) &&
		(IsBelow(mOverWidget, mBaseModalWidget)))
	{
		Widget* aWidget = mOverWidget;
		mOverWidget = NULL;
		MouseLeave(aWidget);
	}

	if ((mLastDownWidget != NULL) && (mBelowModalFlagsMod.mRemoveFlags & WIDGETFLAGS_ALLOW_MOUSE) &&
		(IsBelow(mLastDownWidget, mBaseModalWidget)))
	{
		Widget* aWidget = mLastDownWidget;
		int aDownButtons = mDownButtons;
		mDownButtons = 0;
		mLastDownWidget = NULL;
		DoMouseUps(aWidget, aDownButtons);
	}

	if ((mFocusWidget != NULL) && (mBelowModalFlagsMod.mRemoveFlags & WIDGETFLAGS_ALLOW_FOCUS) &&
		(IsBelow(mFocusWidget, mBaseModalWidget)))
	{
		Widget* aWidget = mFocusWidget;
		mFocusWidget = NULL;
		aWidget->LostFocus();
	}
}

void WidgetManager::AddBaseModal(Widget* theWidget, const FlagsMod& theBelowFlagsMod) //262-271
{
	PreModalInfo aPreModalInfo;
	aPreModalInfo.mBaseModalWidget = theWidget;
	aPreModalInfo.mPrevBaseModalWidget = mBaseModalWidget;
	aPreModalInfo.mPrevFocusWidget = mFocusWidget;
	aPreModalInfo.mPrevBelowModalFlagsMod = mBelowModalFlagsMod;
	mPreModalInfoList.push_back(aPreModalInfo);

	SetBaseModal(theWidget, theBelowFlagsMod);
}

void WidgetManager::AddBaseModal(Widget* theWidget) //274-276
{
	AddBaseModal(theWidget, mDefaultBelowModalFlagsMod);
}

void WidgetManager::RemoveBaseModal(Widget* theWidget) //279-318
{
	DBG_ASSERT(mPreModalInfoList.size() > 0); //280 | 282 BejLiveWin8

	bool first = true;

	while (mPreModalInfoList.size() > 0)
	{
		PreModalInfo* aPreModalInfo = &mPreModalInfoList.back();

		if ((first) && (aPreModalInfo->mBaseModalWidget != theWidget))
		{
			// We don't remove it yet, because we want to restore
			//  its keyboard focused widget and crap later
			return;
		}

		// If we removed a widget's self from pre-modal info before
		//  then that means the dialog got removed out-of-order but we
		//  deferred setting the state back until now
		bool done = (aPreModalInfo->mPrevBaseModalWidget != NULL) ||
			(mPreModalInfoList.size() == 1);

		SetBaseModal(aPreModalInfo->mPrevBaseModalWidget,
			aPreModalInfo->mPrevBelowModalFlagsMod);

		if (mFocusWidget == NULL)
		{
			mFocusWidget = aPreModalInfo->mPrevFocusWidget;
			if (mFocusWidget != NULL)
				mFocusWidget->GotFocus();
		}

		mPreModalInfoList.pop_back();

		if (done)
			break;

		first = false;
	}
}

void WidgetManager::Resize(const Rect& theMouseDestRect, const Rect& theMouseSourceRect) //321-326
{
	mWidth = theMouseDestRect.mWidth + 2 * theMouseDestRect.mX;
	mHeight = theMouseDestRect.mHeight + 2 * theMouseDestRect.mY;
	mMouseDestRect = theMouseDestRect;
	mMouseSourceRect = theMouseSourceRect;
}

void WidgetManager::SetFocus(Widget* aWidget) //329-345
{
	if (aWidget == mFocusWidget)
		return;

	if (mFocusWidget != NULL)
		mFocusWidget->LostFocus();

	if ((aWidget != NULL) && (aWidget->mWidgetManager == this))
	{
		mFocusWidget = aWidget;

		if ((mHasFocus) && (mFocusWidget != NULL))
			mFocusWidget->GotFocus();
	}
	else
		mFocusWidget = NULL;
}

void WidgetManager::GotFocus() //348-356
{
	if (!mHasFocus)
	{
		mHasFocus = true;

		if (mFocusWidget != NULL)
			mFocusWidget->GotFocus();
	}
}

void WidgetManager::LostFocus() //359-374
{
	if (mHasFocus)
	{
		mActualDownButtons = 0;
		for (int aKeyNum = 0; aKeyNum < 0xFF; aKeyNum++)
		{
			if (mKeyDown[aKeyNum])
				KeyUp((KeyCode)aKeyNum);
		}

		mHasFocus = false;

		if (mFocusWidget != NULL)
			mFocusWidget->LostFocus();
	}
}

void WidgetManager::InitModalFlags(ModalFlags* theModalFlags) //377-381
{
	theModalFlags->mIsOver = mBaseModalWidget == NULL;
	theModalFlags->mOverFlags = GetWidgetFlags();
	theModalFlags->mUnderFlags = GetModFlags(theModalFlags->mOverFlags, mBelowModalFlagsMod);
}

void WidgetManager::DrawWidgetsTo(Graphics* g) //Not in H5 | 384-425
{
	g->Translate(mMouseDestRect.mX, mMouseDestRect.mY);
	mCurG = g;

	mDeferredOverlayWidgets.clear();
	ModalFlags aModalFlags;
	InitModalFlags(&aModalFlags);

	WidgetList::iterator anItr = mWidgets.begin();
	while (anItr != mWidgets.end())
	{
		Widget* aWidget = *anItr;

		if (aWidget->mVisible)
		{
			Graphics aG(*g);
			aG.PushState();
			aG.SetFastStretch(!aG.Is3D());
			aG.SetLinearBlend(aG.Is3D());
			aG.Translate(-mMouseDestRect.mX, - mMouseDestRect.mY);
			aG.Translate(aWidget->mX, aWidget->mY);
			aWidget->DrawAll(&aModalFlags, &aG);
			aG.PopState();
		}

		++anItr;
	}
	FlushDeferredOverlayWidgets(0x7FFFFFFF);
	//~mDeferredOverlayWidgets;

	mCurG = NULL;
}

bool WidgetManager::DrawScreen() //428-501
{
	SEXY_AUTO_PERF("WidgetManager::DrawScreen");
	//DWORD start = timeGetTime();

	ModalFlags aModalFlags;
	InitModalFlags(&aModalFlags);

	bool drewStuff = false;

	int aDirtyCount = 0;

	// Survey
	WidgetList::iterator anItr = mWidgets.begin();
	while (anItr != mWidgets.end())
	{
		Widget* aWidget = *anItr;
		if (aWidget->mDirty)
			aDirtyCount++;
		++anItr;
	}

	mMinDeferredOverlayPriority = 0x7FFFFFFF;
	mDeferredOverlayWidgets.resize(0);

	Graphics aScrG(mImage);
	mCurG = &aScrG;

	DeviceImage* aDDImage = dynamic_cast<DeviceImage*>(mImage);
	bool surfaceLocked = false;
	if (aDDImage != NULL)
		surfaceLocked = aDDImage->LockSurface();

	if (aDirtyCount > 0)
	{
		Graphics g(aScrG);
		g.Translate(-mMouseDestRect.mX, -mMouseDestRect.mY);
		bool is3D = mApp->Is3DAccelerated();

		WidgetList::iterator anInnerItr = mWidgets.begin();
		while (anInnerItr != mWidgets.end())
		{
			Widget* aWidget = *anInnerItr;

			if (aWidget == mWidgetManager->mBaseModalWidget)
				aModalFlags.mIsOver = true;

			if ((aWidget->mDirty) && (aWidget->mVisible))
			{
				g.PushState();
				g.SetFastStretch(!is3D);
				g.SetLinearBlend(is3D);
				g.Translate(aWidget->mX, aWidget->mY);
				aWidget->DrawAll(&aModalFlags, &g);

				aDirtyCount++;
				drewStuff = true;
				aWidget->mDirty = false;
				g.PopState();
			}

			++anItr;
		}
	}

	FlushDeferredOverlayWidgets(0x7FFFFFFF);

	if (aDDImage != NULL && surfaceLocked)
		aDDImage->UnlockSurface();

	mCurG = NULL;

	return drewStuff;
}

bool WidgetManager::UpdateFrame() //504-516
{
	SEXY_AUTO_PERF("WidgetManager::UpdateFrame");

	ModalFlags aModalFlags;
	InitModalFlags(&aModalFlags);

	// Keep us from having mLastWMUpdateCount interfere with our own updating
	mUpdateCnt++;
	mLastWMUpdateCount = mUpdateCnt;
	UpdateAll(&aModalFlags);

	return mDirty;
}

bool WidgetManager::UpdateFrameF(float theFrac) //519-527
{
	SEXY_AUTO_PERF("WidgetManager::UpdateFrame");

	ModalFlags aModalFlags;
	InitModalFlags(&aModalFlags);

	UpdateFAll(&aModalFlags, theFrac);
	return mDirty;
}

void WidgetManager::SetPopupCommandWidget(Widget* theList) //530-533
{
	mPopupCommandWidget = theList;
	AddWidget(mPopupCommandWidget);
}

void WidgetManager::RemovePopupCommandWidget() //536-543
{
	if (mPopupCommandWidget != NULL)
	{
		Widget* aWidget = mPopupCommandWidget;
		mPopupCommandWidget = NULL;
		RemoveWidget(aWidget);
	}
}

void WidgetManager::MousePosition(int x, int y) //546-578
{

	int aLastMouseX = mLastMouseX;
	int aLastMouseY = mLastMouseY;

	mLastMouseX = x;
	mLastMouseY = y;

	int aWidgetX;
	int aWidgetY;
	Widget* aWidget = GetWidgetAt(x, y, &aWidgetX, &aWidgetY);

	if (aWidget != mOverWidget)
	{
		Widget* aLastOverWidget = mOverWidget;
		mOverWidget = NULL;

		if (aLastOverWidget != NULL)
			MouseLeave(aLastOverWidget);

		mOverWidget = aWidget;
		if (aWidget != NULL)
		{
			MouseEnter(aWidget);
			aWidget->MouseMove(aWidgetX, aWidgetY);
		}
	}
	else if ((aLastMouseX != x) || (aLastMouseY != y))
	{
		if (aWidget != NULL)
			aWidget->MouseMove(aWidgetX, aWidgetY);
	}
}

void WidgetManager::RehupMouse() //581-598
{
	if (mLastDownWidget != NULL)
	{
		if (mOverWidget != NULL)
		{
			Widget* aWidgetOver = GetWidgetAt(mLastMouseX, mLastMouseY, NULL, NULL);

			if (aWidgetOver != mLastDownWidget)
			{
				Widget* anOverWidget = mOverWidget;
				mOverWidget = NULL;
				MouseLeave(anOverWidget);
			}
		}
	}
	else if (mMouseIn)
		MousePosition(mLastMouseX, mLastMouseY);
}

bool WidgetManager::MouseUp(int x, int y, int theClickCount) //601-634
{
	mLastInputUpdateCnt = mUpdateCnt;

	int aMask;

	if (theClickCount < 0)
		aMask = 0x02;
	else if (theClickCount == 3)
		aMask = 0x04;
	else
		aMask = 0x01;

	// Make sure that we thought this button was down anyway - possibly not, if we 
	//  disabled the widget already or something
	mActualDownButtons &= ~aMask;
	if ((mLastDownWidget != NULL) && ((mDownButtons & aMask) != 0))
	{
		Widget* aLastDownWidget = mLastDownWidget;

		mDownButtons &= ~aMask;
		if (mDownButtons == 0)
			mLastDownWidget = NULL;

		aLastDownWidget->mIsDown = false;
		Point anAbsPos = mLastDownWidget->GetAbsPos();
		aLastDownWidget->MouseUp(x - anAbsPos.mX, y - anAbsPos.mY, theClickCount);
	}
	else
		mDownButtons &= ~aMask;

	MousePosition(x, y);

	return true;
}

bool WidgetManager::MouseDown(int x, int y, int theClickCount) //637-701
{
	mLastInputUpdateCnt = mUpdateCnt;

	if (theClickCount < 0)
		mActualDownButtons |= 0x02;
	else if (theClickCount == 3)
		mActualDownButtons |= 0x04;
	else
		mActualDownButtons |= 0x01;

	MousePosition(x, y);

	if ((mPopupCommandWidget != NULL) && (!mPopupCommandWidget->Contains(x, y)))
		RemovePopupCommandWidget();

	int aWidgetX;
	int aWidgetY;
	Widget* aWidget = GetWidgetAt(x, y, &aWidgetX, &aWidgetY);

	// Begin mouse down options
/*
	// Option 1
	//This code sets a new widget as the mouse drag focus widget and lets the old
	//mousedownwidget think the buttons popped up.
	if ((mLastDownWidget != NULL) && (mLastDownWidget != aWidget))
	{
		DoMouseUps(mLastDownWidget, mDownButtons);
		mDownButtons = 0;
	}
*/
// Option 2
// This code passes all button downs to the mLastDownWidget 
	if (mLastDownWidget != NULL)
		aWidget = mLastDownWidget;

	// End mouse down options

	if (theClickCount < 0)
	{
		mLastDownButtonId = -1;
		mDownButtons |= 0x02;
	}
	else if (theClickCount == 3)
	{
		mLastDownButtonId = 2;
		mDownButtons |= 0x04;
	}
	else
	{
		mLastDownButtonId = 1;
		mDownButtons |= 0x01;
	}

	mLastDownWidget = aWidget;
	if (aWidget != NULL)
	{
		if (aWidget->WantsFocus())
			SetFocus(aWidget);

		aWidget->mIsDown = true;
		aWidget->MouseDown(aWidgetX, aWidgetY, theClickCount);
	}

	return true;
}

bool WidgetManager::MouseMove(int x, int y) //704-714
{
	mLastInputUpdateCnt = mUpdateCnt;

	if (mDownButtons)
		return MouseDrag(x, y);

	mMouseIn = true;
	MousePosition(x, y);

	return true;
}

bool WidgetManager::MouseDrag(int x, int y) //717-761
{
	mLastInputUpdateCnt = mUpdateCnt;

	mMouseIn = true;
	mLastMouseX = x;
	mLastMouseY = y;

	if ((mOverWidget != NULL) && (mOverWidget != mLastDownWidget))
	{
		Widget* anOverWidget = mOverWidget;
		mOverWidget = NULL;
		MouseLeave(anOverWidget);
	}

	if (mLastDownWidget != NULL)
	{
		Point anAbsPos = mLastDownWidget->GetAbsPos();

		int aWidgetX = x - anAbsPos.mX;
		int aWidgetY = y - anAbsPos.mY;
		mLastDownWidget->MouseDrag(aWidgetX, aWidgetY);

		Widget* aWidgetOver = GetWidgetAt(x, y, NULL, NULL);

		if ((aWidgetOver == mLastDownWidget) && (aWidgetOver != NULL))
		{
			if (mOverWidget == NULL)
			{
				mOverWidget = mLastDownWidget;
				MouseEnter(mOverWidget);
			}
		}
		else
		{
			if (mOverWidget != NULL)
			{
				Widget* anOverWidget = mOverWidget;
				mOverWidget = NULL;
				MouseLeave(anOverWidget);
			}
		}
	}

	return true;
}

bool WidgetManager::MouseExit(int x, int y) //764-776
{
	mLastInputUpdateCnt = mUpdateCnt;

	mMouseIn = false;

	if (mOverWidget != NULL)
	{
		MouseLeave(mOverWidget);
		mOverWidget = NULL;
	}

	return true;
}

void WidgetManager::MouseWheel(int theDelta) //779-784
{
	mLastInputUpdateCnt = mUpdateCnt;

	if (mFocusWidget != NULL)
		mFocusWidget->MouseWheel(theDelta);
}

bool WidgetManager::KeyChar(SexyChar theChar) //Not in XNA | 787-807
{
	mLastInputUpdateCnt = mUpdateCnt;

	if (theChar == KEYCODE_TAB)
	{
		//TODO: Check thing

		if (mKeyDown[KEYCODE_CONTROL])
		{
			if (mDefaultTab != NULL)
				mDefaultTab->KeyChar(theChar);

			return true;
		}
	}

	if (mFocusWidget != NULL)
		mFocusWidget->KeyChar(theChar);

	return true;
}

bool WidgetManager::KeyDown(KeyCode key) //810-820
{
	mLastInputUpdateCnt = mUpdateCnt;

	if ((key >= 0) && (key < 0xFF))
		mKeyDown[key] = true;

	if (mFocusWidget != NULL)
		mFocusWidget->KeyDown(key);

	return true;
}

bool WidgetManager::KeyUp(KeyCode key) //823-836
{
	mLastInputUpdateCnt = mUpdateCnt;

	if ((key >= 0) && (key < 0xFF))
		mKeyDown[key] = false;

	if ((key == KEYCODE_TAB) && (mKeyDown[KEYCODE_CONTROL]))
		return true;

	if (mFocusWidget != NULL)
		mFocusWidget->KeyUp(key);

	return true;
}

void WidgetManager::TouchBegan(Touch* touch) //839-870
{
	mLastInputUpdateCnt = mUpdateCnt;
	mActualDownButtons |= 0x01;
	MousePosition(touch->location.mX, touch->location.mY);
	Widget* aWidget = GetWidgetAt(touch->location.mX, touch->location.mY, NULL, NULL);
	if (mLastDownWidget == NULL)
		aWidget = mLastDownWidget;
	if (aWidget != NULL)
	{
		Point p = aWidget->GetAbsPos();
		touch->location.mX -= aWidget->mX;
		touch->location.mY -= aWidget->mY;
		touch->previousLocation.mX -= aWidget->mX;
		touch->previousLocation.mY -= aWidget->mY;
	}
	mLastDownButtonId = 1;
	mDownButtons |= 0x01;
	mLastDownWidget = aWidget;
	if (aWidget != NULL)
	{
		if (aWidget->WantsFocus())
			SetFocus(aWidget);
		aWidget->mIsDown = true;
		aWidget->TouchBegan(touch);
	}
}

void WidgetManager::TouchMoved(Touch* touch) //Correct? | 873-911
{
	mLastInputUpdateCnt = mUpdateCnt;

	mMouseIn = true;
	mLastMouseX = touch->location.mX;
	mLastMouseY = touch->location.mY;

	if ((mOverWidget != NULL) && (mOverWidget != mLastDownWidget))
	{
		Widget* anOverWidget = mOverWidget;
		mOverWidget = NULL;
		MouseLeave(anOverWidget);
	}

	if (mLastDownWidget != NULL)
	{
		Point p = mLastDownWidget->GetAbsPos();

		touch->location.mX - p.mX;
		touch->location.mY - p.mY;
		touch->previousLocation.mX - p.mX;
		touch->previousLocation.mY - p.mY;
		mLastDownWidget->TouchMoved(touch);

		Widget* aWidgetOver = GetWidgetAt(touch->location.mX, touch->location.mY, NULL, NULL); //Move?

		if ((aWidgetOver == mLastDownWidget) && (aWidgetOver != NULL))
		{
			if (mOverWidget == NULL)
			{
				mOverWidget = mLastDownWidget;
				MouseEnter(mOverWidget);
			}
		}
		else
		{
			if (mOverWidget != NULL)
			{
				Widget* anOverWidget = mOverWidget;
				mOverWidget = NULL;
				MouseLeave(anOverWidget);
			}
		}
	}
}

void WidgetManager::TouchEnded(Touch* touch) //914-941
{
	mLastInputUpdateCnt = mUpdateCnt;

	int aMask = 0x1;
	mActualDownButtons &= ~aMask;
	if ((mLastDownWidget != NULL) && ((mDownButtons & aMask) != 0))
	{
		Widget* aLastDownWidget = mLastDownWidget;

		mDownButtons &= ~aMask;
		if (mDownButtons == 0)
			mLastDownWidget = NULL;

		Point p = mLastDownWidget->GetAbsPos();
		touch->location.mX -= p.mX;
		touch->location.mY -= p.mY;
		touch->previousLocation.mX -= p.mX;
		touch->previousLocation.mY -= p.mY;
		aLastDownWidget->mIsDown = false;
		aLastDownWidget->TouchEnded(touch);
	}
	else
		mDownButtons &= ~aMask;

	MousePosition(NO_TOUCH_MOUSE_POS.mX, NO_TOUCH_MOUSE_POS.mY);
}

void WidgetManager::TouchesCanceled() //933-946
{

}

Widget* WidgetManager::GetGamepadSelection() //949-951
{
	return mGamepadSelectionWidget;
}

void WidgetManager::SetGamepadSelection(Widget* theSelectedWidget, int theDirection) //954-970
{
	if (theSelectedWidget == mGamepadSelectionWidget)
		return;

	if (mGamepadSelectionWidget)
		mGamepadSelectionWidget->LostGamepadSelection();

	if (theSelectedWidget && theSelectedWidget->mWidgetManager)
	{
		if (mHasFocus)
		{
			if (mGamepadSelectionWidget)
				mGamepadSelectionWidget->GotGamepadSelection((WidgetLinkDir)theDirection);
		}
		else
			mGamepadSelectionWidget = NULL;
	}
	//Widget mGamepadSelectionWidget2 = mGamepadSelectionWidget; //XNA
}
#ifdef _SEXYDECOMP_USE_LATEST_CODE //These are ints on Win, probably due to controller support wasn't quite ready yet
void WidgetManager::GamepadButtonDown(GamepadButton theButton, int thePlayer, uint theFlags) //Null on XNA
#else
void WidgetManager::GamepadButtonDown(int theButton, int thePlayer, ulong theFlags) //Null on XNA | 973-979
#endif
{
	if (mApp->mGamepadLocked == -1 || mApp->mGamepadLocked == thePlayer)
	{
		mLastInputUpdateCnt = mUpdateCnt;
		if (mGamepadSelectionWidget)
			mGamepadSelectionWidget->GamepadButtonDown(theButton, thePlayer, theFlags);
	}
}
#ifdef _SEXYDECOMP_USE_LATEST_CODE
void WidgetManager::GamepadButtonUp(GamepadButton theButton, int thePlayer, uint theFlags)
#else
void WidgetManager::GamepadButtonUp(int theButton, int thePlayer, ulong theFlags) //Null on XNA | 982-988
#endif
{
	if (mApp->mGamepadLocked == -1 || mApp->mGamepadLocked == thePlayer)
	{
		mLastInputUpdateCnt = mUpdateCnt;
		if (mGamepadSelectionWidget)
			mGamepadSelectionWidget->GamepadButtonUp(theButton, thePlayer, theFlags);
	}
}
#ifdef _SEXYDECOMP_USE_LATEST_CODE
void WidgetManager::GamepadAxisMove(GamepadAxis theAxis, int thePlayer, float theAxisValue) //Null on XNA
#else
void WidgetManager::GamepadAxisMove(int theAxis, int thePlayer, int theAxisValue) //991-997
#endif
{
	if (mApp->mGamepadLocked == -1 || mApp->mGamepadLocked == thePlayer)
	{
		mLastInputUpdateCnt = mUpdateCnt;
		if (mGamepadSelectionWidget)
			mGamepadSelectionWidget->GamepadAxisMove(theAxis, theAxisValue, thePlayer);
	}
}

IGamepad* WidgetManager::GetGamepadForPlayer(int thePlayer) //Null on XNA | 1000-1003
{
	return mApp->mGamepadDriver->GetGamepad(thePlayer);
}
