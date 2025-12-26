#ifndef __DXSURFACE8_H__
#define __DXSURFACE8_H__

#include <d3d8.h>
#include "IDXSurface.h"
//#include "WindowsGraphicsDriver.h"
//#include "D3D8Interface.h"

//IDXSurface* CreateDXSurface8(WindowsGraphicsDriver* theDriver);
namespace Sexy
{
    //class WindowsGraphicsDriver;
    //class D3D8Interface;
    class DXSurface8 : public IDXSurface
    {
    private:
        WindowsGraphicsDriver* mDriver;
        D3D8Interface* mD3DInterface;
        LPDIRECT3DRESOURCE8 mSurface;
        LPDIRECT3DSURFACE8 mTextureSurface;
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
        LPDIRECT3DTEXTURE8 _getTexture() const { return (LPDIRECT3DTEXTURE8) mSurface; } //25
        LPDIRECT3DSURFACE8 _getSurface() const { return (LPDIRECT3DSURFACE8) mSurface; } //26
    public:
        DXSurface8(WindowsGraphicsDriver* theDriver);
        virtual ~DXSurface8();
        bool Lock(_DEVICESURFACEDESC* theDesc);
        void Unlock(void* __formal);
        bool GenerateDeviceSurface(DeviceImage* theImage);
        void Release();
        ulong* GetBits(DeviceImage* theImage);
        HDC GetDC();
        void ReleaseDC(HDC theDC);
        void SetSurface(void* theSurface);
        void GetDimensions(int* theWidth, int* theHeight);
        HRESULT Blt(LPRECT theDestRect, void* theSurface, LPRECT theSrcRect, DWORD theFlags, LPDDBLTFX theBltFx);
        void AddRef();
        int GetVersion() const { return 8; } //32
        bool HasSurface() const { return mSurface != 0; } //38
        void* GetSurfacePtr() const { return mSurface; } //39
    };
}
#endif