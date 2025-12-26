#ifndef __DXSURFACE9_H__
#define __DXSURFACE9_H__

//#include "D3D9Interface.h"
#include <d3d9.h>
#include "IDXSurface.h"

//IDXSurface* CreateDXSurface9(WindowsGraphicsDriver* theDriver);
namespace Sexy
{
    class WindowsGraphicsDriver;
    class D3D9Interface;
    class DXSurface9 : public IDXSurface
    {
    private:
        WindowsGraphicsDriver* mDriver;
        D3D9Interface* mD3DInterface;
        LPDIRECT3DRESOURCE9 mSurface;
        LPDIRECT3DSURFACE9 mTextureSurface;
		ulong mSurfaceFormat;
		int mRedBits;
		int mRedShift;
		int mRedMask;
		int mGreenBits;
		int mGreenShift;
		int mGreenMask;
		int mBlueBits;
		int mBlueShift;
		int mBlueMask;
		bool mIsTexture;
        LPDIRECT3DTEXTURE9 _getTexture() const { return (LPDIRECT3DTEXTURE9)mSurface; } //25
        LPDIRECT3DSURFACE9 _getSurface() const { return (LPDIRECT3DSURFACE9)mSurface; } //26
    public:
        DXSurface9(WindowsGraphicsDriver* theDriver);
        //DXSurface9(const DXSurface9*);
        virtual ~DXSurface9();
        bool Lock(_DEVICESURFACEDESC* theDesc);
        void Unlock(void* formal);
        bool GenerateDeviceSurface(DeviceImage* theImage);
        void Release();
        ulong* GetBits(DeviceImage* theImage); //Todo
        HDC GetDC();
        void ReleaseDC(HDC theDC);
        void SetSurface(void* theSurface);
        void GetDimensions(int* theWidth, int* theHeight);
        HRESULT Blt(RECT* theDestRect, void* theSurface, RECT* theSrcRect, ulong theFlags, DDBLTFX* theBltFx);
        void AddRef();
        int GetVersion() const { return 9; }; //32
        bool HasSurface() const { return mSurface != 0; } //38
        void* GetSurfacePtr() const { return mSurface; }; //39
    };
}
#endif