#ifndef __D3D8INTERFACE_H__
#define __D3D8INTERFACE_H__

#include <d3d8.h> //First to define D3D structs
#include "D3DInterface.h"
#include "D3DStateManager.h"

namespace Sexy
{
	//class WindowsGraphicsDriver;
	D3DInterface* CreateD3D8Interface();
	static D3DFORMAT MakeD3DFORMAT(PixelFormat theFormat);
	class D3D8Interface : public D3DInterface
	{
		static ulong sSupportedScreenFormats;
	protected:
		LPDIRECT3D8 mD3D;
		LPDIRECT3DDEVICE8 mD3DDevice;
		D3DPRESENT_PARAMETERS mD3DPresentParams;
		LPDIRECT3DSURFACE8 mFullscreenZBuffer;
		D3DCAPS8 mDeviceCaps;
		static bool					CheckRequiredCaps(const D3DCAPS8& theCaps);
		virtual bool				InitD3D(IGraphicsDriver::EResult* outErrorResult, bool preTestOnly, bool* outIsRecommended);
		virtual void 				SetupSupportedRenderTargetFormats();
		virtual void 				SetupSupportedTextureFormats(D3DFORMAT theDisplayFormat);
		void 						DrawPrimitiveInternal(ulong inPrimType, ulong inPrimCount, const void* inVertData, ulong inVertStride, ulong inVertFormat);
	public:
		D3D8Interface();
		virtual ~D3D8Interface();
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
	protected:
		//No clue why they used non-D3D vars and use casts instead
		HRESULT				InternalValidateDevice(ulong* outNumPasses);
		HRESULT				InternalCreateVertexShader(const ulong* inFunction, IUnknown** outShader);
		HRESULT				InternalCreatePixelShader(const ulong*, IUnknown** outShader);
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
	class D3DStateManager8 : public D3DStateManager
	{
	protected:
		LPDIRECT3DDEVICE8 mDevice;
		State::FCommitFunc GetCommitFunc(State* inState);
		static bool DoCommitRenderState(State* inState);
		static bool DoCommitTextureStageState(State* inState);
		static bool DoCommitSamplerState(State* inState);
		static D3DTEXTURESTAGESTATETYPE SamplerStateToTextureStageState(ulong theSamplerState);
		static bool DoCommitLightState(State* inState);
		static bool DoCommitMaterialState(State* inState);
		static bool DoCommitStreamState(State* inState);
		static bool DoCommitTransformState(State* inState);
		static bool DoCommitViewportState(State* inState);
		static bool DoCommitMiscState(State* inState);
	public:
		D3DStateManager8(LPDIRECT3DDEVICE8 inDevice);
		~D3DStateManager8();
	};
}
#endif