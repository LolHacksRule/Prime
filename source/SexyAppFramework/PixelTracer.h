#ifndef __PIXELTRACER_H__
#define __PIXELTRACER_H__

#include "Image.h"
#include "RenderStateManager.h"
#include <CommCtrl.h>

static bool gPTWindowDone;
static HWND gButtonWindow;
static HWND gTreeWindow;
static int gPixelTracerPrimCount;
static int gTraceX;
static int gTraceY;
static std::vector<TraceItem> gTraceItems;

namespace Sexy
{
	void PixelTracerStart(int theX, int theY); //C++ only on Win or debug?
	void PixelTracerStop();
	void PixelTracerCheckPrimitives(int thePrimType, ulong thePrimCount, const SexyVertex2D* theVertices, int theVertexSize);
	static bool gTracingPixels;
	static Image* gPixelTracerLastImage;
	const Rect* gPixelTracerSrcRect;
	RenderStateManager* gPixelTracerStateManager;
};

class TraceItem //C++ only on Win or debug?
{
public:
	std::string mText;
	StringVector mStackLines;
	StringVector mStackFileNames;
	std::vector<int> mStackFileLines;
	StringVector mInfoLines;
	StringVector mStates;
	int mFirstCurrentState;

	void BuildStates(RenderStateManager::Context* inContext);
	//TraceItem();
	//~TraceItem();
};

LRESULT CALLBACK PTWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif //__PIXELTRACER_H__