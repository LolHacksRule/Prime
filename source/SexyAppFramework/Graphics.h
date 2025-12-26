#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include "Common.h"
#include "Rect.h"
#include "Color.h"
#include "Image.h"
#include "TriVertex.h"
#include "RenderDevice.h"

namespace Sexy
{

class Font;
class SexyMatrix3;
class SexyMatrix4;
class SexyVector3;
class Transform;

class HRenderContext
{
private:
	union
	{
		DWORD mHandleDword;
		HANDLE mHandlePtr; //C++ only, prob a handle
	};
public:
	HRenderContext(HANDLE inHandlePtr) { mHandleDword = (DWORD)inHandlePtr; }; //Idk | 32
	HRenderContext(DWORD inHandleDword) { mHandleDword = 0; mHandleDword = inHandleDword; }; //33
	bool IsValid() const { return mHandleDword != 0; }; //34

	DWORD GetDword() const;
	void* GetPointer() const { return (HANDLE)mHandleDword; }; //37

	bool operator==(const HRenderContext& inContext) const {} //39
	bool operator!=(const HRenderContext& inContext) const;
};

class NullRenderDevice : public RenderDevice
{
public:
	//NullRenderDevice(); //?
	RenderDevice3D* Get3D(); //haha null
	HRenderContext CreateContext(Image* theDestImage, const HRenderContext& theSourceContext);
	void DeleteContext(const HRenderContext& theContext);
	void SetCurrentContext(const HRenderContext& theContext);
	HRenderContext GetCurrentContext() const;
	void PushState();
	void PopState();
	bool CanFillPoly();
	void ClearRect(const Rect& theRect);
	void FillRect(const Rect& theRect, const Color& theColor, int theDrawMode);
	void FillScanLinesWithCoverage(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode, const uchar* theCoverage, int theCoverX, int theCoverY, int theCoverWidth, int theCoverHeight);
	void FillPoly(const Point* theVertices, int theNumVertices, const Rect* theClipRect, const Color& theColor, int theDrawMode, int tx, int ty);
	void DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode, bool antiAlias = false);
	void Blt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode);
	void BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode);
	void BltRotated(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY);
	void BltMatrix(Image* theImage, float theX, float theY, const SexyMatrix3& theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect& theSrcRect, bool blend);
	void BltTriangles(Image* theImage, const SexyVertex2D* theVertices[][3], int theNumTriangles, const Color& theColor, int theDrawMode, float tx, float ty, bool blend, const Rect* theClipRect);
	void BltMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode);
	void BltStretched(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch, bool mirror);
};

const int MAX_TEMP_SPANS = 8192;

struct Edge
{
    double mX;
    double mDX;
    int i;
	double b;
};

class Graphics; //Huh

enum SEXY3DFORMAT
{
	SEXY3DFMT_UNKNOWN = 0,

	SEXT3DFMT_R8G8B8 = 20,
	SEXY3DFMT_A8R8G8B8 = 21,
	SEXY3DFMT_X8R8G8B8 = 22,
	SEXY3DFMT_R5G6B5 = 23,
	SEXY3DFMT_X1R5G5B5 = 24,
	SEXY3DFMT_A1R5G5B5 = 25,
	SEXY3DFMT_A4R4G4B4 = 26,
	SEXY3DFMT_R3G3B2 = 27,
	SEXY3DFMT_A8 = 28,
	SEXY3DFMT_A8R3G3B2 = 29,
	SEXY3DFMT_X4R4G4B4 = 30,
	SEXY3DFMT_A2B10G10R10 = 31,
	SEXY3DFMT_G16R16 = 34,

	SEXY3DFMT_A8P8 = 40,
	SEXY3DFMT_P8 = 41,

	SEXY3DFMT_L8 = 50,
	SEXY3DFMT_A8L8 = 51,
	SEXY3DFMT_A4L4 = 52,

	SEXY3DFMT_V8U8 = 60,
	SEXY3DFMT_L6V5U5 = 61,
	SEXY3DFMT_X8L8V8U8 = 62,
	SEXY3DFMT_Q8W8V8U8 = 63,
	SEXY3DFMT_V16U16 = 64,
	SEXY3DFMT_W11V11U10 = 65,
	SEXY3DFMT_A2W10V10U10 = 67,

	SEXY3DFMT_UYVY = MAKEFOURCC('U', 'Y', 'V', 'Y'),
	SEXY3DFMT_YUY2 = MAKEFOURCC('Y', 'U', 'Y', '2'),
	SEXY3DFMT_DXT1 = MAKEFOURCC('D', 'X', 'T', '1'),
	SEXY3DFMT_DXT2 = MAKEFOURCC('D', 'X', 'T', '2'),
	SEXY3DFMT_DXT3 = MAKEFOURCC('D', 'X', 'T', '3'),
	SEXY3DFMT_DXT4 = MAKEFOURCC('D', 'X', 'T', '4'),
	SEXY3DFMT_DXT5 = MAKEFOURCC('D', 'X', 'T', '5'),

	SEXY3DFMT_D16_LOCKABLE = 70,
	SEXY3DFMT_D32 = 71,
	SEXY3DFMT_D15S1 = 73,
	SEXY3DFMT_D24S8 = 75,
	SEXY3DFMT_D16 = 80,
	SEXY3DFMT_D24X8 = 77,
	SEXY3DFMT_D24X4S4 = 79,


	SEXY3DFMT_VERTEXDATA = 100,
	SEXY3DFMT_INDEX16 = 101,
	SEXY3DFMT_INDEX32 = 102,

	SEXY3DFMT_FORCE_DWORD = 0x7fffffff
};

class GraphicsState
{
public:
	static Image			mStaticImage;
	Image* mDestImage;
	float					mTransX;
	float					mTransY;
	float					mScaleX;
	float					mScaleY;
	float					mScaleOrigX;
	float					mScaleOrigY;
	Rect					mClipRect;
	ColorVector				mPushedColorVector;
	Color					mFinalColor;
	Color					mColor;
	Font* mFont;
	int						mDrawMode;
	bool					mColorizeImages;
	bool					mFastStretch;
	bool					mWriteColoredString;
	bool					mLinearBlend;
	bool					mIs3D;

public:
	void					CopyStateFrom(const GraphicsState* theState);
};

typedef std::list<GraphicsState> GraphicsStateList;

class Graphics : public GraphicsState
{
public:	
	enum
	{
		DRAWMODE_NORMAL,
		DRAWMODE_ADDITIVE
	};
protected:
	RenderDevice*			mRenderDevice;
	HRenderContext			mRenderContext;
	Graphics3D*				mGraphics3D;
public:
	Edge*					mPFActiveEdgeList;
	int						mPFNumActiveEdges;
	static const Point*		mPFPoints;
	int						mPFNumVertices;

	GraphicsStateList		mStateStack;

protected:	
	static int				PFCompareInd(const void* u, const void* v);
	static int				PFCompareActive(const void* u, const void* v);
	void					PFDelete(int i); 
	void					PFInsert(int i, int y);
	void					InitRenderInfo(Graphics const* theSourceGraphics);
	void					SetAsCurrentContext();
	void					CalcFinalColor();
	void					PushColorMult();
	void					PopColorMult();
	const Color&			GetImageColor();

	void					DrawImageTransformHelper(Image* theImage, const Transform &theTransform, const Rect &theSrcRect, float x, float y, bool useFloat);

public:
	Graphics(const Graphics& theGraphics);
	Graphics(Image* theDestImage = NULL);
	virtual ~Graphics();	

	void					PushState();
	void					PopState();

	Graphics*				Create();
	
	void					SetFont(Font* theFont);
	Font*					GetFont();

	void					SetColor(const Color& theColor);
	const Color&			GetColor();
	
	void					SetDrawMode(int theDrawMode);
	int						GetDrawMode();
	
	void					SetColorizeImages(bool colorizeImages);
	bool					GetColorizeImages();

	void					SetFastStretch(bool fastStretch);
	bool					GetFastStretch();

	void					SetLinearBlend(bool linear); // for DrawImageMatrix, DrawImageTransform, etc...
	bool					GetLinearBlend();

	void					FillRect(int theX, int theY, int theWidth, int theHeight);
	void					FillRect(const Rect& theRect);
	void					DrawRect(int theX, int theY, int theWidth, int theHeight);	
	void					DrawRect(const Rect& theRect);
	void					ClearRect(int theX, int theY, int theWidth, int theHeight);	
	void					ClearRect(const Rect& theRect);
	void					DrawString(const SexyString& theString, int theX, int theY);
	
private:
	bool					DrawLineClipHelper(double* theStartX, double* theStartY, double *theEndX, double* theEndY);
public:
	void					DrawLine(int theStartX, int theStartY, int theEndX, int theEndY);
	void					DrawLineAA(int theStartX, int theStartY, int theEndX, int theEndY);
	void					PolyFill(const Point *theVertexList, int theNumVertices, bool convex = false);
	void					PolyFillAA(const Point *theVertexList, int theNumVertices, bool convex = false);

	void					DrawImage(Image* theImage, int theX, int theY);
	void					DrawImage(Image* theImage, int theX, int theY, const Rect& theSrcRect);
	void					DrawImage(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect);
	void					DrawImage(Image* theImage, int theX, int theY, int theStretchedWidth, int theStretchedHeight);
	void					DrawImageF(Image* theImage, float theX, float theY);
	void					DrawImageF(Image* theImage, float theX, float theY, const Rect& theSrcRect);

	void					DrawImageMirror(Image* theImage, int theX, int theY, bool mirror = true);
	void					DrawImageMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, bool mirror = true);
	void					DrawImageMirror(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, bool mirror = true);

	void					DrawImageRotated(Image* theImage, int theX, int theY, double theRot, const Rect *theSrcRect = NULL);
	void					DrawImageRotated(Image* theImage, int theX, int theY, double theRot, int theRotCenterX, int theRotCenterY, const Rect *theSrcRect = NULL);
	void					DrawImageRotatedF(Image* theImage, float theX, float theY, double theRot, const Rect *theSrcRect = NULL);
	void					DrawImageRotatedF(Image* theImage, float theX, float theY, double theRot, float theRotCenterX, float theRotCenterY, const Rect *theSrcRect = NULL);

	void					DrawImageMatrix(Image* theImage, const SexyMatrix3 &theMatrix, float x = 0, float y = 0);
	void					DrawImageMatrix(Image* theImage, const SexyMatrix3 &theMatrix, const Rect &theSrcRect, float x = 0, float y = 0);
	void					DrawImageTransform(Image* theImage, const Transform &theTransform, float x = 0, float y = 0);
	void					DrawImageTransform(Image* theImage, const Transform &theTransform, const Rect &theSrcRect, float x = 0, float y = 0);
	void					DrawImageTransformF(Image* theImage, const Transform &theTransform, float x = 0, float y = 0);
	void					DrawImageTransformF(Image* theImage, const Transform &theTransform, const Rect &theSrcRect, float x = 0, float y = 0);

	void					DrawTriangleTex(Image *theTexture, const SexyVertex2D &v1, const SexyVertex2D &v2, const SexyVertex2D &v3);
	void					DrawTrianglesTex(Image *theTexture, const SexyVertex2D* theVertices[][3], int theNumTriangles);
	void					DrawTrianglesTex(Image *theTexture, const SexyVertex2D* theVertices[][3], int theNumTriangles, const Color& theColor, int theDrawMode, float tx = 0, float ty = 0, bool blend = true, Rect* theClipRect);
	void					DrawTrianglesTexStrip(Image* theTexture, const SexyVertex2D* theVertices, int theNumTriangles, const Color& theColor, int theDrawMode, float tx = 0, float ty = 0, bool blend = true);
	void					DrawTrianglesTexStrip(Image* theTexture, const SexyVertex2D* theVertices, int theNumTriangles);

	void					DrawImageCel(Image* theImageStrip, int theX, int theY, int theCel);
	void					DrawImageCel(Image* theImageStrip, const Rect& theDestRect, int theCel);
	void					DrawImageCel(Image* theImageStrip, int theX, int theY, int theCelCol, int theCelRow);
	void					DrawImageCel(Image* theImageStrip, const Rect& theDestRect, int theCelCol, int theCelRow);

	void					DrawImageAnim(Image* theImageAnim, int theX, int theY, int theTime);

	void					ClearClipRect();
	void					SetClipRect(int theX, int theY, int theWidth, int theHeight);
	void					SetClipRect(const Rect& theRect);
	void					ClipRect(int theX, int theY, int theWidth, int theHeight);
	void					ClipRect(const Rect& theRect);
	void					Translate(int theTransX, int theTransY);
	void					TranslateF(float theTransX, float theTransY);

	// In progress: Only affects DrawImage
	void					SetScale(float theScaleX, float theScaleY, float theOrigX, float theOrigY);

	int						StringWidth(const SexyString& theString);
	void					DrawImageBox(const Rect& theDest, Image* theComponentImage);
	void					DrawImageBox(const Rect& theSrc, const Rect& theDest, Image* theComponentImage);

	void					DrawImageBoxStretch(const Rect& theDest, Image* theComponentImage);
	void					DrawImageBoxStretch(const Rect& theSrc, const Rect& theDest, Image* theComponentImage);

	int						WriteString(const SexyString& theString, int theX, int theY, int theWidth = -1, int theJustification = 0, bool drawString = true, int theOffset = 0, int theLength = -1, int theOldColor = -1);
	int						WriteWordWrapped(const Rect& theRect, const SexyString& theLine, int theLineSpacing = -1, int theJustification = -1, int *theMaxWidth = NULL, int theMaxChars = -1, int* theLastWidth = NULL, int* theLineCount = NULL);
	int						DrawStringColor(const SexyString& theString, int theX, int theY, int theOldColor = -1); //works like DrawString but can have color tags like ^ff0000^.
	int						DrawStringWordWrapped(const SexyString& theLine, int theX, int theY, int theWrapWidth = 10000000, int theLineSpacing = -1, int theJustification = -1, int *theMaxWidth = NULL); //works like DrawString but also word wraps
	int						GetWordWrappedHeight(int theWidth, const SexyString& theLine, int theLineSpacing = -1, int *theMaxWidth = NULL);

	bool					Is3D() { return mIs3D; } //243
	Graphics3D*				Get3D();

	const Color& GetFinalColor() //148-153
	{
		if (mPushedColorVector.size() > 0)
			return mFinalColor;
		return mColor;
	}
};

class GraphicsAutoState
{
public:
	Graphics*				mG;

public:
	
	GraphicsAutoState(Graphics* theG) : mG(theG)
	{
		mG->PushState();
	}

	~GraphicsAutoState()
	{
		mG->PopState();
	}
};

class Graphics3D
{
public:
	enum EBlendMode
	{
		BLEND_ZERO,
		BLEND_ONE,
		BLEND_SRCCOLOR,
		BLEND_INVSRCCOLOR,
		BLEND_SRCALPHA,
		BLEND_INVSRCALPHA,
		BLEND_DESTCOLOR,
		BLEND_INVDESTCOLOR,
		BLEND_SRCALPHASAT,
		BLEND_DEFAULT,
	};
	enum ECompareFunc
	{
		COMPARE_NEVER = 1,
		COMPARE_LESS,
		COMPARE_EQUAL,
		COMPARE_LESSEQUAL,
		COMPARE_GREATER,
		COMPARE_NOTEQUAL,
		COMPARE_GREATEREQUAL,
		COMPARE_ALWAYS,
	};
	enum ETexCoordGen
	{
		TEXCOORDGEN_NONE,
		TEXCOORDGEN_CAMERASPACENORMAL,
		TEXCOORDGEN_CAMERASPACEPOSITION,
		TEXCOORDGEN_CAMERASPACEREFLECTIONVECTOR
	};
	enum EPrimitiveType
	{
		PT_PointList = 1,
		PT_LineList,
		PT_LineStrip,
		PT_TriangleList,
		PT_TriangleStrip,
		PT_TriangleFan
	};
	class LightColors
	{
	public:
		Color mDiffuse;
		Color mSpecular;
		Color mAmbient;
		float mAutoScale;
		LightColors() {} //329
	};
	enum EMaskMode
	{
		MASKMODE_NONE,
		MASKMODE_WRITE_MASKONLY,
		MASKMODE_WRITE_MASKANDCOLOR,
		MASKMODE_TEST_OUTSIDE,
		MASKMODE_TEST_INSIDE,
	};
	class Camera //Recovered vars from H5 (GameFramework.gfx.Camera) and XNA
	{
	protected:
		SexyCoords3 mCoords;
		float mZNear;
		float mZFar;
	public:
		Camera();
		SexyCoords3 GetCoords() const;
		void SetCoords(const SexyCoords3& inCoords);
		void GetViewMatrix(SexyMatrix4* outM) const;
		virtual void GetProjectionMatrix(SexyMatrix4* outM) const;
		virtual bool IsOrtho() const;
		virtual bool IsPerspective() const;
		//The reset aren't in XNA.
		virtual SexyVector3 EyeToScreen(const SexyVector3& inEyePos) const;
		SexyVector3 WorldToEye(const SexyVector3& inWorldPos) const;
		SexyVector3 WorldToScreen(const SexyVector3& inWorldPos) const;
		bool LookAt(const SexyVector3& inViewPos, const SexyVector3& inTargetPos, const SexyVector3& inUpVector);
		bool LookAt(const SexyVector3& inTargetPos, const SexyVector3& inUpVector);
	};
	class PerspectiveCamera : public Camera //Not in PVZF, recovered vars from H5 (GameFramework.gfx.PerspectiveCamera) and XNA
	{
	protected:
		SexyVector3 mProjS;
		float mProjT;
	public:
		//PerspectiveCamera(const PerspectiveCamera&);
		PerspectiveCamera(float inFovDegrees, float inAspectRatio, float inZNear = 1, float inZFar = 10000);
		PerspectiveCamera();
		void Init(float inFovDegrees, float inAspectRatio, float inZNear = 1, float inZFar = 10000);
		void GetProjectionMatrix(SexyMatrix4* outM) const;
		bool IsOrtho() const;
		bool IsPerspective() const;
		SexyVector3 EyeToScreen(const SexyVector3& inEyePos) const;
#ifdef _SEXYDECOMP_USE_LATEST_CODE
		SexyVector3 ScreenToEye(const SexyVector3& inScreenPos) const; //Not in H5
#endif
	};
	class OffCenterPerspectiveCamera : public Camera //Not in H5 or BejC and PVZF
	{
	protected:
		SexyVector3 mProjS;
		float mProjT, mLeft, mRight, mTop, mBottom;
	public:
		//OffCenterPerspectiveCamera(const OffCenterPerspectiveCamera&);
		OffCenterPerspectiveCamera(float inFovDegrees, float inAspectRatio, float inOffsetX, float inOffsetY, float inZNear = 1, float inZFar = 10000);
		OffCenterPerspectiveCamera();
		void Init(float inFovDegrees, float inAspectRatio, float inOffsetX, float inOffsetY, float inZNear = 1, float inZFar = 10000);
		void GetProjectionMatrix(SexyMatrix4* outM) const;
		bool IsOrtho() const;
		bool IsPerspective() const;
		SexyVector3 EyeToScreen(const SexyVector3& inEyePos) const;
#ifdef _SEXYDECOMP_USE_LATEST_CODE
		SexyVector3 ScreenToEye(const SexyVector3& inScreenPos) const; //Not in H5
#endif
	};
	class OrthoCamera : public Camera //Not in H5 or BejC and PVZF
	{
	protected:
		SexyVector3 mProjS;
		float mProjT, mWidth, mHeight;
	public:
		//OrthoCamera(const OrthoCamera&);
		OrthoCamera(float inWidth, float inHeight, float inZNear = 1, float inZFar = 10000);
		OrthoCamera();
		void Init(float inWidth, float inHeight, float inZNear = 1, float inZFar = 10000);
		void GetProjectionMatrix(SexyMatrix4* outM) const;
		bool IsOrtho() const;
		bool IsPerspective() const;
		SexyVector3 EyeToScreen(const SexyVector3& inEyePos) const;
#ifdef _SEXYDECOMP_USE_LATEST_CODE
		SexyVector3 ScreenToEye(const SexyVector3& inScreenPos) const; //Not in H5
#endif
	};
	class Spline //Not in H5
	{
		virtual SexyVector3 Evaluate(float inTime) const;
	};
	class CatmullRomSpline : public Spline //Not in H5
	{
	public:
		std::vector<SexyVector3> mPoints;
		CatmullRomSpline(const std::vector<SexyVector3>& inPoints);
		CatmullRomSpline(const CatmullRomSpline& inSpline);
		CatmullRomSpline();
		SexyVector3 Evaluate(float inTime) const;
	};
protected:
	Graphics* mGraphics;
	RenderDevice3D* mRenderDevice;
	HRenderContext mRenderContext;
	Graphics3D(Graphics* inGraphics, RenderDevice3D* inRenderDevice, const HRenderContext& inRenderContext);
	void SetAsCurrentContext();
public:
	Graphics* Get2D();
	RenderDevice3D* GetRenderDevice();
	bool SupportsPixelShaders();
	bool SupportsVertexShaders();
	bool SupportsCubeMaps();
	bool SupportsVolumeMaps();
	bool SupportsImageRenderTargets();
	ulong GetMaxTextureStages();
	void DrawPrimitiveEx(ulong theVertexFormat, EPrimitiveType thePrimitiveType, const SexyVertex* theVertices, int thePrimitiveCount, const Color& theColor, int theDrawMode, float tx, float ty, bool blend, ulong theFlags);
	void RenderMesh(Mesh* theMesh, const SexyMatrix4& theMatrix, const Color& theColor, bool doSetup); //C++ only
	void SetBltDepth(float inDepth);
	void PushTransform(const SexyMatrix3& theTransform, bool concatenate);
	void PopTransform(SexyMatrix3& theTransform);
	void PopTransform();
	void ClearColorBuffer(const Color& inColor);
	void ClearDepthBuffer();
	void SetDepthState(ECompareFunc inDepthTestFunc, bool inDepthWriteEnabled);
	void SetAlphaTest(ECompareFunc inAlphaTestFunc, int inRefAlpha);
	void SetWireframe(bool inWireframe);
	void SetBlend(EBlendMode inSrcBlend, EBlendMode inDestBlend);
	void SetBackfaceCulling(bool inCullClockwise, bool inCullCounterClockwise);
	void SetLightingEnabled(bool inLightingEnabled, bool inSetDefaultMaterialState);
	void SetLightEnabled(int inLightIndex, bool inEnabled);
	void SetPointLight(int inLightIndex, const SexyVector3& inPos, const LightColors& inColors, float inRange, const SexyVector3& inAttenuation);
	void SetDirectionalLight(int inLightIndex, const SexyVector3& inDir, const LightColors& inColors);
	void SetGlobalAmbient(const Color& inColor);
	void SetMaterialAmbient(const Color& inColor, int inVertexColorComponent);
	void SetMaterialDiffuse(const Color& inColor, int inVertexColorComponent);
	void SetMaterialSpecular(const Color& inColor, int inVertexColorComponent, float thePower);
	void SetMaterialEmissive(const Color& inColor, int inVertexColorComponent);
	void SetWorldTransform(const SexyMatrix4* inMatrix);
	void SetViewTransform(const SexyMatrix4* inMatrix);
	void SetProjectionTransform(const SexyMatrix4* inMatrix);
	void SetTextureTransform(int inTextureIndex, const SexyMatrix4* inMatrix, int inNumDimensions);
	bool SetTexture(int inTextureIndex, Image* inImage);
	void SetTextureWrap(int inTextureIndex, bool inWrapU, bool inWrapV);
	void SetTextureWrap(int inTextureIndex, bool inWrap);
	void SetTextureLinearFilter(int inTextureIndex, bool inLinear);
	void SetTextureCoordSource(int inTextureIndex, int inUVComponent, ETexCoordGen inTexGen);
	void SetTextureFactor(int inTextureFactor);
	void SetViewport(int theX, int theY, int theWidth, int theHeight, float theMinZ, float theMaxZ);
	RenderEffect* GetEffect(RenderEffectDefinition* inDefinition);
	void Set3DTransformState(const SexyCoords3&, const Camera&); //?
	void SetMasking(EMaskMode inMaskMode, int inAlphaRef, float inFrontDepth, float inBackDepth);
	void ClearMask();
};

}

#endif //__GRAPHICS_H__
