#ifndef __IDXSURFACE_H__
#define __IDXSURFACE_H__

#include "../../../DeviceSurface.h"
#include <d3d.h>

namespace Sexy
{
    class IDXSurface : public DeviceSurface
    {
    public:
        virtual ~IDXSurface() {} //12
        virtual HDC GetDC() = 0;
        virtual void ReleaseDC(HDC theDC) = 0;
        virtual HRESULT Blt(LPRECT theDestRect, void* theSurface, LPRECT theSrcRect, DWORD theFlags, LPDDBLTFX theBltFx) = 0;
        //IDXSurface();
    };
}
#endif