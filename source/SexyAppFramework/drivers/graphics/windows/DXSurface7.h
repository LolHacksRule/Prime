#ifndef __DXSURFACE7_H__
#define __DXSURFACE7_H__

#include "IDXSurface.h"

//IDXSurface* CreateDXSurface7(WindowsGraphicsDriver* theDriver);
namespace Sexy
{
    class WindowsGraphicsDriver;
    class DXSurface7 : public IDXSurface
    {
    private:
        WindowsGraphicsDriver* mDriver;
        LPDIRECTDRAWSURFACE mSurface;
    public:
        DXSurface7(WindowsGraphicsDriver* theDriver);
        ~DXSurface7();
        int GetVersion() const { return 7; } //23
        bool Lock(DeviceSurfaceDesc* theParam);
        void Unlock(void* theParam);
        bool GenerateDeviceSurface(DeviceImage* theImage);
        bool HasSurface() const { return mSurface != 0; } //29
        void* GetSurfacePtr() const { return mSurface; } //30
        void AddRef();
        void Release();
        ulong* GetBits(DeviceImage* theImage); //Todo
        HDC GetDC();
        void ReleaseDC(HDC theDC);
        void SetSurface(void* theSurface);
        void GetDimensions(int* theWidth, int* theHeight);
        HRESULT Blt(LPRECT theDestRect, void* theSurface, LPRECT theSrcRect, DWORD theFlags, LPDDBLTFX theBltFx);
    };
}
#endif