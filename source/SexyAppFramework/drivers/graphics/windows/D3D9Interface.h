#ifndef __D3D9INTERFACE_H__
#define __D3D9INTERFACE_H__

#include <d3d9.h> //First to define D3D structs
#include "D3DInterface.h"
#include "D3DStateManager.h"

namespace Sexy
{
	class WindowsGraphicsDriver;
	D3DInterface* CreateD3D9Interface();
	class D3D9Interface : public D3DInterface
	{
	protected:
		LPDIRECT3D9 mD3D;
		LPDIRECT3DDEVICE9 mD3DDevice;
		D3DPRESENT_PARAMETERS mD3DPresentParams;
		LPDIRECT3DSURFACE9 mFullscreenZBuffer;
		D3DCAPS9 mDeviceCaps;
		LPDIRECT3DQUERY9 mTexMemResQuery;
		D3DDEVINFO_RESOURCEMANAGER mTexMemResQueryStats;
		LPDIRECT3DSURFACE9 mPingPongRenderTarget[2];
		LPDIRECT3DSURFACE9 mPingPongMemSurface;
		bool mPingPongToggle;
		static bool					CheckRequiredCaps(const D3DCAPS9& theCaps);
		virtual bool				InitD3D(IGraphicsDriver::EResult* outErrorResult, bool preTestOnly, bool* outIsRecommended);
		virtual void 				SetupSupportedRenderTargetFormats();
		virtual void 				SetupSupportedTextureFormats(D3DFORMAT theDisplayFormat);
		void 						DrawPrimitiveInternal(ulong inPrimType, ulong inPrimCount, const void* inVertData, ulong inVertStride, ulong inVertFormat);
		virtual bool 				CompileEffect(const char* inSrcFile, Buffer& outBuffer);
	public:
		D3D9Interface();
		virtual ~D3D9Interface();
		bool 				InitFromGraphicsDriver(WindowsGraphicsDriver* theDriver, IGraphicsDriver::EResult* outErrorResult, bool preTestOnly, bool* outIsRecommended);
		void 				Cleanup();
		bool 				Flush(ulong inFlushFlags);
		bool 				Present(const Rect* theSrcRect, const Rect* theDestRect);
		ulong 				GetCapsFlags();
		ulong 				GetMaxTextureStages();
		std::string 		GetInfoString(EInfoString theInfoString);
		void				GetBackBufferDimensions(ulong& outWidth, ulong& outHeight);
		IUnknown*			CreateSurface(int inWidth, int inHeight, bool inRenderTarget, bool inTexture);
		bool				CanBltSurface(bool srcSurfaceIsTexture);
		void				BltSurface(IUnknown* theSurface, const Rect& theDest, const Rect& theSrc);
		void				ClearColorBuffer(const Color& inColor);
		void				ClearDepthBuffer();
		bool				ReloadEffects();
		LPDIRECT3DDEVICE9			GetRawDevice(); //Not a function
	protected:
		HRESULT				InternalValidateDevice(ulong* outNumPasses);
		HRESULT				InternalCreateVertexShader(const ulong* inFunction, IUnknown** outShader);
		HRESULT				InternalCreatePixelShader(const ulong* inFunction, IUnknown** outShader);
		HRESULT				InternalSetPaletteEntries(uint inPaletteNumber, const PALETTEENTRY* inEntries);
		HRESULT				InternalGetPaletteEntries(uint inPaletteNumber, PALETTEENTRY* outEntries);
		HRESULT				InternalCreateTexture(uint inWidth, uint inHeight, uint inLevels, bool inRenderTarget, PixelFormat inFormat, ulong inPool, IUnknown** outTexture);
		HRESULT				InternalCreateCubeTexture(uint inEdgeLength, uint inLevels, ulong inUsage, PixelFormat inFormat, ulong inPool, IUnknown** outCubeTexture);
		HRESULT				InternalCreateVolumeTexture(uint inWidth, uint inHeight, uint inDepth, uint inLevels, ulong inUsage, PixelFormat inFormat, ulong inPool, IUnknown** outVolumeTexture);
		HRESULT				InternalUpdateTexture(IUnknown* inSourceTexture, IUnknown* inDestTexture);
		HRESULT				InternalCreateImageSurface(uint inWidth, uint inHeight, PixelFormat inFormat, IUnknown** outSurface);
		HRESULT				InternalGetRenderTargetData(IUnknown* inRenderTarget, IUnknown* inDestSurface);
		HRESULT				InternalSurfaceLockRect(IUnknown* inSurface, int& outPitch, void*& outBits);
		HRESULT				InternalSurfaceUnlockRect(IUnknown* inSurface);
		HRESULT				InternalTextureGetSurfaceLevel(IUnknown* inTexture, uint inLevel, IUnknown** outSurface);
		HRESULT				InternalTextureMakeDirty(IUnknown* inTexture);
		HRESULT				InternalTextureLockRect(IUnknown* inTexture, int& outPitch, void*& outBits);
		HRESULT				InternalTextureUnlockRect(IUnknown* inTexture);
		HRESULT				InternalCubeTextureLockRect(IUnknown* inCubeTexture, ulong inFace, int& outPitch, void*& outBits);
		HRESULT				InternalCubeTextureUnlockRect(IUnknown* inCubeTexture, ulong inFace);
		HRESULT				InternalVolumeTextureLockBox(IUnknown* inVolumeTexture, int& outRowPitch, int& outSlicePitch, void*& outBits);
		HRESULT				InternalVolumeTextureUnlockBox(IUnknown* inVolumeTexture);
		HRESULT				InternalSetRenderTarget(void* inRenderTargetSurface);
		HRESULT				InternalBeginScene();
		HRESULT				InternalCreateVertexBuffer(uint inLength, bool inIsDynamic, ulong inFVF, ulong inPool, IUnknown** outVertexBuffer);
		HRESULT				InternalCreateIndexBuffer(uint inLength, ulong inPool, IUnknown** outIndexBuffer);
		HRESULT				InternalVertexBufferLock(IUnknown* inVertexBuffer, uint inOffset, uint inSize, void** outData, ulong inLockFlags);
		HRESULT				InternalVertexBufferUnlock(IUnknown* inVertexBuffer);
		HRESULT				InternalIndexBufferLock(IUnknown* inIndexBuffer, uint inOffset, uint inSize, void** outData, ulong inLockFlags);
		HRESULT				InternalIndexBufferUnlock(IUnknown* inIndexBuffer);
		HRESULT				InternalDrawIndexedPrimitive(ulong inPrimType, uint inMinIndex, uint inNumVertices, uint inStartIndex, uint inPrimCount);
		HRESULT				InternalSetStreamSource(uint inStreamNumber, IUnknown* inVertexBuffer, uint inStride);
	};
	class D3DStateManager9 : public D3DStateManager
	{
	protected:
		D3D9Interface* mInterface;
		LPDIRECT3DDEVICE9 mDevice;
		static bool DoCommitRenderState(State* inState);
		static bool DoCommitTextureStageState(State* inState);
		static bool DoCommitSamplerState(State* inState);
		static bool DoCommitLightState(State* inState);
		static bool DoCommitMaterialState(State* inState);
		static bool DoCommitStreamState(State* inState);
		static bool DoCommitTransformState(State* inState);
		static bool DoCommitViewportState(State* inState);
		static bool DoCommitMiscState(State* inState);
		State::FCommitFunc  GetCommitFunc(State* inState); //?
	public:
		//D3DStateManager9(const D3DStateManager9&);
		D3DStateManager9(D3D9Interface* inInterface, LPDIRECT3DDEVICE9 inDevice);
		virtual ~D3DStateManager9();
	};
	class CD3DEffectStateManager : public ID3DXEffectStateManager //Not supported in current DX
	{
	public:
		typedef int ECommand; //Unused?
		ulong mRefCount;
		LPDIRECT3DDEVICE9 mDevice;
		Buffer* mBuffer;
		std::string mErrorStr;
		CD3DEffectStateManager(LPDIRECT3DDEVICE9 inDevice, Buffer* inBuffer);
		void Reset();
		void Log(const std::string& inStr);
		void Error(const std::string& inStr);
		virtual HRESULT QueryInterface(const GUID& iid, void** ppvObject);
		virtual ULONG AddRef();
		virtual ULONG Release();
		virtual HRESULT LightEnable(DWORD Index, BOOL Enable);
		virtual HRESULT SetFVF(DWORD FVF);
		virtual HRESULT SetLight(DWORD Index, const D3DLIGHT9* pLight);
		virtual HRESULT SetMaterial(const D3DMATERIAL9* pMaterial);
		virtual HRESULT SetNPatchMode(float nSegments);
		virtual HRESULT SetPixelShader(void* inShader);
		virtual HRESULT SetPixelShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT RegisterCount);
		virtual HRESULT SetPixelShaderConstantF(UINT StartRegister, const FLOAT* pConstantData, UINT RegisterCount);
		virtual HRESULT SetPixelShaderConstantI(UINT StartRegister, const BOOL* pConstantData, UINT RegisterCount);
		virtual HRESULT SetRenderState(D3DRENDERSTATETYPE State, DWORD Value);
		virtual HRESULT SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value);
		virtual HRESULT SetTexture(DWORD Stage, LPDIRECT3DBASETEXTURE9 pTexture);
		virtual HRESULT SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
		virtual HRESULT SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix);
		virtual HRESULT SetVertexShader(LPDIRECT3DVERTEXSHADER9 pShader);
		virtual HRESULT SetVertexShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT RegisterCount);
		virtual HRESULT SetVertexShaderConstantF(UINT StartRegister, const FLOAT* pConstantData, UINT RegisterCount);
		virtual HRESULT SetVertexShaderConstantI(UINT StartRegister, const BOOL* pConstantData, UINT RegisterCount);
		~CD3DEffectStateManager();
	};
}
#endif