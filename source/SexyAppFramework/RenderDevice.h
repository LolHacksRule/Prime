#ifndef __RENDERDEVICE_H__
#define __RENDERDEVICE_H__

#include "TriVertex.h"
#include "SexyMatrix.h"
#include "SexyAppBase.h"
#include "RenderEffect.h"
#include "Mesh.h"
#include "Graphics.h"

namespace Sexy
{
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
    enum PixelFormat
    {
        PixelFormat_Unknown = 0x0000,
        PixelFormat_A8R8G8B8 = 0x0001,
        PixelFormat_A4R4G4B4 = 0x0002,
        PixelFormat_R5G6B5 = 0x0004,
        PixelFormat_Palette8 = 0x0008,
        PixelFormat_X8R8G8B8 = 0x0010
#ifdef _SEXYDECOMP_USE_LATEST_CODE //Recover from XNA
        PixelFormat_DXT5 = 0x0020
#endif
    };

#ifndef _WIN32
    class RenderSurface
    {
        int mData;
        void* mPtr;
        ulong mRefCount;
    };
#else
    typedef IUnknown RenderSurface; //On Win, IUnknown
#endif

    enum SEXY3DRSS //Given this is in Base*RenderDevice assuming it's here.
    {
        SEXY3DRS_ZENABLE = 7,
        SEXY3DRS_FILLMODE = 8,
        SEXY3DRS_SHADEMODE = 9,
        SEXY3DRS_ZWRITEENABLE = 14,
        SEXY3DRS_ALPHATESTENABLE = 15,
        SEXY3DRS_LASTPIXEL = 16,
        SEXY3DRS_SRCBLEND = 19,
        SEXY3DRS_DESTBLEND = 20,
        SEXY3DRS_CULLMODE = 22,
        SEXY3DRS_ZFUNC = 23,
        SEXY3DRS_ALPHAREF = 24,
        SEXY3DRS_ALPHAFUNC = 25,
        SEXY3DRS_DITHERENABLE = 26,
        SEXY3DRS_ALPHABLENDENABLE = 27,
        SEXY3DRS_FOGENABLE = 28,
        SEXY3DRS_SPECULARENABLE = 29,
        SEXY3DRS_FOGCOLOR = 34,
        SEXY3DRS_FOGTABLEMODE = 35,
        SEXY3DRS_FOGSTART = 36,
        SEXY3DRS_FOGEND = 37,
        SEXY3DRS_FOGDENSITY = 38,
        SEXY3DRS_RANGEFOGENABLE = 48,
        SEXY3DRS_STENCILENABLE = 52,
        SEXY3DRS_STENCILFAIL = 53,
        SEXY3DRS_STENCILZFAIL = 54,
        SEXY3DRS_STENCILPASS = 55,
        SEXY3DRS_STENCILFUNC = 56,
        SEXY3DRS_STENCILREF = 57,
        SEXY3DRS_STENCILMASK = 58,
        SEXY3DRS_STENCILWRITEMASK = 59,
        SEXY3DRS_TEXTUREFACTOR = 60,
        SEXY3DRS_WRAP0 = 128,
        SEXY3DRS_WRAP1 = 129,
        SEXY3DRS_WRAP2 = 130,
        SEXY3DRS_WRAP3 = 131,
        SEXY3DRS_WRAP4 = 132,
        SEXY3DRS_WRAP5 = 133,
        SEXY3DRS_WRAP6 = 134,
        SEXY3DRS_WRAP7 = 135,
        SEXY3DRS_CLIPPING = 136,
        SEXY3DRS_LIGHTING = 137,
        SEXY3DRS_AMBIENT = 139,
        SEXY3DRS_FOGVERTEXMODE = 140,
        SEXY3DRS_COLORVERTEX = 141,
        SEXY3DRS_LOCALVIEWER = 142,
        SEXY3DRS_NORMALIZENORMALS = 143,
        SEXY3DRS_DIFFUSEMATERIALSOURCE = 145,
        SEXY3DRS_SPECULARMATERIALSOURCE = 146,
        SEXY3DRS_AMBIENTMATERIALSOURCE = 147,
        SEXY3DRS_EMISSIVEMATERIALSOURCE = 148,
        SEXY3DRS_VERTEXBLEND = 151,
        SEXY3DRS_CLIPPLANEENABLE = 152,
        SEXY3DRS_POINTSIZE = 154,
        SEXY3DRS_POINTSIZE_MIN = 155,
        SEXY3DRS_POINTSPRITEENABLE = 156,
        SEXY3DRS_POINTSCALEENABLE = 157,
        SEXY3DRS_POINTSCALE_A = 158,
        SEXY3DRS_POINTSCALE_B = 159,
        SEXY3DRS_POINTSCALE_C = 160,
        SEXY3DRS_MULTISAMPLEANTIALIAS = 161,
        SEXY3DRS_MULTISAMPLEMASK = 162,
        SEXY3DRS_PATCHEDGESTYLE = 163,
        SEXY3DRS_DEBUGMONITORTOKEN = 165,
        SEXY3DRS_POINTSIZE_MAX = 166,
        SEXY3DRS_INDEXEDVERTEXBLENDENABLE = 167,
        SEXY3DRS_COLORWRITEENABLE = 168,
        SEXY3DRS_TWEENFACTOR = 170,
        SEXY3DRS_BLENDOP = 171,
        SEXY3DRS_POSITIONDEGREE = 172,
        SEXY3DRS_NORMALDEGREE = 173,
        SEXY3DRS_SCISSORTESTENABLE = 174,
        SEXY3DRS_SLOPESCALEDEPTHBIAS = 175,
        SEXY3DRS_ANTIALIASEDLINEENABLE = 176,
        SEXY3DRS_MINTESSELLATIONLEVEL = 178,
        SEXY3DRS_MAXTESSELLATIONLEVEL = 179,
        SEXY3DRS_ADAPTIVETESS_X = 180,
        SEXY3DRS_ADAPTIVETESS_Y = 181,
        SEXY3DRS_ADAPTIVETESS_Z = 182,
        SEXY3DRS_ADAPTIVETESS_W = 183,
        SEXY3DRS_ENABLEADAPTIVETESSELLATION = 184,
        SEXY3DRS_TWOSIDEDSTENCILMODE = 185,
        SEXY3DRS_CCW_STENCILFAIL = 186,
        SEXY3DRS_CCW_STENCILZFAIL = 187,
        SEXY3DRS_CCW_STENCILPASS = 188,
        SEXY3DRS_CCW_STENCILFUNC = 189,
        SEXY3DRS_COLORWRITEENABLE1 = 190,
        SEXY3DRS_COLORWRITEENABLE2 = 191,
        SEXY3DRS_COLORWRITEENABLE3 = 192,
        SEXY3DRS_BLENDFACTOR = 193,
        SEXY3DRS_SRGBWRITEENABLE = 194,
        SEXY3DRS_DEPTHBIAS = 195,
        SEXY3DRS_WRAP8 = 198,
        SEXY3DRS_WRAP9 = 199,
        SEXY3DRS_WRAP10 = 200,
        SEXY3DRS_WRAP11 = 201,
        SEXY3DRS_WRAP12 = 202,
        SEXY3DRS_WRAP13 = 203,
        SEXY3DRS_WRAP14 = 204,
        SEXY3DRS_WRAP15 = 205,
        SEXY3DRS_SEPARATEALPHABLENDENABLE = 206,
        SEXY3DRS_SRCBLENDALPHA = 207,
        SEXY3DRS_DESTBLENDALPHA = 208,
        SEXY3DRS_BLENDOPALPHA = 209,
        SEXY3DRS_FORCE_DWORD = 0x7fffffff
    };

    enum SEXY3DCUBEMAP_FACES
    {
        SEXY3DCUBEMAP_FACE_POSITIVE_X = 0,
        SEXY3DCUBEMAP_FACE_NEGATIVE_X = 1,
        SEXY3DCUBEMAP_FACE_POSITIVE_Y = 2,
        SEXY3DCUBEMAP_FACE_NEGATIVE_Y = 3,
        SEXY3DCUBEMAP_FACE_POSITIVE_Z = 4,
        SEXY3DCUBEMAP_FACE_NEGATIVE_Z = 5,
        SEXY3DCUBEMAP_FACE_FORCE_DWORD = 0xffffffff
    };

    enum SexyVF //Assuming
    {
        SexyVF_PackedFormat = 1,
        SexyVF_XYZ = 2,
        SexyVF_XYZRHW = 4,
        SexyVF_Normal = 16,
        SexyVF_Diffuse = 64,
        SexyVF_Specular = 128,
        SexyVF_Tex1 = 256,
        SexyVF_Tex2 = 512,
        SexyVF_Tex3 = 768,
        SexyVF_Tex4 = 1024,
        SexyVF_Tex5 = 1280,
        SexyVF_Tex6 = 1536,
        SexyVF_Tex7 = 1792,
        SexyVF_Tex8 = 2048
    };

    class RenderDevice
    {
    public:
        virtual RenderDevice3D* Get3D() = 0;
        virtual bool CanFillPoly() = 0;
        virtual HRenderContext CreateContext(Image*, const HRenderContext&) = 0;
        virtual void DeleteContext(const HRenderContext&) = 0;
        virtual void SetCurrentContext(const HRenderContext& theContext) = 0;
        virtual HRenderContext GetCurrentContext() const = 0;
        virtual void PushState() = 0;
        virtual void PopState() = 0;
        struct Span
        {
            int mY;
            int mX;
            int mWidth;
        };
        virtual void                    ClearRect(const Rect& theRect) = 0;
        virtual void                    FillRect(const Rect& theRect, const Color& theColor, int theDrawMode) = 0;
        virtual void                    FillScanLinesWithCoverage(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode, const BYTE* theCoverage, int theCoverX, int theCoverY, int theCoverWidth, int theCoverHeight) = 0;
        virtual void                    FillPoly(const Point theVertices[], int theNumVertices, const Rect* theClipRect, const Color& theColor, int theDrawMode, int tx, int ty) = 0;
        virtual void                    DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode, bool antiAlias = false) = 0;
        virtual void                    Blt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode) = 0;
        virtual void                    BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode) = 0;
        virtual void                    BltRotated(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY) = 0;
        virtual void                    BltMatrix(Image* theImage, float x, float y, const SexyMatrix3& theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect& theSrcRect, bool blend) = 0;
        virtual void                    BltTriangles(Image* theImage, const SexyVertex2D theVertices[][3], int theNumTriangles, const Color& theColor, int theDrawMode, float tx, float ty, bool blend, const Rect* theClipRect) = 0;
        virtual void                    BltMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode) = 0;
        virtual void                    BltStretched(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch, bool mirror = false) = 0;
        virtual void                    DrawRect(const Rect& theRect, const Color& theColor, int theDrawMode) //171-176
        {
            FillRect(Rect(theRect.mX, theRect.mY, theRect.mWidth + 1, 1), theColor, theDrawMode);
            FillRect(Rect(theRect.mX, theRect.mY + theRect.mHeight, theRect.mWidth + 1, 1), theColor, theDrawMode);
            FillRect(Rect(theRect.mX, theRect.mY + 1, 1, theRect.mHeight - 1), theColor, theDrawMode);
            FillRect(Rect(theRect.mX + theRect.mWidth, theRect.mY + 1, 1, theRect.mHeight - 1), theColor, theDrawMode);
        }
        virtual void                    FillScanLines(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode) //178-184
        {
            for (int i = 0; i < theSpanCount; i++)
            {
                //Something
                Span aSpan = theSpans[i];
                FillRect(Rect(aSpan.mX, aSpan.mY, aSpan.mWidth, 1), theColor, theDrawMode);
            }
        }
    };
    class RenderDevice3D : public RenderDevice
    {
    public:
        enum EFlushFlags
        {
            FLUSHF_BufferedTris = 1,
            FLUSHF_CurrentScene,
            FLUSHF_ManagedResources_Immediate = 4,
            FLUSHF_ManagedResources_OnPresent = 8
#ifdef _SEXYDECOMP_USE_LATEST_CODE
            FLUSHF_BufferedState = 16,
#endif
        };
        virtual bool Flush(ulong inFlushFlags) = 0;
        virtual bool Present(const Rect* theSrcRect, const Rect* theDestRect) = 0;
        enum ECapsFlags
        {
            CAPF_SingleImageTexture = 1,
            CAPF_PixelShaders,
            CAPF_VertexShaders = 4,
            CAPF_ImageRenderTargets = 8,
            CAPF_AutoWindowedVSync = 16,
            CAPF_CubeMaps = 32,
            CAPF_VolumeMaps = 64
#ifdef _SEXYDECOMP_USE_LATEST_CODE
            CAPF_CopyScreenImage = 128,
            CAPF_LastLockScreenImage = 256
#endif
        };
        enum EInfoString
        {
            INFOSTRING_Adapter,
            INFOSTRING_DrvProductVersion,
            INFOSTRING_DisplayMode,
            INFOSTRING_BackBuffer,
            INFOSTRING_TextureMemory,
            INFOSTRING_DrvResourceManager,
            INFOSTRING_DrvProductFeatures
        };
    public:
        virtual ulong       GetCapsFlags() = 0;
        bool                SupportsPixelShaders() { (GetCapsFlags() & CAPF_PixelShaders) != 0; } //249
        bool                SupportsVertexShaders() { (GetCapsFlags() & CAPF_VertexShaders) != 0; }; //250
        bool                SupportsCubeMaps() { (GetCapsFlags() & CAPF_CubeMaps) != 0; }; //251
        bool                SupportsVolumeMaps() { (GetCapsFlags() & CAPF_VolumeMaps) != 0; }; //252
        bool                SupportsImageRenderTargets() { (GetCapsFlags() & CAPF_ImageRenderTargets) != 0; }; //253
        virtual ulong       GetMaxTextureStages() = 0;
        virtual std::string GetInfoString(EInfoString theInfoString) = 0;
        virtual void GetBackBufferDimensions(ulong& outWidth, ulong& outHeight) = 0;
        virtual bool SceneBegun() = 0;
        virtual bool CreateImageRenderData(MemoryImage* inImage) = 0;
        virtual void RemoveImageRenderData(MemoryImage* inImage) = 0;
        virtual bool RecoverImageBitsFromRenderData(MemoryImage* inImage) = 0;
        virtual int GetTextureMemorySize(MemoryImage* theImage) = 0;
        virtual PixelFormat GetTextureFormat(MemoryImage* theImage) = 0;
        virtual Image* SwapScreenImage(DeviceImage*& ioSrcImage, RenderSurface*& ioSrcSurface) = 0;
        virtual void DrawPrimitiveEx(ulong theVertexFormat, Graphics3D::EPrimitiveType thePrimitiveType, const SexyVertex* theVertices, int thePrimitiveCount, const Color& theColor, int theDrawMode, float tx, float ty, bool blend, ulong theFlags) = 0;
        virtual void SetBltDepth(float inDepth) = 0;
        virtual void PushTransform(const SexyMatrix3& theTransform, bool concatenate) = 0;
        virtual void PopTransform(SexyMatrix3& theTransform) = 0;
        virtual void PopTransform() = 0;
        virtual void ClearColorBuffer(const Color& theColor) = 0;
        virtual void ClearDepthBuffer() = 0;
        virtual void SetDepthState(Graphics3D::ECompareFunc inDepthTestFunc, bool inDepthWriteEnabled) = 0;
        virtual void SetAlphaTest(Graphics3D::ECompareFunc inAlphaTestFunc, int inRefAlpha) = 0;
        virtual void SetWireframe(bool inWireframe) = 0;
        virtual void SetBlend(Graphics3D::EBlendMode, Graphics3D::EBlendMode) = 0;
        virtual void SetBackfaceCulling(bool inCullClockwise, bool inCullCounterClockwise) = 0;
        virtual void SetLightingEnabled(bool inLightingEnabled) = 0;
        virtual void SetLightEnabled(int inLightIndex, bool inEnabled) = 0;
        virtual void SetPointLight(int inLightIndex, const SexyVector3& inPos, const Graphics3D::LightColors& inColors, float inRange, const SexyVector3& inAttenuation) = 0;
        virtual void SetDirectionalLight(int inLightIndex, const SexyVector3& inDir, const Graphics3D::LightColors& inColors) = 0;
        virtual void SetGlobalAmbient(const Color& inColor) = 0;
        virtual void SetMaterialAmbient(const Color& inColor, int inVertexColorComponent) = 0;
        virtual void SetMaterialDiffuse(const Color& inColor, int inVertexColorComponent) = 0;
        virtual void SetMaterialSpecular(const Color& inColor, int inVertexColorComponent, float inPower) = 0;
        virtual void SetMaterialEmissive(const Color& inColor, int inVertexColorComponent) = 0;
        virtual void SetWorldTransform(const SexyMatrix4* inMatrix) = 0;
        virtual void SetViewTransform(const SexyMatrix4* inMatrix) = 0;
        virtual void SetProjectionTransform(const SexyMatrix4* inMatrix) = 0;
        virtual void SetTextureTransform(int inTextureIndex, const SexyMatrix4* inMatrix, int inNumDimensions) = 0;
        virtual void SetViewport(int theX, int theY, int theWidth, int theHeight, float theMinZ, float theMaxZ) = 0;
        virtual bool SetTexture(int inTextureIndex, Image* inImage) = 0;
        virtual void SetTextureWrap(int inTextureIndex, bool inWrapU, bool inWrapV) = 0;
        virtual void SetTextureLinearFilter(int inTextureIndex, bool inLinear) = 0;
        virtual void SetTextureCoordSource(int inTextureIndex, int inUVComponent, Graphics3D::ETexCoordGen inTexGen) = 0;
        virtual void SetTextureFactor(int inTextureFactor) = 0;
        virtual RenderEffect* GetEffect(RenderEffectDefinition* inDefinition) = 0;
        virtual bool ReloadEffects() { return false; } //425
        typedef bool (*FBltFilter)(void* theContext, int thePrimType, ulong thePrimCount, const SexyVertex2D* theVertices, int theVertexSize, const Rect theClipRect[]); //Official
        typedef bool (*FDrawPrimFilter)(void* theContext, int thePrimType, ulong thePrimCount, const SexyVertex2D* theVertices, int theVertexSize); //Official
        virtual void SetBltFilter(FBltFilter inFilter, void* inContext) {} //435
        virtual void SetDrawPrimFilter(FDrawPrimFilter inFilter, void* inContext) {} //436
        virtual bool LoadMesh(Mesh* theMesh) = 0;
        virtual void RenderMesh(Mesh* theMesh, const SexyMatrix4& theMatrix, const Color& theColor, bool doSetup) = 0;
    };
}

#endif