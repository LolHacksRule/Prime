#ifndef __MEMORYIMAGE_H__
#define __MEMORYIMAGE_H__

#include "Image.h"
#include "RenderDevice.h"
#include "IGraphicsDriver.h"

#define OPTIMIZE_SOFTWARE_DRAWING
#ifdef OPTIMIZE_SOFTWARE_DRAWING
extern bool gOptimizeSoftwareDrawing;
#endif

namespace Sexy
{

const uint32 MEMORYCHECK_ID = 0x4BEEFADE;

class NativeDisplay;
class SexyAppBase;

class MemoryImage : public Image, public RenderDevice
{
public:
	class TriRep
	{
	public:
		struct Tri
		{
			struct Point
			{
				float u;
				float v;
			};
			union
			{
				Point p[3];
				struct
				{
					Point p0, p1, p2;
				};				
			};
			Tri() {} //61
			Tri(float inU0, float inV0, float inU1, float inV1, float inU2, float inV2) //62-68
			{
				p0.u = inU0;
				p0.v = inV0;
				p1.u = inU1;
				p1.v = inV1;
				p2.u = inU2;
				p2.v = inV2;
			}
		};
		class Level
		{
		public:
			class Region
			{
			public:
				Rect mRect;
				std::vector<Tri> mTris;
			};
			int mDetailX;
			int mDetailY;
			int mRegionWidth;
			int mRegionHeight;
			std::vector<Region> mRegions;
			void GetRegionTris(std::vector<Tri>& outTris, MemoryImage* inImage, const Rect& inSrcRect, bool inAllowRotation);
		};
		std::vector<Level> mLevels;
		Level* GetMinLevel();
		Level* GetMaxLevel();
		Level* GetLevelForScreenSpaceUsage(float inUsageFrac, bool inAllowRotation);
	};
public:
	uint32*					mBits;
	int						mBitsChangedCount;

	uint32*					mColorTable;
	uchar*					mColorIndices;
	
	bool					mForcedMode;
	bool					mHasTrans;
	bool					mHasAlpha;
	bool					mIsVolatile;
	bool					mPurgeBits;
	bool					mWantPal;
	bool					mDither16;
	
	uint32*					mNativeAlphaData;
	uchar*					mRLAlphaData;
	uchar*					mRLAdditiveData;	

	bool					mBitsChanged;
	SexyAppBase*			mApp;
	TriRep					mNormalTriRep;
	TriRep					mAdditiveTriRep;
	
private:
	void					Init();

public:
	virtual void*			GetNativeAlphaData(NativeDisplay *theNative); //Was the param changed
	virtual uchar*			GetRLAlphaData();
	virtual uchar*			GetRLAdditiveData(NativeDisplay *theNative);
	virtual void			PurgeBits();
	virtual void			DeleteSWBuffers();
	virtual void			Delete3DBuffers();	
	virtual void			DeleteExtraBuffers();
	virtual void			ReInit();

	virtual void			BitsChanged();
	virtual void			CommitBits();
	
	virtual void			DeleteNativeData();	

	void					NormalBlt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor);
	void					AdditiveBlt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor);

	void					NormalDrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor);
	void					AdditiveDrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor);

	void					NormalDrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor);
	void					AdditiveDrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor);

	void					SlowStretchBlt(Image* theImage, const Rect& theDestRect, const FRect& theSrcRect, const Color& theColor, int theDrawMode);
	void					FastStretchBlt(Image* theImage, const Rect& theDestRect, const FRect& theSrcRect, const Color& theColor, int theDrawMode);
	bool					BltRotatedClipHelper(float &theX, float &theY, const Rect &theSrcRect, const Rect &theClipRect, double theRot, FRect &theDestRect, float theRotCenterX, float theRotCenterY);
	bool					StretchBltClipHelper(const Rect &theSrcRect, const Rect &theClipRect, const Rect &theDestRect, FRect &theSrcRectOut, Rect &theDestRectOut);
	bool					StretchBltMirrorClipHelper(const Rect &theSrcRect, const Rect &theClipRect, const Rect &theDestRect, FRect &theSrcRectOut, Rect &theDestRectOut);
	void					BltMatrixHelper(Image* theImage, float x, float y, const SexyMatrix3 &theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect &theSrcRect, void *theSurface, int theBytePitch, int thePixelFormat, bool blend);
	void					BltTrianglesTexHelper(Image *theTexture, const SexyVertex2D theVertices[][3], int theNumTriangles, const Rect &theClipRect, const Color &theColor, int theDrawMode, void *theSurface, int theBytePitch, int thePixelFormat, float tx, float ty, bool blend); //Uses SexyVertex2D

	virtual void			FillScanLinesWithCoverage(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode, const BYTE* theCoverage, int theCoverX, int theCoverY, int theCoverWidth, int theCoverHeight); //Uses RenderDevice


public:
	MemoryImage();
	MemoryImage(SexyAppBase* theApp);
	MemoryImage(const MemoryImage& theMemoryImage);	

	virtual ~MemoryImage();
	MemoryImage* AsMemoryImage() { return this; } //167

	virtual void			Clear();
	virtual void			SetBits(ulong* theBits, int theWidth, int theHeight, bool commitBits = true);
	virtual void			Create(int theWidth, int theHeight);
	virtual ulong*			GetBits();	
	
	void			FillRect(const Rect& theRect, const Color& theColor, int theDrawMode);
	void			ClearRect(const Rect& theRect);
	void			DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode, bool antiAlias = false);
	//DrawLineAA is gone

	void			Blt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode);
	void			BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect &theClipRect, const Color& theColor, int theDrawMode);
	void			BltRotated(Image* theImage, float theX, float theY, const Rect &theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY);
	void			BltStretched(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch, bool mirror); //Renamed
	void			BltMatrix(Image* theImage, float x, float y, const SexyMatrix3 &theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect &theSrcRect, bool blend);
	void			BltTriangles(Image* theImage, const SexyVertex2D theVertices[][3], int theNumTriangles, const Color& theColor, int theDrawMode, float tx, float ty, bool blend, const Rect* theClipRect); //Renamed

	virtual void			SetImageMode(bool hasTrans, bool hasAlpha);
	virtual void			SetVolatile(bool isVolatile);	

	virtual bool			Palletize();
	virtual bool			BuildTriRep(TriRep* inTriRep, bool inAdditive, bool inForceRebuild);
	virtual bool			BuildTriReps(bool inForceRebuild);



	RenderDevice3D* Get3D() { return NULL; } //195
	bool			CanFillPoly() { return false; } //196


	HRenderContext	CreateContext(Image* theDestImage, const HRenderContext& theSourceContext) { return HRenderContext(1); } //199
	void			DeleteContext(const HRenderContext& theContext) {} //200
	void			SetCurrentContext(const HRenderContext& theContext) {} //201
	HRenderContext	GetCurrentContext() const { return HRenderContext(1); } //202

	void			PushState() {} //204
	void			PopState() {} //205

	void			FillPoly(const Point* theVertices, int theNumVertices, const Rect* theClipRect, const Color& theColor, int theDrawMode, int tx, int ty) {} //207
	void			BltMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode) {} //208
};

};

//using namespace Sexy; //hack

class TriRepGenerator //C++ only
{
public:
	struct CoverageGrid
	{
	public:
		uchar* mGrid;
		int mGridWidth;
		int mGridHeight;

		CoverageGrid();
		~CoverageGrid();
		void InitFromImage(MemoryImage* inImage, Rect* inSrcRect, int inGridWidth, int inGridHeight, bool inAdditive, float inMaxFillPct);
	};
	struct SpanRow
	{
	public:
		struct Span
		{
		public:
			int mStartX;
			int mEndX;
			int mStartY;
			int mEndY;
			int mGroup;
			bool mMonotoneOpen;
			Span();
		};
	public:
		std::vector<Span> mSpans;
	};
	struct PointGroup
	{
	public:
		struct BarGroup
		{
		public:
			struct Bar
			{
			public:
				int mStartX;
				int mEndX;
				int mY;
				Bar(int inStartX, int inEndX, int inY);
			};
		
			std::vector<BarGroup> mBars;
			SpanRow::Span* mLastSpan;
			BarGroup();
		};
		struct Point
		{
		public:
			enum EPointType
			{
				PT_Default,
				PT_EarStart,
				PT_EarEnd,
				PT_ChainHead,
				PT_Removed,
				PT_PendingRemove,
			};
		public:
			int x;
			int y;
			EPointType mType;
			Point* mChainNext;
			Point* mChainPrev;
			Point* mChainHead;
		};
		std::vector<Point*> mPoints;
		PointGroup::Point mLeftChain;
		PointGroup::Point mRightChain;
		void InitChainHead(Point* inChainHead);
		PointGroup();
		~PointGroup();
		void Init();
		void AddChainPoint(PointGroup::Point* inChainHead, int inX, int inY, Point::EPointType inType);
		void AddLeftChainPoint(int inX, int inY, Point::EPointType inType);
		void AddRightChainPoint(int inX, int inY, Point::EPointType inType);
		void RemoveRedundantPoints();
	};
	TriRepGenerator();
	bool GenerateLevel(MemoryImage::TriRep::Level* inLevel, MemoryImage* inImage, int inGridWidth, int inGridHeight, bool inAdditive);
	struct SpanSet
	{
	public:
		std::vector<SpanRow> mRows;
		int mGroupCount;
		SpanSet();
		~SpanSet();
		void InitFromCoverageGrid(CoverageGrid* inGrid);
		void GroupMonotoneSpans();
		bool IsRangeOpen(int inStartX, int inEndX, int inY);
		void GeneratePointGroups(std::vector<PointGroup>& outPointGroups);
		void TriangulatePointGroups(std::vector<PointGroup>& inPointGroups, std::vector<MemoryImage::TriRep::Tri>& outTris);
		void ConvertToTris(std::vector<MemoryImage::TriRep::Tri>& outTris);
		static int GetWinding(int p0x, int p0y, int p1x, int p1y, int p2x, int p2y);
		static void AddTri(std::vector<MemoryImage::TriRep::Tri&> outTris, PointGroup::Point** inTri, int inWidth, int inHeight, int inGroup);
	};
};

#endif //__MEMORYIMAGE_H__