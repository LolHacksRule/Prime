#include "PixelTracer.h"
#include "Reflection.h"
#include "ResourceManager.h"
//#include <CommCtrl.h>
#include "SEHCatcher.h"

using namespace Sexy;
using namespace Reflection;

/*int Sexy::gPixelTracerPrimCount;
int Sexy::gTraceX;
int Sexy::gTraceY;
HWND gButtonWindow;
HWND gTreeWindow;
bool gPTWindowDone;
bool gTracingPixels; //Does this go here?
Image* gPixelTracerLastImage;*/

void TraceItem::BuildStates(RenderStateManager::Context* inContext) //37-88
{
	CRefSymbolDb* aSymDb = gSexyAppBase->GetReflection();
	mFirstCurrentState = inContext->mJournalFloor;
	int aStateCount = inContext->mJournal.size();
	for (int iState = 0; iState < aStateCount; iState++)
	{
		RenderStateManager::Context::JournalEntry* aStateEntry = &inContext->mJournal[iState];
		std::string aStr = aStateEntry->mState->mName;
		switch (aStateEntry->mNewValue.mType)
		{
		case RenderStateManager::StateValue::SV_Dword:
			REnum* aRefEnum = NULL;
			if (aSymDb != NULL && aStateEntry->mState->mValueEnumName)
				aRefEnum = aSymDb->GetEnums()->GetNamed(aStateEntry->mState->mValueEnumName);
			if (aRefEnum == NULL && aSymDb != NULL && aStateEntry->mState->mValueEnumName)
				aRefEnum = aSymDb->GetEnums()->GetNamed("_" + *aStateEntry->mState->mValueEnumName);
			if (aRefEnum != NULL)
				aStr += StrFormat(" = %s", aRefEnum->InstanceToString(&aStateEntry->mNewValue.mDword).c_str());
			else
				aStr += StrFormat(" = %d", aStateEntry->mNewValue.mDword);
			break;
		case RenderStateManager::StateValue::SV_Float: aStr += StrFormat(" = %f", aStateEntry->mNewValue.mFloat); break;
		case RenderStateManager::StateValue::SV_Ptr: aStr += StrFormat(" = 0x%p", aStateEntry->mNewValue.mPtr); break;
		case RenderStateManager::StateValue::SV_Vector: aStr += StrFormat(" = (%f, %f, %f, %f)", aStateEntry->mNewValue.mFloat, aStateEntry->mNewValue.mY, aStateEntry->mNewValue.mZ, aStateEntry->mNewValue.mW); break;
		default: aStr += " = ?"; break;
		}
		mStates.push_back(aStr);
	}
}

std::vector<TraceItem> gTraceItems; //103

static HTREEITEM TreeViewAddItem(HWND inTreeView, HTREEITEM inParent, const std::string& inString, bool inInsertFirst, bool inIsBold, ulong inTraceInfo) //108-126
{
	if (!inParent)
		inParent = (HTREEITEM)-65536;
	TVINSERTSTRUCT tvis;
	tvis.hParent = inParent;
	tvis.hInsertAfter = (_TREEITEM*)(-inInsertFirst - 65534);
	tvis.itemex.mask = 5;
	tvis.itemex.pszText = (char*)inString.c_str();
	tvis.itemex.cchTextMax = inString.length();
	tvis.itemex.lParam = inTraceInfo;
	if (inIsBold)
	{
		tvis.itemex.mask |= 8u;
		tvis.item.state = 0x1000000010i64; //Cast?
	}
	return (HTREEITEM)SendMessage(inTreeView, TVM_INSERTITEM, 0, (LPARAM)&tvis);
}

void Sexy::PixelTracerStart(int theX, int theY) //129-137
{
	gTracingPixels = true;
	gTraceX = theX;
	gTraceY = theY;
	SEHCatcher::LoadImageHelp();
	gTraceItems.clear();
}

LRESULT CALLBACK PTWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) //140-218
{
	if (uMsg > WM_GETDLGCODE) //?
	{
		if (uMsg == WM_KEYDOWN)
		{
			if (wParam == VK_ESCAPE)
				gPTWindowDone = true;
		}
		else if (uMsg == WM_COMMAND)
		{
			HWND hwndCtl = (HWND)lParam;
			if (hwndCtl == gButtonWindow)
				gPTWindowDone = 1;
		}
	}
	else
	{ }
	switch (uMsg)
	{
	case WM_GETDLGCODE:
		return 4;
	case WM_CLOSE:
		gPTWindowDone = true;
		return 0;
	case WM_NOTIFY:
		LPNMHDR nmhdr = (LPNMHDR)lParam;
		if ((HWND)lParam == gTreeWindow)
		{
			if (nmhdr->code = -412)
			{
				LPNMTVKEYDOWN nmtv = (LPNMTVKEYDOWN)nmhdr;
				if (LOWORD(nmhdr[1].hwndFrom) == 27)
					gPTWindowDone = true;
			}
			else if (nmhdr->code == NM_DBLCLK)
			{
				HTREEITEM hti = (HTREEITEM)SendMessage(gTreeWindow, TVM_GETNEXTITEM, VK_TAB, 0);
				if (hti)
				{
					TVITEM tvi;
					tvi.mask = 4;
					tvi.hItem = hti;
					SendMessage(gTreeWindow, TVM_GETITEM, VK_TAB, (LPARAM)&tvi);
					if (tvi.lParam)
					{
						ulong iTrace = HIWORD(tvi.lParam) - 1;
						ulong iStack = LOWORD(tvi.lParam) - 1;
						if (iTrace < gTraceItems.size())
						{
							TraceItem* trace = gTraceItems[iTrace];
							if (iStack < trace->mStackFileNames.size())
							{
								TraceItem* trace = gTraceItems[iTrace];
								std::string fileName = trace->mStackFileNames[iStack];
								int lineNum = trace->mStackFileLines[iStack];
								if (lineNum > 0)
								{
									std::string cmdArgs = StrFormat("%s %d", fileName.c_str(), lineNum);
									char curDir[0x104];
									ZeroMemory(curDir, sizeof curDir);
									ShellExecuteA(NULL, "open", "VSOpenFile.exe", cmdArgs.c_str(), curDir, 1);
								}
							}
						}
					}
				}
			}
			break;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void Sexy::PixelTracerStop() //CORRECT? | 221-366
{
	if (!gTracingPixels)
		return;

	InitCommonControls();
	gTracingPixels = false;
	std::string aHeaderText = StrFormat("Location: %d, %d:", gTraceX, gTraceY);
	if (gTraceItems.empty())
		aHeaderText += " No traces found!";
	SEHCatcher::UnloadImageHelp();
	gSexyAppBase->BeginPopup();
	ReleaseCapture();
	SetCursor(LoadCursorA(0, IDC_ARROW));

	WNDCLASSA wc;
	wc.style = 0;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = ::GetSysColorBrush(COLOR_BTNFACE);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = ::LoadIcon(NULL, "IDI_MAIN_ICON");
	wc.hInstance = gHInstance;
	wc.lpfnWndProc = PTWindowProc;
	wc.lpszClassName = "PTWindowProc";
	wc.lpszMenuName = NULL;
	RegisterClassA(&wc);

	RECT aRect;
	RECT aParentRect;
	aRect.left = 0;
	aRect.top = 0;
	aRect.right = 440;
	aRect.bottom = 333;
	DWORD aWindowStyle = WS_CLIPCHILDREN | WS_POPUP | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	int worked = AdjustWindowRect(&aRect, aWindowStyle, FALSE);
	GetWindowRect(gSexyAppBase->mHWnd, &aParentRect);

	HWND aHWnd = CreateWindowEx( //Assuming
		0,
		_S("PTWindowProc"),
		_S("PIXEL TRACER"),
		aWindowStyle,
		aParentRect.left + 64,
		aParentRect.right + 64,
		aRect.right - aRect.left,
		aRect.bottom - aRect.top,
		gSexyAppBase->mHWnd,
		NULL,
		gHInstance,
		0);
	HFONT aButtonFont = CreateFontA(MulDiv(9, 96, 72), 0, 0, 0, FW_BOLD, 0, 0,
		false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "Arial");
	gTreeWindow = CreateWindowExA(
		0,
		"SysTreeView32",
		NULL,
		WS_VISIBLE | WS_CAPTION | WS_BORDER | WS_DLGFRAME | WS_SYSMENU | WS_TABSTOP | WS_MAXIMIZEBOX,
		6,
		6,
		428,
		293,
		aHWnd,
		NULL,
		gHInstance,
		0);
	int aFontHeight = -MulDiv(8, 96, 72);
	HFONT aTextFont = CreateFontA(aFontHeight, 0, 0, 0, FW_NORMAL, 0, 0,
		false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
	SendMessage(gTreeWindow, WM_SETFONT, (WPARAM)aTextFont, 0);
	HTREEITEM aRootItem = TreeViewAddItem(gTreeWindow, 0, aHeaderText, false, false, 0);
	for (int iTrace = 0; iTrace < gTraceItems.size(); iTrace++)
	{
		TraceItem* aTrace = gTraceItems[iTrace];
		HTREEITEM aTraceItem = TreeViewAddItem(gTreeWindow, aRootItem, aTrace->mText, true, false, 0);
		for (int iStack = 0; iStack < aTrace->mStackLines.size(); iStack++)
		{
			ulong traceInfo = ((iTrace + 1) << 16) + iStack + 1;
			TreeViewAddItem(gTreeWindow, aTraceItem, aTrace->mStackLines[iStack], false, false, traceInfo);
		}
		if (!aTrace->mStates.empty())
		{
			HTREEITEM aStateHeaderItem = TreeViewAddItem(gTreeWindow, aTraceItem, "(Render Context):", false, false, 0);
			for (int iState = 0; iState < aTrace->mStates.size(); iState++)
			{
				bool bold = iState >= aTrace->mFirstCurrentState;
				HTREEITEM aStateItem = TreeViewAddItem(gTreeWindow, aStateHeaderItem, aTrace->mStates[iState], true, bold, 0);
			}
		}

		if (!aTrace->mInfoLines.empty())
		{
			HTREEITEM aInfoHeaderItem = TreeViewAddItem(gTreeWindow, aTraceItem, "(Image Info):", false, false, 0);
			for (int iInfo = 0; iInfo < aTrace->mInfoLines.size(); iInfo++)
				HTREEITEM aStateItem = TreeViewAddItem(gTreeWindow, aInfoHeaderItem, aTrace->mInfoLines[iInfo], false, false, 0);
		}
	}
	SendMessage(gTreeWindow, 0x1102, 2, (LPARAM)aRootItem); //?
	gButtonWindow = CreateWindowExA(
		0,
		"BUTTON",
		"DONE",
		0x50000001, //?
		6,
		303,
		428,
		24,
		aHWnd,
		NULL,
		gHInstance,
		0);
	SendMessage(gButtonWindow, WM_SETFONT, (WPARAM)aButtonFont, 0);
	ShowWindow(aHWnd, SW_SHOWNORMAL);
	SetFocus(aHWnd);
	gPTWindowDone = false;
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0 && !gPTWindowDone)
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	DestroyWindow(aHWnd);

	DeleteObject(aButtonFont);
	DeleteObject(aTextFont);
	gSexyAppBase->EndPopup();
}

static bool TriContainsPoint(const SexyVertex2D* theVertices) //369-385
{
	SexyVector2 v0(theVertices[2].x - theVertices->x, theVertices[2].y - theVertices->y);
	SexyVector2 v1(theVertices[1].x - theVertices->x, theVertices[1].y - theVertices->y);
	SexyVector2 v2((double)gTraceX - theVertices->x, (double)gTraceY - theVertices->y);
	float dot00 = v0.Dot(v0);
	float dot01 = v0.Dot(v1);
	float dot02 = v0.Dot(v2);
	float dot11 = v1.Dot(v1);
	float dot12 = v1.Dot(v2);
	float invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
	float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
	return u > 0.0 && v > 0.0 && u + v < 1.0;
}

static bool TriContainsPoint(const SexyVertex2D* v1, const SexyVertex2D* v2, const SexyVertex2D* v3) //388-404
{
	SexyVector2 vv0(v3->x - v1->x, v3->y - v1->y);
	SexyVector2 vv1(v2->x - v1->x, v2->y - v1->y);
	SexyVector2 vv2((double)gTraceX - v1->x, (double)gTraceY - v1->y);
	float dot00 = vv0.Dot(vv0);
	float dot01 = vv0.Dot(vv1);
	float dot02 = vv0.Dot(vv2);
	float dot11 = vv1.Dot(vv1);
	float dot12 = vv1.Dot(vv2);
	float invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
	float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
	return u > 0.0 && v > 0.0 && u + v < 1.0;
}

static void PixelTracerAddToTrace() //CORRECT? | 407-705
{
	int aBackTraceCount = 0;
	if (SEHCatcher::mRtlCaptureStackBackTrace)
		aBackTraceCount = SEHCatcher::mRtlCaptureStackBackTrace(0, 63, 0, 0);
	StringVector aDebugDump;
	StringVector aFrameworkDebugDump;
	StringVector aStackFileNames;
	StringVector aFrameworkStackFileNames;
	IntVector aStackFileLines;
	IntVector aFrameworkStackFileLines;
	char aFrameworkBuffer[2048];
	ZeroMemory(aFrameworkBuffer, sizeof aFrameworkBuffer);
	char aUDName[MAX_PATH - 4];
	ZeroMemory(aUDName, sizeof aUDName);
	int aStackLine;
	int aFrameworkStackLine;
	bool foundProgStack;
	if (!aBackTraceCount)
	{
		if (!SEHCatcher::mRtlCaptureContext)
			return;
		CONTEXT aContext;
		ZeroMemory(&aContext, sizeof(aContext));
		aContext.ContextFlags = CONTEXT_FULL;
		SEHCatcher::mRtlCaptureContext(&aContext);
		STACKFRAME sf;
		ZeroMemory(&sf, sizeof(sf));
		sf.AddrPC.Offset = aContext.Eip;
		sf.AddrPC.Mode = AddrModeFlat;
		sf.AddrStack.Offset = aContext.Esp;
		sf.AddrStack.Mode = AddrModeFlat;
		sf.AddrFrame.Offset = aContext.Ebp;
		sf.AddrFrame.Mode = AddrModeFlat;
	}
	int aLevelCount = 0;
	bool addFrameworkStack = false;
	while (!aBackTraceCount || aLevelCount < aBackTraceCount)
	{
		char aBuffer[2048];
		aBuffer[0] = NULL;
		char aStackFileBuffer[MAX_PATH];
		aStackFileBuffer[0] = NULL;
		aStackLine = 0;
		ulong aStackFrameAddress;
		if (aBackTraceCount)
		{
			void* aBackTrace[64];
			ulong aAddress = (ulong)aBackTrace[aLevelCount];
			if (aAddress)
			{
				aStackFrameAddress = 0;
				if (aLevelCount > 0)
					aAddress -= 5;
				PIMAGEHLP_SYMBOL pSymbol;
				uchar symbolBuffer[536];
				*(LPDWORD)symbolBuffer[16] = 532;
				ulong symDisplacement = 0;
				if (SEHCatcher::mSymGetSymFromAddr(GetCurrentProcess, aAddress, &symDisplacement, (PIMAGEHLP_SYMBOL)symbolBuffer))
				{
					SEHCatcher::mUnDecorateSymbolName(pSymbol->Name, aUDName, 256, 18424); //?
					if (SEHCatcher::mSymGetLineFromAddr)
					{
						IMAGEHLP_LINE aLine;
						ulong aLineDisplacement;
						aLine.SizeOfStruct = 20;
						if (!SEHCatcher::mSymGetLineFromAddr(GetCurrentProcess, aAddress, &aLineDisplacement, &aLine))
						{
							aLine.FileName = "(Unknown File Name)";
							aLine.LineNumber = 0;
							aLineDisplacement = 0;
						}
						std::string aRelPath = aLine.FileName;
						if (!strnicmp(gSexyAppBase->mChangeDirTo.c_str(), aRelPath.c_str(), gSexyAppBase->mChangeDirTo.length()))
							aRelPath.erase(aRelPath.begin()), aRelPath.begin()[1], aRelPath.begin() + gSexyAppBase->mChangeDirTo.length();
						if (aRelPath.length() && (aRelPath[0] == '\\' || aRelPath[0] == '/'))
							aRelPath.erase((aRelPath.begin(), aRelPath.begin()[1]));
						bool isFramework = Upper(aRelPath).find("SEXYAPPFRAMEWORK", 0) != -1;
						if (isFramework)
						{
							if (foundProgStack)
								break;
							sprintf(aFrameworkBuffer, "%hs (%s Line %d)", aUDName, aRelPath.c_str(), aLine.LineNumber);
							char aFrameworkStackFileBuffer[MAX_PATH];
							strncpy(aFrameworkStackFileBuffer, aLine.FileName, sizeof aFrameworkStackFileBuffer);
							aFrameworkStackLine = aLine.LineNumber;
						}
						else
							foundProgStack = true;
						if (foundProgStack)
						{
							sprintf(aFrameworkBuffer, "%hs (%s Line %d)", aUDName, aRelPath.c_str(), aLine.LineNumber);
							char aFrameworkStackFileBuffer[MAX_PATH];
							strncpy(aStackFileBuffer, aLine.FileName, sizeof aStackFileBuffer);
							aStackLine = aLine.LineNumber;
						}
					}
					else
						sprintf(aBuffer, "%08X %08X %hs+%X", aStackFrameAddress, aAddress, aUDName, symDisplacement);
				}
				else
				{
					ulong section = 0;
					ulong offset = 0;
					char szModule[MAX_PATH];
					SEHCatcher::GetLogicalAddress((void*)aAddress, szModule, MAX_PATH, section, offset);
					sprintf(aBuffer, "%08X %08X %04X:%08X %s", aStackFrameAddress, aAddress, section, offset, SEHCatcher::GetFilename(szModule).c_str());
					addFrameworkStack = true;
				}
				if (aBuffer[0])
				{
					aDebugDump.push_back(aBuffer);
					aFrameworkDebugDump.push_back(aBuffer);
					if (aStackLine >= 0)
					{
						aStackFileNames.push_back(aStackFileBuffer);
						aFrameworkStackFileNames.push_back(aStackFileBuffer);
						aStackFileLines.push_back(aStackLine);
						aFrameworkStackFileLines.push_back(aStackLine);
					}
					else
					{
						aStackFileNames.push_back("");
						aFrameworkStackFileNames.push_back("");
						aStackFileLines.push_back(0);
						aFrameworkStackFileLines.push_back(0);
					}
				}

				else if (aFrameworkBuffer[0])
				{
					aFrameworkDebugDump.push_back(aFrameworkBuffer);
					if (aFrameworkStackLine >= 0)
					{
						aFrameworkStackFileNames.push_back(aStackFileBuffer);
						aFrameworkStackFileLines.push_back(aFrameworkStackLine);
					}
					else
					{
						aStackFileNames.push_back("");
						aFrameworkStackFileNames.push_back("");
						aFrameworkStackFileLines.push_back(0);
					}
					aFrameworkBuffer[0] = NULL;
				}
				aLevelCount++;
			}
		}
	}
	std::string aPrependText;
	StringVector aInfoLines;
	if (gPixelTracerLastImage)
	{
		std::string aPath = gPixelTracerLastImage->mFilePath;
		if (!aPath.empty())
		{
			std::string anId = gSexyAppBase->mResourceManager->GetIdByPath(aPath);
			if (!anId.empty())
				aPrependText = anId + " (" + aPath + ") ";
			else
				aPrependText = aPath;
		}
		else
			aPrependText = "<Generated Image>";
		CRefSymbolDb* aSymDb = gSexyAppBase->GetReflection();
		RClass* aImageClass = NULL;
		if (aSymDb)
		{
			RNamedType* imageType = aSymDb->GetInstanceType(gPixelTracerLastImage);
			assert(imageType && (imageType->GetTypeCategory() == RType::TC_Named_Class)); //645
			aImageClass = (RClass*)imageType;
		}
		if (aImageClass)
		{
			for (ulong iField = 0; iField < aImageClass->GetFields(true)->GetCount(); iField++)
			{
				RField* field = aImageClass->GetFields(true)->GetIndexed(iField);
				if (!field->GetIsDisputed())
					aInfoLines.push_back(StrFormat("%s = %s", field->GetName(), (field->GetType()->InstanceToString(gPixelTracerLastImage + field->GetFieldOffset()).c_str())));
				else
					aInfoLines.push_back(StrFormat("%s = (disputed; cannot display)", field->GetName()));
			}
		}
		else
		{
			aInfoLines.push_back(StrFormat("Texture Dimensions = %d x %d", gPixelTracerLastImage->mWidth, gPixelTracerLastImage->mHeight));
			aInfoLines.push_back(StrFormat("Image Flags = 0x%08x", gPixelTracerLastImage->GetImageFlags()));
		}
		if (gPixelTracerLastImage->mNumCols > 1 && gPixelTracerLastImage->mNumRows > 1)
		{
			bool hasCell = false;
			if (gPixelTracerSrcRect)
			{
				int aCelWidth = gPixelTracerLastImage->mWidth / Sexy::gPixelTracerLastImage->mNumCols;
				int aCelHeight = gPixelTracerLastImage->mHeight / Sexy::gPixelTracerLastImage->mNumRows;
				if (gPixelTracerSrcRect->mWidth == aCelWidth && gPixelTracerSrcRect->mHeight == aCelHeight)
				{
					int aCelX = gPixelTracerSrcRect->mX / aCelWidth;
					int aCelY = gPixelTracerSrcRect->mY / aCelHeight;
					aInfoLines.push_back(StrFormat("Cell Col = %d (%d of %d)", aCelX, aCelX + 1, gPixelTracerLastImage->mNumCols));
					aInfoLines.push_back(StrFormat("Cell Row = %d (%d of %d)", aCelY, aCelY + 1, gPixelTracerLastImage->mNumRows));
					hasCell = true;
				}
			}
			if (!hasCell)
			{
				aInfoLines.push_back(StrFormat("Cell Col = ? of %d", gPixelTracerLastImage->mNumCols));
				aInfoLines.push_back(StrFormat("Cell Row = ? of %d", gPixelTracerLastImage->mNumRows));
			}
		}
		aInfoLines.push_back(StrFormat("Triangle Count = %d", gPixelTracerPrimCount));
	}
	else
		aPrependText = "<No Texture>";
	TraceItem* aTraceItem;
	gTraceItems.push_back(TraceItem());
	aTraceItem = gTraceItems.back();
	aTraceItem->mText = aPrependText;
	aTraceItem->mStackLines = addFrameworkStack ? aFrameworkDebugDump : aDebugDump;
	aTraceItem->mStackFileNames = addFrameworkStack ? aFrameworkStackFileNames : aStackFileNames;
	aTraceItem->mStackFileLines = addFrameworkStack ? aFrameworkStackFileLines : aStackFileLines;
	aTraceItem->mInfoLines = aInfoLines;
	if (gPixelTracerStateManager)
		aTraceItem->BuildStates(gPixelTracerStateManager->GetContext());
}

void Sexy::PixelTracerCheckPrimitives(int thePrimType, ulong thePrimCount, const SexyVertex2D* theVertices, int theVertexSize) //708-734
{
	bool hasPoint = false;
	switch (thePrimType)
	{
	case 0xFFFFFFFF:
		hasPoint = 1;
		break;
	case Graphics3D::PT_TriangleList:
		for (int aTriNum = 0; aTriNum < thePrimCount; aTriNum++)
			hasPoint |= TriContainsPoint(&theVertices[3 * aTriNum]);
		break;
	case Graphics3D::PT_TriangleStrip:
		for (int aTriNum = 0; aTriNum < thePrimCount; aTriNum++)
			hasPoint |= TriContainsPoint(&theVertices[aTriNum]);
		break;
	case Graphics3D::PT_TriangleFan:
		for (int aTriNum = 0; aTriNum < thePrimCount; aTriNum++)
			hasPoint |= TriContainsPoint(theVertices, &theVertices[aTriNum + 1], &theVertices[aTriNum + 2]);
		break;
	}
	if (hasPoint)
	{
		gPixelTracerPrimCount = thePrimCount;
		PixelTracerAddToTrace();
	}
}