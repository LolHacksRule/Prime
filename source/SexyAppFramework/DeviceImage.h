#ifndef __DEVICEIMAGE_H__
#define __DEVICEIMAGE_H__

#include "MemoryImage.h"
#include "SexyAppBase.h"
#include "DeviceSurface.h"
#include "RenderDevice.h"

namespace Sexy
{
class IGraphicsDriver;
class SysFont;

class DeviceImage : public MemoryImage //Renamed to DeviceImage
{
protected:
	friend class			SysFont;//?
	void					DeleteAllNonSurfaceData();

public:
	IGraphicsDriver*		mDriver;
	bool					mSurfaceSet;
	bool					mNoLock;
	bool					mWantDeviceSurface;
	bool					mDrawToBits;

	int						mLockCount;
	_DEVICESURFACEDESC		mLockedSurfaceDesc;
	DeviceSurface*			mSurface;

private:
	void					Init();

public:
	bool					GenerateDeviceSurface();
	void					DeleteDeviceSurface();
	void			ReInit();

	void			BitsChanged();
	void			CommitBits();

	virtual void			NormalFillRect(const Rect& theRect, const Color& theColor);
	virtual void			AdditiveFillRect(const Rect& theRect, const Color& theColor);
	virtual void			NormalBlt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor);
	virtual void			AdditiveBlt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor);
	virtual void			NormalDrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor);
	virtual void			AdditiveDrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor);
	virtual void			NormalDrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor);
	virtual void			AdditiveDrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor);


	virtual void			NormalBltMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor);
	virtual void			AdditiveBltMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor);

	void			FillScanLinesWithCoverage(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode, const BYTE* theCoverage, int theCoverX, int theCoverY, int theCoverWidth, int theCoverHeight);

	DeviceImage();
	DeviceImage(IGraphicsDriver* theGraphicsDriver);
	DeviceImage(SexyAppBase* theApp);
	~DeviceImage();

	virtual bool			LockSurface();
	virtual bool			UnlockSurface();

	virtual void			SetSurface(void* theSurface);

	void			Create(int theWidth, int theHeight);
	ulong*			GetBits();

	void			FillRect(const Rect& theRect, const Color& theColor, int theDrawMode);
	void			DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode, bool antiAlias);
	void			Blt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode);
	void			BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode);
	void			BltRotated(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY);
	void			BltStretched(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch, bool mirror);
	void			BltMatrix(Image* theImage, float x, float y, const SexyMatrix3& theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect& theSrcRect, bool blend);
	void			BltTriangles(Image* theTexture, const SexyVertex2D theVertices[][3], int theNumTriangles, const Color& theColor, int theDrawMode, float tx, float ty, bool blend, const Rect* theClipRect);

	void			BltMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode);
#ifdef _SEXYDECOMP_USE_LATEST_CODE
	virtual void			StretchBltMirror(Image* theImage, const Rect& theDestRectOrig, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch); //StretchBltMirror is not present in PL_D.dll or XNA despite being in SAF 1.22.
#endif

	bool			Palletize();
	void			PurgeBits();
	void			DeleteNativeData();
	void			DeleteExtraBuffers();

	static bool				CheckCache(const std::string& theSrcFile, const std::string& theAltData);
	static bool				SetCacheUpToDate(const std::string& theSrcFile, const std::string& theAltData);
	static DeviceImage*		ReadFromCache(const std::string& theSrcFile, const std::string& theAltData);
	virtual void			WriteToCache(const std::string& theSrcFile, const std::string& theAltData);

};

}

#endif //__DEVICEIMAGE_H__