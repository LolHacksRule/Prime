#ifndef __D3DINTERFACE_H__
#define __D3DINTERFACE_H__

#include "../../../Common.h"
#include "../../../MemoryImage.h"
#include "../../../SexyMatrix.h"
#include "../../../RenderDevice.h" //Def needed
#include <assert.h>

namespace Sexy
{
class WindowsGraphicsDriver;
class SexyMatrix3;
class SexyVertex2D;
class D3DStateManager;
//class HRenderContext;
struct D3DVertexList;

class D3DDynamicVertexBuffer
{
protected:
	RenderSurface* mVB; //I'm assuming they do this so they can cast 7, 8 or 9
	ulong mVertCount;
	ulong mVertLimit;
public:
	D3DDynamicVertexBuffer(ulong inVertLimit);
	~D3DDynamicVertexBuffer();
	bool InitBuffer(D3DInterface* inInterface);
	void CleanupBuffer();
	ulong Write(D3DInterface* inInterface, ulong inVertCount, const void* inVerts); //?
	int ApplyToDevice(D3DInterface* inInterface, ulong inStreamIndex); //?
	ulong GetVertLimit() { return mVertLimit; } //133
};

class D3DRenderEffectDefInfo
{
public:
	class Annotation
	{
	public:
		enum EAnnotType
		{
			AT_Bool,
			AT_Int,
			AT_Float,
			AT_Vector,
			AT_String,
		};
		EAnnotType mType;
		ByteVector mData;
		std::string mName;
		bool GetBool() const;
		int GetInt() const;
		float GetFloat() const;
		void GetVector(float&, float&, float&, float&) const;
		std::string GetString() const;
		//Annotation();
		//~Annotation();
	};
	class ShaderConstant
	{
	public:
		enum EConstantType
		{
			CT_Float,
			CT_Sampler,
		};
		enum EStandardConstantSemantic
		{
			SCS_None,
			SCS_World,
			SCS_View,
			SCS_Proj = 0x0004,
			SCS_Transpose = 0x0008,
			SCS_Texture = 0x0010,
			SCS_LightAmbient = 0x0020,
			SCS_LightAttenuation,
			SCS_LightDiffuse,
			SCS_LightSpecular,
			SCS_LightDirection,
			SCS_LightPosition,
			SCS_LightMisc,
			SCS_MaterialAmbient,
			SCS_MaterialDiffuse,
			SCS_MaterialSpecular,
			SCS_MaterialEmissive,
			SCS_MaterialPower,
			SCS_GlobalAmbient,
			SCS_TextureFactor,
			SCS_LIGHTFIRST = 0x0020,
			SCS_LIGHTLAST = 0x0026,
			SCS_MATERIALFIRST,
			SCS_MATERIALLAST = 0x002b,
		};
		EConstantType mType;
		std::string mConstantName;
		std::string mSemantic;
		EStandardConstantSemantic mStandardSemantic;
		int mRegisterIndex;
		int mRegisterCount;
		//ShaderConstant();
		//~ShaderConstant();
	};
	class Shader
	{
	public:
		ByteVector mCode;
		std::vector<ShaderConstant> mConstants;
		//Shader();
		//~Shader();
	};
	class StateCommand
	{
	public:
		enum ECommandType
		{
			CMD_None,
			CMD_SetRenderState,
			CMD_SetSamplerState,
			CMD_SetTextureStageState,
		};
		ECommandType mType;
		int mState;
		int mSamplerOrTextureStage;
		int mValue;
	};
	class Pass
	{
	public:
		std::string mPassName;
		std::vector<Annotation> mAnnotations;
		std::vector<StateCommand> mStateCommands;
		Shader mVertexShader;
		Shader mPixelShader;
		//Pass();
		//~Pass();
	};
	class Technique
	{
	public:
		std::string mTechniqueName;
		std::vector<Annotation> mAnnotations;
		std::vector<Pass> mPasses;
		//Technique();
		//~Technique();
	};
public:
	RenderEffectDefinition* mDefinition;
	std::vector<Technique> mTechniques;
	bool Build(RenderEffectDefinition* inDefinition);
	//D3DRenderEffectDefInfo();
	//~D3DRenderEffectDefInfo();
};

class D3DRenderEffect : public RenderEffect
{
public:
	class Technique
	{
	public:
		RenderEffect* mEffect;
		D3DInterface* mInterface;
		D3DRenderEffectDefInfo::Technique* mDefinition;
		Sexy::D3DRenderEffect::ParamCollection* mParams;
		std::vector<Sexy::D3DRenderEffect::Pass*> mPasses;
		Technique* mValidTechnique;
		bool mValidated;
		std::string mCompatFallback;
		Technique(RenderEffect* inEffect, D3DInterface* inInterface, D3DRenderEffectDefInfo::Technique* inDefinition, ulong inIndex, Sexy::D3DRenderEffect::ParamCollection* inParams);
		~Technique();
		RenderEffect* GetEffect();
		void ApplyToDevice();
		Technique* GetValidTechnique(Sexy::D3DRenderEffect::TechniqueNameMap& inTechniqueNameMap); //?
		std::string GetName();
		int Begin();
		void BeginPass(int inPass);
		void EndPass(int inPass);
		void End();
		bool PassUsesVertexShader(int inPass);
		bool PassUsesPixelShader(int inPass);
		void ParametersChanged();
	};
	typedef std::map<std::string, Technique*> TechniqueNameMap;
	class ParamData
	{
	public:
		std::vector<float> mFloatData;
		void SetValue(const float* inFloatData, ulong inFloatCount);
		//ParamData();
		//~ParamData();
	};
	class ParamCollection
	{
	public:
		typedef std::map<std::string, ParamData> ParamMap;
		ParamMap mParamMap;
		ParamData* GetParamNamed(const std::string& inName, bool inAllowCreate);
		//ParamCollection();
		//~ParamCollection();
	};
	class Pass
	{
	public:
		RenderEffect* mEffect;
		D3DInterface* mInterface;
		D3DRenderEffectDefInfo::Pass* mDefinition;
		IUnknown* mVertexShader;
		IUnknown* mPixelShader;
		std::string mTextureRemapStr;
		bool mInProgress;
		Pass(RenderEffect* inEffect, D3DInterface* inInterface, D3DRenderEffectDefInfo::Pass* inDefinition);
		~Pass();
		void ApplyToDevice(ParamCollection* inParams, bool inApplyParamsOnly);
		static ParamData* MakeTempParamForSemantic(ParamData* inParam, D3DInterface* inInterface, ulong inSemantic, ulong inDesiredRegisterCount);
	};
	D3DInterface* mInterface;
	D3DRenderEffectDefInfo* mDefInfo;
	ParamCollection mParams;
	std::vector<Technique*> mTechniques;
	TechniqueNameMap mTechniqueNameMap;
	Technique* mCurrentTechnique;
	int mBeginPassRefCount;
	D3DRenderEffect(D3DInterface* inInterface, D3DRenderEffectDefInfo* inDefInfo);
	~D3DRenderEffect();
	RenderDevice3D* GetDevice();
	RenderEffectDefinition* GetDefinition();
	void SetParameter(const std::string& inParamName, const float* inFloatData, ulong inFloatCount);
	void GetParameterBySemantic(ulong inSemantic, float* outFloatData, ulong inMaxFloatCount);
	void SetCurrentTechnique(const std::string& inName, bool inCheckValid);
	std::string GetCurrentTechniqueName();
	int Begin(const HANDLE& outRunHandle, const HRenderContext& inRenderContext);
	void BeginPass(const HANDLE& inRunHandle, int inPass);
	void EndPass(const HANDLE& inRunHandle, int inPass);
	void End(const HANDLE& inRunHandle);
	bool PassUsesVertexShader(int inPass);
	bool PassUsesPixelShader(int inPass);
};

class D3DTextureData
{
public:
	typedef std::vector<D3DTextureDataPiece> TextureVector;
	D3DInterface* mInterface;

	TextureVector mTextures;

	int mPaletteIndex;
	int mWidth, mHeight;
	int mTexVecWidth, mTexVecHeight;
	int mTexPieceWidth, mTexPieceHeight;
	int mBitsChangedCount;
	int mTexMemSize;
	int mTexMemOriginalSize;
	DWORD mTexMemFlushRevision;
	float mMaxTotalU, mMaxTotalV;
	PixelFormat mPixelFormat;
	DWORD mImageFlags;

	D3DTextureData(D3DInterface* theInterface);
	~D3DTextureData();

	void ReleaseTextures();

	void CreateTextureDimensions(MemoryImage* theImage); //WIP help
	void CreateTextures(MemoryImage* theImage, D3DInterface* theInterface);
	void CheckCreateTextures(MemoryImage* theImage, D3DInterface* theInterface);
	IUnknown* GetTexture(int x, int y, int &width, int &height, float &u1, float &v1, float &u2, float &v2); //Maybe fix?
	IUnknown* GetTextureF(float x, float y, float &width, float &height, float &u1, float &v1, float &u2, float &v2); //Maybe fix?

	IUnknown* GetCubeTexture(); //?
	IUnknown* GetVolumeTexture(); //?

	void Blt(D3DInterface* theInterface, MemoryImage* theImage, int theDrawMode, float theX, float theY, const Rect& theSrcRect, const Color& theColor);// FIX
	void BltTransformed(D3DInterface* theInterface, MemoryImage* theImage, int theDrawMode, const SexyMatrix3& theTrans, const Rect& theSrcRect, const Color& theColor, const Rect* theClipRect = NULL, float theX = 0, float theY = 0, bool center = false); //FIX
	void BltTriangles(D3DInterface* theInterface, const SexyVertex2D theVertices[][3], int theNumTriangles, DWORD theColor, float tx, float ty, const Rect* theClipRect);
};

class D3DMeshPiece : public MeshPiece
{
public:
	ulong mSexyVF;
	int mVertexSize;
	int mVertexBufferCount;
	int mIndexBufferCount;
	void* mVertexData;
	void* mIndexData;
	IUnknown* mVertexBuffer;
	IUnknown* mIndexBuffer;
	D3DMeshPiece();
	virtual ~D3DMeshPiece();
};

struct D3DModelVertex
{
	float mX;
	float mY;
	float mZ;
	float mNormalX;
	float mNormalY;
	float mNormalZ;
	DWORD mColor;
	float mU1;
	float mV1;
	float mU2;
	float mV2;
};

//ImageFlags was moved to Image.h

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct D3DTextureDataPiece
{
	IUnknown* mTexture;
	IUnknown* mCubeTexture;
	IUnknown* mVolumeTexture;
	int mWidth,mHeight;
};

class D3DContext
{
public:
	Image* mDestImage;
	RenderStateManager::Context mStateContext;
	bool mInitialized;
	D3DContext* mParent;
	std::vector<D3DContext*> mChildren;
	D3DContext(Image* inImage);
	D3DContext(const D3DContext& inContext);
	~D3DContext();
};


struct _D3DTLVERTEX : D3DTLVERTEX {}; //Really stupid
//struct _DEVICEPIXELFORMAT : DDPIXELFORMAT {}; //Really stupid
//struct _DEVICESURFACEDESC : DDSURFACEDESC {}; //also really stupid, 

//PixelFormat moved to RenderDevice.h

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class D3DInterface : public RenderDevice3D
{
public:
	enum
	{
		DEFAULT_VERTEX_FVF = 452,
		DEFAULT_VERTEX_SIZE = 32,
		MAX_TEXTURE_SIZE = 4096
	};

	typedef std::list<SexyMatrix3> TransformStack;
	typedef std::set<MemoryImage*> ImageSet;
	typedef std::vector<int> IntVector; //How many omg

	static std::string		sErrorString;

	static int				sMinTextureWidth;
	static int				sMaxTextureWidth;
	static int				sMinTextureHeight;
	static int				sMaxTextureHeight;
	static int				sMaxTextureAspectRatio;
	static ulong			sSupportedTextureFormats;
	static ulong			sSupportedScreenFormats;
	static bool				sTextureSizeMustBePow2;
	static bool				sCanStretchRectFromTextures;

	TransformStack mTransformStack;

	FBltFilter				mBltFilter;
	void*					mBltFilterContext;
	FDrawPrimFilter			mDrawPrimFilter;//?
	void*					mDrawPrimFilterContext;
	HRenderContext			mCurrentContext;
	typedef std::map<RenderEffectDefinition*, D3DRenderEffectDefInfo*> RenderEffectDefInfoMap;
	RenderEffectDefInfoMap	mRenderEffectDefInfo;
	typedef std::map<RenderEffectDefinition*, D3DRenderEffect*> RenderEffectMap;
	RenderEffectMap			mRenderEffects;
	D3DStateManager*		mStateMgr;

	HWND					mHWnd;
	int						mWidth;
	int						mHeight;

	bool					mSceneBegun;
	bool					mIsWindowed;
	int						mFullscreenBits;

	float					mFov;
	float					mNearPlane;
	float					mFarPlane;
	SEXY3DFORMAT			mDisplayFormat;
	bool					mNeedClearZBuffer;
	bool					mNeedEvictManagedResources;
	D3DDynamicVertexBuffer* mDynVB;
	RenderSurface*				mBackBufferSurface; //Prob best to keep as unknown to not break DX9 and 8
	RenderSurface*				mTempRenderTargetSurface; //Prob best to keep as unknown to not break DX9 and 8
	RenderSurface*				mCurRenderTargetSurface; //Prob best to keep as unknown to not break DX9 and 8
	Image*					mCurRenderTargetImage;
	ImageSet				mImageSet;
	IntVector				mAvailPalettes;
	std::string				mAdapterInfoString;
	std::string				mD3DProductVersionString;
	DWORD					mTexMemUsageBytesAlloced;
	DWORD					mTexMemUsageBytesOriginal;
	DWORD					mTexMemUsageBytesCurFrame;
	DWORD					mTexMemUsageFlushRevision;
	_D3DTLVERTEX*			mBatchedTriangleBuffer;
	DWORD					mBatchedTriangleIndex;
	DWORD					mBatchedTriangleSize;
	WindowsGraphicsDriver*	mGraphicsDriver;

	struct 
	{
		ulong mCalls;
		ulong mPrims;
	} mDrawPrimMtx;


public:
	D3DInterface();
	virtual ~D3DInterface();

	virtual bool			InitFromGraphicsDriver(WindowsGraphicsDriver* theDriver, IGraphicsDriver::EResult* outErrorResult, bool preTestOnly, bool* outIsRecommended) = 0;
	RenderDevice3D*	Get3D() { return this; } //240
	bool			CanFillPoly() { return true; } //241
	HRenderContext	CreateContext(Image* theDestImage, const HRenderContext& theSourceContext);
	void			DeleteContext(const HRenderContext& theContext);
	void			SetCurrentContext(const HRenderContext& theContext);
	HRenderContext	GetCurrentContext() const;
	void			PushState();
	void			PopState();

	void			ClearRect(const Rect& theRect);
	void			FillRect(const Rect& theRect, const Color& theColor, int theDrawMode);
	

	void			FillScanLinesWithCoverage(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode, const BYTE* theCoverage, int theCoverX, int theCoverY, int theCoverWidth, int theCoverHeight) //254-274, dummied in XNA, correct? Why is this here
	{
		int l = theSpans[0].mX, t = theSpans[0].mY;
		int r = l + theSpans[0].mWidth, b = t;
		for (int i = 1; i < theSpanCount; ++i)
		{
			l = min(theSpans[i].mX, l);
			r = max(theSpans[i].mX + theSpans[i].mWidth - 1, r);
			t = min(theSpans[i].mY, t);
			b = max(theSpans[i].mY, b);
		}
		for (int i = 0; i < theSpanCount; ++i)
		{
			theSpans[i].mX -= l;
			theSpans[i].mY -= t;
		}

		MemoryImage aTempImage;
		aTempImage.Create(r-l+1, b-t+1);
		aTempImage.FillScanLinesWithCoverage(theSpans, theSpanCount, theColor, theDrawMode, theCoverage, theCoverX - l, theCoverY - t, theCoverWidth, theCoverHeight);
		Blt(&aTempImage, l, t, Rect(0, 0, r-l+1, b-t+1), Color::White, theDrawMode);
		return;
	}
	void			FillPoly(const Point theVertices[], int theNumVertices, const Rect* theClipRect, const Color& theColor, int theDrawMode, int tx, int ty);

	void					DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode, bool antiAlias = false);

	void					Blt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode) //278-280
	{
		BltNoClipF(theImage, theY, theX, theSrcRect, theColor, theDrawMode);
	}

	void			BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode) // 282-294
	{
		FRect aClipRect(theClipRect.mX, theClipRect.mY, theClipRect.mWidth, theClipRect.mHeight);
		FRect aDestRect(theX, theY, theSrcRect.mWidth, theSrcRect.mHeight);
		FRect anIntersect = aDestRect.Intersection(aClipRect);
		if (aDestRect.mWidth == anIntersect.mWidth && aDestRect.mHeight == anIntersect.mHeight)
			BltNoClipF(theImage, theX, theY, theSrcRect, theColor, theDrawMode, 1);
		else if (0.0 != anIntersect.mWidth && 0.0 != anIntersect.mHeight)
			BltClipF(theImage, theX, theY, theSrcRect, theClipRect, theColor, theDrawMode);
	}

	void					BltRotated(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY) // 296-304
	{
		SexyTransform2D aTransform;

		aTransform.Translate(-theRotCenterX, -theRotCenterY);
		aTransform.RotateRad(theRot);
		aTransform.Translate(theX + theRotCenterX, theY + theRotCenterY);

		BltTransformed(theImage, theClipRect, theColor, theDrawMode, theSrcRect, aTransform, true, 0, 0, false);
	}

	void			BltMatrix(Image* theImage, float x, float y, const SexyMatrix3& theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect& theSrcRect, bool blend) //306-308
	{
		BltTransformed(theImage, theClipRect, theColor, theDrawMode, theSrcRect, theMatrix, blend, x, y, true);
	}
	void			BltTriangles(Image* theImage, const SexyVertex2D theVertices[][3], int theNumTriangles, const Color& theColor, int theDrawMode, float tx = 0, float ty = 0, bool blend, const Rect* theClipRect);
	void					BltMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode) //311-319
	{
		SexyTransform2D aTransform;

		aTransform.Translate(-theSrcRect.mWidth, 0);
		aTransform.Scale(-1, 1);
		aTransform.Translate(theX, theY);

		BltTransformed(theImage, 0, theColor, theDrawMode, theSrcRect, aTransform, false); //?
	}
	void			BltStretched(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch, bool mirror = false) //321-336
	{
		float xScale = (double)theDestRect.mWidth / theSrcRect.mWidth;
		float yScale = (double)theDestRect.mHeight / theSrcRect.mHeight;

		SexyTransform2D aTransform;
		if (mirror)
		{
			aTransform.Translate(-theSrcRect.mWidth, 0);
			aTransform.Scale(-xScale, yScale);
		}
		else
			aTransform.Scale(xScale, yScale);

		aTransform.Translate(theDestRect.mX, theDestRect.mY);
		BltTransformed(theImage, theClipRect, theColor, theDrawMode, theSrcRect, aTransform, !fastStretch);
	}

	virtual void			Cleanup();

	bool			SceneBegun();
	bool			CreateImageRenderData(MemoryImage* inImage);
	void			RemoveImageRenderData(MemoryImage* inImage);
	bool			RecoverImageBitsFromRenderData(MemoryImage* inImage);
	int				GetTextureMemorySize(MemoryImage* theImage);
	PixelFormat		GetTextureFormat(MemoryImage* theImage);
	virtual RenderSurface*		GetBackBufferSurface();
	virtual bool			SetRenderTargetSurface(RenderSurface* inSurface);
	virtual bool			SetRenderTarget(Image* theImage);
	Image*			SwapScreenImage(DeviceImage*& ioSrcImage, RenderSurface*& ioSrcSurface);

	virtual RenderSurface* CreateSurface(int inWidth, int inHeight, bool inRenderTarget, bool inTexture) = 0;
	virtual bool			CanBltSurface(bool srcSurfaceIsTextures) = 0;
	virtual void			BltSurface(RenderSurface* theSurface, const Rect& theDest, const Rect& theSrc) = 0;

	void			DrawPrimitiveEx(ulong theVertexFormat, Graphics3D::EPrimitiveType thePrimitiveType, const SexyVertex* theVertices, int thePrimitiveCount, const Color& theColor, int theDrawMode, float tx, float ty, bool blend, ulong theFlags);
	void			SetBltDepth(float inDepth);

	void			PushTransform(const SexyMatrix3& theTransform, bool concatenate = true);
	void			PopTransform();
	void			PopTransform(SexyMatrix3& theTransform);

	void			SetDepthState(Graphics3D::ECompareFunc inDepthTestFunc, bool inDepthWriteEnabled) //374-378
	{
		SetRenderState(D3DRENDERSTATE_ZENABLE, (inDepthWriteEnabled || inDepthTestFunc != Graphics3D::COMPARE_ALWAYS) ? true : false);
		SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, inDepthWriteEnabled);
		SetRenderState(D3DRENDERSTATE_ZFUNC, inDepthTestFunc);
	}

	void			SetAlphaTest(Graphics3D::ECompareFunc inAlphaTestFunc, int inRefAlpha) //380-384
	{
		SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, inAlphaTestFunc != D3DRENDERSTATE_FILLMODE);
		SetRenderState(D3DRENDERSTATE_ALPHAFUNC, inAlphaTestFunc);
		SetRenderState(D3DRENDERSTATE_ALPHAREF, inRefAlpha);
	}

	void			SetWireframe(bool inWireframe) //386-388
	{
		SetRenderState(D3DRENDERSTATE_FILLMODE, 3 - inWireframe);
	}

	void			SetBlend(Graphics3D::EBlendMode inSrcBlend, Graphics3D::EBlendMode inDestBlend);

	void			SetBackfaceCulling(bool inCullClockwise, bool inCullCounterClockwise) //391-407
	{
		if (inCullClockwise)
		{
			if (inCullCounterClockwise) //Did this to restore assert integrity
			{
				assert(false && "SetBackfaceCulling called with both windings culled; was this deliberate?"); //396
			}
			SetRenderState(D3DRENDERSTATE_CULLMODE, 2);
		}
		else
		{
			SetRenderState(D3DRENDERSTATE_CULLMODE, inCullCounterClockwise ? 3 : 1);
		}
	}

	void			SetLightingEnabled(bool inLightingEnabled) //410-412
	{
		SetRenderState(D3DRENDERSTATE_LIGHTING, inLightingEnabled);
	}
	void			SetLightEnabled(int inLightIndex, bool inEnabled);
	void			SetPointLight(int inLightIndex, const SexyVector3& inPos, const Graphics3D::LightColors& inColors, float inRange, const SexyVector3& inAttenuation);
	void			SetDirectionalLight(int inLightIndex, const SexyVector3& inDir, const Graphics3D::LightColors& inColors);
	void			SetGlobalAmbient(const Color& inColor) //417-419
	{
		SetRenderState(D3DRENDERSTATE_AMBIENT, inColor.ToInt());
	}

	void			SetMaterialAmbient(const Color& inColor, int inVertexColorComponent);
	void			SetMaterialDiffuse(const Color& inColor, int inVertexColorComponent);
	void			SetMaterialSpecular(const Color& inColor, int inVertexColorComponent, float inPower);
	void			SetMaterialEmissive(const Color& inColor, int inVertexColorComponent);
	void			SetWorldTransform(const SexyMatrix4* inMatrix) //426-434
	{
		SexyMatrix4 aTempMat;
		if (inMatrix)
		{
			aTempMat.LoadIdentity();
			inMatrix = &aTempMat;
		}
		SetTransform(256, inMatrix);
	}

	void			SetViewTransform(const SexyMatrix4* inMatrix) //436-444
	{
		SexyMatrix4 aTempMat;
		if (inMatrix)
		{
			aTempMat.LoadIdentity();
			inMatrix = &aTempMat;
		}
		SetTransform(D3DTRANSFORMSTATE_VIEW, inMatrix);
	}

	void			SetProjectionTransform(const SexyMatrix4* inMatrix) //446-454
	{
		SexyMatrix4 aTempMat;
		if (inMatrix)
		{
			aTempMat.LoadIdentity();
			inMatrix = &aTempMat;
		}
		SetTransform(D3DTRANSFORMSTATE_PROJECTION, inMatrix);
	}

	void			SetTextureTransform(int inTextureIndex, const SexyMatrix4* inMatrix, int inNumDimensions) //456-466
	{
		if (inMatrix && inNumDimensions > 0)
		{
			SetTransform(inTextureIndex + D3DTRANSFORMSTATE_TEXTURE0, inMatrix);
			SetTextureStageState(inTextureIndex, D3DTSS_TEXTURETRANSFORMFLAGS, inNumDimensions);
		}
		else
		{
			SetTextureStageState(inTextureIndex, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
		}
	}
	bool			SetTexture(int inTextureIndex, Image* inImage);
	void			SetTextureWrap(int inTextureIndex, bool inWrapU, bool inWrapV);
	void			SetTextureLinearFilter(int inTextureIndex, bool inLinear);
	void			SetTextureCoordSource(int inTextureIndex, int inUVComponent, Graphics3D::ETexCoordGen inTexGen);
	void			SetTextureFactor(int inTextureFactor) //473-475
	{
		SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, inTextureFactor);
	}
	void			SetViewport(int theX, int theY, int theWidth, int theHeight, float theMinZ, float theMaxZ);
	RenderEffect*	GetEffect(RenderEffectDefinition* inDefinition);
	void			SetBltFilter(FBltFilter inFilter, void* inContext) //482-485
	{
		mBltFilter = inFilter;
		mBltFilterContext = inContext;
	}
	void			SetDrawPrimFilter(FDrawPrimFilter inFilter, void* inContext) //487-490
	{
		mDrawPrimFilter = inFilter;
		mDrawPrimFilterContext = inContext;
	}
	bool			LoadMesh(Mesh* theMesh);
	void			RenderMesh(Mesh* theMesh, const SexyMatrix4& theMatrix, const Color& thecolor, bool doSetup);

protected:
	bool							PreDraw();
	void							SetDefaultState(Image* inImage, bool inIsInScene);
	void							SetupDrawMode(int theDrawMode);
	virtual void					BltClipF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode) //509-514
	{
		SexyTransform2D aTransform;
		aTransform.Translate(theX, theY);
		BltTransformed(theImage, theClipRect, theColor, theDrawMode, theSrcRect, aTransform, true);
	}
	virtual void					BltNoClipF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode, bool linearFilter = false);
	void							BltTransformed(Image* theImage, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect& theSrcRect, const SexyMatrix3& theTransform, bool linearFilter, float theX = 0, float theY = 0, bool center = false);
	void							BufferedDrawPrimitive(int thePrimType, ulong thePrimCount, const _D3DTLVERTEX* theVertices, int theVertexSize, ulong theVertexFormat);
	void							FlushBufferedTriangles();
	virtual void							DrawPrimitiveInternal(ulong inPrimType, ulong inPrimCount, const void* inVertData, ulong inVertStride, ulong inVertFormat) = 0;
	IGraphicsDriver::ERenderMode	GetEffectiveRenderMode();

private:
	void							DrawPolyClipped(const Rect* theClipRect, const D3DVertexList& theList);
	void							SetRenderState(ulong theRenderState, ulong theValue);
	void							SetSamplerState(ulong theSampler, ulong theState, ulong theValue);
	void							SetTextureStageState(ulong theTextureStage, ulong theState, ulong theValue);
	void							SetTransform(ulong theTransformState, const SexyMatrix4* theMatrix);
	void							SetTextureDirect(ulong theTexture, void* theTexSurface);	

public:
	virtual HRESULT				InternalValidateDevice(ulong* outNumPasses) = 0;
	virtual HRESULT				InternalCreateVertexShader(const ulong* inFunction, IUnknown** outShader) = 0;
	virtual HRESULT				InternalCreatePixelShader(const ulong*, IUnknown** outShader) = 0;
	virtual HRESULT				InternalSetPaletteEntries(uint inPaletteNumber, const PALETTEENTRY* inEntries) = 0;
	virtual HRESULT				InternalGetPaletteEntries(uint inPaletteNumber, PALETTEENTRY* outEntries) = 0;
	virtual HRESULT				InternalCreateTexture(uint inWidth, uint inHeight, uint inLevels, bool inRenderTarget, PixelFormat inFormat, ulong inPool, IUnknown** outTexture) = 0;
	virtual HRESULT				InternalCreateCubeTexture(uint inEdgeLength, uint inLevels, ulong inUsage, PixelFormat inFormat, ulong inPool, IUnknown** outCubeTexture) = 0;
	virtual HRESULT				InternalCreateVolumeTexture(uint inWidth, uint inHeight, uint inDepth, uint inLevels, ulong inUsage, PixelFormat inFormat, ulong inPool, IUnknown** outVolumeTexture) = 0;
	virtual HRESULT				InternalUpdateTexture(IUnknown* inSourceTexture, IUnknown* inDestTexture) = 0;
	virtual HRESULT				InternalCreateImageSurface(uint inWidth, uint inHeight, PixelFormat inFormat, IUnknown** outSurface) = 0;
	virtual HRESULT				InternalGetRenderTargetData(IUnknown* inRenderTarget, IUnknown* inDestSurface) = 0;
	virtual HRESULT				InternalSurfaceLockRect(IUnknown* inSurface, int& outPitch, void*& outBits) = 0;
	virtual HRESULT				InternalSurfaceUnlockRect(IUnknown* inSurface) = 0;
	virtual HRESULT				InternalTextureGetSurfaceLevel(IUnknown* inTexture, uint inLevel, IUnknown** outSurface) = 0;
	virtual HRESULT				InternalTextureMakeDirty(IUnknown* inTexture) = 0;
	virtual HRESULT				InternalTextureLockRect(IUnknown* inTexture, int& outPitch, void*& outBits) = 0;
	virtual HRESULT				InternalTextureUnlockRect(IUnknown* inTexture) = 0;
	virtual HRESULT				InternalCubeTextureLockRect(IUnknown* inCubeTexture, ulong inFace, int& outPitch, void*& outBits) = 0;
	virtual HRESULT				InternalCubeTextureUnlockRect(IUnknown* inCubeTexture, ulong inFace) = 0;
	virtual HRESULT				InternalVolumeTextureLockBox(IUnknown* inVolumeTexture, int& outRowPitch, int& outSlicePitch, void*& outBits) = 0;
	virtual HRESULT				InternalVolumeTextureUnlockBox(IUnknown* inVolumeTexture) = 0;
	virtual HRESULT				InternalSetRenderTarget(void* inRenderTargetSurface) = 0;
	virtual HRESULT				InternalBeginScene() = 0;
	virtual HRESULT				InternalCreateVertexBuffer(uint inLength, bool inIsDynamic, ulong inFVF, ulong inPool, IUnknown** outVertexBuffer) = 0;
	virtual HRESULT				InternalCreateIndexBuffer(uint inLength, ulong inPool, IUnknown** outIndexBuffer) = 0;
	virtual HRESULT				InternalVertexBufferLock(IUnknown* inVertexBuffer, uint inOffset, uint inSize, void** outData, ulong inLockFlags) = 0;
	virtual HRESULT				InternalVertexBufferUnlock(IUnknown* inVertexBuffer) = 0;
	virtual HRESULT				InternalIndexBufferLock(IUnknown* inIndexBuffer, uint inOffset, uint inSize, void** outData, ulong inLockFlags) = 0;
	virtual HRESULT				InternalIndexBufferUnlock(IUnknown* inIndexBuffer) = 0;
	virtual HRESULT				InternalDrawIndexedPrimitive(ulong inPrimType, uint inMinIndex, uint inNumVertices, uint inStartIndex, uint inPrimCount) = 0;
	virtual HRESULT				InternalSetStreamSource(uint inStreamNumber, IUnknown* inVertexBuffer, uint inStride) = 0;

	struct PtrData
	{
		StringVector mFiles;
		IntVector mLines;
		//PtrData();
		//~PtrData();
	};

	typedef std::map<HANDLE, PtrData> PtrDataMap;

	static PtrDataMap sPtrData; //?

	static void					D3DAddRef(void* thePtr, const char* theFile, int theLine);
	static void					D3DDelRef(void* thePtr, const char* theFile, int theLine);
	static void				PrintRefsFor(void* thePtr);
	static void				PrintRemainingRefs();
	static bool				CheckDXError(HRESULT theError, const char *theMsg="");	
};

static void SexyMatrixMultiply_Static(SexyMatrix4* pOut, const SexyMatrix4* pM1, SexyMatrix4* pM2);

static IUnknown* CreateTextureSurface(D3DInterface* theInterface, int theWidth, int theHeight, PixelFormat theFormat, bool renderTarget);

static IUnknown* CreateCubeTextureSurface(D3DInterface* theInterface, int theDim, PixelFormat theFormat);

static IUnknown* CreateVolumeTextureSurface(D3DInterface* theInterface, int theDim, PixelFormat theFormat);

static void CopyImageToTexture8888(D3DInterface* theInterface, void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight, bool rightPad);

static void CopyTexture8888ToImage(void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight);

static void CopyImageToTexture4444(void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight, bool rightPad);

static void CopyTexture4444ToImage(void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight);

static void CopyImageToTexture565(void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight, bool rightPad);

static void CopyTexture565ToImage(void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight);

static void CopyImageToTexturePalette8(void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight, bool rightPad);

static void CopyTexturePalette8ToImage(void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight, int thePaletteIndex, D3DInterface* theInterface);

static void CopyImageToLockedRect(D3DInterface* theInterface, void* theLockedBits, int theLockedPitch, MemoryImage* theImage, int offx, int offy, int texWidth, int texHeight, PixelFormat theFormat);

static void CopyImageToTexture(D3DInterface* theInterface, IUnknown* theTexture, MemoryImage* theImage, int offx, int offy, int texWidth, int texHeight, PixelFormat theFormat);

static void CopyImageToCubeMap(D3DInterface* theInterface, IUnknown* theCubeTexture, MemoryImage* theImage, PixelFormat theFormat);

static void CopyImageToVolumeMap(D3DInterface* theInterface, IUnknown* theVolumeTexture, MemoryImage* theImage, PixelFormat theFormat);

static int GetClosestPowerOf2Above(int theNum);

static bool IsPowerOf2(int theNum);

static void GetBestTextureDimensions(int& theWidth, int& theHeight, bool isEdge, bool usePow2, DWORD theImageFlags);

inline static float GetCoord(const _D3DTLVERTEX& theVertex, int theCoord);

inline static _D3DTLVERTEX Interpolate(const _D3DTLVERTEX& v1, const _D3DTLVERTEX& v2, float t);

static void AdjustPrimCount(ulong* ioPrimCount);

static void MetricsAddPrimitive(ulong thePrimType, ulong thePrimCount);

}

#endif //__D3DINTERFACE_H__