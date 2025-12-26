#include "Graphics.h"
#include "Image.h"
#include "Font.h"
#include "MemoryImage.h"
#include "Rect.h"
#include "Debug.h"
#include "SexyMatrix.h"
#include "RenderDevice.h"
#include "RenderEffect.h"
#include <math.h>

using namespace Sexy;

RenderDevice3D* NullRenderDevice::Get3D() { return NULL; } //24
HRenderContext NullRenderDevice::CreateContext(Image* theDestImage, const HRenderContext& theSourceContext) { return HRenderContext(1); } //25
void NullRenderDevice::DeleteContext(const HRenderContext& theContext) {} //26
void NullRenderDevice::SetCurrentContext(const HRenderContext& theContext) {} //27
HRenderContext NullRenderDevice::GetCurrentContext() const { return HRenderContext(1); } //28

void NullRenderDevice::PushState() {} //30
void NullRenderDevice::PopState() {} //31

bool NullRenderDevice::CanFillPoly() { return false; } //33

void NullRenderDevice::ClearRect(const Rect& theRect) {} //35
void NullRenderDevice::FillRect(const Rect& theRect, const Color& theColor, int theDrawMode) {} //36
void NullRenderDevice::FillScanLinesWithCoverage(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode, const uchar* theCoverage, int theCoverX, int theCoverY, int theCoverWidth, int theCoverHeight) {} //37
void NullRenderDevice::FillPoly(const Point* theVertices, int theNumVertices, const Rect* theClipRect, const Color& theColor, int theDrawMode, int tx, int ty) {} //38
void NullRenderDevice::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode, bool antiAlias) {} //39
void NullRenderDevice::Blt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode) {} //40
void NullRenderDevice::BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode) {} //41
void NullRenderDevice::BltRotated(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY) {} //42
void NullRenderDevice::BltMatrix(Image* theImage, float theX, float theY, const SexyMatrix3& theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect& theSrcRect, bool blend) {} //43
void NullRenderDevice::BltTriangles(Image* theImage, const SexyVertex2D* theVertices[][3], int theNumTriangles, const Color& theColor, int theDrawMode, float tx, float ty, bool blend, const Rect* theClipRect) {} //44
void NullRenderDevice::BltMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode) {} //45
void NullRenderDevice::BltStretched(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch, bool mirror) {} //46

Image GraphicsState::mStaticImage; //52
const Point* Graphics::mPFPoints;

//////////////////////////////////////////////////////////////////////////

void GraphicsState::CopyStateFrom(const GraphicsState* theState) //58-77
{
	mDestImage = theState->mDestImage;
	mTransX = theState->mTransX;
	mTransY = theState->mTransY;
	mClipRect = theState->mClipRect;
	mFont = theState->mFont;
	mPushedColorVector = theState->mPushedColorVector;
	mColor = theState->mColor;
	mFinalColor = theState->mFinalColor;
	mDrawMode = theState->mDrawMode;
	mColorizeImages = theState->mColorizeImages;
	mFastStretch = theState->mFastStretch;
	mWriteColoredString = theState->mWriteColoredString;
	mLinearBlend = theState->mLinearBlend;
	mScaleX = theState->mScaleX;
	mScaleY = theState->mScaleY;
	mScaleOrigX = theState->mScaleOrigX;
	mScaleOrigY = theState->mScaleOrigY;
	mIs3D = theState->mIs3D;
}

void Graphics::InitRenderInfo(Graphics const* theSourceGraphics) //TODO | 82-149
{
	static NullRenderDevice sNullRenderDevice; //Possibly calls sNullRenderDevice on Windows
	mGraphics3D = NULL;
	mIs3D = false;
	RenderDevice3D* aHardwareDevice3D = gSexyAppBase->mGraphicsDriver->GetRenderDevice3D();
	if (aHardwareDevice3D)
	{
		//HRenderContext context = new HRenderContext(0);
		HRenderContext context = ((theSourceGraphics == NULL) ? aHardwareDevice3D->CreateContext(mDestImage) : aHardwareDevice3D->CreateContext(mDestImage, theSourceGraphics->mRenderContext));
		//context.mHandleDword = aHardwareDevice3D->CreateContext(mDestImage, theSourceGraphics->mRenderContext->mHandleDword); //PC
		if (context.IsValid()) //PC
		{
			mRenderDevice = aHardwareDevice3D;
			mRenderContext = context;
			mGraphics3D = new Graphics3D(aHardwareDevice3D, &mRenderContext);
			mIs3D = true;
		}
	}
	else
	{
		if (mRenderContext.IsValid())
			return;

		RenderDevice* aHardwareDevice = gSexyAppBase->mGraphicsDriver->GetRenderDevice();
		if (aHardwareDevice != NULL)
		{
			HRenderContext context = ((theSourceGraphics == NULL) ? aHardwareDevice->CreateContext(mDestImage) : aHardwareDevice->CreateContext(mDestImage, theSourceGraphics->mRenderContext));
			if (context.IsValid())
			{
				mRenderDevice = aHardwareDevice;
				mRenderContext = context;
				mGraphics3D = NULL;
				mIs3D = false;
			}
		}
	}
}

Graphics::Graphics(Image* theDestImage) //152-175
{
	mTransX = 0;
	mTransY = 0;
	mScaleX = 1;
	mScaleY = 1;
	mScaleOrigX = 0;
	mScaleOrigY = 0;
	mFont = NULL; //Not here in XNA
	mDestImage = theDestImage;
	mDrawMode = DRAWMODE_NORMAL;
	mColorizeImages = false;
	mFastStretch = false;
	mWriteColoredString = true;
	mLinearBlend = false;

	if (mDestImage == NULL)
	{
		mDestImage = &mStaticImage;
	}

	mClipRect = Rect(0, 0, mDestImage->GetWidth(), mDestImage->GetHeight());

	InitRenderInfo(NULL);
}

Graphics::Graphics(const Graphics& theGraphics) //178-181
{
	CopyStateFrom(&theGraphics);
	InitRenderInfo(&theGraphics);
}

Graphics::~Graphics() //184-189
{
	//delete mGraphics3D; //?
	mRenderDevice->DeleteContext(mRenderContext);
}

Graphics3D* Graphics::Get3D() //192-194
{
	return mGraphics3D;
}

void Graphics::SetAsCurrentContext() //197-199
{
	mRenderDevice->SetCurrentContext(mRenderContext);
}

void Graphics::CalcFinalColor() //202-214
{
	if (mPushedColorVector.size() > 0)
	{
		Color& aLastMult = mPushedColorVector.back();

		//?
		mFinalColor.mRed = mColor.mRed * aLastMult.mRed / 255;
		mFinalColor.mGreen = mColor.mGreen * aLastMult.mGreen / 255;
		mFinalColor.mBlue = mColor.mBlue * aLastMult.mBlue / 255;
		mFinalColor.mAlpha = mColor.mAlpha * aLastMult.mAlpha / 255;
		//mFinalColor = new Color((int)min(255, (float)(aLastMult.mRed * mColor.mRed) / 255), (int)min(255, (float)(aLastMult.mGreen * mColor.mGreen) / 255), (int)min(255, (float)(aLastMult.mBlue * mColor.mBlue) / 255), (int)min(255, (float)(aLastMult.mAlpha * mColor.mAlpha) / 255)); //?
	}
	else
	{
		mFinalColor.mRed = mColor.mRed;
		mFinalColor.mGreen = mColor.mGreen;
		mFinalColor.mBlue = mColor.mBlue;
		mFinalColor.mAlpha = mColor.mAlpha;
		//mFinalColor = mColor;
	}
}

const Color& Graphics::GetImageColor() //217-230
{
	if (mPushedColorVector.size())
	{
		if (mColorizeImages)
			return mFinalColor;
		else
			return mPushedColorVector.back();
	}
	else if (mColorizeImages)
		return mColor;
	else
		return Color::White;
}

void Graphics::PushState() //233-239
{
	mStateStack.push_back(GraphicsState());
	mStateStack.back().CopyStateFrom(this);


	SetAsCurrentContext();
	mRenderDevice->PushState();
}

void Graphics::PopState() //242-252
{
	DBG_ASSERTE(mStateStack.size() > 0); //243 | 260 in BejLiveWin8
	if (mStateStack.size() > 0)
	{
		CopyStateFrom(&mStateStack.back());
		mStateStack.pop_back();
	}

	SetAsCurrentContext();
	mRenderDevice->PushState();
}

Graphics* Graphics::Create()
{	
	return new Graphics(*this);
}

Font* Graphics::GetFont() //262-264
{
	return mFont;
}

void Graphics::SetFont(Sexy::Font* theFont) //267-269
{
	mFont = theFont;
}

void Graphics::SetColor(const Color& theColor) //272-275
{
	mColor = theColor;
	CalcFinalColor();
}

const Color& Graphics::GetColor() //278-280
{
	return mColor;
}

void Graphics::PushColorMult() //283-286
{
	mPushedColorVector.push_back(mFinalColor);
	CalcFinalColor();
}

void Graphics::PopColorMult() //289-292
{
	mPushedColorVector.pop_back();
	CalcFinalColor();
}

void Graphics::SetDrawMode(int theDrawMode) //295-297
{
	mDrawMode = theDrawMode;
}

int Graphics::GetDrawMode() //300-302
{
	return mDrawMode;
}

void Graphics::SetColorizeImages(bool colorizeImages) //305-307
{
	mColorizeImages = colorizeImages;
}

bool Graphics::GetColorizeImages() //310-312
{
	return mColorizeImages;
}

void Graphics::SetFastStretch(bool fastStretch) //315-317
{
	mFastStretch = fastStretch;
}

bool Graphics::GetFastStretch() //320-322
{
	return mFastStretch;
}

void Graphics::SetLinearBlend(bool linear) //325-327
{
	mLinearBlend = linear;
}

bool Graphics::GetLinearBlend() //330-332
{
	return mLinearBlend;
}

void Graphics::ClearRect(int theX, int theY, int theWidth, int theHeight) //335-340
{
	SetAsCurrentContext();

	Rect aDestRect = Rect(theX + mTransX, theY + mTransY, theWidth, theHeight).Intersection(mClipRect);
	mRenderDevice->ClearRect(aDestRect);
}

void Graphics::ClearRect(const Rect& theRect) //343-345
{
	ClearRect(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

void Graphics::FillRect(int theX, int theY, int theWidth, int theHeight) //348-357
{
	const Color& aColor = GetFinalColor();
	if (aColor.mAlpha == 0)
		return;

	Rect aDestRect = Rect(theX + mTransX, theY + mTransY, theWidth, theHeight).Intersection(mClipRect);
	mRenderDevice->FillRect(aDestRect, aColor, mDrawMode);
}

void Graphics::FillRect(const Rect& theRect) //360-362
{
	FillRect(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

void Graphics::DrawRect(int theX, int theY, int theWidth, int theHeight) //365-396
{
	const Color& aColor = GetFinalColor();
	if (aColor.mAlpha == 0)
		return;

	Rect aDestRect = Rect(theX + mTransX, theY + mTransY, theWidth, theHeight);	
	Rect aFullDestRect = Rect(theX + mTransX, theY + mTransY, theWidth + 1, theHeight + 1);	
	Rect aFullClippedRect = aFullDestRect.Intersection(mClipRect);

	if (aFullDestRect == aFullClippedRect)
	{
		SetAsCurrentContext();
		mRenderDevice->DrawRect(aDestRect, aColor, mDrawMode);
	}
	else
	{
		FillRect(theX, theY, theWidth + 1, 1);
		FillRect(theX, theY + theHeight, theWidth + 1, 1);
		FillRect(theX, theY + 1, 1, theHeight - 1);
		FillRect(theX + theWidth, theY + 1, 1, theHeight - 1);

		/*if (aClippedRect.mX == aDestRect.mX)
			mDestImage->FillRect(Rect(aClippedRect.mX, aClippedRect.mY, 1, aClippedRect.mHeight), mColor, mDrawMode);
		if (aClippedRect.mY == aDestRect.mY)
			mDestImage->FillRect(Rect(aClippedRect.mX, aClippedRect.mY, aClippedRect.mWidth, 1), mColor, mDrawMode);
		if (aClippedRect.mX + aClippedRect.mWidth == aDestRect.mX + aDestRect.mWidth)
			mDestImage->FillRect(Rect(aClippedRect.mX + aClippedRect.mWidth, aClippedRect.mY, 1, aClippedRect.mHeight), mColor, mDrawMode);
		if (aClippedRect.mY + aClippedRect.mHeight == aDestRect.mY + aDestRect.mHeight)
			mDestImage->FillRect(Rect(aClippedRect.mX, aClippedRect.mY + aClippedRect.mHeight, aClippedRect.mWidth, 1), mColor, mDrawMode);*/
	}
}

void Graphics::DrawRect(const Rect& theRect) //399-401 (C++ only)
{
	DrawRect(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

int Graphics::PFCompareInd(const void* u, const void* v) //404-406 (C++ only)
{
	return mPFPoints[*((int*) u)].mY <= mPFPoints[*((int*) v)].mY ? -1 : 1;
}

int Graphics::PFCompareActive(const void* u, const void* v) //409-411 (C++ only)
{
	return ((Edge*) u)->mX <= ((Edge*) v)->mX ? -1 : 1;
}

void Graphics::PFDelete(int i) // remove edge i from active list | 414-422 (C++ only)
{
    int j;

    for (j=0; j<mPFNumActiveEdges && mPFActiveEdgeList[j].i!=i; j++);    
	if (j>=mPFNumActiveEdges) return;	/* edge not in active list; happens at aMinY*/
    
	mPFNumActiveEdges--;
    memcpy(&mPFActiveEdgeList[j], &mPFActiveEdgeList[j+1], (mPFNumActiveEdges-j)*sizeof mPFActiveEdgeList[0]);
}

void Graphics::PFInsert(int i, int y) // append edge i to end of active list | 425-447
{
    int j;
    double dx;
    const Point *p, *q;

    j = i<mPFNumVertices-1 ? i+1 : 0;
    if (mPFPoints[i].mY < mPFPoints[j].mY) 
	{
		p = &mPFPoints[i]; 
		q = &mPFPoints[j];
	}
    else		   
	{
		p = &mPFPoints[j]; 
		q = &mPFPoints[i];
	}
    /* initialize x position at intersection of edge with scanline y */
    mPFActiveEdgeList[mPFNumActiveEdges].mDX = dx = (q->mX - p->mX)/(double) (q->mY - p->mY);
    mPFActiveEdgeList[mPFNumActiveEdges].mX = dx*(y+0.5 - p->mY - mTransY) + p->mX + mTransX;
    mPFActiveEdgeList[mPFNumActiveEdges].i = i;
	mPFActiveEdgeList[mPFNumActiveEdges].b = p->mY - 1.0/dx * p->mX;
    mPFNumActiveEdges++;
}

void Graphics::PolyFill(const Point *theVertexList, int theNumVertices, bool convex) //Accurate? | 450-546
{
	SetAsCurrentContext();
	if (convex && mRenderDevice->CanFillPoly())
	{
		mRenderDevice->FillPoly(&theVertexList, theNumVertices, &mClipRect, GetFinalColor(), mDrawMode, mTransX, mTransY);
		return;
	}
	//The rest is only in C++
	RenderDevice::Span aSpans[MAX_TEMP_SPANS];
	int aSpanPos = 0;

    int k, y0, y1, y, i, j, xl, xr;
    int *ind;		/* list of vertex indices, sorted by mPFPoints[ind[j]].y */		

	int aMinX = mClipRect.mX;
	int aMaxX = mClipRect.mX + mClipRect.mWidth - 1;
	int aMinY = mClipRect.mY;
	int aMaxY = mClipRect.mY + mClipRect.mHeight - 1;

    mPFNumVertices = theNumVertices;
    mPFPoints = theVertexList;
    
	if (mPFNumVertices<=0) return;

    ind = new int[mPFNumVertices];
    mPFActiveEdgeList = new Edge[mPFNumVertices];

    /* create y-sorted array of indices ind[k] into vertex list */
    for (k=0; k<mPFNumVertices; k++)
		ind[k] = k;
    qsort(ind, mPFNumVertices, sizeof ind[0], PFCompareInd);	/* sort ind by mPFPoints[ind[k]].y */

    mPFNumActiveEdges = 0;				/* start with empty active list */
    k = 0;				/* ind[k] is next vertex to process */
    y0 = (int) max(aMinY, ceil(mPFPoints[ind[0]].mY-0.5 + mTransY));		/* ymin of polygon */
    y1 = (int) min(aMaxY, floor(mPFPoints[ind[mPFNumVertices-1]].mY-0.5 + mTransY));	/* ymax of polygon */

    for (y=y0; y<=y1; y++) 
	{
		// step through scanlines 
		// scanline y is at y+.5 in continuous coordinates 

		// check vertices between previous scanline and current one, if any 
		for (; (k < mPFNumVertices) && (mPFPoints[ind[k]].mY + mTransY <= y + 0.5); k++) 
		{
			// to simplify, if mPFPoints.mY=y+.5, pretend it's above 
			// invariant: y-.5 < mPFPoints[i].mY <= y+.5 
			i = ind[k];				
			// insert or delete edges before and after vertex i (i-1 to i,
			// and i to i+1) from active list if they cross scanline y			 

			j = i>0 ? i-1 : mPFNumVertices-1;	// vertex previous to i 
			if (mPFPoints[j].mY + mTransY <= y-0.5)	// old edge, remove from active list 
				PFDelete(j);
			else if (mPFPoints[j].mY + mTransY > y+0.5)	// new edge, add to active list 
				PFInsert(j, y);

			j = i<mPFNumVertices-1 ? i+1 : 0;	// vertex next after i 
			if (mPFPoints[j].mY + mTransY <= y-0.5)	// old edge, remove from active list 
				PFDelete(i);
			else if (mPFPoints[j].mY + mTransY > y+0.5)	// new edge, add to active list 
				PFInsert(i, y);
		}

		// sort active edge list by active[j].mX 
		qsort(mPFActiveEdgeList, mPFNumActiveEdges, sizeof mPFActiveEdgeList[0], PFCompareActive);

		// draw horizontal segments for scanline y 
		for (j = 0; j < mPFNumActiveEdges; j += 2) 
		{	// draw horizontal segments 
			// span 'tween j & j+1 is inside, span tween j+1 & j+2 is outside 
			xl = (int) ceil(mPFActiveEdgeList[j].mX-0.5);		// left end of span 
			if (xl<aMinX) 
				xl = aMinX;
			xr = (int) floor(mPFActiveEdgeList[j+1].mX-0.5);	// right end of span 
			if (xr>aMaxX) 
				xr = aMaxX;
			
			if ((xl <= xr) && (aSpanPos < MAX_TEMP_SPANS))
			{
				RenderDevice::Span* aSpan = &aSpans[aSpanPos++];
				aSpan->mY = y;
				aSpan->mX = xl;
				aSpan->mWidth = xr - xl + 1;
			}			
			
			mPFActiveEdgeList[j].mX += mPFActiveEdgeList[j].mDX;	// increment edge coords 
			mPFActiveEdgeList[j+1].mX += mPFActiveEdgeList[j+1].mDX;
		}
	}

	mRenderDevice->FillScanLines(aSpans, aSpanPos, GetFinalColor(), mDrawMode);

	delete ind;
	delete mPFActiveEdgeList;
}

void Graphics::PolyFillAA(const Point *theVertexList, int theNumVertices, bool convex) //Accurate? | 549-742
{
	SetAsCurrentContext();
	if (convex && mRenderDevice->CanFillPoly())
	{
		mRenderDevice->FillPoly(&theVertexList, theNumVertices, &mClipRect, mColor, mDrawMode, mTransX, mTransY);
		return;
	}

	int i;

	RenderDevice::Span aSpans[MAX_TEMP_SPANS];
	int aSpanPos = 0;

	static BYTE aCoverageBuffer[256*256];
	int aCoverWidth = 256, aCoverHeight = 256; 
	int aCoverLeft, aCoverRight, aCoverTop, aCoverBottom;

	for (i = 0; i < theNumVertices; ++i)
	{
		const Point* aPt = &theVertexList[i];
		if (i == 0)
		{
			aCoverLeft = aCoverRight = aPt->mX;
			aCoverTop = aCoverBottom = aPt->mY;
		}
		else
		{
			aCoverLeft = min(aCoverLeft, aPt->mX);
			aCoverRight = max(aCoverRight, aPt->mX);
			aCoverTop = min(aCoverTop, aPt->mY);
			aCoverBottom = max(aCoverBottom, aPt->mY);
		}
	}
	BYTE* coverPtr = aCoverageBuffer;
	if ((aCoverRight-aCoverLeft+1) > aCoverWidth || (aCoverBottom-aCoverTop+1) > aCoverHeight)
	{
		aCoverWidth = aCoverRight-aCoverLeft+1;
		aCoverHeight = aCoverBottom-aCoverTop+1;
		coverPtr = new BYTE[aCoverWidth*aCoverHeight];
	}
	memset(coverPtr, 0, aCoverWidth*aCoverHeight);

    int k, y0, y1, y, j, xl, xr;
    int *ind;		/* list of vertex indices, sorted by mPFPoints[ind[j]].y */		

	int aMinX = mClipRect.mX;
	int aMaxX = mClipRect.mX + mClipRect.mWidth - 1;
	int aMinY = mClipRect.mY;
	int aMaxY = mClipRect.mY + mClipRect.mHeight - 1;

    mPFNumVertices = theNumVertices;
    mPFPoints = theVertexList;
    
	if (mPFNumVertices<=0) return;

    ind = new int[mPFNumVertices];
    mPFActiveEdgeList = new Edge[mPFNumVertices];

    /* create y-sorted array of indices ind[k] into vertex list */
    for (k=0; k<mPFNumVertices; k++)
		ind[k] = k;
    qsort(ind, mPFNumVertices, sizeof ind[0], PFCompareInd);	/* sort ind by mPFPoints[ind[k]].y */

    mPFNumActiveEdges = 0;				/* start with empty active list */
    k = 0;				/* ind[k] is next vertex to process */
    y0 = (int) max(aMinY, ceil(mPFPoints[ind[0]].mY-0.5 + mTransY));		/* ymin of polygon */
    y1 = (int) min(aMaxY, floor(mPFPoints[ind[mPFNumVertices-1]].mY-0.5 + mTransY));	/* ymax of polygon */

    for (y=y0; y<=y1; y++) 
	{
		// step through scanlines 
		// scanline y is at y+.5 in continuous coordinates 

		// check vertices between previous scanline and current one, if any 
		for (; (k < mPFNumVertices) && (mPFPoints[ind[k]].mY + mTransY <= y + 0.5); k++) 
		{
			// to simplify, if mPFPoints.mY=y+.5, pretend it's above 
			// invariant: y-.5 < mPFPoints[i].mY <= y+.5 
			i = ind[k];				
			// insert or delete edges before and after vertex i (i-1 to i,
			// and i to i+1) from active list if they cross scanline y			 

			j = i>0 ? i-1 : mPFNumVertices-1;	// vertex previous to i 
			if (mPFPoints[j].mY + mTransY <= y-0.5)	// old edge, remove from active list 
				PFDelete(j);
			else if (mPFPoints[j].mY + mTransY > y+0.5)	// new edge, add to active list 
				PFInsert(j, y);

			j = i<mPFNumVertices-1 ? i+1 : 0;	// vertex next after i 
			if (mPFPoints[j].mY + mTransY <= y-0.5)	// old edge, remove from active list 
				PFDelete(i);
			else if (mPFPoints[j].mY + mTransY > y+0.5)	// new edge, add to active list 
				PFInsert(i, y);
		}

		// sort active edge list by active[j].mX 
		qsort(mPFActiveEdgeList, mPFNumActiveEdges, sizeof mPFActiveEdgeList[0], PFCompareActive);

		// draw horizontal segments for scanline y 
		for (j = 0; j < mPFNumActiveEdges; j += 2) 
		{	// draw horizontal segments 
			// span 'tween j & j+1 is inside, span tween j+1 & j+2 is outside 
			xl = (int) ceil(mPFActiveEdgeList[j].mX-0.5);		// left end of span 
			int lErr = int((fabs((mPFActiveEdgeList[j].mX-0.5) - xl)) * 255);
			if (xl<aMinX)
			{
				xl = aMinX;
				lErr = 255;
			}
			xr = (int) floor(mPFActiveEdgeList[j+1].mX-0.5);	// right end of span 
			int rErr = int((fabs((mPFActiveEdgeList[j+1].mX-0.5) - xr)) * 255);
			if (xr>aMaxX) 
			{
				xr = aMaxX;
				rErr = 255;
			}
			
			if ((xl <= xr) && (aSpanPos < MAX_TEMP_SPANS))
			{
				RenderDevice::Span* aSpan = &aSpans[aSpanPos++];
				aSpan->mY = y;
				aSpan->mX = xl;
				aSpan->mWidth = xr - xl + 1;

				BYTE* coverRow = coverPtr + (y - aCoverTop) * aCoverWidth;
				if (xr == xl)
				{
					coverRow[xl-aCoverLeft] = min(255, coverRow[xl-aCoverLeft] + ((lErr*rErr)>>8));
				}
				else
				{
					if (fabs(mPFActiveEdgeList[j].mDX) > 1.0f) // mostly horizontal on the left edge
					{
						double m = 1.0 / mPFActiveEdgeList[j].mDX, 
								b = mPFActiveEdgeList[j].b, 
								c = fabs(mPFActiveEdgeList[j].mDX);
						do
						{
							double _y =	m * xl + b;
							lErr = min(255, int(fabs((_y) - y - .5) * 255));
							coverRow[xl-aCoverLeft] = min(255, coverRow[xl-aCoverLeft] + lErr);
							xl++;
							c -= 1.0;
						} while (xl <= xr && c > 0);
					}
					else
					{
						coverRow[xl-aCoverLeft] = min(255, coverRow[xl-aCoverLeft] + lErr);
						xl++;
					}

					if (fabs(mPFActiveEdgeList[j+1].mDX) > 1.0f) // mostly horizontal on the right edge
					{
						double m = 1.0 / mPFActiveEdgeList[j+1].mDX, 
								b = mPFActiveEdgeList[j+1].b, 
								c = fabs(mPFActiveEdgeList[j+1].mDX);
						do
						{
							double _y =	m * xr + b;
							rErr = min(255, int(fabs((_y) - y - .5) * 255));
							coverRow[xr-aCoverLeft] = min(255, coverRow[xr-aCoverLeft] + rErr);
							xr--;
							c -= 1.0;
						} while (xr >= xl && c > 0);
					}
					else
					{
						coverRow[xr-aCoverLeft] = min(255, coverRow[xr-aCoverLeft] + rErr);
						xr--;
					}

					if (xl <= xr)
						memset(&coverRow[xl-aCoverLeft], 255, xr-xl+1);
				}
			}			
			
			mPFActiveEdgeList[j].mX += mPFActiveEdgeList[j].mDX;	// increment edge coords 
			mPFActiveEdgeList[j+1].mX += mPFActiveEdgeList[j+1].mDX;
		}
	}

	mRenderDevice->FillScanLinesWithCoverage(aSpans, aSpanPos, GetFinalColor(), mDrawMode, coverPtr, aCoverLeft, aCoverTop, aCoverWidth, aCoverHeight);
	
	if (coverPtr != aCoverageBuffer) delete[] coverPtr;
	delete[] ind;
	delete[] mPFActiveEdgeList;
}


bool Graphics::DrawLineClipHelper(double* theStartX, double* theStartY, double* theEndX, double* theEndY) //746-814
{
	double aStartX = *theStartX;
	double aStartY = *theStartY;
	double aEndX = *theEndX;
	double aEndY = *theEndY;

	// Clip X
	if (aStartX > aEndX)
	{
		std::swap(aStartX,aEndX);
		std::swap(aStartY,aEndY);
	}

	if (aStartX < mClipRect.mX)
	{
		if (aEndX < mClipRect.mX)
			return false;
					
		double aSlope = (aEndY - aStartY) / (aEndX - aStartX);
		aStartY += (mClipRect.mX - aStartX ) * aSlope;
		aStartX = mClipRect.mX;
	}

	if (aEndX >= mClipRect.mX + mClipRect.mWidth)
	{
		if (aStartX >= mClipRect.mX + mClipRect.mWidth)
			return false;

		double aSlope = (aEndY - aStartY) / (aEndX - aStartX);
		aEndY += (mClipRect.mX + mClipRect.mWidth - 1 - aEndX) * aSlope;
		aEndX = mClipRect.mX + mClipRect.mWidth - 1;
	}

	// Clip Y
	if (aStartY > aEndY)
	{
		std::swap(aStartX,aEndX);
		std::swap(aStartY,aEndY);
	}


	if (aStartY < mClipRect.mY)
	{
		if (aEndY < mClipRect.mY)
			return false;
					
		double aSlope = (aEndX - aStartX) / (aEndY - aStartY);
		aStartX += (mClipRect.mY - aStartY ) * aSlope;			

		aStartY = mClipRect.mY;
	}

	if (aEndY >= mClipRect.mY + mClipRect.mHeight)
	{
		if (aStartY >= mClipRect.mY + mClipRect.mHeight)
			return false;

		double aSlope = (aEndX - aStartX) / (aEndY - aStartY);
		aEndX += (mClipRect.mY + mClipRect.mHeight - 1 - aEndY) * aSlope;			
		aEndY = mClipRect.mY + mClipRect.mHeight - 1;
	}

	*theStartX = aStartX;
	*theStartY = aStartY;
	*theEndX = aEndX;
	*theEndY = aEndY;

	return true;
}

void Graphics::DrawLine(int theStartX, int theStartY, int theEndX, int theEndY) //817-828
{
	double aStartX = theStartX + mTransX;
	double aStartY = theStartY + mTransY;
	double aEndX = theEndX + mTransX;
	double aEndY = theEndY + mTransY;

	if (!DrawLineClipHelper(&aStartX, &aStartY, &aEndX, &aEndY))
		return;

	SetAsCurrentContext();
	mRenderDevice->DrawLine(aStartX, aStartY, aEndX, aEndY, GetFinalColor(), mDrawMode);
}

void Graphics::DrawLineAA(int theStartX, int theStartY, int theEndX, int theEndY) //832-842
{
	double aStartX = theStartX + mTransX;
	double aStartY = theStartY + mTransY;
	double aEndX = theEndX + mTransX;
	double aEndY = theEndY + mTransY;

	if (!DrawLineClipHelper(&aStartX, &aStartY, &aEndX, &aEndY))
		return;

	SetAsCurrentContext();
	mRenderDevice->DrawLine(aStartX, aStartY, aEndX, aEndY, GetFinalColor(), mDrawMode, true);
}

void Graphics::DrawString(const SexyString& theString, int theX, int theY) //845-848
{
	if (mFont != NULL)
		mFont->DrawString(this, theX, theY, theString, GetFinalColor(), mClipRect);
}

void Graphics::DrawImage(Image* theImage, int theX, int theY) //852-869
{
	if (mScaleX!=1 || mScaleY!=1)
	{
		DrawImage(theImage,theX,theY,Rect(0,0,theImage->mWidth,theImage->mHeight));
		return;
	}

	theX += mTransX;
	theY += mTransY;	

	Rect aDestRect = Rect(theX, theY, theImage->GetWidth(), theImage->GetHeight()).Intersection(mClipRect);
	Rect aSrcRect(aDestRect.mX - theX, aDestRect.mY - theY, aDestRect.mWidth, aDestRect.mHeight);

	if ((aSrcRect.mWidth > 0) && (aSrcRect.mHeight > 0))
	{
		SetAsCurrentContext();
		mRenderDevice->Blt(theImage, aDestRect.mX, aDestRect.mY, aSrcRect, GetImageColor(), mDrawMode);
	}
}

void Graphics::DrawImage(Image* theImage, int theX, int theY, const Rect& theSrcRect) //872-900
{
	DBG_ASSERTE(theSrcRect.mX + theSrcRect.mWidth <= theImage->GetWidth());	 //873
	DBG_ASSERTE(theSrcRect.mY + theSrcRect.mHeight <= theImage->GetHeight()); //874

	if ((theSrcRect.mX + theSrcRect.mWidth > theImage->GetWidth()) ||
		(theSrcRect.mY + theSrcRect.mHeight > theImage->GetHeight()))
		return;	

	theX += mTransX;
	theY += mTransY;

	if (mScaleX!=1 || mScaleY!=1)
	{
		Rect aDestRect(mScaleOrigX+floor((theX-mScaleOrigX)*mScaleX),mScaleOrigY+floor((theY-mScaleOrigY)*mScaleY),ceil(theSrcRect.mWidth*mScaleX),ceil(theSrcRect.mHeight*mScaleY));
		SetAsCurrentContext();
		mRenderDevice->BltStretched(theImage, aDestRect, theSrcRect, mClipRect, GetImageColor(), mDrawMode, mFastStretch);
		return;
	}

	Rect aDestRect = Rect(theX, theY, theSrcRect.mWidth, theSrcRect.mHeight).Intersection(mClipRect);
	Rect aSrcRect(theSrcRect.mX + aDestRect.mX - theX, theSrcRect.mY + aDestRect.mY - theY, aDestRect.mWidth, aDestRect.mHeight);

	if ((aSrcRect.mWidth > 0) && (aSrcRect.mHeight > 0))
	{
		SetAsCurrentContext();
		mRenderDevice->Blt(theImage, aDestRect.mX, aDestRect.mY, aSrcRect, GetImageColor(), mDrawMode);
	}
}

void Graphics::DrawImageMirror(Image* theImage, int theX, int theY, bool mirror) //903-905
{
	DrawImageMirror(theImage,theX,theY,Rect(0,0,theImage->mWidth,theImage->mHeight),mirror);
}

void Graphics::DrawImageMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, bool mirror) //908-938
{
	if (!mirror)
	{
		DrawImage(theImage, theX, theY, theSrcRect);
		return;
	}

	theX += mTransX;
	theY += mTransY;

	DBG_ASSERTE(theSrcRect.mX + theSrcRect.mWidth <= theImage->GetWidth());	 //918
	DBG_ASSERTE(theSrcRect.mY + theSrcRect.mHeight <= theImage->GetHeight()); //919

	if ((theSrcRect.mX + theSrcRect.mWidth > theImage->GetWidth()) ||
		(theSrcRect.mY + theSrcRect.mHeight > theImage->GetHeight()))
		return;	

	Rect aDestRect = Rect(theX, theY, theSrcRect.mWidth, theSrcRect.mHeight).Intersection(mClipRect);

	int aTotalClip = theSrcRect.mWidth - aDestRect.mWidth;
	int aLeftClip = aDestRect.mX - theX;
	int aRightClip = aTotalClip-aLeftClip;

	Rect aSrcRect(theSrcRect.mX + aRightClip, theSrcRect.mY + aDestRect.mY - theY, aDestRect.mWidth, aDestRect.mHeight);

	if ((aSrcRect.mWidth > 0) && (aSrcRect.mHeight > 0))
	{
		SetAsCurrentContext();
		mRenderDevice->BltMirror(theImage, aDestRect.mX, aDestRect.mY, aSrcRect, GetImageColor(), mDrawMode);
	}
}

void Graphics::DrawImageMirror(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, bool mirror) //941-952
{
	if (!mirror)
	{
		DrawImage(theImage,theDestRect,theSrcRect);
		return;
	}

	Rect aDestRect = Rect(theDestRect.mX + mTransX, theDestRect.mY + mTransY, theDestRect.mWidth, theDestRect.mHeight);

	SetAsCurrentContext();
	mRenderDevice->BltStretched(theImage, aDestRect, theSrcRect, &mClipRect, GetImageColor(), mDrawMode, mFastStretch, true);
}


void Graphics::DrawImage(Image* theImage, int theX, int theY, int theStretchedWidth, int theStretchedHeight) //956-962
{
	Rect aDestRect = Rect(theX + mTransX, theY + mTransY, theStretchedWidth, theStretchedHeight);
	Rect aSrcRect = Rect(0, 0, theImage->mWidth, theImage->mHeight);

	SetAsCurrentContext();
	mRenderDevice->BltStretched(theImage, aDestRect, aSrcRect, mClipRect, GetImageColor(), mDrawMode, mFastStretch);
}

void Graphics::DrawImage(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect) //965-977
{	
	Rect aDestRect = Rect(theDestRect.mX + mTransX, theDestRect.mY + mTransY, theDestRect.mWidth, theDestRect.mHeight);

	SetAsCurrentContext();
	mRenderDevice->BltStretched(theImage, aDestRect, theSrcRect, &mClipRect, GetImageColor(), mDrawMode, mFastStretch);
}

void Graphics::DrawImageF(Image* theImage, float theX, float theY) //980-988
{
	theX += mTransX;
	theY += mTransY;	

	Rect aSrcRect(0, 0, theImage->mWidth, theImage->mHeight);

	SetAsCurrentContext();
	mRenderDevice->BltF(theImage, theX, theY, aSrcRect, mClipRect, GetImageColor(), mDrawMode);
}

void Graphics::DrawImageF(Image* theImage, float theX, float theY, const Rect& theSrcRect) //991-1000
{
	DBG_ASSERTE(theSrcRect.mX + theSrcRect.mWidth <= theImage->GetWidth());	 //992
	DBG_ASSERTE(theSrcRect.mY + theSrcRect.mHeight <= theImage->GetHeight()); //993

	theX += mTransX;
	theY += mTransY;
	
	SetAsCurrentContext();
	mRenderDevice->BltF(theImage, theX, theY, theSrcRect, mClipRect, GetImageColor(), mDrawMode);
}

void Graphics::DrawImageRotated(Image* theImage, int theX, int theY, double theRot, const Rect *theSrcRect) //1003-1018
{	
	if (theSrcRect == NULL)
	{
		int aRotCenterX = theImage->GetWidth() / 2;
		int aRotCenterY = theImage->GetHeight() / 2;

		DrawImageRotatedF(theImage, theX, theY, theRot, aRotCenterX, aRotCenterY, theSrcRect);
	}
	else
	{
		int aRotCenterX = theSrcRect->mWidth / 2;
		int aRotCenterY = theSrcRect->mHeight / 2;

		DrawImageRotatedF(theImage, theX, theY, theRot, aRotCenterX, aRotCenterY, theSrcRect);
	}
}

void Graphics::DrawImageRotatedF(Image* theImage, float theX, float theY, double theRot, const Rect *theSrcRect) //1021-1036
{	
	if (theSrcRect == NULL)
	{
		float aRotCenterX = theImage->GetWidth() / 2.0f;
		float aRotCenterY = theImage->GetHeight() / 2.0f;

		DrawImageRotatedF(theImage, theX, theY, theRot, aRotCenterX, aRotCenterY, theSrcRect);
	}
	else
	{
		float aRotCenterX = theSrcRect->mWidth / 2.0f;
		float aRotCenterY = theSrcRect->mHeight / 2.0f;

		DrawImageRotatedF(theImage, theX, theY, theRot, aRotCenterX, aRotCenterY, theSrcRect);
	}
}

void Graphics::DrawImageRotated(Image* theImage, int theX, int theY, double theRot, int theRotCenterX, int theRotCenterY, const Rect *theSrcRect) //1039-1041
{
	DrawImageRotatedF(theImage,theX,theY,theRot,theRotCenterX,theRotCenterY,theSrcRect);
}

void Graphics::DrawImageRotatedF(Image* theImage, float theX, float theY, double theRot, float theRotCenterX, float theRotCenterY, const Rect *theSrcRect) //1044-1057
{
	theX += mTransX;
	theY += mTransY;	

	if (theSrcRect==NULL)
	{
		Rect aSrcRect(0,0,theImage->mWidth,theImage->mHeight);

		SetAsCurrentContext();
		mRenderDevice->BltRotated(theImage, theX, theY, aSrcRect, mClipRect, GetImageColor(), mDrawMode, theRot, theRotCenterX, theRotCenterY);
	}
	else
	{
		SetAsCurrentContext();
		mRenderDevice->BltRotated(theImage, theX, theY, *theSrcRect, mClipRect, GetImageColor(), mDrawMode, theRot, theRotCenterX, theRotCenterY);
	}
}

void Graphics::DrawImageMatrix(Image* theImage, const SexyMatrix3 &theMatrix, float x, float y) //1060-1065
{	
	Rect aSrcRect(0,0,theImage->mWidth,theImage->mHeight);

	SetAsCurrentContext();
	mRenderDevice->BltMatrix(theImage,x+mTransX,y+mTransY,theMatrix,mClipRect,mColorizeImages?mColor:Color::White,mDrawMode,aSrcRect,mLinearBlend);
}

void Graphics::DrawImageMatrix(Image* theImage, const SexyMatrix3 &theMatrix, const Rect &theSrcRect, float x, float y) //1068-1071
{
	SetAsCurrentContext();
	mRenderDevice->BltMatrix(theImage,x+mTransX,y+mTransY,theMatrix,mClipRect,mColorizeImages?mColor:Color::White,mDrawMode,theSrcRect,mLinearBlend);
}

void Graphics::DrawImageTransformHelper(Image* theImage, const Transform &theTransform, const Rect &theSrcRect, float x, float y, bool useFloat) //1074-1132
{
	if (theTransform.mComplex || (Get3D() && useFloat))
	{
		DrawImageMatrix(theImage,theTransform.GetMatrix(),theSrcRect,x,y);
		return;
	}

	// Translate into appropriate graphics call
	float w2 = theSrcRect.mWidth/2.0f;
	float h2 = theSrcRect.mHeight/2.0f;

	if (theTransform.mHaveRot)
	{
		float rx = w2-theTransform.mTransX1;
		float ry = h2-theTransform.mTransY1;
		
		x = x + theTransform.mTransX2 - rx + 0.5f;
		y = y + theTransform.mTransY2 - ry + 0.5f;

		if (useFloat)
			DrawImageRotatedF(theImage,x,y,theTransform.mRot,rx,ry,&theSrcRect);
		else
			DrawImageRotated(theImage,x,y,theTransform.mRot,rx,ry,&theSrcRect);
	}
	else if (theTransform.mHaveScale)
	{
		bool mirror = false;
		if (theTransform.mScaleX==-1)
		{
			if (theTransform.mScaleY==1)
			{
				x = x + theTransform.mTransX1 + theTransform.mTransX2 - w2 + 0.5f;
				y = y + theTransform.mTransY1 + theTransform.mTransY2 - h2 + 0.5f;
				DrawImageMirror(theImage,x,y,theSrcRect);
				return;
			}
			mirror = true;
		}

		float sw = w2*theTransform.mScaleX;
		float sh = h2*theTransform.mScaleY;

		x = x + theTransform.mTransX2 - sw;
		y = y + theTransform.mTransY2 - sh;
	
		Rect aDestRect(x,y,sw*2,sh*2);
		DrawImageMirror(theImage,aDestRect,theSrcRect,mirror);
	}
	else
	{
		x = x + theTransform.mTransX1 + theTransform.mTransX2 - w2 + 0.5f;
		y = y + theTransform.mTransY1 + theTransform.mTransY2 - h2 + 0.5f;

		if (useFloat)
			DrawImageF(theImage,x,y,theSrcRect);
		else
			DrawImage(theImage,x,y,theSrcRect);
	}
}

void Graphics::DrawImageTransform(Image* theImage, const Transform &theTransform, float x, float y) //1135-1137
{
	DrawImageTransformHelper(theImage,theTransform,Rect(0,0,theImage->mWidth,theImage->mHeight),x,y,false);
}

void Graphics::DrawImageTransformF(Image* theImage, const Transform &theTransform, float x, float y) //1140-1142
{
	DrawImageTransformHelper(theImage,theTransform,Rect(0,0,theImage->mWidth,theImage->mHeight),x,y,true);
}

void Graphics::DrawImageTransform(Image* theImage, const Transform &theTransform, const Rect &theSrcRect, float x, float y) //1145-1147
{
	DrawImageTransformHelper(theImage,theTransform,theSrcRect,x,y,false);
}

void Graphics::DrawImageTransformF(Image* theImage, const Transform &theTransform, const Rect &theSrcRect, float x, float y) //1150-1152
{
	DrawImageTransformHelper(theImage,theTransform,theSrcRect,x,y,true);
}

void Graphics::DrawTriangleTex(Image *theTexture, const SexyVertex2D &v1, const SexyVertex2D &v2, const SexyVertex2D &v3) //1155-1160
{
	SexyVertex2D v[1][3] = {{v1,v2,v3}};
	SetAsCurrentContext();
	mRenderDevice->BltTriangles(theTexture,v,1,GetImageColor(),mDrawMode,mTransX,mTransY,mLinearBlend,&mClipRect);
}

void Graphics::DrawTrianglesTex(Image *theTexture, const SexyVertex2D* theVertices[][3], int theNumTriangles) //1163-1166
{
	SetAsCurrentContext();
	mRenderDevice->BltTriangles(theTexture, theVertices, theNumTriangles, GetImageColor(), mDrawMode, mTransX, mTransY, mLinearBlend, &mClipRect);
}
void Graphics::DrawTrianglesTex(Image* theTexture, const SexyVertex2D* theVertices[][3], int theNumTriangles, const Color& theColor, int theDrawMode, float tx, float ty, bool blend, Rect* theClipRect) //1168-1171
{
	SetAsCurrentContext();
	mRenderDevice->BltTriangles(theTexture, theVertices, theNumTriangles, theColor, theDrawMode, tx, ty, blend, theClipRect);
}

void Graphics::DrawTrianglesTexStrip(Image* theTexture, const SexyVertex2D* theVertices, int theNumTriangles) //1174-1176
{
	DrawTrianglesTexStrip(theTexture, theVertices, theNumTriangles, GetImageColor(), mDrawMode, mTransX, mTransY, mLinearBlend);
}
void Graphics::DrawTrianglesTexStrip(Image* theTexture, const SexyVertex2D* theVertices, int theNumTriangles, const Color& theColor, int theDrawMode, float tx = 0, float ty = 0, bool blend = true) //Todo | 1178-1195
{
	SexyVertex2D* aList[100][3];
	SetAsCurrentContext();
	int aTriNum = 0;
	while (aTriNum < theNumTriangles)
	{
		int aMaxTriangles = min(100, theNumTriangles - aTriNum);
		for (int i = 0; i < aMaxTriangles; i++)
		{
			aList[i][0] = theVertices[aTriNum];
			aList[i][1] = theVertices[aTriNum + 1];
			aList[i][2] = theVertices[aTriNum + 2];
			aTriNum++;
		}
		mRenderDevice->BltTriangles(theTexture, aList, aTriNum, theColor, theDrawMode, tx, ty, blend, NULL);
	}
}

void Graphics::ClearClipRect() //1198-1200
{
	mClipRect = Rect(0, 0, mDestImage->GetWidth(), mDestImage->GetHeight());
}

void Graphics::SetClipRect(int theX, int theY, int theWidth, int theHeight) //1203-1207
{
	mClipRect = 
		Rect(0, 0, mDestImage->GetWidth(), mDestImage->GetHeight()).Intersection(
			Rect(theX + mTransX, theY + mTransY, theWidth, theHeight));
}

void Graphics::SetClipRect(const Rect& theRect) //1210-1212
{
	SetClipRect(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

void Graphics::ClipRect(int theX, int theY, int theWidth, int theHeight) //1215-1217
{
	mClipRect = mClipRect.Intersection(Rect(theX + mTransX, theY + mTransY, theWidth, theHeight));
}

void Graphics::ClipRect(const Rect& theRect) //1220-1222
{
	ClipRect(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

void Graphics::Translate(int theTransX, int theTransY) //1225-1228
{
	mTransX += theTransX;
	mTransY += theTransY;
}

void Graphics::TranslateF(float theTransX, float theTransY) //1231-1234
{
	mTransX += theTransX;
	mTransY += theTransY;
}

void Graphics::SetScale(float theScaleX, float theScaleY, float theOrigX, float theOrigY) //1237-1242
{
	mScaleX = theScaleX;
	mScaleY = theScaleY;
	mScaleOrigX = theOrigX + mTransX;
	mScaleOrigY = theOrigY + mTransY;
}

int Graphics::StringWidth(const SexyString& theString) //1245-1247
{
	return mFont->StringWidth(theString);
}

void Graphics::DrawImageBox(const Rect& theDest, Image* theComponentImage) //1250-1252
{
	DrawImageBox(Rect(0,0,theComponentImage->mWidth,theComponentImage->mHeight),theDest,theComponentImage);
}

void Graphics::DrawImageBox(const Rect& theSrc, const Rect &theDest, Image* theComponentImage) //TODO | 1255-1357
{	
	if (theSrc.mWidth<=0 || theSrc.mHeight<=0)
		return;

	int cw = theSrc.mWidth/3;
	int ch = theSrc.mHeight/3;
	int cx = theSrc.mX;
	int cy = theSrc.mY;
	int cmw = theSrc.mWidth - cw*2;
	int cmh = theSrc.mHeight - ch*2;
	bool stretched = false;
		
	// Draw 4 corners
	DrawImage(theComponentImage, theDest.mX, theDest.mY, Rect(cx,cy, cw, ch));
	DrawImage(theComponentImage, theDest.mX + theDest.mWidth-cw, theDest.mY, Rect(cx + cw + cmw, cy, cw, ch));
	DrawImage(theComponentImage, theDest.mX, theDest.mY + theDest.mHeight-ch, Rect(cx, cy + ch + cmh, cw, ch));
	DrawImage(theComponentImage, theDest.mX + theDest.mWidth-cw, theDest.mY + theDest.mHeight-ch, Rect(cx + cw + cmw, cy + ch + cmh, cw, ch));

	// Draw top and bottom
	Graphics aVertClip(*this);
	aVertClip.ClipRect(theDest.mX + cw, theDest.mY, theDest.mWidth-cw*2, theDest.mHeight);
	int aCol, aRow;
	for (aCol = 0; aCol < (theDest.mWidth-cw*2+cmw-1)/cmw; aCol++)
	{
		aVertClip.DrawImage(theComponentImage, theDest.mX + cw + aCol*cmw, theDest.mY, Rect(cx + cw, cy, cmw, ch));
		aVertClip.DrawImage(theComponentImage, theDest.mX + cw + aCol*cmw, theDest.mY + theDest.mHeight-ch, Rect(cx + cw, cy + ch + cmh, cmw, ch));
	}

	// Draw sides
	Graphics aHorzClip(*this);
	aHorzClip.ClipRect(theDest.mX, theDest.mY + ch, theDest.mWidth, theDest.mHeight-ch*2);
	for (aRow = 0; aRow < (theDest.mHeight-ch*2+cmh-1)/cmh; aRow++)
	{
		aHorzClip.DrawImage(theComponentImage, theDest.mX, theDest.mY + ch + aRow*cmh, Rect(cx, cy + ch, cw, cmh));
		aHorzClip.DrawImage(theComponentImage, theDest.mX + theDest.mWidth-cw, theDest.mY + ch + aRow*cmh, Rect(cx + cw + cmw, cy + ch, cw, cmh));
	}

	// Draw middle
	Graphics aMidClip(*this);
	aMidClip.ClipRect(theDest.mX + cw, theDest.mY + ch, theDest.mWidth-cw*2, theDest.mHeight-ch*2);
	for (aCol = 0; aCol < (theDest.mWidth-cw*2+cmw-1)/cmw; aCol++)
		for (aRow = 0; aRow < (theDest.mHeight-ch*2+cmh-1)/cmh; aRow++)
			aMidClip.DrawImage(theComponentImage, theDest.mX + cw + aCol*cmw, theDest.mY + ch + aRow*cmh, Rect(cx + cw, cy + ch, cmw, cmh));
	
	PopState();
}

void Graphics::DrawImageBoxStretch(const Rect& theDest, Image* theComponentImage) //1360-1362
{
	DrawImageBoxStretch(Rect(0, 0, theComponentImage->mWidth, theComponentImage->mHeight), theDest, theComponentImage);
}

void Graphics::DrawImageBoxStretch(const Rect& theSrc, const Rect& theDest, Image* theComponentImage) //TODO | 1365-1417
{
	if (theSrc.mWidth <= 0 || theSrc.mHeight <= 0)
		return;

	int cw = theSrc.mWidth / 3;
	int ch = theSrc.mHeight / 3;
	int cx = theSrc.mX;
	int cy = theSrc.mY;
	int cmw = theSrc.mWidth - cw * 2;
	int cmh = theSrc.mHeight - ch * 2;
	bool stretched = false;

	// Draw 4 corners
	DrawImage(theComponentImage, theDest.mX, theDest.mY, Rect(cx, cy, cw, ch));
	DrawImage(theComponentImage, theDest.mX + theDest.mWidth - cw, theDest.mY, Rect(cx + cw + cmw, cy, cw, ch));
	DrawImage(theComponentImage, theDest.mX, theDest.mY + theDest.mHeight - ch, Rect(cx, cy + ch + cmh, cw, ch));
	DrawImage(theComponentImage, theDest.mX + theDest.mWidth - cw, theDest.mY + theDest.mHeight - ch, Rect(cx + cw + cmw, cy + ch + cmh, cw, ch));

	// Draw top and bottom
	Graphics aVertClip(*this);
	aVertClip.ClipRect(theDest.mX + cw, theDest.mY, theDest.mWidth - cw * 2, theDest.mHeight);
	int aCol, aRow;
	for (aCol = 0; aCol < (theDest.mWidth - cw * 2 + cmw - 1) / cmw; aCol++)
	{
		aVertClip.DrawImage(theComponentImage, theDest.mX + cw + aCol * cmw, theDest.mY, Rect(cx + cw, cy, cmw, ch));
		aVertClip.DrawImage(theComponentImage, theDest.mX + cw + aCol * cmw, theDest.mY + theDest.mHeight - ch, Rect(cx + cw, cy + ch + cmh, cmw, ch));
	}

	// Draw sides
	Graphics aHorzClip(*this);
	aHorzClip.ClipRect(theDest.mX, theDest.mY + ch, theDest.mWidth, theDest.mHeight - ch * 2);
	for (aRow = 0; aRow < (theDest.mHeight - ch * 2 + cmh - 1) / cmh; aRow++)
	{
		aHorzClip.DrawImage(theComponentImage, theDest.mX, theDest.mY + ch + aRow * cmh, Rect(cx, cy + ch, cw, cmh));
		aHorzClip.DrawImage(theComponentImage, theDest.mX + theDest.mWidth - cw, theDest.mY + ch + aRow * cmh, Rect(cx + cw + cmw, cy + ch, cw, cmh));
	}

	// Draw middle
	Graphics aMidClip(*this);
	aMidClip.ClipRect(theDest.mX + cw, theDest.mY + ch, theDest.mWidth - cw * 2, theDest.mHeight - ch * 2);
	for (aCol = 0; aCol < (theDest.mWidth - cw * 2 + cmw - 1) / cmw; aCol++)
		for (aRow = 0; aRow < (theDest.mHeight - ch * 2 + cmh - 1) / cmh; aRow++)
			aMidClip.DrawImage(theComponentImage, theDest.mX + cw + aCol * cmw, theDest.mY + ch + aRow * cmh, Rect(cx + cw, cy + ch, cmw, cmh));

	PopState();
}

void Graphics::DrawImageCel(Image* theImageStrip, int theX, int theY, int theCel) //1420-1422
{
	DrawImageCel(theImageStrip, theX, theY, theCel % theImageStrip->mNumCols, theCel / theImageStrip->mNumCols); 
}

void Graphics::DrawImageCel(Image* theImageStrip, const Rect& theDestRect, int theCel) //1425-1427
{
	DrawImageCel(theImageStrip, theDestRect, theCel % theImageStrip->mNumCols, theCel / theImageStrip->mNumCols); 
}

void Graphics::DrawImageCel(Image* theImageStrip, int theX, int theY, int theCelCol, int theCelRow) //1430-1439
{
	if (theCelRow<0 || theCelCol<0 || theCelRow >= theImageStrip->mNumRows || theCelCol >= theImageStrip->mNumCols)
		return;

	int aCelWidth = theImageStrip->mWidth / theImageStrip->mNumCols;
	int aCelHeight = theImageStrip->mHeight / theImageStrip->mNumRows;
	Rect aSrcRect(aCelWidth*theCelCol, aCelHeight*theCelRow, aCelWidth, aCelHeight);

	DrawImage(theImageStrip,theX,theY,aSrcRect);
}

void Graphics::DrawImageAnim(Image* theImageAnim, int theX, int theY, int theTime) //1442-1444
{
	DrawImageCel(theImageAnim, theX, theY, theImageAnim->GetAnimCel(theTime));
}

void Graphics::DrawImageCel(Image* theImageStrip, const Rect& theDestRect, int theCelCol, int theCelRow) //1447-1456
{
	if (theCelRow<0 || theCelCol<0 || theCelRow >= theImageStrip->mNumRows || theCelCol >= theImageStrip->mNumCols)
		return;

	int aCelWidth = theImageStrip->mWidth / theImageStrip->mNumCols;
	int aCelHeight = theImageStrip->mHeight / theImageStrip->mNumRows;
	Rect aSrcRect(aCelWidth*theCelCol, aCelHeight*theCelRow, aCelWidth, aCelHeight);

	DrawImage(theImageStrip,theDestRect,aSrcRect);
}

int Graphics::WriteString(const SexyString& theString, int theX, int theY, int theWidth, int theJustification, bool drawString, int theOffset, int theLength, int theOldColor) //1459-1547
{
	Font* aFont = GetFont();
	if (theOldColor==-1)
		theOldColor = mColor.ToInt();
	
	if (drawString)
	{
		switch (theJustification)
		{	
		case 0:
			theX += (theWidth - WriteString(theString, theX, theY, theWidth, -1, false, theOffset, theLength, theOldColor))/2;
			break;
		case 1:
			theX += theWidth - WriteString(theString, theX, theY, theWidth, -1, false, theOffset, theLength, theOldColor);
			break;
		}
	}

	if(theLength<0 || theOffset+theLength>(int)theString.length())
		theLength = (int)theString.length();
	else
		theLength = theOffset + theLength;


	SexyString aString;
	int aXOffset = 0;

	for (int i = theOffset; i < theLength; i++)
	{
		if ((theString[i] == '^') && mWriteColoredString)
		{
			if (i+1<theLength && theString[i+1] == '^') // literal '^'
			{
				aString += _S('^');
				i++;
			}
			else if (i>theLength-8) // badly formatted color specification
				break;
			else // change color instruction
			{
				DWORD aColor = 0;
				if (theString[i+1]==_S('o'))
				{
					if (sexystrncmp(theString.c_str()+i+1, _S("oldclr"), 6) == 0)
						aColor = theOldColor;
				}
				else
				{
					for (int aDigitNum = 0; aDigitNum < 6; aDigitNum++)
					{
						SexyChar aChar = theString[i+aDigitNum+1];
						int aVal = 0;

						if ((aChar >= _S('0')) && (aChar <= _S('9')))
							aVal = aChar - _S('0');
						else if ((aChar >= _S('A')) && (aChar <= _S('F')))
							aVal = (aChar - _S('A')) + 10;
						else if ((aChar >= _S('a')) && (aChar <= _S('f')))
							aVal = (aChar - _S('a')) + 10;

						aColor += (aVal << ((5 - aDigitNum) * 4));
					}				
				}

				if (drawString)
				{
					DrawString(aString, theX + aXOffset, theY);
					SetColor(Color((aColor >> 16) & 0xFF, (aColor >> 8) & 0xFF, (aColor) & 0xFF, GetColor().mAlpha));					
				}

				i += 7;

				aXOffset += GetFont()->StringWidth(aString);

				aString = _S("");
			}
		}
		else
			aString += theString[i];
	}

	if (drawString)
	{
		DrawString(aString, theX + aXOffset, theY);
	}

	aXOffset += GetFont()->StringWidth(aString);

	return aXOffset;
}

static int WriteWordWrappedHelper(Graphics *g, const SexyString& theString, int theX, int theY, int theWidth, int theJustification, bool drawString, int theOffset, int theLength, int theOldColor, int theMaxChars) //1550-1559
{
	if (theOffset+theLength>theMaxChars)
	{
		theLength = theMaxChars-theOffset;
		if (theLength<=0)
			return -1;
	}

	return g->WriteString(theString,theX,theY,theWidth,theJustification,drawString,theOffset,theLength,theOldColor);
}

int	Graphics::WriteWordWrapped(const Rect& theRect, const SexyString& theLine, int theLineSpacing, int theJustification, int *theMaxWidth, int theMaxChars, int *theLastWidth, int *theLineCount) //1562-1721
{
	Color anOrigColor = GetColor();
	int anOrigColorInt = anOrigColor.ToInt();
	if ((anOrigColorInt&0xFF000000)==0xFF000000)
		anOrigColorInt &= ~0xFF000000;

	if (theMaxChars<0)
		theMaxChars = (int)theLine.length();

	Font* aFont = GetFont();						

	int aYOffset = aFont->GetAscent() - aFont->GetAscentPadding();

	if (theLineSpacing == -1)
		theLineSpacing = aFont->GetLineSpacing();

	SexyString aCurString;
	ulong aCurPos = 0;
	int aLineStartPos = 0;
	int aCurWidth = 0;
	SexyChar aCurChar = 0;
	SexyChar aPrevChar = 0;
	int aSpacePos = -1;
	int aMaxWidth = 0;
	int anIndentX = 0;

	if (theLastWidth != NULL)
	{
		anIndentX = *theLastWidth;
		aCurWidth = anIndentX;
	}

	while (aCurPos < theLine.length())
	{	
		aCurChar = theLine[aCurPos];
		if(aCurChar==_S('^') && mWriteColoredString) // Handle special color modifier
		{
			if(aCurPos+1<theLine.length())
			{
				if(theLine[aCurPos+1]==_S('^'))
					aCurPos++; // literal '^' -> just skip the extra '^'
				else 
				{
					aCurPos+=8;
					continue; // skip the color specifier when calculating the width
				}
			}
		}
		else if(aCurChar==_S(' '))
			aSpacePos = aCurPos;
		else if(aCurChar==_S('\n'))
		{
			aCurWidth = theRect.mWidth+1; // force word wrap
			aSpacePos = aCurPos;
			aCurPos++; // skip enter on next go round
		}

		aCurWidth += aFont->CharWidthKern(aCurChar, aPrevChar);
		aPrevChar = aCurChar;

		if(aCurWidth > theRect.mWidth) // need to wrap
		{
			int aWrittenWidth;
			if(aSpacePos!=-1)
			{
				//aWrittenWidth = WriteWordWrappedHelper(this, theLine, theRect.mX, theRect.mY + aYOffset, theRect.mWidth, 
				//	theJustification, true, aLineStartPos, aSpacePos-aLineStartPos, anOrigColorInt, theMaxChars);

				int aPhysPos = theRect.mY + aYOffset + mTransY;
				if ((aPhysPos >= mClipRect.mY) && (aPhysPos < mClipRect.mY + mClipRect.mHeight + theLineSpacing))
				{
					WriteWordWrappedHelper(this, theLine, theRect.mX + anIndentX, theRect.mY + aYOffset, theRect.mWidth, 
						theJustification, true, aLineStartPos, aSpacePos-aLineStartPos, anOrigColorInt, theMaxChars);

					/*WriteString(theLine, theRect.mX + anIndentX, theRect.mY + aYOffset, theRect.mWidth, 
					theJustification, true, aLineStartPos, aSpacePos-aLineStartPos);*/
				}

				aWrittenWidth = aCurWidth + anIndentX;

				if (aWrittenWidth<0)
					break;

				aCurPos = aSpacePos+1;
				if (aCurChar != _S('\n'))
				{
					while (aCurPos<theLine.length() && theLine[aCurPos]==_S(' '))
						aCurPos++;
				}
				aLineStartPos = aCurPos;
			}
			else
			{
				if((int)aCurPos<aLineStartPos+1)
					aCurPos++; // ensure at least one character gets written

				aWrittenWidth = WriteWordWrappedHelper(this, theLine, theRect.mX + anIndentX, theRect.mY + aYOffset, theRect.mWidth, 
					theJustification, true, aLineStartPos, aCurPos-aLineStartPos, anOrigColorInt, theMaxChars);

				if (aWrittenWidth<0)
					break;

				if (theMaxWidth!=NULL && aWrittenWidth>*theMaxWidth)
					*theMaxWidth = aWrittenWidth;
				if (theLastWidth!=NULL)
					*theLastWidth = aWrittenWidth;
			}

			if (aWrittenWidth > aMaxWidth)
				aMaxWidth = aWrittenWidth;

			aLineStartPos = aCurPos;
			aSpacePos = -1;
			aCurWidth = 0;
			aPrevChar = 0;
			anIndentX = 0;
			aYOffset += theLineSpacing;
		}
		else
			aCurPos++;
	}

	if(aLineStartPos<(int)theLine.length()) // write the last piece
	{
		int aWrittenWidth = WriteWordWrappedHelper(this, theLine, theRect.mX + anIndentX, theRect.mY + aYOffset, theRect.mWidth, 
			theJustification, true, aLineStartPos, theLine.length()-aLineStartPos, anOrigColorInt, theMaxChars);

		if (aWrittenWidth>=0)
		{
			if (aWrittenWidth > aMaxWidth)
				aMaxWidth = aWrittenWidth;

			if (theMaxWidth!=NULL && aWrittenWidth>*theMaxWidth)
				*theMaxWidth = aWrittenWidth;
			if (theLastWidth!=NULL)
				*theLastWidth = aWrittenWidth;

			aYOffset += theLineSpacing;
		}
	}
	else if (aCurChar == '\n')
	{
		aYOffset += theLineSpacing;
		if (theLastWidth != NULL)
			*theLastWidth = 0;
	}

	SetColor(anOrigColor);

	if (theMaxWidth!=NULL)
		*theMaxWidth = aMaxWidth;

	return aYOffset + aFont->GetDescent() - theLineSpacing;
}

int	Graphics::DrawStringColor(const SexyString& theLine, int theX, int theY, int theOldColor) //1724-1726
{
	return WriteString(theLine, theX, theY, -1, -1,true,0,-1,theOldColor);
}

int	Graphics::DrawStringWordWrapped(const SexyString& theLine, int theX, int theY, int theWrapWidth, int theLineSpacing, int theJustification, int *theMaxWidth) //1729-1734
{
	int aYOffset = mFont->GetAscent() - mFont->GetAscentPadding();

	Rect aRect(theX,theY-aYOffset,theWrapWidth,0);
	return WriteWordWrapped(aRect, theLine, theLineSpacing, theJustification, theMaxWidth);
}

int	Graphics::GetWordWrappedHeight(int theWidth, const SexyString& theLine, int theLineSpacing, int *theMaxWidth) //1737-1743
{
	Graphics aTestG;
	aTestG.SetFont(mFont);
	int aHeight = aTestG.WriteWordWrapped(Rect(0, 0, theWidth, 0), theLine, theLineSpacing, -1, theMaxWidth);	

	return aHeight;	
}

void Graphics3D::SetAsCurrentContext() //1749-1751
{
	mRenderDevice->SetCurrentContext(mRenderContext);
}

Graphics3D::Graphics3D(Graphics* inGraphics, RenderDevice3D* inRenderDevice, const HRenderContext& inRenderContext) { mGraphics = inGraphics; mRenderDevice = inRenderDevice; mRenderContext = inRenderContext; } //1757

Graphics* Graphics3D::Get2D() //1760-1762
{
	return mGraphics;
}

bool Graphics3D::SupportsPixelShaders() //1765-1768
{

	return mRenderDevice->SupportsPixelShaders();
}
bool Graphics3D::SupportsVertexShaders() //1770-1773
{

	return mRenderDevice->SupportsVertexShaders();
}
bool Graphics3D::SupportsCubeMaps() //1775-1778
{

	return mRenderDevice->SupportsCubeMaps();
}
bool Graphics3D::SupportsVolumeMaps() //1780-1783
{

	return mRenderDevice->SupportsVolumeMaps();
}

bool Graphics3D::SupportsImageRenderTargets() //1786-1788
{
	return mRenderDevice->SupportsImageRenderTargets();
}

ulong Graphics3D::GetMaxTextureStages() //1791-1794
{
	return mRenderDevice->GetMaxTextureStages();
}

void Graphics3D::DrawPrimitiveEx(ulong theVertexFormat, EPrimitiveType thePrimitiveType, const SexyVertex* theVertices, int thePrimitiveCount, const Color& theColor, int theDrawMode, float tx, float ty, bool blend, ulong theFlags) //1797-1800
{
	SetAsCurrentContext();
	mRenderDevice->DrawPrimitiveEx(theVertexFormat, thePrimitiveType, theVertices, thePrimitiveCount, theColor, theDrawMode, tx, ty, blend, theFlags);
}

void Graphics3D::RenderMesh(Mesh* theMesh, const SexyMatrix4& theMatrix, const Color& theColor, bool doSetup) //1803-1806
{
	SetAsCurrentContext();
	mRenderDevice->RenderMesh(theMesh, theMatrix, theColor, doSetup);
}

void Graphics3D::SetBltDepth(float inDepth) //1809-1812
{
	SetAsCurrentContext();
	mRenderDevice->SetBltDepth(inDepth);
}
void Graphics3D::PushTransform(const SexyMatrix3& theTransform, bool concatenate) //1814-1817
{
	SetAsCurrentContext();
	mRenderDevice->PushTransform(theTransform, concatenate);
}
void Graphics3D::PopTransform() //1819-1822
{
	SetAsCurrentContext();
	mRenderDevice->PopTransform();
}

void Graphics3D::PopTransform(SexyMatrix3& theTransform) //1825-1828
{
	SetAsCurrentContext();
	mRenderDevice->PopTransform(theTransform);
}

void Graphics3D::ClearColorBuffer(const Color& inColor) //1831-1834
{
	SetAsCurrentContext();
	mRenderDevice->ClearColorBuffer(inColor);
}
void Graphics3D::ClearDepthBuffer() //1836-1839
{
	SetAsCurrentContext();
	mRenderDevice->ClearDepthBuffer();
}

void Graphics3D::SetDepthState(ECompareFunc inDepthTestFunc, bool inDepthWriteEnabled) //1842-1845
{
	SetAsCurrentContext();
	mRenderDevice->SetDepthState(inDepthTestFunc, inDepthWriteEnabled);
}
void Graphics3D::SetAlphaTest(ECompareFunc inAlphaTestFunc, int inRefAlpha) //1847-1850
{
	SetAsCurrentContext();
	mRenderDevice->SetAlphaTest(inAlphaTestFunc, inRefAlpha);
}
void Graphics3D::SetWireframe(bool inWireframe) //1852-1855
{
	SetAsCurrentContext();
	mRenderDevice->SetWireframe(inWireframe);
}
void Graphics3D::SetBlend(EBlendMode inSrcBlend, EBlendMode inDestBlend) //1857-1860
{
	SetAsCurrentContext();
	mRenderDevice->SetBlend(inSrcBlend, inDestBlend);
}
void Graphics3D::SetBackfaceCulling(bool inCullClockwise, bool inCullCounterClockwise) //1862-1865
{
	SetAsCurrentContext();
	mRenderDevice->SetBackfaceCulling(inCullClockwise, inCullCounterClockwise);
}

void Graphics3D::SetLightingEnabled(bool inLightingEnabled, bool inSetDefaultMaterialState) //1868-1879
{
	SetAsCurrentContext();
	mRenderDevice->SetLightingEnabled(inLightingEnabled);
	if (inLightingEnabled != 0 && inSetDefaultMaterialState)
	{
		SetMaterialAmbient(Color::White, -1);
		SetMaterialDiffuse(Color::White, 0);
		SetMaterialSpecular(Color::White, -1, 0.0);
		SetMaterialEmissive(Color::Black, -1);
	}
}
void Graphics3D::SetLightEnabled(int inLightIndex, bool inEnabled) //1881-1884
{
	SetAsCurrentContext();
	mRenderDevice->SetLightEnabled(inLightIndex, inEnabled);
}
void Graphics3D::SetPointLight(int inLightIndex, const SexyVector3& inPos, const LightColors& inColors, float inRange, const SexyVector3& inAttenuation) //1886-1889
{
	SetAsCurrentContext();
	mRenderDevice->SetPointLight(inLightIndex, inPos, inColors, inRange, inAttenuation); //Not in XNA
}
void Graphics3D::SetDirectionalLight(int inLightIndex, const SexyVector3& inDir, const LightColors& inColors) //1891-1894
{
	SetAsCurrentContext();
	mRenderDevice->SetDirectionalLight(inLightIndex, inDir, inColors); //Not in XNA
}
void Graphics3D::SetGlobalAmbient(const Color& inColor) //1896-1899
{
	SetAsCurrentContext();
	mRenderDevice->SetGlobalAmbient(inColor);
}
void Graphics3D::SetMaterialAmbient(const Color& inColor, int inVertexColorComponent) //1901-1904
{
	SetAsCurrentContext();
	mRenderDevice->SetMaterialAmbient(inColor, inVertexColorComponent);
}
void Graphics3D::SetMaterialDiffuse(const Color& inColor, int inVertexColorComponent) //1906-1909
{
	SetAsCurrentContext();
	mRenderDevice->SetMaterialDiffuse(inColor, inVertexColorComponent);
}
void Graphics3D::SetMaterialSpecular(const Color& inColor, int inVertexColorComponent, float inPower) //1911-1914
{
	SetAsCurrentContext();
	mRenderDevice->SetMaterialSpecular(inColor, inVertexColorComponent, inPower);
}
void Graphics3D::SetMaterialEmissive(const Color& inColor, int inVertexColorComponent) //1916-1919
{
	SetAsCurrentContext();
	mRenderDevice->SetMaterialEmissive(inColor, inVertexColorComponent);
}

void Graphics3D::SetWorldTransform(const SexyMatrix4* inMatrix) //1922-1925
{
	SetAsCurrentContext();
	mRenderDevice->SetWorldTransform(inMatrix);
}
void Graphics3D::SetViewTransform(const SexyMatrix4* inMatrix) //1927-1930
{
	SetAsCurrentContext();
	mRenderDevice->SetViewTransform(inMatrix);
}
void Graphics3D::SetProjectionTransform(const SexyMatrix4* inMatrix) //1932-1935
{
	SetAsCurrentContext();
	mRenderDevice->SetProjectionTransform(inMatrix);
}
void Graphics3D::SetTextureTransform(int inTextureIndex, const SexyMatrix4* inMatrix, int inNumDimensions) //1937-1940
{
	SetAsCurrentContext();
	mRenderDevice->SetTextureTransform(inTextureIndex, inMatrix, inNumDimensions);
}

bool Graphics3D::SetTexture(int inTextureIndex, Image* inImage) //1943-1946
{
	SetAsCurrentContext();
	mRenderDevice->SetTexture(inTextureIndex, inImage);
}
void Graphics3D::SetTextureWrap(int inTextureIndex, bool inWrap) //1948-1951
{
	SetAsCurrentContext();
	mRenderDevice->SetTextureWrap(inTextureIndex, inWrap, inWrap);
}

void Graphics3D::SetTextureWrap(int inTextureIndex, bool inWrapU, bool inWrapV) //1954-1957
{
	SetAsCurrentContext();
	mRenderDevice->SetTextureWrap(inTextureIndex, inWrapU, inWrapV);
}

void Graphics3D::SetTextureLinearFilter(int inTextureIndex, bool inLinear) //1960-1963
{
	SetAsCurrentContext();
	mRenderDevice->SetTextureLinearFilter(inTextureIndex, inLinear);
}
void Graphics3D::SetTextureCoordSource(int inTextureIndex, int inUVComponent, ETexCoordGen inTexGen) //1965-1968
{
	SetAsCurrentContext();
	mRenderDevice->SetTextureCoordSource(inTextureIndex, inUVComponent, inTexGen);
}
void Graphics3D::SetTextureFactor(int inTextureFactor) //1970-1973
{
	SetAsCurrentContext();
	mRenderDevice->SetTextureFactor(inTextureFactor);
}

RenderEffect* Graphics3D::GetEffect(RenderEffectDefinition* inDefinition) //1976-1979
{
	return mRenderDevice->GetEffect(inDefinition);
}

void Graphics3D::SetViewport(int theX, int theY, int theWidth, int theHeight, float theMinZ, float theMaxZ) //1982-1985
{
	SetAsCurrentContext();
	mRenderDevice->SetViewport(mGraphics->mTransX + theX, mGraphics->mTransY + theY, theWidth, theHeight, theMinZ, theMaxZ);
}

void Graphics3D::SetMasking(EMaskMode inMaskMode, int inAlphaRef, float inFrontDepth, float inBackDepth) //1988-2028
{
	SetAsCurrentContext();
	switch (inMaskMode)
	{
		case MASKMODE_NONE:
			mRenderDevice->SetAlphaTest(COMPARE_ALWAYS, 0);
			mRenderDevice->SetDepthState(COMPARE_ALWAYS, false);
			mRenderDevice->SetBltDepth(inBackDepth);
			mRenderDevice->SetBlend(BLEND_DEFAULT, BLEND_DEFAULT);
			break;
		case MASKMODE_WRITE_MASKONLY:
		case MASKMODE_WRITE_MASKANDCOLOR:
			mRenderDevice->SetAlphaTest(COMPARE_GREATER, inAlphaRef);
			mRenderDevice->SetDepthState(COMPARE_ALWAYS, true);
			mRenderDevice->SetBltDepth(inFrontDepth);
			if (inMaskMode == MASKMODE_WRITE_MASKONLY)
				mRenderDevice->SetBlend(BLEND_ZERO, BLEND_ONE);
			else
				mRenderDevice->SetBlend(BLEND_DEFAULT, BLEND_DEFAULT);
			break;
		case MASKMODE_TEST_INSIDE:
		case MASKMODE_TEST_OUTSIDE:
			mRenderDevice->SetAlphaTest(COMPARE_GREATER, 0);
			mRenderDevice->SetDepthState((inMaskMode == MASKMODE_TEST_OUTSIDE) ? COMPARE_LESSEQUAL : COMPARE_GREATEREQUAL, false);
			mRenderDevice->SetBltDepth(inBackDepth);
			mRenderDevice->SetBlend(BLEND_DEFAULT, BLEND_DEFAULT);
			break;
		default:
			DBG_ASSERTE(false && "Invalid mask mode"); //2024 | 2052 in BejLiveWin8
	}
}

void Graphics3D::ClearMask() //2031-2034
{
	SetAsCurrentContext();
	mRenderDevice->ClearDepthBuffer();
}